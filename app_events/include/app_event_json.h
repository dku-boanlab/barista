/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "base64.h"
#include "mac2int.h"

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to export binary data to a JSON string
 * \param id Application ID
 * \param type App event type
 * \param input Binary data
 * \param output JSON string
 */
static int export_to_json(uint32_t id, uint16_t type, const void *input, char *output)
{
    switch (type) {

    // upstream
    case AV_DP_RECEIVE_PACKET:
        {
            const pktin_t *in = (const pktin_t *)input;

            char buffer[__MAX_EXT_DATA_SIZE] = {0};

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"xid\": %u, \"buffer_id\": %u, \"reason\": %u, \"proto\": %u, "
                            "\"vlan_id\": %u, \"vlan_pcp\": %u, \"ip_tos\": %u, \"src_mac\": %lu, \"dst_mac\": %lu, \"src_ip\": %u, \"dst_ip\": %u, "
                            "\"src_port\": %u, \"dst_port\": %u, \"total_len\": %u, \"data\": \"%s\"}",
                    id, type, in->dpid, in->port, in->xid, in->buffer_id, in->reason, in->proto, in->vlan_id, in->vlan_pcp, in->ip_tos,
                    mac2int(in->src_mac), mac2int(in->dst_mac), in->src_ip, in->dst_ip, in->src_port, in->dst_port, 
                    in->total_len, (in->total_len) ? base64_encode_w_buffer((const char *)in->data, in->total_len, buffer) : "None");
        }
        break;
    case AV_DP_FLOW_EXPIRED:
    case AV_DP_FLOW_DELETED:
        {
            const flow_t *fl = (const flow_t *)input;

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"xid\": %u, \"cookie\": %lu, \"priority\": %u, \"idle_timeout\": %u, \"reason\": %u, "
                            "\"wildcards\": %u, \"proto\": %u, \"vlan_id\": %u, \"vlan_pcp\": %u, \"ip_tos\": %u, \"src_mac\": %lu, \"dst_mac\": %lu, "
                            "\"src_ip\": %u, \"dst_ip\": %u, \"src_port\": %u, \"dst_port\": %u, \"duration_sec\": %u, \"duration_nsec\": %u, "
                            "\"pkt_count\": %lu, \"byte_count\": %lu}",
                    id, type, fl->dpid, fl->port, fl->xid, fl->cookie, fl->priority, fl->idle_timeout, fl->reason,
                    fl->wildcards, fl->proto, fl->vlan_id, fl->vlan_pcp, fl->ip_tos, mac2int(fl->src_mac), mac2int(fl->dst_mac),
                    fl->src_ip, fl->dst_ip, fl->src_port, fl->dst_port, fl->duration_sec, fl->duration_nsec, fl->pkt_count, fl->byte_count);
        }
        break;
    case AV_DP_PORT_ADDED:
    case AV_DP_PORT_MODIFIED:
    case AV_DP_PORT_DELETED:
        {
            const port_t *pt = (const port_t *)input;

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"hw_addr\": %lu, \"config\": %u, \"state\": %u, "
                            "\"curr\": %u, \"advertised\": %u, \"supported\": %u, \"peer\": %u}",
                    id, type, pt->dpid, pt->port, mac2int(pt->hw_addr), pt->config, pt->state,
                    pt->curr, pt->advertised, pt->supported, pt->peer);
        }
        break;

    // downstream
    case AV_DP_SEND_PACKET:
        {
            const pktout_t *out = (const pktout_t *)input;

            char buffer[__MAX_EXT_DATA_SIZE] = {0};
            char actions[__CONF_STR_LEN] = {0};

            int i;
            for (i=0; i<out->num_actions; i++) {
                char action[__CONF_SHORT_LEN] = {0};

                if (out->action[i].type == ACTION_SET_SRC_MAC || out->action[i].type == ACTION_SET_DST_MAC) {
                    sprintf(action, "%u:%lu", out->action[i].type, mac2int(out->action[i].mac_addr));
                } else {
                    sprintf(action, "%u:%u", out->action[i].type, out->action[i].ip_addr);
                }

                strcat(actions, action);

                if ((i+1) < out->num_actions)
                    strcat(actions, ",");
            }

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"xid\": %u, \"buffer_id\": %u, \"total_len\": %u, \"data\": \"%s\", \"actions\": \"%s\"}",
                    id, type, out->dpid, out->port, out->xid, out->buffer_id, out->total_len, 
                    (out->total_len) ? base64_encode_w_buffer((const char *)out->data, out->total_len, buffer) : "None", actions);
        }
        break;
    case AV_DP_INSERT_FLOW:
    case AV_DP_MODIFY_FLOW:
    case AV_DP_DELETE_FLOW:
        {
            const flow_t *fl = (const flow_t *)input;

            char actions[__CONF_STR_LEN] = {0};

            int i;
            for (i=0; i<fl->num_actions; i++) {
                char action[__CONF_SHORT_LEN] = {0};

                if (fl->action[i].type == ACTION_SET_SRC_MAC || fl->action[i].type == ACTION_SET_DST_MAC) {
                    sprintf(action, "%u:%lu", fl->action[i].type, mac2int(fl->action[i].mac_addr));
                } else {
                    sprintf(action, "%u:%u", fl->action[i].type, fl->action[i].ip_addr);
                }

                strcat(actions, action);

                if ((i+1) < fl->num_actions)
                    strcat(actions, ",");
            }

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"xid\": %u, \"buffer_id\": %u, \"cookie\": %lu, \"idle_timeout\": %u, \"hard_timeout\": %u, "
                            "\"priority\": %u, \"flags\": %u, \"wildcards\": %u, \"proto\": %u, \"vlan_id\": %u, \"vlan_pcp\": %u, \"ip_tos\": %u, "
                            "\"src_mac\": %lu, \"dst_mac\": %lu, \"src_ip\": %u, \"dst_ip\": %u, \"src_port\": %u, \"dst_port\": %u, \"actions\": \"%s\"}",
                    id, type, fl->dpid, fl->port, fl->xid, fl->buffer_id, fl->cookie, fl->idle_timeout, fl->hard_timeout, fl->priority, fl->flags,
                    fl->wildcards, fl->proto, fl->vlan_id, fl->vlan_pcp, fl->ip_tos, mac2int(fl->src_mac), mac2int(fl->dst_mac),
                    fl->src_ip, fl->dst_ip, fl->src_port, fl->dst_port, actions);
        }
        break;

    // internal (notification)
    case AV_SW_CONNECTED:
    case AV_SW_DISCONNECTED:
        {
            const switch_t *sw = (const switch_t *)input;

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"fd\": %u, \"remote\": %u}", id, type, sw->dpid, sw->fd, sw->remote);
        }
        break;
    case AV_HOST_ADDED:
    case AV_HOST_DELETED:
        {
            const host_t *ht = (const host_t *)input;

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"mac\": %lu, \"ip\": %u, \"remote\": %u}",
                    id, type, ht->dpid, ht->port, ht->mac, ht->ip, ht->remote);
        }
        break;
    case AV_LINK_ADDED:
    case AV_LINK_DELETED:
        {
            const port_t *pt = (const port_t *)input;

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"next_dpid\": %lu, \"next_port\": %u, \"remote\": %u}",
                    id, type, pt->dpid, pt->port, pt->next_dpid, pt->next_port, pt->remote);
        }
        break;
    case AV_FLOW_ADDED:
    case AV_FLOW_MODIFIED:
    case AV_FLOW_DELETED:
        {
            const flow_t *fl = (const flow_t *)input;

            char actions[__CONF_STR_LEN] = {0};

            int i;
            for (i=0; i<fl->num_actions; i++) {
                char action[__CONF_SHORT_LEN] = {0};

                if (fl->action[i].type == ACTION_SET_SRC_MAC || fl->action[i].type == ACTION_SET_DST_MAC) {
                    sprintf(action, "%u:%lu", fl->action[i].type, mac2int(fl->action[i].mac_addr));
                } else {
                    sprintf(action, "%u:%u", fl->action[i].type, fl->action[i].ip_addr);
                }

                strcat(actions, action);

                if ((i+1) < fl->num_actions)
                    strcat(actions, ",");
            }

            sprintf(output, "{\"id\":%u, \"type\": %u, \"dpid\": %lu, \"port\": %u, \"idle_timeout\": %u, \"hard_timeout\": %u, "
                            "\"wildcards\": %u, \"proto\": %u, \"vlan_id\": %u, \"vlan_pcp\": %u, \"ip_tos\": %u, \"src_mac\": %lu, \"dst_mac\": %lu, "
                            "\"src_ip\": %u, \"dst_ip\": %u, \"src_port\": %u, \"dst_port\": %u, \"actions\": \"%s\"}",
                    id, type, fl->dpid, fl->port, fl->idle_timeout, fl->hard_timeout,
                    fl->wildcards, fl->proto, fl->vlan_id, fl->vlan_pcp, fl->ip_tos, mac2int(fl->src_mac), mac2int(fl->dst_mac),
                    fl->src_ip, fl->dst_ip, fl->src_port, fl->dst_port, actions);
        }
        break;

    // log
    case AV_LOG_DEBUG:
    case AV_LOG_INFO:
    case AV_LOG_WARN:
    case AV_LOG_ERROR:
    case AV_LOG_FATAL:
        {
            const char *log = (const char *)input;

            sprintf(output, "{\"id\":%u, \"type\": %u, \"msg\": \"%s\"}", id, type, log);
        }
        break;

    }

    return 0;
}

