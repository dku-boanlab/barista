/*
 * Copyright 2015-2018 NSSLab, KAIST
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

/**
 * \brief Function to add a flow
 * \param list Flow table mapped to a datapath ID
 * \param flow NEW flow
 */
static int add_flow(flow_table_t *list, const flow_t *flow)
{
    int ret = TRUE;

    pthread_spin_lock(&list->lock);

    flow_t *curr = list->head;
    while (curr != NULL) {
        if (FLOW_COMPARE(curr, flow)) {
            curr->insert_time = time(NULL);
            ret = FALSE;
            break;
        }

        curr = curr->next;
    }

    pthread_spin_unlock(&list->lock);

    if (ret) {
        flow_t *new = flow_dequeue();
        if (new == NULL) return FALSE;

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

        num_flows++;

        pthread_spin_unlock(&list->lock);

        if (new->remote == FALSE) {
            flow_t out;
            memmove(&out, new, sizeof(flow_t));
            ev_flow_added(FLOW_MGMT_ID, &out);
        }
    }

    return ret;
}

/**
 * \brief Function to modify a flow
 * \param list Flow table mapped to a datapath ID
 * \param flow Existing flow to modify
 */
static int modify_flow(flow_table_t *list, const flow_t *flow)
{
    int ret = TRUE;

    pthread_spin_lock(&list->lock);

    flow_t *curr = list->head;
    while (curr != NULL) {
        if (FLOW_COMPARE(curr, flow)) {
            ret = FALSE;
            break;
        }

        curr = curr->next;
    }

    if (!ret) {
        curr->dpid = flow->dpid;
        curr->port = flow->port;

        curr->remote = flow->remote;
        curr->insert_time = time(NULL);

        curr->meta.idle_timeout = flow->meta.idle_timeout;
        curr->meta.hard_timeout = flow->meta.hard_timeout;

        memmove(&curr->match, &flow->match, sizeof(pkt_info_t));

        curr->num_actions = flow->num_actions;
        memmove(curr->action, flow->action, sizeof(action_t) * curr->num_actions);

        curr->prev = curr->next = curr->r_next = NULL;

        pthread_spin_unlock(&list->lock);

        if (curr->remote == FALSE) {
            flow_t out;
            memmove(&out, curr, sizeof(flow_t));
            ev_flow_modified(FLOW_MGMT_ID, &out);
        }
    } else {
        pthread_spin_unlock(&list->lock);
    }

    return ret;
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
            list->head = tmp->next;
            tmp->next->prev = NULL;
        } else if (tmp->prev != NULL && tmp->next == NULL) {
            list->tail = tmp->prev;
            tmp->prev->next = NULL;
        } else if (tmp->prev == NULL && tmp->next == NULL) {
            list->head = NULL;
            list->tail = NULL;
        }

        num_flows--;

        tmp->prev = tmp->next = tmp->r_next = NULL;

        if (tmp->remote == FALSE)
            ev_flow_deleted(FLOW_MGMT_ID, tmp);

        flow_enqueue(tmp);
    }

    pthread_spin_unlock(&list->lock);

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
            list->head = tmp->next;
            tmp->next->prev = NULL;
        } else if (tmp->prev != NULL && tmp->next == NULL) {
            list->tail = tmp->prev;
            tmp->prev->next = NULL;
        } else if (tmp->prev == NULL && tmp->next == NULL) {
            list->head = NULL;
            list->tail = NULL;
        }

        num_flows--;

        tmp->prev = tmp->next = tmp->r_next = NULL;

        if (tmp->remote == FALSE)
            ev_flow_deleted(FLOW_MGMT_ID, tmp);

        flow_enqueue(tmp);
    }

    pthread_spin_unlock(&list->lock);

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
int flow_mgmt_on;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to find and delete expired flows
 * \return NULL
 */
