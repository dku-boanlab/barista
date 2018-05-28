/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app
 * @{
 * \defgroup l2_shortest L2 Shortest-Path
 * \brief (Network) L2 shortest path
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "l2_shortest.h"

/** \brief L2 shortest path ID */
#define L2_SHORTEST_ID 123970670

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a MAC entry */
typedef struct _mac_entry_t {
    uint64_t dpid; /**< Datapath ID */
    uint16_t port; /**< Port */

    uint64_t mac; /**< MAC address */
    uint32_t ip; /**< IP address */

    struct _mac_entry_t *prev; /**< Previous entry */
    struct _mac_entry_t *next; /**< Next entry */

    struct _mac_entry_t *r_next; /**< Next entry for removal */
} mac_entry_t;

/** \brief The structure of a MAC table */
typedef struct _mac_table_t {
    mac_entry_t *head; /**< The head pointer */
    mac_entry_t *tail; /**< The tail pointer */

    pthread_rwlock_t lock; /**< The lock for management */
} mac_table_t;

/** \brief The size of a MAC table */
#define MAC_HASH_SIZE 8192

/** \brief MAC table */
mac_table_t *mac_table;

/////////////////////////////////////////////////////////////////////

#include "mac_queue.h"

/////////////////////////////////////////////////////////////////////

/** \brief The maximum hop counts */
#define MAX_HOP_COUNT 30

/** \brief The infinite number for Floyd-Warshall algorithm */
#define INF 99999

/** \brief The list to convert datapath IDs to small numbers */
uint64_t *dpid_list;

/** \brief The port list to jump to next switches */
int **dport_list;

/** \brief The tables used for Floyd-Warshall algorithm */
int **path, **dist, **next;

/** \brief The spinlock for path updates */
pthread_rwlock_t path_lock;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to calculate shortest paths for all pairs
 * \return None
 */
static int floyd_warshall(void)
{
    // initialze dist and next
    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        int j;
        for (j=0; j<__MAX_NUM_SWITCHES; j++) {
            dist[i][j] = path[i][j];

            if (i != j) {
                next[i][j] = j + 1;
            } else {
                next[i][j] = 0;
            }
        }
    }

    // update dist and next
    int k;
    for (k=0; k<__MAX_NUM_SWITCHES; k++) {
        int i;
        for (i=0; i<__MAX_NUM_SWITCHES; i++) {
            int j;
            for (j=0; j<__MAX_NUM_SWITCHES; j++) {
                if (dist[i][j] > dist[i][k] + dist[k][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    next[i][j] = next[i][k];
                }
            }
        }
    }

    return 0;
}

/**
 * \brief Function to add a new switch
 * \param sw New switch
 */
static int add_switch(const switch_t *sw)
{
    uint64_t dpid = sw->dpid;

    pthread_rwlock_wrlock(&path_lock);

    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        if (dpid_list[i] == dpid) {
            pthread_rwlock_unlock(&path_lock);
            return -1;
        }
    }

    int src = 0;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        if (dpid_list[i] == 0) {
            src = i;
            dpid_list[i] = dpid;
            DEBUG("add -> dpid_list[%d] = %lu\n", i, dpid);
            break;
        }
    }

    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        if (src == i)
            path[src][i] = 0;
        else
            path[src][i] = INF;

        dport_list[src][i] = 0;
    }

    floyd_warshall();

    ALOG_INFO(L2_SHORTEST_ID, "Recalculated shortest-paths");

    pthread_rwlock_unlock(&path_lock);

    return 0;
}

/**
 * \brief Function to remove an existing switch
 * \param sw Existing switch
 */
