/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, FUNC_TYPE *data)
{
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
        ev_out.FUNC_DATA = data;

#ifdef __ENABLE_META_EVENTS
        ev_ctx->num_events[type]++;
#endif /* __ENABLE_META_EVENTS */

        int i;
        for (i=0; i<ev_num; i++) {
            compnt_t *c = ev_list[i];

            if (!c) continue;
            else if (!c->activated) continue; // not activated yet

            if (c->site == COMPNT_INTERNAL) { // internal site
#ifdef __ENABLE_META_EVENTS
                c->num_events[type]++;
#endif /* __ENABLE_META_EVENTS */

                int ret = c->handler(ev, &ev_out);
                if (ret && c->perm & COMPNT_EXECUTE) {
                    break;
                }
            } else { // external site
                event_out_t *out = &ev_out;

#ifdef __ENABLE_META_EVENTS
                c->num_events[type]++;
#endif /* __ENABLE_META_EVENTS */

                int ret = ev_send_ext_msg(c, id, type, len, data, out);
                if (ret && c->perm & COMPNT_EXECUTE) {
                    break;
                }
            }
        }
    }

    return 0;
}
