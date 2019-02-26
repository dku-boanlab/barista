/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, const FUNC_TYPE *data)
{
    int av_num = av_ctx->av_num[type];
    app_t **av_list = av_ctx->av_list[type];

    if (av_list == NULL) return -1;

    app_event_out_t av_out;
    app_event_t *av = (app_event_t *)&av_out;

    av_out.id = id;
    av_out.type = type;
    av_out.length = len;

    av->FUNC_DATA = data;

#ifdef __ENABLE_META_EVENTS
    av_ctx->num_app_events[type]++;
#endif /* __ENABLE_META_EVENTS */

#ifdef __ANALYZE_BARISTA
    print_current_app_event(type);
#endif /* __ANALYZE_BARISTA */

    int i;
    for (i=0; i<av_num; i++) {
        app_t *a = av_list[i];

        if (!a) continue;
        else if (!a->activated) continue; // not activated yet

#ifdef ODP_FUNC
        if (ODP_FUNC(a->odp, data)) continue;
#endif /* ODP_FUNC */

        if (a->perm & APP_WRITE) {
            if (a->site == APP_INTERNAL) { // internal site
#ifdef __ENABLE_META_EVENTS
                a->num_app_events[type]++;
#endif /* __ENABLE_META_EVENTS */

#ifdef __ANALYZE_BARISTA
                start_to_measure_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                int ret = a->handler(av, &av_out);
#ifdef __ANALYZE_BARISTA
                stop_measuring_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                if (ret && a->perm & APP_EXECUTE) {
                    break;
                }
            } else { // external site
                app_event_out_t *out = &av_out;

#ifdef __ENABLE_META_EVENTS
                a->num_app_events[type]++;
#endif /* __ENABLE_META_EVENTS */

#ifdef __ANALYZE_BARISTA
                start_to_measure_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                int ret = av_send_ext_msg(a, id, type, len, data, out->data);
#ifdef __ANALYZE_BARISTA
                stop_measuring_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                if (ret && a->perm & APP_EXECUTE) {
                    break;
                }
            }
        } else {
            if (a->site == APP_INTERNAL) { // internal site
#ifdef __ENABLE_META_EVENTS
                a->num_app_events[type]++;
#endif /* __ENABLE_META_EVENTS */

#ifdef __ANALYZE_BARISTA
                start_to_measure_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                int ret = a->handler(av, NULL);
#ifdef __ANALYZE_BARISTA
                stop_measuring_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                if (ret && a->perm & APP_EXECUTE) {
                    break;
                }
            } else { // external site
                app_event_out_t *out = &av_out;

#ifdef __ENABLE_META_EVENTS
                a->num_app_events[type]++;
#endif /* __ENABLE_META_EVENTS */

                if (a->perm & APP_EXECUTE) {
#ifdef __ANALYZE_BARISTA
                    start_to_measure_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                    int ret = av_send_ext_msg(a, id, type, len, data, out->data);
#ifdef __ANALYZE_BARISTA
                    stop_measuring_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                    if (ret) {
                        break;
                    }
                } else {
#ifdef __ANALYZE_BARISTA
                    start_to_measure_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                    av_push_ext_msg(a, id, type, len, data);
#ifdef __ANALYZE_BARISTA
                    stop_measuring_app_time(a->name, type);
#endif /* __ANALYZE_BARISTA */
                }
            }
        }
    }

    return 0;
}