void *timeout_thread(void *arg)
{
    while (flow_mgmt_on) {
        time_t current_time = time(NULL);

        int i;
        for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
            flow_table_t tmp_list = {0};

            pthread_spin_lock(&flow_table[i].lock);

            flow_t *curr = flow_table[i].head;
            while (curr != NULL) {
                if ((current_time - curr->insert_time) > curr->meta.hard_timeout + 10) {
                    if (tmp_list.head == NULL) {
                        tmp_list.head = curr;
                        tmp_list.tail = curr;
                        curr->r_next = NULL;
                    } else {
                        tmp_list.tail->r_next = curr;
                        tmp_list.tail = curr;
                        curr->r_next = NULL;
                    }

                    num_flows--;
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
                    flow_table[i].head = tmp->next;
                    tmp->next->prev = NULL;
                } else if (tmp->prev != NULL && tmp->next == NULL) {
                    flow_table[i].tail = tmp->prev;
                    tmp->prev->next = NULL;
                } else if (tmp->prev == NULL && tmp->next == NULL) {
                    flow_table[i].head = NULL;
                    flow_table[i].tail = NULL;
                }

                tmp->prev = tmp->next = tmp->r_next = NULL;

                if (tmp->remote == FALSE)
                    ev_flow_deleted(FLOW_MGMT_ID, tmp);

                flow_enqueue(tmp);
            }

            pthread_spin_unlock(&flow_table[i].lock);
        }

        waitsec(1, 0);
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

    flow_mgmt_on = TRUE;

    num_flows = 0;
    flow_table = (flow_table_t *)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(flow_table_t));
    if (flow_table == NULL) {
        LOG_ERROR(FLOW_MGMT_ID, "calloc() failed");
        return -1;
    }

    int i;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
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

    flow_mgmt_on = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
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
    int i, cnt = 0;

    cli_print(cli, "<Flow Tables>");
    cli_print(cli, "  The total number of flows: %d", num_flows);

    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_spin_lock(&flow_table[i].lock);

        flow_t *curr = flow_table[i].head;
        while (curr != NULL) {
            char proto[__CONF_SHORT_LEN];

            if (curr->match.proto & PROTO_ARP)
                strcpy(proto, "ARP");
            else if (curr->match.proto & PROTO_LLDP)
                strcpy(proto, "LLDP");
            else if (curr->match.proto & PROTO_DHCP)
                strcpy(proto, "DHCP");
            if (curr->match.proto & PROTO_TCP)
                strcpy(proto, "TCP");
            else if (curr->match.proto & PROTO_UDP)
                strcpy(proto, "UDP");
            else if (curr->match.proto & PROTO_ICMP)
                strcpy(proto, "ICMP");
            else if (curr->match.proto & PROTO_IPV4)
                strcpy(proto, "IPv4");
            else
                strcpy(proto, "Unknown");

            if (curr->match.proto & PROTO_ICMP) {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                               "    src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                               "    vlan: %u, vlan_pcp: %u, %s, src_ip: %s, dst_ip: %s, icmp_type: %u, icmp_code: %u\n"
                               "    # of actions: %d, pkt_count: %lu, byte_count: %lu", 
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local", 
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port, 
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5], 
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5], 
                          curr->match.vlan_id, curr->match.vlan_pcp, proto, ip_addr_str(curr->match.src_ip), ip_addr_str(curr->match.dst_ip),
                          curr->match.icmp_type, curr->match.icmp_code, curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            } else if (curr->match.proto & PROTO_TCP || curr->match.proto & PROTO_UDP || curr->match.proto & PROTO_DHCP) {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                               "    src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                               "    vlan: %u, vlan_pcp: %u, %s, src_ip: %s, dst_ip: %s, src_port: %u, dst_port: %u\n"
                               "    # of actions: %d, pkt_count: %lu, byte_count: %lu", 
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local", 
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port, 
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5], 
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5], 
                          curr->match.vlan_id, curr->match.vlan_pcp, proto, ip_addr_str(curr->match.src_ip), ip_addr_str(curr->match.dst_ip),
                          curr->match.src_port, curr->match.dst_port, curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            } else if (curr->match.proto & PROTO_IPV4) {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                          "     src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                          "     vlan: %u, vlan_pcp: %u, %s, src_ip: %s, dst_ip: %s\n"
                          "     # of actions: %d, pkt_count: %lu, byte_count: %lu", 
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local",
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port, 
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5], 
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5], 
                          curr->match.vlan_id, curr->match.vlan_pcp, proto, ip_addr_str(curr->match.src_ip), ip_addr_str(curr->match.dst_ip),
                          curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            } else {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                               "     src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                               "     vlan: %u, vlan_pcp: %u, %s\n"
                               "     # of actions: %d, pkt_count: %lu, byte_count: %lu",
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local",
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port,
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5],
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5],
                          curr->match.vlan_id, curr->match.vlan_pcp, proto,
                          curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            }

            curr = curr->next;
        }

        pthread_spin_unlock(&flow_table[i].lock);
    }

    return 0;
}

