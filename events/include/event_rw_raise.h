/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, FUNC_TYPE *data)
{
    int ret = 0;

    int ev_num = ev_ctx->ev_num[type];
    compnt_t **ev_list = ev_ctx->ev_list[type];

    if (ev_list == NULL) return -1;

    // only for request-reponse events
    if (EV_ALL_DOWNSTREAM < type && type < EV_WRT_INTSTREAM) {
        event_out_t ev_out;
        event_t *ev = (event_t *)&ev_out;

        ev_out.id = id;
        ev_out.type = type;
        ev_out.length = len;
        ev_out.checksum = 0;

        ev_out.FUNC_DATA = data;

        ev_ctx->num_events[type]++;

        int i;
        for (i=0; i<ev_num; i++) {
            compnt_t *compnt = ev_list[i];

            if (!compnt) continue;
            else if (!compnt->activated) continue; // not activated yet

            compnt->num_events[type]++;

            if (compnt->site == COMPNT_INTERNAL) { // internal site
                ret = compnt->handler(ev, &ev_out);
            } else { // external site
                ret = ev_send_msg(compnt, id, type, len, data, ev_out.data);
            }

            if (ret && compnt->in_perm[type] & COMPNT_EXECUTE) break;
        }
    }

    return ret;
}