static int del_switch(const switch_t *sw)
{
    uint64_t dpid = sw->dpid;

    pthread_rwlock_wrlock(&path_lock);

    int i, src = 0;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        if (dpid_list[i] == dpid) {
            src = i;
            dpid_list[i] = 0;
            DEBUG("del -> dpid_list[%d]\n", i);
            break;
        }
    }

    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        if (src == i)
            path[src][i] = 0;
        else
            path[src][i] = INF;

        dport_list[src][i] = 0;
    }

    floyd_warshall();

    ALOG_INFO(L2_SHORTEST_ID, "Recalculated shortest-paths");

    pthread_rwlock_unlock(&path_lock);

    return 0;
}

/**
 * \brief Function to get an index where a datapath ID is located
 * \param dpid Datapath ID
 */
static int get_index_from_dpid(uint64_t dpid)
{
    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        if (dpid_list[i] == dpid) {
            return i;
        }
    }

    return 0;
}

/**
 * \brief Function to add a new link
 * \param link New link
 */
static int add_link(const port_t *link)
{
    pthread_rwlock_wrlock(&path_lock);

    int src = get_index_from_dpid(link->dpid);
    int dst = get_index_from_dpid(link->target_dpid);

    DEBUG("add -> src: %d, dst: %d\n", src, dst);

    path[src][dst] = src + dst;
    dport_list[src][dst] = link->port;

    floyd_warshall();

    ALOG_INFO(L2_SHORTEST_ID, "Recalculated shortest-paths");

    pthread_rwlock_unlock(&path_lock);

    return 0;
}

/**
 * \brief Function to remove an existing link
 * \param link Existing link
 */
static int del_link(const port_t *link)
{
    pthread_rwlock_wrlock(&path_lock);

    int src = get_index_from_dpid(link->dpid);
    int dst = get_index_from_dpid(link->target_dpid);

    DEBUG("del -> src: %d, dst: %d\n", src, dst);

    path[src][dst] = INF;
    dport_list[src][dst] = 0;

    floyd_warshall();

    ALOG_INFO(L2_SHORTEST_ID, "Recalculated shortest-paths");

    pthread_rwlock_unlock(&path_lock);

    return 0;
}

/**
 * \brief Function to get the shortest path from source to destination
 * \param route The list to store the shortest path from source to destination
 * \param src_dpid Source DPID
 * \param dst_dpid Destination DPID
 */
static int shortest_path(int *route, uint64_t src_dpid, uint64_t dst_dpid)
{
    pthread_rwlock_rdlock(&path_lock);

    int src = get_index_from_dpid(src_dpid);
    int dst = get_index_from_dpid(dst_dpid);

    int u = src + 1;
    int v = dst + 1;

    if (dist[src][dst] == INF) {
        pthread_rwlock_unlock(&path_lock);
        return -1;
    }

    int hop = 0;
    route[hop++] = u-1;

    DEBUG("%d->%d %2d %d", u-1, v-1, dist[src][dst], u-1);

    do {
        u = next[u-1][v-1];
        route[hop++] = u-1;
        DEBUG("->%d", u-1);
    } while (u != v);

    DEBUG("\n");

    pthread_rwlock_unlock(&path_lock);

    return hop;
}

/**
 * \brief Function to initialize tables for Floyd-Warshall algorithm
 * \return None
 */
