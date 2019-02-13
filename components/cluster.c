/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup cluster Cluster
 * \brief (Admin) cluster
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "cluster.h"

/** \brief Cluster ID */
#define CLUSTER_ID 235171752

/////////////////////////////////////////////////////////////////////

/** \brief The cluster node ID defined in the event handler */
extern int cluster_id;

/** \brief The latest event ID seen in the distributed storage */
volatile int curr_ev_id;

/** \brief The IP address of the current machine */
char ctrl_ip[__CONF_SHORT_LEN];

/////////////////////////////////////////////////////////////////////

/** \brief The batch size of queries */
#define BATCH_SIZE 1024

/////////////////////////////////////////////////////////////////////

/** \brief The waiting queue for queries */
char query[BATCH_SIZE][__CONF_LSTR_LEN];

/** \brief The number of queries in a waiting queue */
int write_query;

/** \brief The lock for queue management */
pthread_spinlock_t cluster_lock;

/////////////////////////////////////////////////////////////////////

#ifdef __ENABLE_CLUSTER

#include "db_password.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to connect a DB
 * \return The connection of a DB
 */
static MYSQL *connect_db(void)
{
    MYSQL *conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, "localhost", CLUSTER_MYID, CLUSTER_MYPW, "Barista", 0, NULL, 0)) {
        PERROR("mysql_real_connect");
        return NULL;
    } else {
        return conn;
    }
}

/**
 * \brief Function to close a DB
 * \param The connection of a DB
 */
static int close_db(MYSQL *conn)
{
    if (conn) {
        mysql_close(conn);
        conn = NULL;
    }

    return 0;
}

/**
 * \brief Function to execute a query
 * \param conn The connection of a DB
 * \param sql_query SQL query
 */
static MYSQL_RES *run_query(MYSQL *conn, char *sql_query)
{
    if (conn == NULL) {
        return NULL;
    } else if (mysql_query(conn, sql_query)) {
        LOG_ERROR(CLUSTER_ID, "Failed SQL query: %s", sql_query);
        return NULL;
    } else {
        return mysql_use_result(conn);
    }
}

/**
 * \brief Function to push a query into a waiting queue
 * \param sql_query SQL query
 */
static int push_query_into_queue(char *sql_query)
{
    pthread_spin_lock(&cluster_lock);

    if (write_query >= BATCH_SIZE-1) {
        MYSQL *conn = connect_db();
        if (conn == NULL) {
            pthread_spin_unlock(&cluster_lock);
            return -1;
        }

        mysql_query(conn, "START TRANSACTION");

        int i;
        for (i=0; i<write_query; i++) {
            MYSQL_RES *res = run_query(conn, query[i]);
            if (res) mysql_free_result(res);
        }

        mysql_query(conn, "COMMIT");

        close_db(conn);

        memset(query, 0, write_query * __CONF_LSTR_LEN);

        write_query = 0;
    }

    snprintf(query[write_query++], __CONF_LSTR_LEN-1, "%s", sql_query);

    pthread_spin_unlock(&cluster_lock);

    return 0;
}

/**
 * \brief Function to encode an event and push a query into a waiting queue
 * \param ev Event
 */
static int insert_event_data(const event_t *ev)
{
    char *encode = base64_encode(ev->log, ev->length);

    char sql_query[__CONF_LSTR_LEN] = {0};
    snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.events (event, data, ctrl_ip) values (%u, '%s', '%s')", 
             ev->type, encode, ctrl_ip);
    push_query_into_queue(sql_query);

    FREE(encode);

    return 0;
}

/////////////////////////////////////////////////////////////////////

