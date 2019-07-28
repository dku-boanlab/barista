/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app_events
 * @{
 * \defgroup av_json AppEvent-to-JSON convertor
 * \brief Functions to convert the forms of app events
 * @{
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
 * \param return Return value
 */
static int export_to_json(uint32_t id, uint16_t type, const void *input, char *output, int ret)
{
    switch (type) {

    // upstream
    case AV_DP_RECEIVE_PACKET:
        {
            const pktin_t *in = (const pktin_t *)input;
            const pkt_info_t *info = &in->pkt_info;

            char buf[__CONF_LONG_STR_LEN] = {0};

            sprintf(output, "{'id':%u, 'type': %u, "
                            "'dpid': %lu, 'port': %u, 'xid': %u, 'buffer_id': %u, 'reason': %u, "
                            "'proto': %u, 'vlan_id': %u, 'vlan_pcp': %u, 'ip_tos': %u, "
                            "'src_mac': %lu, 'dst_mac': %lu, 'src_ip': %u, 'dst_ip': %u, 'src_port': %u, 'dst_port': %u, "
                            "'total_len': %u, 'data': '%s', 'return': %d}",
                    id, type, in->dpid, in->port, in->xid, in->buffer_id, in->reason, 
                    info->proto, info->vlan_id, info->vlan_pcp, info->ip_tos,
                    mac2int(info->src_mac), mac2int(info->dst_mac), info->src_ip, info->dst_ip, info->src_port, info->dst_port, 
                    in->total_len, (in->total_len) ? base64_encode_w_buffer((const char *)in->data, in->total_len, buf) : "", 
                    ret);
        }
        break;
    case AV_DP_FLOW_EXPIRED:
    case AV_DP_FLOW_DELETED:
        {
            const flow_t *fl = (const flow_t *)input;
            const pkt_info_t *info = &fl->pkt_info;

            sprintf(output, "{'id':%u, 'type': %u, "
                            "'dpid': %lu, 'port': %u, 'xid': %u, "
                            "'cookie': %lu, 'idle_timeout': %u, 'priority': %u, 'reason': %u, 'wildcards': %u, "
                            "'proto': %u, 'vlan_id': %u, 'vlan_pcp': %u, 'ip_tos': %u, "
                            "'src_mac': %lu, 'dst_mac': %lu, 'src_ip': %u, 'dst_ip': %u, 'src_port': %u, 'dst_port': %u, "
                            "'duration_sec': %u, 'duration_nsec': %u, 'pkt_count': %lu, 'byte_count': %lu, 'return': %d}",
                    id, type, fl->dpid, fl->port, fl->info.xid, 
                    fl->meta.cookie, fl->meta.idle_timeout, fl->meta.priority, fl->meta.reason, info->wildcards, 
                    info->proto, info->vlan_id, info->vlan_pcp, info->ip_tos, 
                    mac2int(info->src_mac), mac2int(info->dst_mac), info->src_ip, info->dst_ip, info->src_port, info->dst_port, 
                    fl->stat.duration_sec, fl->stat.duration_nsec, fl->stat.pkt_count, fl->stat.byte_count, ret);
        }
        break;
    case AV_DP_PORT_ADDED:
    case AV_DP_PORT_MODIFIED:
    case AV_DP_PORT_DELETED:
        {
            const port_t *pt = (const port_t *)input;

            sprintf(output, "{'id':%u, 'type': %u, "
                            "'dpid': %lu, 'port': %u, 'name': '%s', 'hw_addr': %lu, 'config': %u, 'state': %u, "
                            "'curr': %u, 'advertized': %u, 'supported': %u, 'peer': %u, 'return': %d}",
                    id, type, 
                    pt->dpid, pt->port, pt->info.name, mac2int(pt->info.hw_addr), pt->info.config, pt->info.state,
                    pt->info.curr, pt->info.advertized, pt->info.supported, pt->info.peer, ret);
        }
        break;

    // downstream
    case AV_DP_SEND_PACKET:
        {
            const pktout_t *out = (const pktout_t *)input;

            char actions[__CONF_STR_LEN] = {0};

            int i;
            for (i=0; i<out->num_actions; i++) {
                if (out->action[i].type == ACTION_OUTPUT || out->action[i].type == ACTION_SET_VLAN_VID)
                    sprintf(actions + strlen(actions), "%u:%u", out->action[i].type, out->action[i].port);
                else if (out->action[i].type == ACTION_SET_SRC_MAC || out->action[i].type == ACTION_SET_DST_MAC)
                    sprintf(actions + strlen(actions), "%u:%lu", out->action[i].type, mac2int(out->action[i].mac_addr));
                else if (out->action[i].type == ACTION_SET_SRC_IP || out->action[i].type == ACTION_SET_DST_IP)
                    sprintf(actions + strlen(actions), "%u:%u", out->action[i].type, out->action[i].ip_addr);
                else if (out->action[i].type == ACTION_SET_VLAN_PCP || out->action[i].type == ACTION_SET_IP_TOS)
                    sprintf(actions + strlen(actions), "%u:%u", out->action[i].type, out->action[i].vlan_pcp);
                else
                    sprintf(actions + strlen(actions), "%u:%u", out->action[i].type, out->action[i].vendor);

                if ((i+1) < out->num_actions)
                    strcat(actions, ",");
            }

            char buf[__CONF_LONG_STR_LEN] = {0};

            sprintf(output, "{'id':%u, 'type': %u, "
                            "'dpid': %lu, 'port': %u, 'xid': %u, 'buffer_id': %u, "
                            "'total_len': %u, 'data': '%s', 'actions': '%s', 'return': %d}",
                    id, type, 
                    out->dpid, out->port, out->xid, out->buffer_id, 
                    out->total_len, (out->total_len) ? base64_encode_w_buffer((const char *)out->data, out->total_len, buf) : "", 
                    actions, ret);
        }
        break;
    case AV_DP_INSERT_FLOW:
    case AV_DP_MODIFY_FLOW:
    case AV_DP_DELETE_FLOW:
        {
            const flow_t *fl = (const flow_t *)input;
            const pkt_info_t *info = &fl->pkt_info;

            char actions[__CONF_STR_LEN] = {0};

            int i;
            for (i=0; i<fl->num_actions; i++) {
                if (fl->action[i].type == ACTION_OUTPUT || fl->action[i].type == ACTION_SET_VLAN_VID)
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].port);
                else if (fl->action[i].type == ACTION_SET_SRC_MAC || fl->action[i].type == ACTION_SET_DST_MAC)
                    sprintf(actions + strlen(actions), "%u:%lu", fl->action[i].type, mac2int(fl->action[i].mac_addr));
                else if (fl->action[i].type == ACTION_SET_SRC_IP || fl->action[i].type == ACTION_SET_DST_IP)
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].ip_addr);
                else if (fl->action[i].type == ACTION_SET_VLAN_PCP || fl->action[i].type == ACTION_SET_IP_TOS)
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].vlan_pcp);
                else
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].vendor);

                if ((i+1) < fl->num_actions)
                    strcat(actions, ",");
            }

            sprintf(output, "{'id':%u, 'type': %u, "
                            "'dpid': %lu, 'port': %u, 'xid': %u, 'buffer_id': %u, "
                            "'cookie': %lu, 'idle_timeout': %u, 'hard_timeout': %u, 'priority': %u, 'flags': %u, 'wildcards': %u, "
                            "'proto': %u, 'vlan_id': %u, 'vlan_pcp': %u, 'ip_tos': %u, "
                            "'src_mac': %lu, 'dst_mac': %lu, 'src_ip': %u, 'dst_ip': %u, 'src_port': %u, 'dst_port': %u, "
                            "'actions': '%s', 'return': %d}",
                    id, type, fl->dpid, fl->port, fl->info.xid, fl->info.buffer_id,
                    fl->meta.cookie, fl->meta.idle_timeout, fl->meta.hard_timeout, fl->meta.priority, fl->meta.flags, info->wildcards,
                    info->proto, info->vlan_id, info->vlan_pcp, info->ip_tos,
                    mac2int(info->src_mac), mac2int(info->dst_mac), info->src_ip, info->dst_ip, info->src_port, info->dst_port,
                    actions, ret);
        }
        break;

    // internal (notification)
    case AV_SW_CONNECTED:
    case AV_SW_DISCONNECTED:
        {
            const switch_t *sw = (const switch_t *)input;

            sprintf(output, "{'id':%u, 'type': %u, 'dpid': %lu, 'remote': %u, 'return': %d}", id, type, sw->dpid, sw->remote, ret);
        }
        break;
    case AV_HOST_ADDED:
    case AV_HOST_DELETED:
        {
            const host_t *ht = (const host_t *)input;

            sprintf(output, "{'id':%u, 'type': %u, 'dpid': %lu, 'port': %u, 'mac': %lu, 'ip': %u, 'remote': %u, 'return': %d}",
                    id, type, ht->dpid, ht->port, ht->mac, ht->ip, ht->remote, ret);
        }
        break;
    case AV_LINK_ADDED:
    case AV_LINK_DELETED:
        {
            const port_t *pt = (const port_t *)input;

            sprintf(output, "{'id':%u, 'type': %u, 'src_dpid': %lu, 'src_port': %u, 'dst_dpid': %lu, 'dst_port': %u, "
                            "'remote': %u, 'return': %d}",
                    id, type, pt->dpid, pt->port, pt->link.dpid, pt->link.port, pt->remote, ret);
        }
        break;
    case AV_FLOW_ADDED:
    case AV_FLOW_MODIFIED:
    case AV_FLOW_DELETED:
        {
            const flow_t *fl = (const flow_t *)input;
            const pkt_info_t *info = &fl->pkt_info;

            char actions[__CONF_STR_LEN] = {0};

            int i;
            for (i=0; i<fl->num_actions; i++) {
                if (fl->action[i].type == ACTION_OUTPUT || fl->action[i].type == ACTION_SET_VLAN_VID)
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].port);
                else if (fl->action[i].type == ACTION_SET_SRC_MAC || fl->action[i].type == ACTION_SET_DST_MAC)
                    sprintf(actions + strlen(actions), "%u:%lu", fl->action[i].type, mac2int(fl->action[i].mac_addr));
                else if (fl->action[i].type == ACTION_SET_SRC_IP || fl->action[i].type == ACTION_SET_DST_IP)
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].ip_addr);
                else if (fl->action[i].type == ACTION_SET_VLAN_PCP || fl->action[i].type == ACTION_SET_IP_TOS)
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].vlan_pcp);
                else
                    sprintf(actions + strlen(actions), "%u:%u", fl->action[i].type, fl->action[i].vendor);

                if ((i+1) < fl->num_actions)
                    strcat(actions, ",");
            }

            sprintf(output, "{'id':%u, 'type': %u, "
                            "'dpid': %lu, 'port': %u, 'idle_timeout': %u, 'hard_timeout': %u, 'wildcards': %u, "
                            "'proto': %u, 'vlan_id': %u, 'vlan_pcp': %u, 'ip_tos': %u, "
                            "'src_mac': %lu, 'dst_mac': %lu, 'src_ip': %u, 'dst_ip': %u, 'src_port': %u, 'dst_port': %u, "
                            "'actions': '%s', 'remote': %u, 'return': %d}",
                    id, type, fl->dpid, fl->port,
                    fl->meta.idle_timeout, fl->meta.hard_timeout, info->wildcards,
                    info->proto, info->vlan_id, info->vlan_pcp, info->ip_tos,
                    mac2int(info->src_mac), mac2int(info->dst_mac), info->src_ip, info->dst_ip, info->src_port, info->dst_port,
                    actions, fl->remote, ret);
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

            sprintf(output, "{'id':%u, 'type': %u, 'msg': '%s', 'return': %d}", id, type, log, ret);
        }
        break;

    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

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

            GET_JSON_VALUE("proto", in->pkt_info.proto);
            GET_JSON_VALUE("vlan_id", in->pkt_info.vlan_id);
            GET_JSON_VALUE("vlan_pcp", in->pkt_info.vlan_pcp);
            GET_JSON_VALUE("ip_tos", in->pkt_info.ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, in->pkt_info.src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, in->pkt_info.dst_mac);

            GET_JSON_VALUE("src_ip", in->pkt_info.src_ip);
            GET_JSON_VALUE("dst_ip", in->pkt_info.dst_ip);
            GET_JSON_VALUE("src_port", in->pkt_info.src_port);
            GET_JSON_VALUE("dst_port", in->pkt_info.dst_port);
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

            GET_JSON_VALUE("xid", fl->info.xid);
            GET_JSON_VALUE("cookie", fl->meta.cookie);
            GET_JSON_VALUE("idle_timeout", fl->meta.idle_timeout);
            GET_JSON_VALUE("priority", fl->meta.priority);
            GET_JSON_VALUE("reason", fl->meta.reason);

            GET_JSON_VALUE("wildcards", fl->pkt_info.wildcards);
            GET_JSON_VALUE("proto", fl->pkt_info.proto);
            GET_JSON_VALUE("vlan_id", fl->pkt_info.vlan_id);
            GET_JSON_VALUE("vlan_pcp", fl->pkt_info.vlan_pcp);
            GET_JSON_VALUE("ip_tos", fl->pkt_info.ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, fl->pkt_info.src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, fl->pkt_info.dst_mac);

            GET_JSON_VALUE("src_ip", fl->pkt_info.src_ip);
            GET_JSON_VALUE("dst_ip", fl->pkt_info.dst_ip);
            GET_JSON_VALUE("src_port", fl->pkt_info.src_port);
            GET_JSON_VALUE("dst_port", fl->pkt_info.dst_port);

            GET_JSON_VALUE("duration_sec", fl->stat.duration_sec);
            GET_JSON_VALUE("duration_nsec", fl->stat.duration_nsec);
            GET_JSON_VALUE("pkt_count", fl->stat.pkt_count);
            GET_JSON_VALUE("byte_count", fl->stat.byte_count);

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
            int2mac(hw_addr, pt->info.hw_addr);

            GET_JSON_VALUE("config", pt->info.config);
            GET_JSON_VALUE("state", pt->info.state);
            GET_JSON_VALUE("curr", pt->info.curr);
            GET_JSON_VALUE("advertised", pt->info.advertized);
            GET_JSON_VALUE("supported", pt->info.supported);
            GET_JSON_VALUE("peer", pt->info.peer);

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

            out->num_actions = 0;
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

                    if (a_type == ACTION_OUTPUT || a_type == ACTION_SET_VLAN_VID) {
                        out->action[out->num_actions].port = a_value;
                    } else if (a_type == ACTION_SET_SRC_MAC || a_type == ACTION_SET_DST_MAC) {
                        int2mac(a_value, out->action[out->num_actions].mac_addr);
                    } else if (a_type == ACTION_SET_SRC_IP || a_type == ACTION_SET_DST_IP) {
                        out->action[out->num_actions].ip_addr = a_value;
                    } else if (a_type == ACTION_SET_VLAN_PCP || a_type == ACTION_SET_IP_TOS) {
                        out->action[out->num_actions].vlan_pcp = a_value;
                    } else {
                        out->action[out->num_actions].vendor = a_value;
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

            GET_JSON_VALUE("xid", fl->info.xid);
            GET_JSON_VALUE("buffer_id", fl->info.buffer_id);

            GET_JSON_VALUE("cookie", fl->meta.cookie);
            GET_JSON_VALUE("idle_timeout", fl->meta.idle_timeout);
            GET_JSON_VALUE("hard_timeout", fl->meta.hard_timeout);
            GET_JSON_VALUE("priority", fl->meta.priority);
            GET_JSON_VALUE("flags", fl->meta.flags);

            GET_JSON_VALUE("wildcards", fl->pkt_info.wildcards);
            GET_JSON_VALUE("proto", fl->pkt_info.proto);
            GET_JSON_VALUE("vlan_id", fl->pkt_info.vlan_id);
            GET_JSON_VALUE("vlan_pcp", fl->pkt_info.vlan_pcp);
            GET_JSON_VALUE("ip_tos", fl->pkt_info.ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, fl->pkt_info.src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, fl->pkt_info.dst_mac);

            GET_JSON_VALUE("src_ip", fl->pkt_info.src_ip);
            GET_JSON_VALUE("dst_ip", fl->pkt_info.dst_ip);
            GET_JSON_VALUE("src_port", fl->pkt_info.src_port);
            GET_JSON_VALUE("dst_port", fl->pkt_info.dst_port);

            fl->num_actions = 0;
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

                    if (a_type == ACTION_OUTPUT || a_type == ACTION_SET_VLAN_VID) {
                        fl->action[fl->num_actions].port = a_value;
                    } else if (a_type == ACTION_SET_SRC_MAC || a_type == ACTION_SET_DST_MAC) {
                        int2mac(a_value, fl->action[fl->num_actions].mac_addr);
                    } else if (a_type == ACTION_SET_SRC_IP || a_type == ACTION_SET_DST_IP) {
                        fl->action[fl->num_actions].ip_addr = a_value;
                    } else if (a_type == ACTION_SET_VLAN_PCP || a_type == ACTION_SET_IP_TOS) {
                        fl->action[fl->num_actions].vlan_pcp = a_value;
                    } else {
                        fl->action[fl->num_actions].vendor = a_value;
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

            GET_JSON_VALUE("src_dpid", pt->dpid);
            GET_JSON_VALUE("src_port", pt->port);
            GET_JSON_VALUE("dst_dpid", pt->link.dpid);
            GET_JSON_VALUE("dst_port", pt->link.port);

            GET_JSON_VALUE("remote", pt->remote);

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

            GET_JSON_VALUE("idle_timeout", fl->meta.idle_timeout);
            GET_JSON_VALUE("hard_timeout", fl->meta.hard_timeout);

            GET_JSON_VALUE("wildcards", fl->pkt_info.wildcards);
            GET_JSON_VALUE("proto", fl->pkt_info.proto);
            GET_JSON_VALUE("vlan_id", fl->pkt_info.vlan_id);
            GET_JSON_VALUE("vlan_pcp", fl->pkt_info.vlan_pcp);
            GET_JSON_VALUE("ip_tos", fl->pkt_info.ip_tos);

            uint64_t src_mac;
            GET_JSON_VALUE("src_mac", src_mac);
            int2mac(src_mac, fl->pkt_info.src_mac);

            uint64_t dst_mac;
            GET_JSON_VALUE("dst_mac", dst_mac);
            int2mac(dst_mac, fl->pkt_info.dst_mac);

            GET_JSON_VALUE("src_ip", fl->pkt_info.src_ip);
            GET_JSON_VALUE("dst_ip", fl->pkt_info.dst_ip);
            GET_JSON_VALUE("src_port", fl->pkt_info.src_port);
            GET_JSON_VALUE("dst_port", fl->pkt_info.dst_port);

            fl->num_actions = 0;
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

                    if (a_type == ACTION_OUTPUT || a_type == ACTION_SET_VLAN_VID) {
                        fl->action[fl->num_actions].port = a_value;
                    } else if (a_type == ACTION_SET_SRC_MAC || a_type == ACTION_SET_DST_MAC) {
                        int2mac(a_value, fl->action[fl->num_actions].mac_addr);
                    } else if (a_type == ACTION_SET_SRC_IP || a_type == ACTION_SET_DST_IP) {
                        fl->action[fl->num_actions].ip_addr = a_value;
                    } else if (a_type == ACTION_SET_VLAN_PCP || a_type == ACTION_SET_IP_TOS) {
                        fl->action[fl->num_actions].vlan_pcp = a_value;
                    } else {
                        fl->action[fl->num_actions].vendor = a_value;
                    }

                    fl->num_actions++;

                    action = strtok(NULL, actions);
                }
            }

            GET_JSON_VALUE("remote", fl->remote);

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

            GET_JSON_VALUE("return", ret);
        }
        break;
    }

    json_decref(json);

    return ret;
}

/**
 * @}
 *
 * @}
 */