#define GET_JSON_VALUE(x, y) \
{ \
    json_t *j_val = json_object_get(json, x); \
    if (json_is_integer(j_val)) \
        y = json_integer_value(j_val); \
    else \
        y = 0; \
}

/**
 * \brief Function to import binary data from a JSON string
 * \param id Application ID
 * \param type App event type
 * \param input JSON string
 * \param output Binary data
 */
static int import_from_json(uint32_t *id, uint16_t *type, char *input, void *output)
{
    int ret = 0;

    json_t *json = NULL;
    json_error_t error;

    json = json_loads(input, 0, &error);
    if (!json) {
        PERROR("json_loads");
        return -1;
    }

    GET_JSON_VALUE("id", *id);
    GET_JSON_VALUE("type", *type);

    switch (*type) {

    // upstream
    case AV_DP_RECEIVE_PACKET:
        {
            pktin_t *in = (pktin_t *)output;

            GET_JSON_VALUE("dpid", in->dpid);
            GET_JSON_VALUE("port", in->port);

            GET_JSON_VALUE("xid", in->xid);
            GET_JSON_VALUE("buffer_id", in->buffer_id);
            GET_JSON_VALUE("reason", in->reason);

            GET_JSON_VALUE("proto", in->proto);
            GET_JSON_VALUE("vlan_id", in->vlan_id);
            GET_JSON_VALUE("vlan_pcp", in->vlan_pcp);
            GET_JSON_VALUE("ip_tos", in->ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, in->src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, in->dst_mac);

            GET_JSON_VALUE("src_ip", in->src_ip);
            GET_JSON_VALUE("dst_ip", in->dst_ip);
            GET_JSON_VALUE("src_port", in->src_port);
            GET_JSON_VALUE("dst_port", in->dst_port);
            GET_JSON_VALUE("total_len", in->total_len);

            json_t *j_data = json_object_get(json, "data");
            if (json_is_string(j_data))
                base64_decode_w_buffer(json_string_value(j_data), (char *)in->data);

            GET_JSON_VALUE("return", ret);
        }
        break;
    case AV_DP_FLOW_EXPIRED:
    case AV_DP_FLOW_DELETED:
        {
            flow_t *fl = (flow_t *)output;

            GET_JSON_VALUE("dpid", fl->dpid);
            GET_JSON_VALUE("port", fl->port);

            GET_JSON_VALUE("xid", fl->xid);
            GET_JSON_VALUE("cookie", fl->cookie);
            GET_JSON_VALUE("priority", fl->priority);
            GET_JSON_VALUE("idle_timeout", fl->idle_timeout);
            GET_JSON_VALUE("reason", fl->reason);

            GET_JSON_VALUE("wildcards", fl->wildcards);
            GET_JSON_VALUE("proto", fl->proto);
            GET_JSON_VALUE("vlan_id", fl->vlan_id);
            GET_JSON_VALUE("vlan_pcp", fl->vlan_pcp);
            GET_JSON_VALUE("ip_tos", fl->ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, fl->src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, fl->dst_mac);

            GET_JSON_VALUE("src_ip", fl->src_ip);
            GET_JSON_VALUE("dst_ip", fl->dst_ip);
            GET_JSON_VALUE("src_port", fl->src_port);
            GET_JSON_VALUE("dst_port", fl->dst_port);

            GET_JSON_VALUE("duration_sec", fl->duration_sec);
            GET_JSON_VALUE("duration_nsec", fl->duration_nsec);
            GET_JSON_VALUE("pkt_count", fl->pkt_count);
            GET_JSON_VALUE("byte_count", fl->byte_count);

            GET_JSON_VALUE("return", ret);
        }
        break;
    case AV_DP_PORT_ADDED:
    case AV_DP_PORT_MODIFIED:
    case AV_DP_PORT_DELETED:
        {
            port_t *pt = (port_t *)output;

            GET_JSON_VALUE("dpid", pt->dpid);
            GET_JSON_VALUE("port", pt->port);

            uint64_t hw_addr;
            GET_JSON_VALUE("hw_addr", hw_addr);
            int2mac(hw_addr, pt->hw_addr);

            GET_JSON_VALUE("config", pt->config);
            GET_JSON_VALUE("state", pt->state);
            GET_JSON_VALUE("curr", pt->curr);
            GET_JSON_VALUE("advertised", pt->advertised);
            GET_JSON_VALUE("supported", pt->supported);
            GET_JSON_VALUE("peer", pt->peer);

            GET_JSON_VALUE("return", ret);
        }
        break;

    // downstream
    case AV_DP_SEND_PACKET:
        {
            pktout_t *out = (pktout_t *)output;

            GET_JSON_VALUE("dpid", out->dpid);
            GET_JSON_VALUE("port", out->port);

            GET_JSON_VALUE("xid", out->xid);
            GET_JSON_VALUE("buffer_id", out->buffer_id);

            GET_JSON_VALUE("total_len", out->total_len);

            json_t *j_data = json_object_get(json, "data");
            if (json_is_string(j_data))
                base64_decode_w_buffer(json_string_value(j_data), (char *)out->data);

            char actions[__CONF_STR_LEN] = {0};
            json_t *j_actions = json_object_get(json, "actions");
            if (json_is_string(j_actions)) {
                strcpy(actions, json_string_value(j_actions));

                char *action = strtok(actions, ",");
                while (action != NULL) {
                    uint32_t a_type;
                    uint64_t a_value;

                    sscanf(action, "%u:%lu", &a_type, &a_value);

                    out->action[out->num_actions].type = a_type;

                    if (a_type == ACTION_SET_SRC_MAC || a_type == ACTION_SET_DST_MAC) {
                        int2mac(a_value, out->action[out->num_actions].mac_addr);
                    } else {
                        out->action[out->num_actions].ip_addr = a_value;
                    }

                    out->num_actions++;

                    action = strtok(NULL, actions);
                }
            }

            GET_JSON_VALUE("return", ret);
        }
        break;
    case AV_DP_INSERT_FLOW:
    case AV_DP_MODIFY_FLOW:
    case AV_DP_DELETE_FLOW:
        {
            flow_t *fl = (flow_t *)output;

            GET_JSON_VALUE("dpid", fl->dpid);
            GET_JSON_VALUE("port", fl->port);

            GET_JSON_VALUE("xid", fl->xid);
            GET_JSON_VALUE("buffer_id", fl->buffer_id);
            GET_JSON_VALUE("cookie", fl->cookie);
            GET_JSON_VALUE("idle_timeout", fl->idle_timeout);
            GET_JSON_VALUE("hard_timeout", fl->hard_timeout);
            GET_JSON_VALUE("priority", fl->priority);
            GET_JSON_VALUE("flags", fl->flags);

            GET_JSON_VALUE("wildcards", fl->wildcards);
            GET_JSON_VALUE("proto", fl->proto);
            GET_JSON_VALUE("vlan_id", fl->vlan_id);
            GET_JSON_VALUE("vlan_pcp", fl->vlan_pcp);
            GET_JSON_VALUE("ip_tos", fl->ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, fl->src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, fl->dst_mac);

            GET_JSON_VALUE("src_ip", fl->src_ip);
            GET_JSON_VALUE("dst_ip", fl->dst_ip);
            GET_JSON_VALUE("src_port", fl->src_port);
            GET_JSON_VALUE("dst_port", fl->dst_port);

            char actions[__CONF_STR_LEN] = {0};
            json_t *j_actions = json_object_get(json, "actions");
            if (json_is_string(j_actions)) {
                strcpy(actions, json_string_value(j_actions));

                char *action = strtok(actions, ",");
                while (action != NULL) {
                    uint32_t a_type;
                    uint64_t a_value;

                    sscanf(action, "%u:%lu", &a_type, &a_value);

                    fl->action[fl->num_actions].type = a_type;

                    if (a_type == ACTION_SET_SRC_MAC || a_type == ACTION_SET_DST_MAC) {
                        int2mac(a_value, fl->action[fl->num_actions].mac_addr);
                    } else {
                        fl->action[fl->num_actions].ip_addr = a_value;
                    }

                    fl->num_actions++;

                    action = strtok(NULL, actions);
                }
            }

            GET_JSON_VALUE("return", ret);
        }
        break;

    // internal (notification)
    case AV_SW_CONNECTED:
    case AV_SW_DISCONNECTED:
        {
            switch_t *sw = (switch_t *)output;

            GET_JSON_VALUE("dpid", sw->dpid);
            GET_JSON_VALUE("fd", sw->fd);
            GET_JSON_VALUE("remote", sw->remote);

            GET_JSON_VALUE("return", ret);
        }
        break;
    case AV_HOST_ADDED:
    case AV_HOST_DELETED:
        {
            host_t *ht = (host_t *)output;

            GET_JSON_VALUE("dpid", ht->dpid);
            GET_JSON_VALUE("port", ht->port);
            GET_JSON_VALUE("mac", ht->mac);
            GET_JSON_VALUE("ip", ht->ip);
            GET_JSON_VALUE("remote", ht->remote);

            GET_JSON_VALUE("return", ret);
        }
        break;
    case AV_LINK_ADDED:
    case AV_LINK_DELETED:
        {
            port_t *pt = (port_t *)output;

            GET_JSON_VALUE("dpid", pt->dpid);
            GET_JSON_VALUE("port", pt->port);
            GET_JSON_VALUE("next_dpid", pt->next_dpid);
            GET_JSON_VALUE("next_port", pt->next_port);

            GET_JSON_VALUE("return", ret);
        }
        break;
    case AV_FLOW_ADDED:
    case AV_FLOW_MODIFIED:
    case AV_FLOW_DELETED:
        {
            flow_t *fl = (flow_t *)output;

            GET_JSON_VALUE("dpid", fl->dpid);
            GET_JSON_VALUE("port", fl->port);

            GET_JSON_VALUE("idle_timeout", fl->idle_timeout);
            GET_JSON_VALUE("hard_timeout", fl->hard_timeout);

            GET_JSON_VALUE("wildcards", fl->wildcards);
            GET_JSON_VALUE("proto", fl->proto);
            GET_JSON_VALUE("vlan_id", fl->vlan_id);
            GET_JSON_VALUE("vlan_pcp", fl->vlan_pcp);
            GET_JSON_VALUE("ip_tos", fl->ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, fl->src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, fl->dst_mac);

            GET_JSON_VALUE("src_ip", fl->src_ip);
            GET_JSON_VALUE("dst_ip", fl->dst_ip);
            GET_JSON_VALUE("src_port", fl->src_port);
            GET_JSON_VALUE("dst_port", fl->dst_port);

            char actions[__CONF_STR_LEN] = {0};
            json_t *j_actions = json_object_get(json, "actions");
            if (json_is_string(j_actions)) {
                strcpy(actions, json_string_value(j_actions));

                char *action = strtok(actions, ",");
                while (action != NULL) {
                    uint32_t a_type;
                    uint64_t a_value;

                    sscanf(action, "%u:%lu", &a_type, &a_value);

                    fl->action[fl->num_actions].type = a_type;

                    if (a_type == ACTION_SET_SRC_MAC || a_type == ACTION_SET_DST_MAC) {
                        int2mac(a_value, fl->action[fl->num_actions].mac_addr);
                    } else {
                        fl->action[fl->num_actions].ip_addr = a_value;
                    }

                    fl->num_actions++;

                    action = strtok(NULL, actions);
                }
            }

            GET_JSON_VALUE("return", ret);
        }
        break;

    // log
    case AV_LOG_DEBUG:
    case AV_LOG_INFO:
    case AV_LOG_WARN:
    case AV_LOG_ERROR:
    case AV_LOG_FATAL:
        {
            char *log = (char *)output;

            json_t *j_val = json_object_get(json, "msg");
            if (json_is_string(j_val))
                strcpy(log, json_string_value(j_val));
        }
        break;
    }

    json_decref(json);

    return ret;
}
