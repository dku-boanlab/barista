/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup ofp_veri OpenFlow 1.0 Message Verification
 * \brief (Security) OpenFlow 1.0 message verification
 * @{
 */

/**
 * \file
 * \author Hyeonseong Jo <hsjjo@kaist.ac.kr>
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "ofp10_veri.h"

/** \brief OpenFlow structure */
#include "openflow10.h"

/** \brief OpenFlow message verification ID */
#define OFP10_VERI_ID 1489022063

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to verify an OpenFlow message
 * \param id Component ID
 * \param msg OpenFlow message
 */
static int ofp_msg_verification(uint32_t id, const raw_msg_t *msg)
{
    struct ofp_header *ofph = (struct ofp_header *)msg->data;

    if (ofph->version != OFP_VERSION)
        return 0;

    if ((ofph->type < OFPT_HELLO) || (ofph->type > OFPT_QUEUE_GET_CONFIG_REPLY)) {
        LOG_ERROR(OFP10_VERI_ID, " %u -> wrong type", id);
        return -1;
    }
    if (ntohs(ofph->length) < sizeof(struct ofp_header)) {
        LOG_ERROR(OFP10_VERI_ID, " %u -> wrong length", id);
        return -1;
    }
    // skip xid verification

    switch (ofph->type) {
    case OFPT_HELLO:
        if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
            LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_HELLO length error", id);
            return -1;
        }
        break;
    case OFPT_ERROR:
        {
            struct ofp_error_msg *data = (struct ofp_error_msg *)msg->data;
            if ((ntohs(data->type) < OFPET_HELLO_FAILED) || (ntohs(data->type) > OFPET_QUEUE_OP_FAILED)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | type error", id);
                return -1;
            }
            if (ntohs(data->type) == OFPET_HELLO_FAILED) {
                if ((ntohs(data->code) < OFPHFC_INCOMPATIBLE) || (ntohs(data->code) > OFPHFC_EPERM)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_HELLO_FAILED error", id);
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_BAD_REQUEST) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);
                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_BAD_REQUEST data length error", id);
                    return -1;
                }
                if ((ntohs(data->code) < OFPBRC_BAD_VERSION) || (ntohs(data->code) > OFPBRC_BUFFER_UNKNOWN)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_BAD_REQUEST code error", id);
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_BAD_ACTION) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);

                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_BAD_ACTION data length error", id);
                    return -1;
                }
                if ((ntohs(data->code) < OFPBAC_BAD_TYPE) || (ntohs(data->code) > OFPBAC_BAD_QUEUE)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPBRC_BAD_ACTION code error", id);
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_FLOW_MOD_FAILED) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);

                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_FLOW_MOD_FAILED data length error", id);
                    return -1;
                }
                if ((ntohs(data->code) < OFPFMFC_ALL_TABLES_FULL) || (ntohs(data->code) > OFPFMFC_UNSUPPORTED)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_FLOW_MOD_FAILED code error", id);
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_PORT_MOD_FAILED) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);

                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_PORT_MOD_FAILED data length error", id);
                    return -1;
                }
                if ((ntohs(data->code) < OFPPMFC_BAD_PORT) || (ntohs(data->code) > OFPPMFC_BAD_HW_ADDR)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_PORT_MOD_FAILED code error", id);
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_QUEUE_OP_FAILED) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);

                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_QUEUE_OP_FAILED data length error", id);
                    return -1;
                }
                if ((ntohs(data->code) < OFPQOFC_BAD_PORT) || (ntohs(data->code) > OFPQOFC_EPERM)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | OFPET_QUEUE_OP_FAILED code error", id);
                    return -1;
                }
            }
        }
        break;
    case OFPT_ECHO_REQUEST:
        // do nothing
        break;
    case OFPT_ECHO_REPLY:
        // do nothing
        break;
    case OFPT_VENDOR:
        // do nothing
        break;
    case OFPT_FEATURES_REQUEST:
        if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
            LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FEATURES_REQUEST length error", id);
            return -1;
        }
        break;
    case OFPT_FEATURES_REPLY:
        {
            struct ofp_switch_features *data = (struct ofp_switch_features *)msg->data;

            if ((ntohl(data->capabilities) < OFPC_FLOW_STATS) || (ntohl(data->capabilities) >= (OFPC_ARP_MATCH_IP << 1))) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FEATURES_REPLY | capabilities error", id);
                return -1;
            }
            if ((ntohl(data->actions) < OFPAT_OUTPUT) || (ntohl(data->actions) >= (OFPAT_VENDOR << 1))) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FEATURES_REPLY | action error", id);
                return -1;
            }
        }
        break;
    case OFPT_GET_CONFIG_REQUEST:
        if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
            LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_GET_CONFIG_REQUEST length error", id);
            return -1;
        }
        break;
    case OFPT_GET_CONFIG_REPLY:
        {
            struct ofp_switch_config *data = (struct ofp_switch_config *)msg->data;
            if ((ntohs(data->flags) < OFPC_FRAG_NORMAL) || (ntohs(data->flags) > OFPC_FRAG_MASK)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_GET_CONFIG_REPLY flag error", id);
                return -1;
            }
        }
        break;
    case OFPT_SET_CONFIG:
        {
            struct ofp_switch_config *data = (struct ofp_switch_config *)msg->data;
            if ((ntohs(data->flags) < OFPC_FRAG_NORMAL) || (ntohs(data->flags) > OFPC_FRAG_MASK)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_SET_CONFIG flag error", id);
                return -1;
            }
        }
        break;
    case OFPT_PACKET_IN:
        {
            struct ofp_packet_in *data = (struct ofp_packet_in *)msg->data;
            if ((data->reason < OFPR_NO_MATCH) || (data->reason > OFPR_ACTION)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_PACKET_IN reason error", id);
                return -1;
            }
        }
        break;
    case OFPT_FLOW_REMOVED:
        {
            struct ofp_flow_removed *data = (struct ofp_flow_removed *)msg->data;
            if ((data->reason < OFPRR_IDLE_TIMEOUT) || (data->reason > OFPRR_DELETE)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FLOW_REMOVED reason error", id);
                return -1;
            }
        }
        break;
    case OFPT_PORT_STATUS:
        {
            struct ofp_port_status *data = (struct ofp_port_status *)msg->data;
            if ((data->reason < OFPPR_ADD) || (data->reason > OFPPR_MODIFY)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_PORT_STATUS reason error", id);
                return -1;
            }
        }
        break;
    case OFPT_PACKET_OUT:
        {
            struct ofp_packet_out *data = (struct ofp_packet_out *)msg->data;
            if (ntohl(data->buffer_id) == (uint32_t)-1) {
                int size = ntohs(data->header.length) - (sizeof(struct ofp_packet_out) + ntohs(data->actions_len));
                if (size < 0) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_PACKET_OUT length error", id);
                    return -1;
                }
            }
        }
        break;
    case OFPT_FLOW_MOD:
        {
            struct ofp_flow_mod *data = (struct ofp_flow_mod *)msg->data;
            if (data->cookie == 0xffffffffffffffff) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FLOW_MOD | cookie reserved data error", id);
                return -1;
            }
            if ((ntohs(data->command) < OFPFC_ADD) || (ntohs(data->command) > OFPFC_DELETE_STRICT)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FLOW_MOD | command error", id);
                return -1;
            }
            if ((ntohs(data->flags) < OFPFF_SEND_FLOW_REM) || (ntohs(data->flags) >= (OFPFF_EMERG << 1))) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FLOW_MOD | flags error", id);
                return -1;
            }
        }
        break;
    case OFPT_PORT_MOD:
        {
            struct ofp_port_mod *data = (struct ofp_port_mod *)msg->data;
            if ((ntohl(data->config) < OFPPC_PORT_DOWN) || (ntohl(data->config) >= (OFPPC_NO_PACKET_IN << 1))) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_PORT_MOD | port config error", id);
                return -1;
            }
        }
        break;
    case OFPT_STATS_REQUEST:
        {
            struct ofp_stats_request *data = (struct ofp_stats_request *)msg->data;
            if (((ntohs(data->type) < OFPST_DESC) || (ntohs(data->type) > OFPST_QUEUE)) && (ntohs(data->type) != OFPST_VENDOR)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_STATS_REQUEST type error", id);
                return -1;
            }
        }
        break;
    case OFPT_STATS_REPLY:
        {
            struct ofp_stats_reply *data = (struct ofp_stats_reply *)msg->data;
            if (((ntohs(data->type) < OFPST_DESC) || (ntohs(data->type) > OFPST_QUEUE)) && (ntohs(data->type) != OFPST_VENDOR)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_STATS_REPLY type error", id);
                return -1;
            }
        }
        break;
    case OFPT_BARRIER_REQUEST:
        if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
            LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_BARRIER_REQUEST length error", id);
            return -1;
        }
        break;
    case OFPT_BARRIER_REPLY:
        if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
            LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_BARRIER_REQUEST length error", id);
            return -1;
        }
        break;
    case OFPT_QUEUE_GET_CONFIG_REQUEST:
        {
            struct ofp_queue_get_config_request *data = (struct ofp_queue_get_config_request *)msg->data;
            if (ntohs(data->port) > OFPP_MAX) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_QUEUE_GET_CONFIG_REQUEST port error", id);
                return -1;
            }
        }
        break;
    case OFPT_QUEUE_GET_CONFIG_REPLY:
        // do nothing
        break;
    default:
        break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int ofp10_veri_main(int *activated, int argc, char **argv)
{
    LOG_INFO(OFP10_VERI_ID, "Init - OpenFlow 1.0 message verification");

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int ofp10_veri_cleanup(int *activated)
{
    LOG_INFO(OFP10_VERI_ID, "Clean up - OpenFlow 1.0 message verification");

    deactivate();

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int ofp10_veri_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int ofp10_veri_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_OFP_MSG_IN:
        PRINT_EV("EV_OFP_MSG_IN\n");
        {
            const raw_msg_t *msg = ev->raw_msg;
            if (ofp_msg_verification(ev->id, msg)) {
                //return -1;
                return 0;
            }
        }
        break;
    case EV_OFP_MSG_OUT:
        PRINT_EV("EV_OFP_MSG_OUT\n");
        {
            const raw_msg_t *msg = ev->raw_msg;
            if (ofp_msg_verification(ev->id, msg)) {
                //return -1;
                return 0;
            }
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