#endif /* __ENABLE_CLUSTER */

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int cluster_main(int *activated, int argc, char **argv)
{
    LOG_INFO(CLUSTER_ID, "Init - Cluster");

#ifdef __ENABLE_CLUSTER

    memset(query, 0, sizeof(char) * BATCH_SIZE * __CONF_LSTR_LEN);

    pthread_spin_init(&cluster_lock, PTHREAD_PROCESS_PRIVATE);

    FILE *pp = popen("ip route get 8.8.8.8 | awk 'NR==1 {print $NF}'", "r");
    if (pp) {
        memset(ctrl_ip, 0, __CONF_SHORT_LEN);
        if (fgets(ctrl_ip, __CONF_SHORT_LEN, pp) == NULL) {
            PERROR("fgets");
            return -1;
        } else {
            pclose(pp);
        }
        ctrl_ip[strlen(ctrl_ip)-1] = '\0';
        LOG_INFO(CLUSTER_ID, "Controller IP: %s", ctrl_ip);
    } else {
        memset(ctrl_ip, 0, __CONF_SHORT_LEN);
        strcpy(ctrl_ip, "127.0.0.1");
        LOG_INFO(CLUSTER_ID, "Controller IP: %s", ctrl_ip);
    }

    MYSQL *conn = connect_db();
    if (conn) {
        LOG_INFO(CLUSTER_ID, "Successful DB connection");

        char sql_query[__CONF_LSTR_LEN] = {0};
        snprintf(sql_query, __CONF_LSTR_LEN-1, "show status like 'wsrep_local_index';");
        MYSQL_RES *res = run_query(conn, sql_query);
        MYSQL_ROW row = mysql_fetch_row(res);
        if (strcmp(row[0], "wsrep_local_index") == 0)
            cluster_id = atoi(row[1]);
        else
            cluster_id = 0;
        if (res) mysql_free_result(res);

        LOG_INFO(CLUSTER_ID, "Cleaning up all old data for '%s'", ctrl_ip);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.clusters (cluster_id, ctrl_ip) values (%d, '%s') "
                                               "on duplicate key update cluster_id = %d, ctrl_ip = '%s';", 
                                               cluster_id, ctrl_ip, cluster_id, ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.switches where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.hosts where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);
        
        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.links where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.flows where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.traffic where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.resources where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.logs where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.events where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        LOG_INFO(CLUSTER_ID, "Cleaned up all old data for '%s'", ctrl_ip);

        close_db(conn);
    } else {
        LOG_ERROR(CLUSTER_ID, "Failed to connect the database");

        return -1;
    }

#else /* !__ENABLE_CLUSTER */

    LOG_WARN(CLUSTER_ID, "Cluster is not enabled");

#endif /* !__ENABLED_CLUSTER */

    activate();

    waitsec(1, 0);

