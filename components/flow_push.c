/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup flow_push Static Flow Enforcement
 * \brief (Network) static flow enforcement
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "flow_push.h"

/** \brief Static flow rule enforcement ID */
#define FLOW_PUSH_ID 2991799864

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int flow_push_main(int *activated, int argc, char **argv)
{
    LOG_INFO(FLOW_PUSH_ID, "Init - Static flow enforcement");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int flow_push_cleanup(int *activated)
{
    LOG_INFO(FLOW_PUSH_ID, "Clean up - Static flow enforcement");

    deactivate();

    return 0;
}

/**
 * \brief Function to generate a flow from a user input
 * \param cli The CLI pointer
 * \param cmd Command
 * \param dpid Datapath ID
 * \param options Match and action data
 */
static int flow_push(cli_t *cli, char *cmd, char *dpid, char *options)
{
    flow_t flow = {0};

    // dpid
    flow.dpid = strtoull(dpid, NULL, 0);

    // default setup
    flow.buffer_id = -1;

    // action check
    int action_check = 0;

    // fields
    char *token = strtok(options, " ;");

    do {
        char field[__CONF_WORD_LEN] = {0};
        char value[__CONF_WORD_LEN] = {0};

        if (sscanf(token, "%[^'=']=%[^'=']", field, value) != 2) {
            cli_print(cli, "wrong format");
            return -1;
        }

        if (strcmp(field, "priority") == 0) {
            flow.priority = atoi(value);
        } else if (strcmp(field, "idle_timeout") == 0) {
            flow.idle_timeout = atoi(value);
        } else if (strcmp(field, "hard_timeout") == 0) {
            flow.hard_timeout = atoi(value);
        } else if (strcmp(field, "proto") == 0) {
            if (strcmp(value, "ipv4") == 0) {
                flow.proto |= PROTO_IPV4;
            } else if (strcmp(value, "tcp") == 0) {
                flow.proto |= PROTO_TCP;
            } else if (strcmp(value, "udp") == 0) {
                flow.proto |= PROTO_UDP;
            } else if (strcmp(value, "icmp") == 0) {
                flow.proto |= PROTO_ICMP;
            }
        } else if (strcmp(field, "vlan_id") == 0) {
            flow.proto |= PROTO_VLAN;
            flow.vlan_id = atoi(value);
        } else if (strcmp(field, "vlan_pcp") == 0) {
            if (flow.proto & PROTO_VLAN) {
                flow.vlan_pcp = atoi(value);
            }
        } else if (strcmp(field, "src_mac") == 0) {
            int val[ETH_ALEN];
            if (sscanf(value, "%02x:%02x:%02x:%02x:%02x:%02x", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]) != 6) {
                cli_print(cli, "wrong value");
                return -1;
            }
            int i;
            for (i=0; i<ETH_ALEN; i++) {
                flow.src_mac[i] = (uint8_t)val[i];
            }
        } else if (strcmp(field, "dst_mac") == 0) {
            int val[ETH_ALEN];
            if (sscanf(value, "%02x:%02x:%02x:%02x:%02x:%02x", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]) != 6) {
                cli_print(cli, "wrong value");
                return -1;
            }
            int i;
            for (i=0; i<ETH_ALEN; i++) {
                flow.dst_mac[i] = (uint8_t)val[i];
            }
        } else if ((flow.proto & PROTO_IPV4) && strcmp(field, "src_ip") == 0) {
            flow.src_ip = inet_addr(value);
        } else if ((flow.proto & PROTO_IPV4) && strcmp(field, "dst_ip") == 0) {
            flow.dst_ip = inet_addr(value);
        } else if ((flow.proto & PROTO_TCP || flow.proto & PROTO_UDP) && strcmp(field, "src_port") == 0) {
            flow.src_port = atoi(value);
        } else if ((flow.proto & PROTO_TCP || flow.proto & PROTO_UDP) && strcmp(field, "dst_port") == 0) {
            flow.dst_port = atoi(value);
        } else if (flow.proto & PROTO_ICMP && strcmp(field, "icmp_type") == 0) {
            flow.type = atoi(value);
        } else if (flow.proto & PROTO_ICMP && strcmp(field, "icmp_code") == 0) {
            flow.code = atoi(value);
        } else if (strcmp(field, "in_port") == 0) {
            flow.port = atoi(value);
        } else if (strcmp(field, "action") == 0) {
            if (flow.num_actions >= __MAX_NUM_ACTIONS) {
                cli_print(cli, "action overflow");
                return -1;
            }

            if (strncmp(value, "output", 6) == 0) {
                char forward[__CONF_WORD_LEN] = {0};
                char out_port[__CONF_WORD_LEN] = {0};

                if (sscanf(value, "%[^','],%[^',']", forward, out_port) == 2) {
                    action_check = TRUE;
                    flow.action[flow.num_actions].type = ACTION_OUTPUT;
                    flow.action[flow.num_actions].port = atoi(out_port);
                    flow.num_actions++;
                } else {
                    cli_print(cli, "wrong value");
                    return -1;
                }
            } else if (strcmp(value, "broadcast") == 0) {
                action_check = TRUE;
                flow.action[flow.num_actions].type = ACTION_OUTPUT;
                flow.action[flow.num_actions].port = PORT_FLOOD;
                flow.num_actions++;
            } else {
                cli_print(cli, "wrong value");
                return -1; 
            }
        } else {
            cli_print(cli, "wrong field");
            return -1;
        }
    } while ((token = strtok(NULL, " ;")));

    if (action_check == FALSE) {
        cli_print(cli, "no action");
        return -1;
    }

    if (!(flow.proto & PROTO_VLAN)) {
        flow.vlan_id = VLAN_NONE;
    }

    if (strcmp(cmd, "add") == 0) {
        ev_dp_insert_flow(FLOW_PUSH_ID, &flow);
    } else if (strcmp(cmd, "del") == 0) {
        ev_dp_delete_flow(FLOW_PUSH_ID, &flow);
    }

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int flow_push_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && args[1] != NULL && args[2] != NULL && args[3] == NULL) {
        if (flow_push(cli, args[0], args[1], args[2]) == 0) 
            return 0;
        else
            return -1;
    }

    cli_print(cli, "<Command Usage>");
    cli_print(cli, "  flow_push [command] [datapath ID] [field=value];[field=value];...;[action=value]}");
    cli_print(cli, "command -> { add | del }");
    cli_print(cli, "dpid    -> datapath id");
    cli_print(cli, "fields  -> priority=[priority] (default: 32768)");
    cli_print(cli, "           idle_timeout=[time(sec)] (default: 0)");
    cli_print(cli, "           hard_timeout=[time(sec)] (default: 0)");
    cli_print(cli, "           proto={ ipv4 | tcp | udp | icmp }");
    cli_print(cli, "           vlan_id=[vlan id]");
    cli_print(cli, "           vlan_pcp=[vlan priority]");
    cli_print(cli, "           src_mac=[xx:xx:xx:xx:xx:xx]");
    cli_print(cli, "           dst_mac=[xx.xx.xx.xx.xx.xx]");
    cli_print(cli, "           src_ip=[x.x.x.x]");
    cli_print(cli, "           dst_ip=[x.x.x.x]");
    cli_print(cli, "           src_port=[port number] for TCP/UDP");
    cli_print(cli, "           dst_port=[port number] for TCP/UDP");
    cli_print(cli, "           icmp_type=[type] for ICMP");
    cli_print(cli, "           icmp_code=[code] for ICMP");
    cli_print(cli, "           in_port=[port number]");
    cli_print(cli, "actions -> action={ output,[port_number] | broadcast }");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int flow_push_handler(const event_t *ev, event_out_t *ev_out)
{
    return 0;
}

/**
 * @}
 *
 * @}
 */
