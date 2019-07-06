/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, const FUNC_TYPE *data)
{
    int ev_num = ev_ctx->ev_num[type];
    compnt_t **ev_list = ev_ctx->ev_list[type];

    if (ev_list == NULL) return -1;

    event_out_t ev_out;
    event_t *ev = (event_t *)&ev_out;

    ev_out.id = id;
    ev_out.type = type;
    ev_out.length = len;
    ev_out.checksum = 0;

    ev->FUNC_DATA = data;

#ifdef __ENABLE_META_EVENTS
    ev_ctx->num_events[type]++;
#endif /* __ENABLE_META_EVENTS */

    compnt_t *one_by_one = NULL;

    int i;
    for (i=0; i<ev_num; i++) {
        compnt_t *c = ev_list[i];

        if (!c) continue;

        if (c->role == COMPNT_SECURITY_V2) {
            if (c->activated)
                one_by_one = c;
            else
                one_by_one = NULL;
        }

        if (!c->activated) continue; // not activated yet

#ifdef ODP_FUNC
        if (ODP_FUNC(c->odp, data)) continue;
#endif /* ODP_FUNC */

#ifdef __ENABLE_META_EVENTS
        c->num_events[type]++;
#endif /* __ENABLE_META_EVENTS */

        if (c->in_perm[type] & COMPNT_WRITE) {
            if (c->site == COMPNT_INTERNAL) { // internal site
                int ret = c->handler(ev, &ev_out);
                if (ret && c->in_perm[type] & COMPNT_EXECUTE) break;
                ev_out.checksum = 0; // reset checksum
            } else { // external site
                event_out_t *out = &ev_out;
                int ret = ev_send_msg(c, id, type, len, data, out->data);
                if (ret && c->in_perm[type] & COMPNT_EXECUTE) break;
            }
        } else {
            if (c->site == COMPNT_INTERNAL) { // internal site
                int ret = c->handler(ev, NULL);
                if (ret && c->in_perm[type] & COMPNT_EXECUTE) break;
            } else { // external site
                event_out_t *out = &ev_out;
                if (c->in_perm[type] & COMPNT_EXECUTE) {
                    if(ev_send_msg(c, id, type, len, data, out->data)) break;
                } else {
                    ev_push_msg(c, id, type, len, data);
                }
            }
        }

        if (one_by_one) {
            c = one_by_one;

#ifdef __ENABLE_META_EVENTS
            c->num_events[type]++;
#endif /* __ENABLE_META_EVENTS */

            if (c->in_perm[type] & COMPNT_WRITE) {
                if (c->site == COMPNT_INTERNAL) { // internal site
                    int ret = c->handler(ev, &ev_out);
                    if (ret && c->in_perm[type] & COMPNT_EXECUTE) break;
                } else { // external site
                    event_out_t *out = &ev_out;
                    int ret = ev_send_msg(c, id, type, len, data, out->data);
                    if (ret && c->in_perm[type] & COMPNT_EXECUTE) break;
                }
            } else {
                if (c->site == COMPNT_INTERNAL) { // internal site
                    int ret = c->handler(ev, NULL);
                    if (ret && c->in_perm[type] & COMPNT_EXECUTE) break;
                } else { // external site
                    event_out_t *out = &ev_out;
                    if (c->in_perm[type] & COMPNT_EXECUTE) {
                        if (ev_send_msg(c, id, type, len, data, out->data)) break;
                    } else {
                        ev_push_msg(c, id, type, len, data);
                    }
                }
            }
        }
    }

    return 0;
}