#ifdef __ENABLE_CLUSTER

    while (*activated) {
        // push data

        pthread_spin_lock(&cluster_lock);

        if (write_query) {
            MYSQL *conn = connect_db();
            if (conn == NULL) {
                pthread_spin_unlock(&cluster_lock);
                waitsec(0, CLUSTER_UPDATE_TIME_NS);
                continue;
            }

            MYSQL_RES *res = run_query(conn, "START TRANSACTION");
            if (res) mysql_free_result(res);

            int i;
            for (i=0; i<write_query; i++) {
                MYSQL_RES *res = run_query(conn, query[i]);
                if (res) mysql_free_result(res);
            }

            res = run_query(conn, "COMMIT");
            if (res) mysql_free_result(res);

            memset(query, 0, write_query * __CONF_LSTR_LEN);
            write_query = 0;

            close_db(conn);
        }

        pthread_spin_unlock(&cluster_lock);

        // pop data

        MYSQL *conn = connect_db();
        if (conn) {
            char sql_query[__CONF_LSTR_LEN] = {0};

            if (curr_ev_id == 0) {
                snprintf(sql_query, __CONF_LSTR_LEN-1, "select * from Barista.events where ctrl_ip != '%s'", ctrl_ip);
            } else {
                snprintf(sql_query, __CONF_LSTR_LEN-1, "select * from Barista.events where id > %u and ctrl_ip != '%s'", 
                         curr_ev_id, ctrl_ip);
            }

            MYSQL_RES *res = run_query(conn, sql_query);
            MYSQL_ROW row;

            while ((row = mysql_fetch_row(res))) {
                curr_ev_id = atoi(row[0]);
                int type = atoi(row[1]);
                char *data = base64_decode(row[2]);

                switch (type) {
                case EV_DP_PORT_STATS:
                    {
                        port_t *port = (port_t *)data;
                        port->remote = TRUE;
                        ev_dp_port_stats(CLUSTER_ID, port);
                    }
                    break;
                case EV_SW_DISCONNECTED:
                    {
                        switch_t *sw = (switch_t *)data;
                        sw->remote = TRUE;
                        sw->fd = (CLUSTER_DELIMITER * (cluster_id + 1)) + sw->fd;
                        ev_sw_disconnected(CLUSTER_ID, sw);
                    }
                    break;
                case EV_SW_UPDATE_CONFIG:
                    {
                        switch_t *sw = (switch_t *)data;
                        sw->remote = TRUE;
                        sw->fd = (CLUSTER_DELIMITER * (cluster_id + 1)) + sw->fd;
                        ev_sw_update_config(CLUSTER_ID, sw);
                    }
                    break;
                case EV_SW_UPDATE_DESC:
                    {
                        switch_t *sw = (switch_t *)data;
                        sw->remote = TRUE;
                        sw->fd = (CLUSTER_DELIMITER * (cluster_id + 1)) + sw->fd;
                        ev_sw_update_desc(CLUSTER_ID, sw);
                    }
                    break;
                case EV_HOST_ADDED:
                    {
                        host_t *host = (host_t *)data;
                        host->remote = TRUE;
                        ev_host_added(CLUSTER_ID, host);
                    }
                    break;
                case EV_HOST_DELETED:
                    {
                        host_t *host = (host_t *)data;
                        host->remote = TRUE;
                        ev_host_deleted(CLUSTER_ID, host);
                    }
                    break;
                case EV_LINK_ADDED:
                    {
                        port_t *port = (port_t *)data;
                        port->remote = TRUE;
                        ev_link_added(CLUSTER_ID, port);
                    }
                    break;
                case EV_LINK_DELETED:
                    {
                        port_t *port = (port_t *)data;
                        port->remote = TRUE;
                        ev_link_deleted(CLUSTER_ID, port);
                    }
                    break;
                case EV_FLOW_ADDED:
                    {
                        flow_t *flow = (flow_t *)data;
                        flow->remote = TRUE;
                        ev_flow_added(CLUSTER_ID, flow);
                    }
                    break;
                case EV_FLOW_MODIFIED:
                    {
                        flow_t *flow = (flow_t *)data;
                        flow->remote = TRUE;
                        ev_flow_modified(CLUSTER_ID, flow);
                    }
                case EV_FLOW_DELETED:
                    {
                        flow_t *flow = (flow_t *)data;
                        flow->remote = TRUE;
                        ev_flow_deleted(CLUSTER_ID, flow);
                    }
                    break;
                case EV_DP_FLOW_STATS:
                    {
                        flow_t *flow = (flow_t *)data;
                        flow->remote = TRUE;
                        ev_dp_flow_stats(CLUSTER_ID, flow);
                    }
                    break;
                default:
                    break;
                }

                FREE(data);
            }

            if (res) mysql_free_result(res);

            close_db(conn);
        }

        waitsec(0, CLUSTER_UPDATE_TIME_NS);
    }

