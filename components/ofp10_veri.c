/*
 * Copyright 2015-2019 NSSLab, KAIST
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

/** \brief OpenFlow message verification ID */
#define OFP10_VERI_ID 1489022063

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to verify an OpenFlow message
 * \param id Component ID
 * \param msg OpenFlow message
 */
static int ofp_msg_verification(uint32_t id, const msg_t *msg)
{
    struct ofp_header *ofph = (struct ofp_header *)msg->data;

    if (ofph->version != OFP_VERSION)
        return 0;

    if (ofph->type > OFPT_QUEUE_GET_CONFIG_REPLY) {
        LOG_ERROR(OFP10_VERI_ID, " %u -> wrong ofp type (%u)", id, ofph->type);
        return -1;
    }
    if (ntohs(ofph->length) < sizeof(struct ofp_header)) {
        LOG_ERROR(OFP10_VERI_ID, " %u -> wrong ofp length (%u)", id, ntohs(ofph->length));
        return -1;
    }
    // skip xid verification

    switch (ofph->type) {
    case OFPT_HELLO:
        {
            if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_HELLO length (%u)", id, ntohs(ofph->length));
                return -1;
            }
        }
        break;
    case OFPT_ERROR:
        {
            struct ofp_error_msg *data = (struct ofp_error_msg *)msg->data;

            if (ntohs(data->type) > OFPET_QUEUE_OP_FAILED) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong ofp_error_type (%u)", id, ntohs(data->type));
                return -1;
            }

            if (ntohs(data->type) == OFPET_HELLO_FAILED) {
                if (ntohs(ofph->length) < sizeof(struct ofp_error_msg)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPET_HELLO_FAILED length (%u)", id, ntohs(ofph->length));
                    return -1;
                }

                if (ntohs(data->code) > OFPHFC_EPERM) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_HELLO_FAILED code (%u)", id, ntohs(data->code));
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_BAD_REQUEST) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);
                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_BAD_REQUEST data length (%u)", id, dataSize);
                    return -1;
                }

                if (ntohs(data->code) > OFPBRC_BUFFER_UNKNOWN) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_BAD_REQUEST code (%u)", id, ntohs(data->code));
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_BAD_ACTION) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);
                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_BAD_ACTION data length (%u)", id, dataSize);
                    return -1;
                }

                if (ntohs(data->code) > OFPBAC_BAD_QUEUE) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPBRC_BAD_ACTION code (%u)", id, ntohs(data->code));
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_FLOW_MOD_FAILED) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);
                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_FLOW_MOD_FAILED data length (%u)", id, dataSize);
                    return -1;
                }

                if (ntohs(data->code) > OFPFMFC_UNSUPPORTED) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_FLOW_MOD_FAILED code (%u)", id, ntohs(data->code));
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_PORT_MOD_FAILED) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);
                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_PORT_MOD_FAILED data length (%u)", id, dataSize);
                    return -1;
                }

                if (ntohs(data->code) > OFPPMFC_BAD_HW_ADDR) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_PORT_MOD_FAILED code (%u)", id, ntohs(data->code));
                    return -1;
                }
            }
            else if (ntohs(data->type) == OFPET_QUEUE_OP_FAILED) {
                int dataSize = ntohs(ofph->length) - sizeof(struct ofp_error_msg);
                if (dataSize < 64) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_QUEUE_OP_FAILED data length (%u)", id, dataSize);
                    return -1;
                }

                if (ntohs(data->code) > OFPQOFC_EPERM) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_ERROR | wrong OFPET_QUEUE_OP_FAILED code (%u)", id, ntohs(data->code));
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
        {
            if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_FEATURES_REQUEST length (%u)", id, ntohs(ofph->length));
                return -1;
            }
        }
        break;
    case OFPT_FEATURES_REPLY:
        {
            struct ofp_switch_features *data = (struct ofp_switch_features *)msg->data;

            if (ntohs(ofph->length) < sizeof(struct ofp_switch_features)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_FEATURES_REPLY length (%u)", id, ntohs(ofph->length));
                return -1;
            }

            if (ntohl(data->capabilities) & 0xffffff00) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FEATURES_REPLY | wrong capabilities bitmap (%u)", id, ntohl(data->capabilities));
                return -1;
            }

            if (ntohl(data->actions) & 0xffffe000) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FEATURES_REPLY | wrong action bitmap (%u)", id, ntohl(data->actions));
                return -1;
            }

            // ports
        }
        break;
    case OFPT_GET_CONFIG_REQUEST:
        {
            if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_GET_CONFIG_REQUEST length (%u)", id, ntohs(ofph->length));
                return -1;
            }
        }
        break;
    case OFPT_GET_CONFIG_REPLY:
        {
            struct ofp_switch_config *data = (struct ofp_switch_config *)msg->data;

            if (ntohs(ofph->length) != sizeof(struct ofp_switch_config)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_GET_CONFIG_REPLY length (%u)", id, ntohs(ofph->length));
                return -1;
            }

            if (ntohs(data->flags) > OFPC_FRAG_MASK) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_GET_CONFIG_REPLY flags (%u)", id, ntohs(data->flags));
                return -1;
            }
        }
        break;
    case OFPT_SET_CONFIG:
        {
            struct ofp_switch_config *data = (struct ofp_switch_config *)msg->data;

            if (ntohs(ofph->length) != sizeof(struct ofp_switch_config)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_SET_CONFIG length (%u)", id, ntohs(ofph->length));
                return -1;
            }

            if (ntohs(data->flags) > OFPC_FRAG_MASK) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_SET_CONFIG flags (%u)", id, ntohs(data->flags));
                return -1;
            }
        }
        break;
    case OFPT_PACKET_IN:
        {
            struct ofp_packet_in *data = (struct ofp_packet_in *)msg->data;

            if (ntohs(ofph->length) < sizeof(struct ofp_packet_in)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PACKET_IN length (%u)", id, ntohs(ofph->length));
                return -1;
            }

            if (ntohs(data->in_port) > MIN(OFPP_MAX, __MAX_NUM_PORTS) && ntohs(data->in_port) < OFPP_IN_PORT) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PACKET_IN in_port (%u)", id, ntohs(data->in_port));
                return -1;
            }

            if (data->reason > OFPR_ACTION) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PACKET_IN reason (%u)", id, data->reason);
                return -1;
            }
        }
        break;
    case OFPT_FLOW_REMOVED:
        {
            struct ofp_flow_removed *data = (struct ofp_flow_removed *)msg->data;

            if (ntohs(ofph->length) != sizeof(struct ofp_flow_removed)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_FLOW_REMOVED length (%u)", id, ntohs(ofph->length));
                return -1;
            }

            if (ntohs(data->match.in_port) > MIN(OFPP_MAX, __MAX_NUM_PORTS) && ntohs(data->match.in_port) < OFPP_IN_PORT) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_FLOW_REMOVED in_port (%u)", id, ntohs(data->match.in_port));
                return -1;
            }

            if (data->reason > OFPRR_DELETE) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_FLOW_REMOVED reason (%u)", id, data->reason);
                return -1;
            }
        }
        break;
    case OFPT_PORT_STATUS:
        {
            struct ofp_port_status *data = (struct ofp_port_status *)msg->data;

            if (ntohs(ofph->length) != sizeof(struct ofp_port_status)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PORT_STATUS length (%u)", id, ntohs(ofph->length));
                return -1;
            }

            if (data->reason > OFPPR_MODIFY) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PORT_STATUS reason (%u)", id, data->reason);
                return -1;
            }

            // desc.config
            // desc.state

            // desc.curr
            // desc.advertised
            // desc.supported
            // desc.peer
        }
        break;
    case OFPT_PACKET_OUT:
        {
            struct ofp_packet_out *data = (struct ofp_packet_out *)msg->data;

            if (ntohl(data->buffer_id) == (uint32_t)-1) {
                int size = ntohs(data->header.length) - (sizeof(struct ofp_packet_out) + ntohs(data->actions_len));
                if (size < 0) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PACKET_OUT length (%u)", id, ntohs(data->header.length));
                    return -1;
                }
            } else {
                if (ntohs(ofph->length) != sizeof(struct ofp_packet_out)) {
                    LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PACKET_OUT length (%u)", id, ntohs(ofph->length));
                    return -1;
                }
            }

            if (ntohs(data->in_port) > MIN(OFPP_MAX, __MAX_NUM_PORTS) && ntohs(data->in_port) < OFPP_IN_PORT) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_PACKET_OUT in_port (%u)", id, ntohs(data->in_port));
                return -1;
            }
        }
        break;
    case OFPT_FLOW_MOD:
        {
            struct ofp_flow_mod *data = (struct ofp_flow_mod *)msg->data;

            if (ntohs(data->match.in_port) > MIN(OFPP_MAX, __MAX_NUM_PORTS) && ntohs(data->match.in_port) < OFPP_IN_PORT) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FLOW_MOD | wrong in_port (%u)", id, ntohs(data->match.in_port));
                return -1;
            }

            if (ntohs(data->command) > OFPFC_DELETE_STRICT) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FLOW_MOD | wrong command (%u)", id, ntohs(data->command));
                return -1;
            }

            if (ntohs(data->flags) & 0xfff8) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_FLOW_MOD | wrong flags (%u)", id, ntohs(data->flags));
                return -1;
            }
        }
        break;
    case OFPT_PORT_MOD:
        {
            struct ofp_port_mod *data = (struct ofp_port_mod *)msg->data;

            if (ntohl(data->config) & 0xffffff80) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_PORT_MOD | wrong config (%u)", id, ntohl(data->config));
                return -1;
            }

            if (ntohl(data->mask) & 0xffffff80) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_PORT_MOD | wrong mask (%u)", id, ntohl(data->mask));
                return -1;
            }

            if (ntohl(data->advertise) & 0xfffff000) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> OFPT_PORT_MOD | wrong advertise (%u)", id, ntohl(data->advertise));
                return -1;
            }
        }
        break;
    case OFPT_STATS_REQUEST:
        {
            struct ofp_stats_request *data = (struct ofp_stats_request *)msg->data;

            if (ntohs(data->type) > OFPST_QUEUE && ntohs(data->type) != OFPST_VENDOR) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_STATS_REQUEST type", id);
                return -1;
            }

            // and ??
        }
        break;
    case OFPT_STATS_REPLY:
        {
            struct ofp_stats_reply *data = (struct ofp_stats_reply *)msg->data;

            if (ntohs(data->type) > OFPST_QUEUE && ntohs(data->type) != OFPST_VENDOR) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_STATS_REPLY type", id);
                return -1;
            }

            // and ??
        }
        break;
    case OFPT_BARRIER_REQUEST:
        {
            if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_BARRIER_REQUEST length (%u)", id, ntohs(ofph->length));
                return -1;
            }
        }
        break;
    case OFPT_BARRIER_REPLY:
        {
            if (ntohs(ofph->length) != sizeof(struct ofp_header)) {
                LOG_ERROR(OFP10_VERI_ID, " %u -> wrong OFPT_BARRIER_REQUEST length (%u)", id, ntohs(ofph->length));
                return -1;
            }
        }
        break;
    case OFPT_QUEUE_GET_CONFIG_REQUEST:
        // do nothing
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
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int ofp10_veri_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

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
            const msg_t *msg = ev->msg;
            if (ofp_msg_verification(ev->id, msg)) {
                //return -1;
                return 0;
            }
        }
        break;
    case EV_OFP_MSG_OUT:
        PRINT_EV("EV_OFP_MSG_OUT\n");
        {
            const msg_t *msg = ev->msg;
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
