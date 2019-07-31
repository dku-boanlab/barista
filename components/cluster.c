/*
 * Copyright 2015-2019 NSSLab, KAIST
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

static char hostname[__CONF_SHORT_LEN];

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to flush all events in a queue
 * \return None
 */
static int flush_events_in_queue()
{
    if (num_cluster_events) {
        database_t cluster_db;

        if (init_database(&cluster_info, &cluster_db)) {
            LOG_ERROR(CLUSTER_ID, "Failed to connect a cluster database");
            destroy_database(&cluster_db);
            return -1;
        }

        execute_query(&cluster_db, "START TRANSACTION");

        int i;
        for (i=0; i<num_cluster_events; i++) {
            char query[__CONF_LONG_STR_LEN];
            sprintf(query, "insert into cluster_events (EV_ID, EV_TYPE, EV_LENGTH, DATA, INSTANCE) values (%u, %u, %u, '%s', '%s')",
                    cluster_events[i].id, cluster_events[i].type, cluster_events[i].length, cluster_events[i].data, hostname);

            execute_query(&cluster_db, query);
        }

        execute_query(&cluster_db, "COMMIT");

        destroy_database(&cluster_db);

        memset(cluster_events, 0, num_cluster_events * sizeof(cluster_event_t));
        num_cluster_events = 0;
    }

    return 0;
}

/**
 * \brief Function to encode an event and push it into a queue
 * \param ev Event
 */
static int insert_event_data(const event_t *ev)
{
    char data[__CLUSTER_EVENT_SIZE];
    base64_encode_w_buffer((char *)ev->data, ev->length, data);

    pthread_spin_lock(&cluster_lock);

    if (num_cluster_events == __CLUSTER_BATCH_SIZE)
        flush_events_in_queue();

    cluster_events[num_cluster_events].id = ev->id;
    cluster_events[num_cluster_events].type = ev->type;
    cluster_events[num_cluster_events].length = ev->length;
    strcpy(cluster_events[num_cluster_events].data, data);

    num_cluster_events++;

    pthread_spin_unlock(&cluster_lock);

    return 0;
}

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

    if (hostname[0] == '\0')
        gethostname(hostname, __CONF_SHORT_LEN);

    if (get_database_info(&cluster_info, "barista_mgmt")) {
        LOG_ERROR(CLUSTER_ID, "Failed to get the information of a cluster database");
        return -1;
    }

    reset_table(&cluster_info, "cluster_events", FALSE);

    num_cluster_events = 0;
    cluster_events = (cluster_event_t *)CALLOC(__CLUSTER_BATCH_SIZE, sizeof(cluster_event_t));
    if (cluster_events == NULL) {
        LOG_ERROR(CLUSTER_ID, "calloc() failed");
        return -1;
    }

    pthread_spin_init(&cluster_lock, PTHREAD_PROCESS_PRIVATE);

    database_t cluster_db;

    if (init_database(&cluster_info, &cluster_db)) {
        LOG_ERROR(CLUSTER_ID, "Failed to connect a cluster database");
        destroy_database(&cluster_db);
    }

    activate();

    while (*activated) {
        // push data

        pthread_spin_lock(&cluster_lock);

        flush_events_in_queue();

        pthread_spin_unlock(&cluster_lock);

        // pop data

        char conditions[__CONF_STR_LEN];
        sprintf(conditions, "ID > %lu", event_num);

        char query[__CONF_STR_LEN];
        sprintf(query, "select ID, EV_ID, EV_TYPE, DATA from cluster_events where ID > %lu and INSTANCE != '%s'", event_num, hostname);

        if (execute_query(&cluster_db, query)) continue;

        query_result_t *result = get_query_result(&cluster_db);
        query_row_t row;

        while ((row = fetch_query_row(result)) != NULL) {
            if (*activated == FALSE) break;

            event_num = strtoull(row[0], NULL, 0);

            uint32_t id = strtoul(row[1], NULL, 0);
            uint32_t type = strtoul(row[2], NULL, 0);

            char data[__CONF_LONG_STR_LEN] = {0};
            base64_decode_w_buffer(row[3], data);

            switch (type) {
            case EV_SW_CONNECTED:
                {
                    switch_t *sw = (switch_t *)data;
                    sw->remote = TRUE;
                    ev_sw_connected(id, sw);
                }
                break;
            case EV_SW_DISCONNECTED:
                {
                    switch_t *sw = (switch_t *)data;
                    sw->remote = TRUE;
                    ev_sw_disconnected(id, sw);
                }
                break;
            case EV_HOST_ADDED:
                {
                    host_t *host = (host_t *)data;
                    host->remote = TRUE;
                    ev_host_added(id, host);
                }
                break;
            case EV_HOST_DELETED:
                {
                    host_t *host = (host_t *)data;
                    host->remote = TRUE;
                    ev_host_deleted(id, host);
                }
                break;
            case EV_LINK_ADDED:
                {
                    port_t *link = (port_t *)data;
                    link->remote = TRUE;
                    ev_link_added(id, link);
                }
                break;
            case EV_LINK_DELETED:
                {
                    port_t *link = (port_t *)data;
                    link->remote = TRUE;
                    ev_link_deleted(id, link);
                }
                break;
            case EV_FLOW_ADDED:
                {
                    flow_t *flow = (flow_t *)data;
                    flow->remote = TRUE;
                    ev_flow_added(id, flow);
                }
                break;
            case EV_FLOW_MODIFIED:
                {
                    flow_t *flow = (flow_t *)data;
                    flow->remote = TRUE;
                    ev_flow_modified(id, flow);
                }
                break;
            case EV_FLOW_DELETED:
                {
                    flow_t *flow = (flow_t *)data;
                    flow->remote = TRUE;
                    ev_flow_deleted(id, flow);
                }
                break;
            default:
                break;
            }
        }

        release_query_result(result);

        int i;
        for (i=0; i<__CLUSTER_UPDATE_TIME; i++) {
            if (*activated == FALSE) break;
            else waitsec(1, 0);
        }
    }

    destroy_database(&cluster_db);

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

    pthread_spin_lock(&cluster_lock);

    flush_events_in_queue();

    pthread_spin_unlock(&cluster_lock);
    pthread_spin_destroy(&cluster_lock);

    FREE(cluster_events);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int cluster_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int cluster_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_SW_CONNECTED:
    case EV_SW_DISCONNECTED:
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == TRUE) break;

            insert_event_data(ev);
        }
        break;
    case EV_HOST_ADDED:
    case EV_HOST_DELETED:
        {
            const host_t *host = ev->host;

            if (host->remote == TRUE) break;

            insert_event_data(ev);
        }
        break;
    case EV_LINK_ADDED:
    case EV_LINK_DELETED:
        {
            const port_t *link = ev->port;

            if (link->remote == TRUE) break;

            insert_event_data(ev);
        }
        break;
    case EV_FLOW_ADDED:
    case EV_FLOW_MODIFIED:
    case EV_FLOW_DELETED:
        {
            const flow_t *flow = ev->flow;

            if (flow->remote == TRUE) break;

            insert_event_data(ev);
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