static int path_init(void)
{
    dpid_list = (uint64_t *)MALLOC(sizeof(uint64_t) * __DEFAULT_TABLE_SIZE);
    if (dpid_list == NULL) {
        PERROR("malloc");
        return -1;
    } else {
        memset(dpid_list, 0, sizeof(uint64_t) * __DEFAULT_TABLE_SIZE);
    }

    dport_list = (int **)MALLOC(sizeof(int *) * __MAX_NUM_SWITCHES);
    if (dport_list == NULL) {
        PERROR("malloc");
        return -1;
    } else {
        int i;
        for (i=0; i<__MAX_NUM_SWITCHES; i++) {
            dport_list[i] = (int *)MALLOC(sizeof(int) * __MAX_NUM_SWITCHES);
            if (dport_list[i] == NULL) {
                PERROR("malloc");
                return -1;
            } else {
                memset(dport_list[i], 0, sizeof(int) * __MAX_NUM_SWITCHES);
            }
        }
    }

    path = (int **)MALLOC(sizeof(int *) * __MAX_NUM_SWITCHES);
    if (path == NULL) {
        PERROR("malloc");
        return -1;
    } else {
        int i;
        for (i=0; i<__MAX_NUM_SWITCHES; i++) {
            path[i] = (int *)MALLOC(sizeof(int) * __MAX_NUM_SWITCHES);
            if (path[i] == NULL) {
                PERROR("malloc");
                return -1;
            } else {
                int j;
                for (j=0; j<__MAX_NUM_SWITCHES; j++) {
                    if (i == j) {
                        path[i][j] = 0;
                    } else {
                        path[i][j] = INF;
                    }
                }
            }
        }
    }

    dist = (int **)MALLOC(sizeof(int *) * __MAX_NUM_SWITCHES);
    if (dist == NULL) {
        PERROR("malloc");
        return -1;
    } else {
        int i;
        for (i=0; i<__MAX_NUM_SWITCHES; i++) {
            dist[i] = (int *)MALLOC(sizeof(int) * __MAX_NUM_SWITCHES);
            if (dist[i] == NULL) {
                PERROR("malloc");
                return -1;
            }
        }
    }

    next = (int **)MALLOC(sizeof(int *) * __MAX_NUM_SWITCHES);
    if (next == NULL) {
        PERROR("malloc");
        return -1;
    } else {
        int i;
        for (i=0; i<__MAX_NUM_SWITCHES; i++) {
            next[i] = (int *)MALLOC(sizeof(int) * __MAX_NUM_SWITCHES);
            if (next[i] == NULL) {
                PERROR("malloc");
                return -1;
            }
        }
    }

    pthread_rwlock_init(&path_lock, NULL);

    return 0;
}

/**
 * \brief Function to destroy the tables for Floyd-Warshall algorithm
 * \return None
 */