#endif /* __ENABLED_CLUSTER */

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int cluster_cleanup(int *activated)
{
    LOG_INFO(CLUSTER_ID, "Clean up - Cluster");

    deactivate();

#ifdef __ENABLE_CLUSTER

    pthread_spin_lock(&cluster_lock);

    MYSQL *conn = connect_db();
    if (conn) {
        char sql_query[__CONF_LSTR_LEN] = {0};

        if (write_query) {
            MYSQL_RES *res = run_query(conn, "START TRANSACTION");
            if (res) mysql_free_result(res);

            int i;
            for (i=0; i<write_query; i++) {
                MYSQL_RES *res = run_query(conn, query[i]);
                if (res) mysql_free_result(res);
            }

            res = run_query(conn, "COMMIT");
            if (res) mysql_free_result(res);

            write_query = 0;
        }

        LOG_INFO(CLUSTER_ID, "Cleaning up all old data for '%s'", ctrl_ip);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.clusters where ctrl_ip = '%s';", ctrl_ip);
        MYSQL_RES *res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.switches where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.hosts where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.links where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.flows where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.traffic where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.resources where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.logs where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        memset(sql_query, 0, __CONF_LSTR_LEN);
        snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.events where ctrl_ip = '%s';", ctrl_ip);
        res = run_query(conn, sql_query);
        if (res) mysql_free_result(res);

        LOG_INFO(CLUSTER_ID, "Cleaned up all old data for '%s'", ctrl_ip);

        close_db(conn);
    } else {
        LOG_ERROR(CLUSTER_ID, "Failed to connect the database");
    }

    pthread_spin_unlock(&cluster_lock);
    pthread_spin_destroy(&cluster_lock);

#endif /* __ENABLE_CLUSTER */

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int cluster_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int cluster_handler(const event_t *ev, event_out_t *ev_out)
{
#ifdef __ENABLE_CLUSTER

    switch (ev->type) {
    case EV_DP_PORT_STATS:
        PRINT_EV("EV_DP_PORT_STATS\n");
        {
            const port_t *port = ev->port;

            if (port->remote == TRUE) break;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "update Barista.links set "
                                                   "rx_packets = %lu, rx_bytes = %lu, tx_packets = %lu, tx_bytes = %lu "
                                                   "where src_dpid = %lu and src_port = %u and ctrl_ip = '%s';",
                                                   port->rx_packets, port->rx_bytes, port->tx_packets, port->tx_bytes, 
                                                   port->dpid, port->port, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == TRUE) break;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.switches where dpid = %lu and ctrl_ip = '%s';", 
                                                    sw->dpid, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_SW_UPDATE_CONFIG:
        PRINT_EV("EV_SW_UPDATE_CONFIG\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == TRUE) break;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.switches (dpid, num_tables, num_buffers, "
                                                   "capabilities, actions, ctrl_ip) "
                                                   "values (%lu, %u, %u, %u, %u, '%s') "
                                                   "on duplicate key update dpid = %lu, num_tables = %u, "
                                                   "num_buffers = %u, capabilities = %u, actions = %u, ctrl_ip = '%s';", 
                                                   sw->dpid, sw->n_tables, sw->n_buffers, sw->capabilities, 
                                                   sw->actions, ctrl_ip, sw->dpid, sw->n_tables, sw->n_buffers, 
                                                   sw->capabilities, sw->actions, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_SW_UPDATE_DESC:
        PRINT_EV("EV_SW_UPDATE_DESC\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == TRUE) break;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "update Barista.switches set mfr_desc = '%s', hw_desc = '%s', "
                                                   "serial_num = '%s', dp_desc = '%s' where dpid = %lu "
                                                   "and ctrl_ip = '%s';", sw->mfr_desc, sw->hw_desc,
                                                   sw->serial_num, sw->dp_desc, sw->dpid, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_HOST_ADDED:
        PRINT_EV("EV_HOST_ADDED\n");
        {
            const host_t *host = ev->host;

            if (host->remote == TRUE) break;

            uint8_t mac[ETH_ALEN];
            int2mac(host->mac, mac);

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.hosts (dpid, port, mac, ip, ctrl_ip) "
                                                   "values (%lu, %u, '%02x:%02x:%02x:%02x:%02x:%02x', '%s', '%s');", 
                                                   host->dpid, host->port, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], 
                                                   ip_addr_str(host->ip), ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_HOST_DELETED:
        PRINT_EV("EV_HOST_DELETED\n");
        {
            const host_t *host = ev->host;

            if (host->remote == TRUE) break;

            uint8_t mac[ETH_ALEN];
            int2mac(host->mac, mac);

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.hosts where dpid = %lu "
                                                   "and mac = '%02x:%02x:%02x:%02x:%02x:%02x' "
                                                   "and ip = '%s' and ctrl_ip = '%s';", 
                                                   host->dpid, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], 
                                                   ip_addr_str(host->ip), ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_LINK_ADDED:
        PRINT_EV("EV_LINK_ADDED\n");
        {
            const port_t *link = ev->port;

            if (link->remote == TRUE) break;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.links (src_dpid, src_port, dst_dpid, dst_port, ctrl_ip) "
                                                   "values (%lu, %u, %lu, %u, '%s');", 
                                                   link->dpid, link->port, link->next_dpid, link->next_port, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_LINK_DELETED:
        PRINT_EV("EV_LINK_DELETED\n");
        {
            const port_t *link = ev->port;

            if (link->remote == TRUE) break;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.links "
                                                   "where src_dpid = %lu and src_port = %u and ctrl_ip = '%s';", 
                                                   link->dpid, link->port, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_FLOW_ADDED:
        PRINT_EV("EV_FLOW_ADDED\n");
        {
            const flow_t *flow = ev->flow;

            if (flow->remote == TRUE) break;

            char proto[__CONF_SHORT_LEN] = {0};
            if (flow->proto & PROTO_TCP)
                strcpy(proto, "TCP");
            else if (flow->proto & PROTO_DHCP)
                strcpy(proto, "DHCP");
            else if (flow->proto & PROTO_UDP)
                strcpy(proto, "UDP");
            else if (flow->proto & PROTO_ICMP)
                strcpy(proto, "ICMP");
            else if (flow->proto & PROTO_IPV4)
                strcpy(proto, "IPv4");
            else if (flow->proto & PROTO_ARP)
                strcpy(proto, "ARP");
            else if (flow->proto & PROTO_LLDP)
                strcpy(proto, "LLDP");
            else
                strcpy(proto, "Unknown");

            char smac[__CONF_WORD_LEN] = {0};
            mac2str(flow->src_mac, smac);

            char dmac[__CONF_WORD_LEN] = {0};
            mac2str(flow->dst_mac, dmac);

            char actions[__CONF_WORD_LEN] = {0};
            sprintf(actions, "%u", flow->num_actions);

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.flows (dpid, port, idle_timeout, hard_timeout, wildcards, "
                                                   "vlan_id, vlan_pcp, proto, ip_tos, src_mac, dst_mac, src_ip, dst_ip, src_port, "
                                                   "dst_port, actions, ctrl_ip) values (%lu, %u, %u, %u, %u, %u, %u, '%s', %u, "
                                                   "'%s', '%s', '%s', '%s', %u, %u, '%s', '%s');", 
                                                   flow->dpid, flow->port, flow->idle_timeout, flow->hard_timeout, flow->wildcards,
                                                   flow->vlan_id, flow->vlan_pcp, proto, flow->ip_tos, smac, dmac,
                                                   ip_addr_str(flow->src_ip), ip_addr_str(flow->dst_ip),
                                                   flow->src_port, flow->dst_port, actions, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_FLOW_MODIFIED:
        PRINT_EV("EV_FLOW_MODIFIED\n");
        {
            //TODO
        }
        break;
    case EV_FLOW_DELETED:
        PRINT_EV("EV_FLOW_DELETED\n");
        {
            const flow_t *flow = ev->flow;

            if (flow->remote == TRUE) break;

            char proto[__CONF_SHORT_LEN] = {0};
            if (flow->proto & PROTO_TCP)
                strcpy(proto, "TCP");
            else if (flow->proto & PROTO_DHCP)
                strcpy(proto, "DHCP");
            else if (flow->proto & PROTO_UDP)
                strcpy(proto, "UDP");
            else if (flow->proto & PROTO_ICMP)
                strcpy(proto, "ICMP");
            else if (flow->proto & PROTO_IPV4)
                strcpy(proto, "IPv4");
            else if (flow->proto & PROTO_ARP)
                strcpy(proto, "ARP");
            else if (flow->proto & PROTO_LLDP)
                strcpy(proto, "LLDP");
            else
                strcpy(proto, "Unknown");

            char smac[__CONF_WORD_LEN] = {0};
            mac2str(flow->src_mac, smac);

            char dmac[__CONF_WORD_LEN] = {0};
            mac2str(flow->dst_mac, dmac);

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "delete from Barista.flows where dpid = %lu and port = %u and proto = '%s' "
                                                   "and src_mac = '%s' and dst_mac = '%s' and src_ip = '%s' and dst_ip = '%s' "
                                                   "and src_port = %u and dst_port = %u and ctrl_ip = '%s';", 
                                                   flow->dpid, flow->port, proto, smac, dmac,
                                                   ip_addr_str(flow->src_ip), ip_addr_str(flow->dst_ip),
                                                   flow->src_port, flow->dst_port, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_DP_FLOW_STATS:
        PRINT_EV("EV_DP_FLOW_STATS\n");
        {
            const flow_t *flow = ev->flow;

            if (flow->remote == TRUE)
                break;

            char proto[__CONF_SHORT_LEN] = {0};
            if (flow->proto & PROTO_TCP)
                strcpy(proto, "TCP");
            else if (flow->proto & PROTO_DHCP)
                strcpy(proto, "DHCP");
            else if (flow->proto & PROTO_UDP)
                strcpy(proto, "UDP");
            else if (flow->proto & PROTO_ICMP)
                strcpy(proto, "ICMP");
            else if (flow->proto & PROTO_IPV4)
                strcpy(proto, "IPv4");
            else if (flow->proto & PROTO_ARP)
                strcpy(proto, "ARP");
            else if (flow->proto & PROTO_LLDP)
                strcpy(proto, "LLDP");
            else
                strcpy(proto, "Unknown");

            char smac[__CONF_WORD_LEN] = {0};
            mac2str(flow->src_mac, smac);

            char dmac[__CONF_WORD_LEN] = {0};
            mac2str(flow->dst_mac, dmac);

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "update Barista.flows set pkt_count = %lu and byte_count = %lu "
                                                   "where dpid = %lu and port = %u and proto = '%s' "
                                                   "and src_mac = '%s' and dst_mac = '%s' and src_ip = '%s' and dst_ip = '%s' "
                                                   "and src_port = %u and dst_port = %u and ctrl_ip = '%s';",
                                                   flow->pkt_count, flow->byte_count,
                                                   flow->dpid, flow->port, proto, smac, dmac, 
                                                   ip_addr_str(flow->src_ip), ip_addr_str(flow->dst_ip),
                                                   flow->src_port, flow->dst_port, ctrl_ip);
            push_query_into_queue(sql_query);

            insert_event_data(ev);
        }
        break;
    case EV_RS_UPDATE_USAGE:
        PRINT_EV("EV_RS_UPDATE_USAGE\n");
        {
            const resource_t *resource = ev->resource;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.resources (cpu, mem, ctrl_ip) values (%lf, %lf, '%s');", 
                                                   resource->cpu, resource->mem, ctrl_ip);
            push_query_into_queue(sql_query);
        }
        break;
    case EV_TR_UPDATE_STATS:
        PRINT_EV("EV_TR_UPDATE_STATS\n");
        {
            const traffic_t *traffic = ev->traffic;

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.traffic (in_pkt_cnt, in_byte_cnt, out_pkt_cnt, "
                                                   "out_byte_cnt, ctrl_ip) values (%lu, %lu, %lu, %lu, '%s');", 
                                                   traffic->in_pkt_cnt, traffic->in_byte_cnt, traffic->out_pkt_cnt, 
                                                   traffic->out_byte_cnt, ctrl_ip);
            push_query_into_queue(sql_query);
        }
        break;
    case EV_LOG_UPDATE_MSGS:
        PRINT_EV("EV_LOG_UPDATE_MSGS\n");
        {
            const char *ev_log = ev->log;
            char log[__CONF_LSTR_LEN] = {0};

            strcpy(log, ev_log);
            log[strlen(ev_log)-1] = '\0';

            char sql_query[__CONF_LSTR_LEN] = {0};
            snprintf(sql_query, __CONF_LSTR_LEN-1, "insert into Barista.logs (log, ctrl_ip) values ('%s', '%s');", log, ctrl_ip);
            push_query_into_queue(sql_query);
        }
        break;
    default:
        break;
    }

#endif /* __ENABLE_CLUSTER */

    return 0;
}

/**
 * @}
 *
 * @}
 */
