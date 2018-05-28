/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, FUNC_TYPE *data)
{
    int av_num = av_ctx->av_num[type];
    app_t **av_list = av_ctx->av_list[type];

    if (av_list == NULL) return -1;

    // only for request-response events
    if (AV_ALL_DOWNSTREAM < type && type < AV_WRT_INTSTREAM) {
        app_event_out_t av_out = {0};
        app_event_t *av = (app_event_t *)&av_out;

        av_out.id = id;
        av_out.type = type;
        av_out.length = len;
        av_out.FUNC_DATA = data;

        av_ctx->num_app_events[type]++;

        int i;
        for (i=0; i<av_num; i++) {
            app_t *c = av_list[i];

            if (!c) continue;
            else if (!c->activated) continue; // not activated yet

            if (c->site == APP_INTERNAL) { // internal site
                c->num_app_events[type]++;

                int ret = c->handler(av, &av_out);
                if (ret && c->perm & APP_EXECUTE) {
                    break;
                }
            } else { // external site
                app_event_out_t *out = &av_out;

                c->num_app_events[type]++;

                int ret = AV_SEND_EXT_MSG(c, id, type, len, data, out);
                if (ret && c->perm & APP_EXECUTE) {
                    break;
                }
            }
        }
    }

    return 0;
}