static int path_destroy(void)
{
    pthread_rwlock_wrlock(&path_lock);

    FREE(dpid_list);

    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        FREE(dport_list[i]);
    }

    FREE(dport_list);

    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        FREE(path[i]);
        FREE(dist[i]);
        FREE(next[i]);
    }

    FREE(path);
    FREE(dist);
    FREE(next);

    pthread_rwlock_unlock(&path_lock);
    pthread_rwlock_destroy(&path_lock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to conduct an output action into the data plane
 * \param pktin Pktin message
 * \param port Port
 */
static int send_packet(const pktin_t *pktin, uint16_t port)
{
    pktout_t out = {0};

    PKTOUT_INIT(out, pktin);

    out.num_actions = 1;
    out.action[0].type = ACTION_OUTPUT;
    out.action[0].port = port;

    av_dp_send_packet(L2_SHORTEST_ID, &out);

    return 0;
}

/**
 * \brief Function to insert a flow rule into the data plane
 * \param pktin Pktin message
 * \param port Port
 */
static int insert_flow(const pktin_t *pktin, uint16_t port)
{
    flow_t out = {0};

    FLOW_INIT(out, pktin);

    out.idle_timeout = DEFAULT_IDLE_TIMEOUT;
    out.hard_timeout = DEFAULT_HARD_TIMEOUT;
    out.priority = DEFAULT_PRIORITY;

    out.num_actions = 1;
    out.action[0].type = ACTION_OUTPUT;
    out.action[0].port = port;

    av_dp_insert_flow(L2_SHORTEST_ID, &out);

    return 0;
}

/**
 * \brief Function to insert a flow rule to forward packets to the next hop
 * \param pktin Pktin message
 * \param hop The number of hops from source to destination
 * \param route The shortest path from source to destination
 * \param port The port where the destination is connected
 */
static int insert_flow_next(const pktin_t *pktin, int hop, int *route, uint16_t port)
{
    flow_t out = {0};

    FLOW_INIT(out, pktin);

    out.idle_timeout = DEFAULT_IDLE_TIMEOUT;
    out.hard_timeout = DEFAULT_HARD_TIMEOUT;
    out.priority = DEFAULT_PRIORITY;

    out.num_actions = 1;
    out.action[0].type = ACTION_OUTPUT;
    out.action[0].port = dport_list[route[0]][route[1]];

    av_dp_insert_flow(L2_SHORTEST_ID, &out);

    return 0;
}

/**
 * \brief Function to conduct a drop action into the data plane
 * \param pktin Pktin message
 */
static int discard_packet(const pktin_t *pktin)
{
    pktout_t out = {0};

    PKTOUT_INIT(out, pktin);

    out.num_actions = 1;
    out.action[0].type = ACTION_DISCARD;

    av_dp_send_packet(L2_SHORTEST_ID, &out);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle pktin messages
 * \param pktin Pktin message
 */
static int l2_shortest(const pktin_t *pktin)
{
#ifndef __ENABLE_CBENCH
    uint64_t mac = mac2int(pktin->src_mac);
    uint32_t mkey = hash_func((uint32_t *)&mac, 2) % MAC_HASH_SIZE;

    pthread_rwlock_rdlock(&mac_table[mkey].lock);

    // check source MAC
    mac_entry_t *curr = mac_table[mkey].head;
    while (curr != NULL) {
        if (curr->mac == mac) {
            break;
        }
        curr = curr->next;
    }

    pthread_rwlock_unlock(&mac_table[mkey].lock);

    // new MAC?
    if (curr == NULL) {
        curr = mac_dequeue();
        if (curr == NULL) {
            discard_packet(pktin);
            return -1;
        }

        curr->dpid = pktin->dpid;
        curr->port = pktin->port;

        curr->ip = pktin->src_ip;
        curr->mac = mac;

        curr->prev = curr->next = NULL;

        pthread_rwlock_wrlock(&mac_table[mkey].lock);

        if (mac_table[mkey].head == NULL) {
            mac_table[mkey].head = curr;
            mac_table[mkey].tail = curr;
        } else {
            curr->prev = mac_table[mkey].tail;
            mac_table[mkey].tail->next = curr;
            mac_table[mkey].tail = curr;
        }

        pthread_rwlock_unlock(&mac_table[mkey].lock);
    }

    // get destination MAC
    mac = mac2int(pktin->dst_mac);

    // broadcast?
    if (mac == 0xffffffffffff) {
        int pass = FALSE;

        int i;
        for (i=0; i<MAC_HASH_SIZE; i++) {
            pthread_rwlock_rdlock(&mac_table[i].lock);

            mac_entry_t *curr = mac_table[i].head;
            while (curr != NULL) {
                if (curr->ip == pktin->dst_ip) {
                    mac = curr->mac;
                    pass = TRUE;
                    break;
                }

                curr = curr->next;
            }

            pthread_rwlock_unlock(&mac_table[i].lock);

            if (pass)
                break;
        }

        if (!pass) {
            send_packet(pktin, PORT_FLOOD);
            return 0;
        }
    }

    uint64_t dpid = 0;
    uint16_t port = 0;

    mkey = hash_func((uint32_t *)&mac, 2) % MAC_HASH_SIZE;

    pthread_rwlock_rdlock(&mac_table[mkey].lock);

    // known destination?
    curr = mac_table[mkey].head;
    while (curr != NULL) {
        if (curr->mac == mac) {
            dpid = curr->dpid;
            port = curr->port;
            break;
        }
        curr = curr->next;
    }

    pthread_rwlock_unlock(&mac_table[mkey].lock);

    // unknown destination?
    if (port == 0) {
        send_packet(pktin, PORT_FLOOD);
        return 0;
    }

    // source dpid is equal to destination dpid?
    if (pktin->dpid == dpid) {
        if (pktin->proto & PROTO_ARP) {
            send_packet(pktin, port);
        } else {
            insert_flow(pktin, port);
        }
        return 0;
    } else {
        // get the shortest path from src to dst
        int route[MAX_HOP_COUNT] = {0};
        int hop = shortest_path(route, pktin->dpid, dpid);

        if (hop > 0) {
            insert_flow_next(pktin, hop, route, port);
        } else {
            discard_packet(pktin);
        }
    }

    return 0;
#else /* __ENABLE_CBENCH */
    send_packet(pktin, PORT_FLOOD);
    return 0;
#endif /* __ENABLE_CBENCH */
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int l2_shortest_main(int *activated, int argc, char **argv)
{
    ALOG_INFO(L2_SHORTEST_ID, "Init - L2 shortest-path");

    mac_table = (mac_table_t *)CALLOC(MAC_HASH_SIZE, sizeof(mac_table_t));
    if (mac_table == NULL) {
        PERROR("calloc");
        return -1;
    }

    int i;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_init(&mac_table[i].lock, NULL);
    }

    mac_q_init();

    path_init();

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int l2_shortest_cleanup(int *activated)
{
    ALOG_INFO(L2_SHORTEST_ID, "Clean up - L2 shortest-path");

    deactivate();

    int i;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_wrlock(&mac_table[i].lock);

        mac_entry_t *curr = mac_table[i].head;
        while (curr != NULL) {
            mac_entry_t *tmp = curr;
            curr = curr->next;
            FREE(tmp);
        }

        pthread_rwlock_unlock(&mac_table[i].lock);
        pthread_rwlock_destroy(&mac_table[i].lock);
    }

    mac_q_destroy();

    path_destroy();

    FREE(mac_table);

    return 0;
}

/**
 * \brief Function to list up all MAC tables
 * \param cli CLI pointer
 */
static int list_all_entries(cli_t *cli)
{
    cli_print(cli, "<MAC Tables>");

    int i, cnt = 0;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_rdlock(&mac_table[i].lock);

        mac_entry_t *curr = mac_table[i].head;
        while (curr != NULL) {
            uint8_t mac[ETH_ALEN];
            int2mac(curr->mac, mac);

            char macaddr[__CONF_WORD_LEN] = {0};
            mac2str(mac, macaddr);

            struct in_addr ip_addr;
            ip_addr.s_addr = curr->ip;

            cli_print(cli, "  %lu: %s (%s), %u", curr->dpid, macaddr, inet_ntoa(ip_addr), curr->port);

            cnt++;

            curr = curr->next;
        }

        pthread_rwlock_unlock(&mac_table[i].lock);
    }

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief Function to show the MAC table for a specific switch
 * \param cli CLI pointer
 * \param dpid_str Datapath ID
 */
static int show_entry_switch(cli_t *cli, char *dpid_str)
{
    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "<MAC Table for Switch [%lu]>", dpid);

    int i, cnt = 0;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_rdlock(&mac_table[i].lock);

        mac_entry_t *curr = mac_table[i].head;
        while (curr != NULL) {
            if (curr->dpid == dpid) {
                uint8_t mac[ETH_ALEN];
                int2mac(curr->mac, mac);

                char macaddr[__CONF_WORD_LEN] = {0};
                mac2str(mac, macaddr);

                struct in_addr ip_addr;
                ip_addr.s_addr = curr->ip;

                cli_print(cli, "  %lu: %s (%s), %u", curr->dpid, macaddr, inet_ntoa(ip_addr), curr->port);

                cnt++;
            }

            curr = curr->next;
        }

        pthread_rwlock_unlock(&mac_table[i].lock);
    }

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief Function to find a MAC entry with a MAC address
 * \param cli CLI pointer
 * \param macaddr MAC address
 */
static int show_entry_mac(cli_t *cli, const char *macaddr)
{
    uint8_t mac[ETH_ALEN] = {0};

    str2mac(macaddr, mac);

    uint64_t imac = mac2int(mac);

    cli_print(cli, "<MAC Entry [%s]>", macaddr);

    int i, cnt = 0;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_rdlock(&mac_table[i].lock);

        mac_entry_t *curr = mac_table[i].head;
        while (curr != NULL) {
            if (curr->mac == imac) {
                struct in_addr ip_addr;
                ip_addr.s_addr = curr->ip;

                cli_print(cli, "  %lu: %s (%s), %u", curr->dpid, macaddr, inet_ntoa(ip_addr), curr->port);

                cnt++;
            }

            curr = curr->next;
        }

        pthread_rwlock_unlock(&mac_table[i].lock);
    }

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief Function to find a MAC entry with an IP address
 * \param cli CLI pointer
 * \param ipaddr IP address
 */
static int show_entry_ip(cli_t *cli, const char *ipaddr)
{
    struct in_addr ip_addr;
    inet_aton(ipaddr, &ip_addr);

    uint32_t ip = ip_addr.s_addr;

    cli_print(cli, "<MAC Entry [%s]>", ipaddr);

    int i, cnt = 0;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_rdlock(&mac_table[i].lock);

        mac_entry_t *curr = mac_table[i].head;
        while (curr != NULL) {
            if (curr->ip == ip) {
                uint8_t mac[ETH_ALEN];
                int2mac(curr->mac, mac);

                char macaddr[__CONF_WORD_LEN] = {0};
                mac2str(mac, macaddr);

                cli_print(cli, "  %lu: %s (%s), %u", curr->dpid, macaddr, ipaddr, curr->port);

                cnt++;
            }

            curr = curr->next;
        }

        pthread_rwlock_unlock(&mac_table[i].lock);
    }

    if (!cnt)
        cli_print(cli, "  No entry");

    return 0;
}

