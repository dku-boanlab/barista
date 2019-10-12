/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup flow_mgmt Flow Management
 * \brief (Management) flow management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "flow_mgmt.h"

/** \brief Flow management ID */
#define FLOW_MGMT_ID 635676162

/////////////////////////////////////////////////////////////////////

/* The flag to enable the CBENCH mode */
int cbench_enabled;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to add a flow
 * \param list Flow table mapped to a datapath ID
 * \param flow NEW flow
 */
static int add_flow(flow_table_t *list, const flow_t *flow)
{
    if (cbench_enabled)
        return 0;

    pthread_spin_lock(&list->lock);

    flow_t *curr = list->head;
    while (curr != NULL) {
        if (FLOW_COMPARE(curr, flow)) {
            curr->insert_time = time(NULL);
            break;
        }

        curr = curr->next;
    }

    pthread_spin_unlock(&list->lock);

    if (curr == NULL) {
        flow_t *new = flow_dequeue();
        if (new == NULL) {
            LOG_ERROR(FLOW_MGMT_ID, "flow_dequeue() failed");
            return -1;
        }

        new->dpid = flow->dpid;
        new->port = flow->port;

        new->remote = flow->remote;
        new->insert_time = time(NULL);

        new->meta.idle_timeout = flow->meta.idle_timeout;
        new->meta.hard_timeout = flow->meta.hard_timeout;

        memmove(&new->match, &flow->match, sizeof(pkt_info_t));

        new->num_actions = flow->num_actions;
        memmove(new->action, flow->action, sizeof(action_t) * new->num_actions);

        pthread_spin_lock(&list->lock);

        if (list->head == NULL) {
            list->head = new;
            list->tail = new;
        } else {
            new->prev = list->tail;
            list->tail->next = new;
            list->tail = new;
        }

        pthread_spin_unlock(&list->lock);

        if (new->remote == FALSE)
            ev_flow_added(FLOW_MGMT_ID, new);
    }

    return 0;
}

/**
 * \brief Function to modify a flow
 * \param list Flow table mapped to a datapath ID
 * \param flow Existing flow to modify
 */
static int modify_flow(flow_table_t *list, const flow_t *flow)
{
    return 0;
}

/**
 * \brief Function to delete a flow
 * \param list Flow table mapped to a datapath ID
 * \param flow Existing flow to delete
 */
static int delete_flow(flow_table_t *list, const flow_t *flow)
{
    flow_table_t tmp_list = {0};

    pthread_spin_lock(&list->lock);

    flow_t *curr = list->head;
    while (curr != NULL) {
        if (FLOW_COMPARE(curr, flow)) {
            flow_t *tmp = curr;

            curr = curr->next;

            if (tmp->prev != NULL && tmp->next != NULL) {
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
            } else if (tmp->prev == NULL && tmp->next != NULL) {
                list->head = tmp->next;
                tmp->next->prev = NULL;
            } else if (tmp->prev != NULL && tmp->next == NULL) {
                list->tail = tmp->prev;
                tmp->prev->next = NULL;
            } else if (tmp->prev == NULL && tmp->next == NULL) {
                list->head = NULL;
                list->tail = NULL;
            }

            if (tmp_list.head == NULL) {
                tmp_list.head = tmp;
                tmp_list.tail = tmp;
                tmp->prev = NULL;
                tmp->next = NULL;
            } else {
                tmp_list.tail->next = tmp;
                tmp->prev = tmp_list.tail;
                tmp_list.tail = tmp;
                tmp->next = NULL;
            }
        } else {
            curr = curr->next;
        }
    }

    pthread_spin_unlock(&list->lock);

    curr = tmp_list.head;
    while (curr != NULL) {
        flow_t *tmp = curr;

        curr = curr->next;

        if (tmp->remote == FALSE)
            ev_flow_deleted(FLOW_MGMT_ID, tmp);

        flow_enqueue(tmp);
    }

    return 0;
}

/**
 * \brief Function to delete all flows
 * \param list Flow table mapped to a datapath ID
 * \param dpid Datapath ID
 */
