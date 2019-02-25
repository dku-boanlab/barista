/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup conflict Flow Rule Conflict Resolution
 * \brief (Security) flow rule conflict resolution
 * @{
 */

/**
 * \file
 * \author Hyeonseong Jo <hsjjo@kaist.ac.kr>
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "conflict.h"

/** \brief Rule conflict resolution ID */
#define CONFLICT_ID 1782591023

/////////////////////////////////////////////////////////////////////

#include "arr_queue.h"

/////////////////////////////////////////////////////////////////////

/** \brief Rule tables */
rule_table_t *rule_table;

/** \brief The number of rules */
int num_rules;

/** \brief Key for table lookup */
#define RULE_KEY(a) (a->dpid % __DEFAULT_TABLE_SIZE)

/////////////////////////////////////////////////////////////////////

#define MAX_MATCH_LIST 20
#define MAX_NUM_INFO 50
#define MAX_MSG_LEN 50

enum { DROP, FORWARD, };
enum { NOT_CONFLICT, CONFLICT, };

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to check any conflicts between a new flow and existing flows
 * \param rule New flow
 * \return CONFLICT or NOT_CONFLICT
 */
static int flow_rule_conflict(const flow_t *rule)
{
    flow_t list[MAX_MATCH_LIST];

    int curr = 1;
    int conflict = NOT_CONFLICT;

    int action = DROP;
    int num_actions = rule->num_actions;

    memmove(&list[curr++], &rule, sizeof(flow_t));

    int i, j;
    for (i=0; i<num_actions; i++) {
        int type = rule->action[i].type;

        if (type == ACTION_OUTPUT) {
            action = FORWARD;
        } else if (type == ACTION_SET_VLAN_VID) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    list[curr].vlan_id = rule->action[i].vlan_id;
            }
        } else if (type == ACTION_SET_VLAN_PCP) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    list[curr].vlan_pcp = rule->action[i].vlan_pcp;
            }
        } else if (type == ACTION_SET_SRC_MAC) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    memmove(&list[curr].src_mac, rule->action[i].mac_addr, ETH_ALEN);
            }
        } else if (type == ACTION_SET_DST_MAC) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    memmove(&list[curr].dst_mac, rule->action[i].mac_addr, ETH_ALEN);
            }
        } else if (type == ACTION_SET_SRC_IP) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    list[curr].src_ip = rule->action[i].ip_addr;
            }
        } else if (type == ACTION_SET_DST_IP) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    list[curr].dst_ip = rule->action[i].ip_addr;
            }
        } else if (type == ACTION_SET_IP_TOS) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    list[curr].ip_tos = rule->action[i].ip_tos;
            }
        } else if (type == ACTION_SET_SRC_PORT) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    list[curr].src_port = rule->action[i].port;
            }
        } else if (type == ACTION_SET_DST_PORT) {
            for (j=0; j<(2 * (i + 1)); j++, curr++) {
                memmove(&list[curr], &list[curr/2], sizeof(flow_t));
                if ((j % 2) != 0)
                    list[curr].dst_port = rule->action[i].port;
            }
        }
    }

    for (i=(curr/2); i<curr; i++) {
        rule_table_t *rule_tbl = &rule_table[RULE_KEY(rule)];
        flow_t *target = &list[i];

        pthread_rwlock_rdlock(&rule_tbl->lock);

        flow_t *curr = rule_tbl->head;
        while (curr != NULL) {
            if (FLOW_COMPARE(curr, target)) {
                int target_action = DROP;

                for (j=0; j<curr->num_actions; j++) {
                    if (curr->action[j].type == ACTION_OUTPUT)
                        target_action = FORWARD;
                }

                if (action != target_action) {
                    conflict = CONFLICT;
                }

                break;
            }

            curr = curr->next;
        }

        pthread_rwlock_unlock(&rule_tbl->lock);
    }

    if (conflict == CONFLICT)
        return CONFLICT;
    else
        return NOT_CONFLICT;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int conflict_main(int *activated, int argc, char **argv)
{
    LOG_INFO(CONFLICT_ID, "Init - Flow rule conflict resolution");

    num_rules = 0;

    rule_table = (rule_table_t *)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(rule_table_t));
    if (rule_table == NULL) {
        LOG_ERROR(CONFLICT_ID, "calloc() error");
        return -1;
    }

    int i;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_init(&rule_table[i].lock, NULL);
    }

    arr_q_init();

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int conflict_cleanup(int *activated)
{
    LOG_INFO(CONFLICT_ID, "Clean up - Flow rule conflict resolution");

    deactivate();

    int i;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_wrlock(&rule_table[i].lock);

        flow_t *curr = rule_table[i].head;
        while (curr != NULL) {
            flow_t *tmp = curr;
            curr = curr->next;
            FREE(tmp);
        }

        pthread_rwlock_unlock(&rule_table[i].lock);
        pthread_rwlock_destroy(&rule_table[i].lock);
    }

    FREE(rule_table);

    arr_q_destroy();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int conflict_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int conflict_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_DP_INSERT_FLOW:
        PRINT_EV("EV_DP_INSERT_FLOW\n");
        {
            const flow_t *rule = ev->flow;

            if (flow_rule_conflict(rule)) {
                LOG_WARN(CONFLICT_ID, "Block - a new flow rule is conflict to existing flow rules");
                return -1;
            }
        }
        break;
    case EV_DP_MODIFY_FLOW:
        PRINT_EV("EV_DP_MODIFY_FLOW\n");
        {
            //TODO
        }
        break;
    case EV_DP_DELETE_FLOW:
        PRINT_EV("EV_DP_DELETE_FLOW\n");
        {
            //TODO
        }
        break;
    case EV_FLOW_ADDED:
        PRINT_EV("EV_FLOW_ADDED\n");
        {
            const flow_t *flow = ev->flow;
            rule_table_t *rule_tbl = &rule_table[RULE_KEY(flow)];

            flow_t *new = arr_dequeue();
            if (new == NULL) break;

            memmove(new, flow, sizeof(flow_t));

            pthread_rwlock_wrlock(&rule_tbl->lock);

            if (rule_tbl->head == NULL) {
                rule_tbl->head = new;
                rule_tbl->tail = new;
            } else {
                new->prev = rule_tbl->tail;
                rule_tbl->tail->next = new;
                rule_tbl->tail = new;
            }

            num_rules++;

            pthread_rwlock_unlock(&rule_tbl->lock);
        }
        break;
    case EV_FLOW_DELETED:
        PRINT_EV("EV_FLOW_DELETED\n");
        {
            const flow_t *flow = ev->flow;
            rule_table_t *rule_tbl = &rule_table[RULE_KEY(flow)];

            rule_table_t tmp_list = {0};

            pthread_rwlock_wrlock(&rule_tbl->lock);

            flow_t *curr = rule_tbl->head;
            while (curr != NULL) {
                if (FLOW_COMPARE(curr, flow)) {
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
                flow_t *tmp = curr;

                curr = curr->r_next;

                if (tmp->prev != NULL && tmp->next != NULL) {
                    tmp->prev->next = tmp->next;
                    tmp->next->prev = tmp->prev;
                } else if (tmp->prev == NULL && tmp->next != NULL) {
                    rule_tbl->head = tmp->next;
                    tmp->next->prev = NULL;
                } else if (tmp->prev != NULL && tmp->next == NULL) {
                    rule_tbl->tail = tmp->prev;
                    tmp->prev->next = NULL;
                } else if (tmp->prev == NULL && tmp->next == NULL) {
                    rule_tbl->head = NULL;
                    rule_tbl->tail = NULL;
                }

                num_rules--;

                arr_enqueue(tmp);
            }

            pthread_rwlock_unlock(&rule_tbl->lock);
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