/**
 * \brief Function to show the flows in a specific switch
 * \param cli The pointer of the Barista CLI
 * \param dpid_str Datapath ID
 */
static int flow_showup(cli_t *cli, char *dpid_str)
{
    int i, cnt = 0;

    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "<Flow Table for Switch [%lu]>", dpid);

    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_spin_lock(&flow_table[i].lock);

        flow_t *curr = flow_table[i].head;
        while (curr != NULL) {
            if (curr->dpid != dpid) {
                curr = curr->next;
                continue;
            }

            char proto[__CONF_SHORT_LEN];

            if (curr->match.proto & PROTO_ARP)
                strcpy(proto, "ARP");
            else if (curr->match.proto & PROTO_LLDP)
                strcpy(proto, "LLDP");
            else if (curr->match.proto & PROTO_DHCP)
                strcpy(proto, "DHCP");
            if (curr->match.proto & PROTO_TCP)
                strcpy(proto, "TCP");
            else if (curr->match.proto & PROTO_UDP)
                strcpy(proto, "UDP");
            else if (curr->match.proto & PROTO_ICMP)
                strcpy(proto, "ICMP");
            else if (curr->match.proto & PROTO_IPV4)
                strcpy(proto, "IPv4");
            else
                strcpy(proto, "Unknown");

            if (curr->match.proto & PROTO_ICMP) {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                               "    src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                               "    vlan: %u, vlan_pcp: %u, %s, src_ip: %s, dst_ip: %s, icmp_type: %u, icmp_code: %u\n"
                               "    # of actions: %d, pkt_count: %lu, byte_count: %lu",
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local",
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port,
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5],
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5],
                          curr->match.vlan_id, curr->match.vlan_pcp, proto, ip_addr_str(curr->match.src_ip), ip_addr_str(curr->match.dst_ip),
                          curr->match.icmp_type, curr->match.icmp_code, curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            } else if (curr->match.proto & PROTO_TCP || curr->match.proto & PROTO_UDP || curr->match.proto & PROTO_DHCP) {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                               "    src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                               "    vlan: %u, vlan_pcp: %u, %s, src_ip: %s, dst_ip: %s, src_port: %u, dst_port: %u\n"
                               "    # of actions: %d, pkt_count: %lu, byte_count: %lu",
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local",
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port,
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5],
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5],
                          curr->match.vlan_id, curr->match.vlan_pcp, proto, ip_addr_str(curr->match.src_ip), ip_addr_str(curr->match.dst_ip),
                          curr->match.src_port, curr->match.dst_port, curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            } else if (curr->match.proto & PROTO_IPV4) {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                          "     src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                          "     vlan: %u, vlan_pcp: %u, %s, src_ip: %s, dst_ip: %s\n"
                          "     # of actions: %d, pkt_count: %lu, byte_count: %lu",
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local",
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port,
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5],
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5],
                          curr->match.vlan_id, curr->match.vlan_pcp, proto, ip_addr_str(curr->match.src_ip), ip_addr_str(curr->match.dst_ip),
                          curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            } else {
                cli_print(cli, "  Flow #%d\n    %s, DPID: %lu, idle_timeout: %u, hard_timeout: %u, in_port: %u,\n"
                               "     src_mac: %02x:%02x:%02x:%02x:%02x:%02x, dst_mac: %02x:%02x:%02x:%02x:%02x:%02x,\n"
                               "     vlan: %u, vlan_pcp: %u, %s\n"
                               "     # of actions: %d, pkt_count: %lu, byte_count: %lu",
                          ++cnt, (curr->remote == TRUE) ? "Remote" : "Local",
                          curr->dpid, curr->meta.idle_timeout, curr->meta.hard_timeout, curr->port,
                          curr->match.src_mac[0], curr->match.src_mac[1], curr->match.src_mac[2], curr->match.src_mac[3], curr->match.src_mac[4], curr->match.src_mac[5],
                          curr->match.dst_mac[0], curr->match.dst_mac[1], curr->match.dst_mac[2], curr->match.dst_mac[3], curr->match.dst_mac[4], curr->match.dst_mac[5],
                          curr->match.vlan_id, curr->match.vlan_pcp, proto,
                          curr->num_actions, curr->stat.pkt_count, curr->stat.byte_count);
            }

            curr = curr->next;
        }

        pthread_spin_unlock(&flow_table[i].lock);
    }

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
