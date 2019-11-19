/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, const FUNC_TYPE *data)
{
    int ret = 0;

    int av_num = av_ctx->av_num[type];
    app_t **av_list = av_ctx->av_list[type];

    if (av_list == NULL) return -1;

    // outbound check
    int i, j, pass = 0;
    for (i=0; i<av_ctx->num_apps; i++) {
        app_t *app = av_ctx->app_list[i];
        if (app->id == id) {
            for (j=0; j<app->out_num; j++) {
                if (app->out_list[j] == type) {
                    pass = 1;
                    break;
                }
            }
            if (pass) break;
        }
    }
    if (!pass) return -1;

    app_event_out_t av_out;
    app_event_t *av = (app_event_t *)&av_out;

    av_out.id = id;
    av_out.type = type;
    av_out.length = len;

    if (API_monitor_enabled)
        clock_gettime(CLOCK_REALTIME, &av_out.time);

    av->FUNC_DATA = data;

    av_ctx->num_app_events[type]++;

    for (i=0; i<av_num; i++) {
        app_t *app = av_list[i];

        if (!app) continue;
        else if (!app->activated) continue; // not activated yet

#ifdef ODP_FUNC
        if (ODP_FUNC(app->odp, data)) continue;
#endif /* ODP_FUNC */

        app->num_app_events[type]++;

        if (app->in_perm[type] & APP_WRITE) {
            if (app->site == APP_INTERNAL) { // internal site
                ret = app->handler(av, &av_out);
            } else { // external site
                ret = av_send_msg(app, id, type, len, data, av_out.data);
            }
            if (ret && app->in_perm[type] & APP_EXECUTE) break;
        } else {
            if (app->site == APP_INTERNAL) { // internal site
                ret = app->handler(av, NULL);
                if (ret && app->in_perm[type] & APP_EXECUTE) break;
            } else { // external site
                if (app->in_perm[type] & APP_EXECUTE) {
                    ret = av_send_msg(app, id, type, len, data, NULL);
                    if (ret) break;
                } else {
                    ret = av_push_msg(app, id, type, len, data);
                }
            }
        }
    }

    return ret;
}
