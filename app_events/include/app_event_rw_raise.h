/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, FUNC_TYPE *data)
{
    int ret = 0;

    int av_num = av_ctx->av_num[type];
    app_t **av_list = av_ctx->av_list[type];

    if (av_list == NULL) return -1;

    // only for request-response events
    if (AV_ALL_DOWNSTREAM < type && type < AV_WRT_INTSTREAM) {
        app_event_out_t av_out;
        app_event_t *av = (app_event_t *)&av_out;

        av_out.id = id;
        av_out.type = type;
        av_out.length = len;

        av_out.FUNC_DATA = data;

        av_ctx->num_app_events[type]++;

        int i;
        for (i=0; i<av_num; i++) {
            app_t *app = av_list[i];

            if (!app) continue;
            else if (!app->activated) continue; // not activated yet

            app->num_app_events[type]++;

            if (app->site == APP_INTERNAL) { // internal site
                ret = app->handler(av, &av_out);
            } else { // external site
                ret = av_send_msg(app, id, type, len, data, av_out.data);
            }

            if (ret && app->in_perm[type] & APP_EXECUTE) break;
        }
    }

    return ret;
}