static int delete_all_flows(flow_table_t *list, const uint64_t dpid)
{
    flow_table_t tmp_list = {0};

    pthread_spin_lock(&list->lock);

    flow_t *curr = list->head;
    while (curr != NULL) {
        if (curr->dpid == dpid) {
            flow_t *tmp = curr;

            curr = curr->next;

            if (tmp->prev != NULL && tmp->next != NULL) {
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
            } else if (tmp->prev == NULL && tmp->next != NULL) {
                list->head = tmp->next;
                tmp->next->prev = NULL;
            } else if (tmp->prev != NULL && tmp->next == NULL) {
                list->tail = tmp->prev;
                tmp->prev->next = NULL;
            } else if (tmp->prev == NULL && tmp->next == NULL) {
                list->head = NULL;
                list->tail = NULL;
            }

            if (tmp_list.head == NULL) {
                tmp_list.head = tmp;
                tmp_list.tail = tmp;
                tmp->prev = NULL;
                tmp->next = NULL;
            } else {
                tmp_list.tail->next = tmp;
                tmp->prev = tmp_list.tail;
                tmp_list.tail = tmp;
                tmp->next = NULL;
            }
        } else {
            curr = curr->next;
        }
    }

    pthread_spin_unlock(&list->lock);

    curr = tmp_list.head;
    while (curr != NULL) {
        flow_t *tmp = curr;

        curr = curr->next;

        if (tmp->remote == FALSE)
            ev_flow_deleted(FLOW_MGMT_ID, tmp);

        flow_enqueue(tmp);
    }

    return 0;
}

/**
 * \brief Function to update a flow
 * \param list Flow table mapped to a datapath ID
 * \param flow Existing flow
 */
static int update_flow(flow_table_t *list, const flow_t *flow)
{
    pthread_spin_lock(&list->lock);

    flow_t *curr = list->head;
    while (curr != NULL) {
        if (FLOW_COMPARE(curr, flow)) {
            curr->stat.pkt_count += flow->stat.pkt_count;
            curr->stat.byte_count += flow->stat.byte_count;
            break;
        }

        curr = curr->next;
    }

    pthread_spin_unlock(&list->lock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/** \brief The running flag for flow management */
int timeout_thread_on;

/**
 * \brief Function to find and delete expired flows
 * \return NULL
 */
void *timeout_thread(void *arg)
{
    while (timeout_thread_on) {
        time_t current_time = time(NULL);

        int i;
        for (i=0; i<__MAX_NUM_SWITCHES; i++) {
            flow_table_t tmp_list = {0};

            pthread_spin_lock(&flow_table[i].lock);

            flow_t *curr = flow_table[i].head;
            while (curr != NULL) {
                if ((current_time - curr->insert_time) > curr->meta.hard_timeout + 2) {
                    flow_t *tmp = curr;

                    curr = curr->next;

                    if (tmp->prev != NULL && tmp->next != NULL) {
                        tmp->prev->next = tmp->next;
                        tmp->next->prev = tmp->prev;
                    } else if (tmp->prev == NULL && tmp->next != NULL) {
                        flow_table[i].head = tmp->next;
                        tmp->next->prev = NULL;
                    } else if (tmp->prev != NULL && tmp->next == NULL) {
                        flow_table[i].tail = tmp->prev;
                        tmp->prev->next = NULL;
                    } else if (tmp->prev == NULL && tmp->next == NULL) {
                        flow_table[i].head = NULL;
                        flow_table[i].tail = NULL;
                    }

                    if (tmp_list.head == NULL) {
                        tmp_list.head = tmp;
                        tmp_list.tail = tmp;
                        tmp->prev = NULL;
                        tmp->next = NULL;
                    } else {
                        tmp_list.tail->next = tmp;
                        tmp->prev = tmp_list.tail;
                        tmp_list.tail = tmp;
                        tmp->next = NULL;
                    }
                } else {
                    ev_dp_request_flow_stats(FLOW_MGMT_ID, curr);

                    curr = curr->next;
                }
            }

            pthread_spin_unlock(&flow_table[i].lock);

            curr = tmp_list.head;
            while (curr != NULL) {
                flow_t *tmp = curr;

                curr = curr->next;

                if (tmp->remote == FALSE)
                    ev_flow_deleted(FLOW_MGMT_ID, tmp);

                flow_enqueue(tmp);
            }
        }

        waitsec(FLOW_MGMT_UPDATE_TIME, 0);
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int flow_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(FLOW_MGMT_ID, "Init - Flow management");

    const char *CBENCH = getenv("CBENCH");
    if (CBENCH != NULL && strcmp(CBENCH, "CBENCH") == 0)
        cbench_enabled = TRUE;

    timeout_thread_on = TRUE;

    flow_table = (flow_table_t *)CALLOC(__MAX_NUM_SWITCHES, sizeof(flow_table_t));
    if (flow_table == NULL) {
        LOG_ERROR(FLOW_MGMT_ID, "calloc() failed");
        return -1;
    }

    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        pthread_spin_init(&flow_table[i].lock, PTHREAD_PROCESS_PRIVATE);
    }

    flow_q_init();

    pthread_t thread;
    if (pthread_create(&thread, NULL, timeout_thread, NULL) < 0) {
        LOG_ERROR(FLOW_MGMT_ID, "pthread_create() failed");
    }

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int flow_mgmt_cleanup(int *activated)
{
    LOG_INFO(FLOW_MGMT_ID, "Clean up - Flow management");

    deactivate();

    timeout_thread_on = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__MAX_NUM_SWITCHES; i++) {
        pthread_spin_lock(&flow_table[i].lock);

        flow_t *curr = flow_table[i].head;
        while (curr != NULL) {
            flow_t *tmp = curr;
            curr = curr->next;
            FREE(tmp);
        }

        pthread_spin_unlock(&flow_table[i].lock);
        pthread_spin_destroy(&flow_table[i].lock);
    }

    flow_q_destroy();

    FREE(flow_table);

    return 0;
}

/**
 * \brief Function to print all flows
 * \param cli The pointer of the Barista CLI
 */
static int flow_listup(cli_t *cli)
{
    int cnt = 0;

    cli_print(cli, "<Flow Tables>");

    //TODO

    if (!cnt)
        cli_print(cli, "  No flow");

    return 0;
}

/**
 * \brief Function to show the flows in a specific switch
 * \param cli The pointer of the Barista CLI
 * \param dpid_str Datapath ID
 */
static int flow_showup(cli_t *cli, char *dpid_str)
{
    int cnt = 0;

    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "<Flow Table for Switch [%lu]>", dpid);

    //TODO

    if (!cnt)
        cli_print(cli, "  No flow");

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int flow_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0 && args[1] != NULL && strcmp(args[1], "flows") == 0 && args[2] == NULL) {
        flow_listup(cli);
        return 0;
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0 && args[1] != NULL && args[2] == NULL) {
        flow_showup(cli, args[1]);
        return 0;
    }

    PRINTF("<Available Commands>\n");
    PRINTF("  flow_mgmt list flows\n");
    PRINTF("  flow_mgmt show [datapath ID]\n");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int flow_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    switch(ev->type) {
    case EV_DP_INSERT_FLOW:
        PRINT_EV("EV_DP_INSERT_FLOW\n");
        {
            const flow_t *flow = ev->flow;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            add_flow(flow_tbl, ev->flow);
        }
        break;
    case EV_DP_MODIFY_FLOW:
        PRINT_EV("EV_DP_MODIFY_FLOW\n");
        {
            const flow_t *flow = ev->flow;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            modify_flow(flow_tbl, ev->flow);
        }
        break;
    case EV_DP_DELETE_FLOW:
        PRINT_EV("EV_DP_DELETE_FLOW\n");
        {
            const flow_t *flow = ev->flow;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            delete_flow(flow_tbl, ev->flow);
        }
        break;
    case EV_DP_FLOW_EXPIRED:
        PRINT_EV("EV_DP_FLOW_EXPIRED\n");
        {
            const flow_t *flow = ev->flow;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            delete_flow(flow_tbl, ev->flow);
        }
        break;
    case EV_DP_FLOW_DELETED:
        PRINT_EV("EV_DP_FLOW_DELETED\n");
        {
            const flow_t *flow = ev->flow;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            delete_flow(flow_tbl, ev->flow);
        }
        break;
    case EV_DP_FLOW_STATS:
        PRINT_EV("EV_DP_FLOW_STATS\n");
        {
            const flow_t *flow = ev->flow;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            update_flow(flow_tbl, ev->flow);
        }
        break;
    case EV_SW_CONNECTED:
        PRINT_EV("EV_SW_CONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(sw)];
            delete_all_flows(flow_tbl, ev->sw->dpid);
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(sw)];
            delete_all_flows(flow_tbl, ev->sw->dpid);
        }
        break;
    case EV_FLOW_ADDED:
        PRINT_EV("EV_FLOW_ADDED\n");
        {
            const flow_t *flow = ev->flow;

            if (flow->remote == FALSE) break;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            add_flow(flow_tbl, ev->flow);
        }
        break;
    case EV_FLOW_DELETED:
        PRINT_EV("EV_FLOW_DELETED\n");
        {
            const flow_t *flow = ev->flow;

            if (flow->remote == FALSE) break;

            flow_table_t *flow_tbl = &flow_table[FLOW_KEY(flow)];
            delete_flow(flow_tbl, ev->flow);
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