/**
 * \brief The CLI function
 * \param cli CLI pointer
 * \param args Arguments
 */
int l2_shortest_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0) {
        if (args[1] != NULL && strcmp(args[1], "tables") == 0 && args[2] == NULL) {
            list_all_entries(cli);
            return 0;
        }
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0) {
        if (args[1] != NULL && strcmp(args[1], "switch") == 0 && args[2] != NULL && args[3] == NULL) {
            show_entry_switch(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "mac") == 0 && args[2] != NULL && args[3] == NULL) {
            show_entry_mac(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "ip") == 0 && args[2] != NULL && args[3] == NULL) {
            show_entry_ip(cli, args[2]);
            return 0;
        }
    }

    cli_print(cli, "<Available Commands>");
    cli_print(cli, "  l2_shortest list tables");
    cli_print(cli, "  l2_shortest show switch [DPID]");
    cli_print(cli, "  l2_shortest show mac [MAC address]");
    cli_print(cli, "  l2_shortest show ip [IP address]");

    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int l2_shortest_handler(const app_event_t *av, app_event_out_t *av_out)
{
    switch (av->type) {
    case AV_DP_RECEIVE_PACKET:
        PRINT_EV("AV_DP_RECEIVE_PACKET\n");
        {
            const pktin_t *pktin = av->pktin;

            l2_shortest(pktin);
        }
        break;
    case AV_DP_PORT_ADDED:
        PRINT_EV("AV_DP_PORT_ADDED\n");
        {
            const port_t *port = av->port;

            int i;
            for (i=0; i<MAC_HASH_SIZE; i++) {
                mac_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&mac_table[i].lock);

                mac_entry_t *curr = mac_table[i].head;
                while (curr != NULL) {
                    if (curr->dpid == port->dpid && curr->port == port->port) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }
                    }

                    curr = curr->next;
                }

                curr = tmp_list.head;
                while (curr != NULL) {
                    mac_entry_t *tmp = curr;

                    curr = curr->r_next;

                    if (tmp->prev != NULL && tmp->next != NULL) {
                        tmp->prev->next = tmp->next;
                        tmp->next->prev = tmp->prev;
                    } else if (tmp->prev == NULL && tmp->next != NULL) {
                        mac_table[i].head = tmp->next;
                        tmp->next->prev = NULL;
                    } else if (tmp->prev != NULL && tmp->next == NULL) {
                        mac_table[i].tail = tmp->prev;
                        tmp->prev->next = NULL;
                    } else if (tmp->prev == NULL && tmp->next == NULL) {
                        mac_table[i].head = NULL;
                        mac_table[i].tail = NULL;
                    }

                    mac_enqueue(tmp);
                }

                pthread_rwlock_unlock(&mac_table[i].lock);
            }
        }
        break;
    case AV_DP_PORT_DELETED:
        PRINT_EV("AV_DP_PORT_DELETED\n");
        {
            const port_t *port = av->port;

            int i;
            for (i=0; i<MAC_HASH_SIZE; i++) {
                mac_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&mac_table[i].lock);

                mac_entry_t *curr = mac_table[i].head;
                while (curr != NULL) {
                    if (curr->dpid == port->dpid && curr->port == port->port) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }
                    }

                    curr = curr->next;
                }

                curr = tmp_list.head;
                while (curr != NULL) {
                    mac_entry_t *tmp = curr;

                    curr = curr->r_next;

                    if (tmp->prev != NULL && tmp->next != NULL) {
                        tmp->prev->next = tmp->next;
                        tmp->next->prev = tmp->prev;
                    } else if (tmp->prev == NULL && tmp->next != NULL) {
                        mac_table[i].head = tmp->next;
                        tmp->next->prev = NULL;
                    } else if (tmp->prev != NULL && tmp->next == NULL) {
                        mac_table[i].tail = tmp->prev;
                        tmp->prev->next = NULL;
                    } else if (tmp->prev == NULL && tmp->next == NULL) {
                        mac_table[i].head = NULL;
                        mac_table[i].tail = NULL;
                    }

                    mac_enqueue(tmp);
                }

                pthread_rwlock_unlock(&mac_table[i].lock);
            }
        }
        break;
    case AV_SW_CONNECTED:
        PRINT_EV("AV_SW_CONNECTED\n");
        {
            const switch_t *sw = av->sw;

            add_switch(sw);
        }
        break;
    case AV_SW_DISCONNECTED:
        PRINT_EV("AV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = av->sw;

            int i;
            for (i=0; i<MAC_HASH_SIZE; i++) {
                mac_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&mac_table[i].lock);

                mac_entry_t *curr = mac_table[i].head;
                while (curr != NULL) {
                    if (curr->dpid == sw->dpid) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }
                    }

                    curr = curr->next;
                }

                curr = tmp_list.head;
                while (curr != NULL) {
                    mac_entry_t *tmp = curr;

                    curr = curr->next;

                    if (tmp->prev != NULL && tmp->next != NULL) {
                        tmp->prev->next = tmp->next;
                        tmp->next->prev = tmp->prev;
                    } else if (tmp->prev == NULL && tmp->next != NULL) {
                        mac_table[i].head = tmp->next;
                        tmp->next->prev = NULL;
                    } else if (tmp->prev != NULL && tmp->next == NULL) {
                        mac_table[i].tail = tmp->prev;
                        tmp->prev->next = NULL;
                    } else if (tmp->prev == NULL && tmp->next == NULL) {
                        mac_table[i].head = NULL;
                        mac_table[i].tail = NULL;
                    }

                    mac_enqueue(tmp);
                }

                pthread_rwlock_unlock(&mac_table[i].lock);
            }

            del_switch(sw);
        }
        break;
    case AV_LINK_ADDED:
        PRINT_EV("AV_LINK_ADDED\n");
        {
            const port_t *link = av->port;

            add_link(link);
        }
        break;
    case AV_LINK_DELETED:
        PRINT_EV("AV_LINK_DELETED\n");
        {
            const port_t *link = av->port;

            del_link(link);
        }
        break;
    default:
        break;
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */
