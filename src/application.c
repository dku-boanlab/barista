/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup app_mgmt Application Management
 * \brief Functions to configure and manage applications
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "application.h"
#include "application_list.h"
#include "app_event.h"

/////////////////////////////////////////////////////////////////////

/** \brief Global context structure */
ctx_t *app_ctx;

/////////////////////////////////////////////////////////////////////

/** \brief The number of base applications */
#define BASE_APPS 1

/** \brief The list of base applications */
char base[BASE_APPS][__CONF_WORD_LEN] = {"appint"};

/////////////////////////////////////////////////////////////////////

/** \brief The app event list to convert an app event string to an app event ID */
const char app_event_string[AV_NUM_EVENTS+1][__CONF_WORD_LEN] = {
#include "app_event_string.h"
};

/**
 * \brief Function to convert an app event string to an app event ID
 * \param app_event App event name
 * \return App event ID
 */
static int app_event_type(const char *app_event)
{
    int i;
    for (i=0; i<AV_NUM_EVENTS+1; i++) {
        if (strcmp(app_event, app_event_string[i]) == 0) {
            return i;
        }
    }
    return AV_NUM_EVENTS;
}

/**
 * \brief Function to print applications that listen to an app event
 * \param cli CLI context
 * \param id App event ID
 */
static int app_event_print(cli_t *cli, int id)
{
    char buf[__CONF_STR_LEN];

    cli_bufcls(buf);
    cli_buffer(buf, "  ");

    int i;
    for (i=0; i<app_ctx->av_num[id]; i++) {
        if (app_ctx->av_list[id][i]->status == APP_ENABLED) {
            cli_buffer(buf, "%s ", app_ctx->av_list[id][i]->name);
        } else {
            cli_buffer(buf, "(%s) ", app_ctx->av_list[id][i]->name);
        }
    }

    cli_bufprt(cli, buf);

    return 0;
}

/**
 * \brief Function to print an app event and all applications that listen to the app event
 * \param cli CLI context
 * \param name App event name
 */
int app_event_show(cli_t *cli, char *name)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    if (strcmp(name, "AV_NONE") == 0) {
        return -1;
    } else if (strcmp(name, "AV_ALL_UPSTREAM") == 0) {
        cli_print(cli, "<Application Event - AV_ALL_UPSTREAM>");
        int i;
        for (i=AV_NONE+1; i<AV_ALL_UPSTREAM; i++)
            app_event_print(cli, i);
        return 0;
    } else if (strcmp(name, "AV_ALL_DOWNSTREAM") == 0) {
        cli_print(cli, "<Application Event - AV_ALL_DOWNSTREAM>");
        int i;
        for (i=AV_ALL_UPSTREAM+1; i<AV_ALL_DOWNSTREAM; i++)
            app_event_print(cli, i);
        return 0;
    } else if (strcmp(name, "AV_WRT_INTSTREAM") == 0) {
        cli_print(cli, "<Application Event - AV_WRT_INTSTREAM>");
        int i;
        for (i=AV_ALL_DOWNSTREAM+1; i<AV_WRT_INTSTREAM; i++)
            app_event_print(cli, i);
        return 0;
    } else if (strcmp(name, "AV_ALL_INTSTREAM") == 0) {
        cli_print(cli, "<Application Event - AV_ALL_INTSTREAM>");
        int i;
        for (i=AV_WRT_INTSTREAM+1; i<AV_ALL_INTSTREAM; i++)
            app_event_print(cli, i);
        return 0;
    } else {
        int i;
        for (i=0; i<AV_ALL_INTSTREAM; i++) {
            if (strcmp(name, app_event_string[i]) == 0) {
                cli_print(cli, "<Application Event - %s>", name);
                app_event_print(cli, i);
                return 0;
            }
        }
    }

    cli_print(cli, "No application event whose name is %s", name);

    return -1;
}

/**
 * \brief Function to print all app events
 * \param cli CLI context
 */
int app_event_list(cli_t *cli)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    cli_print(cli, "<Application Event List>");

    int i;
    for (i=0; i<AV_ALL_INTSTREAM; i++) {
        if (strcmp(app_event_string[i], "AV_NONE") == 0)
            continue;
        else if (strcmp(app_event_string[i], "AV_ALL_UPSTREAM") == 0)
            continue;
        else if (strcmp(app_event_string[i], "AV_ALL_DOWNSTREAM") == 0)
            continue;
        else if (strcmp(app_event_string[i], "AV_WRT_INTSTREAM") == 0)
            continue;
        else if (strcmp(app_event_string[i], "AV_ALL_INTSTREAM") == 0)
            continue;
        else {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "  %s: ", app_event_string[i]);

            int j;
            for (j=0; j<app_ctx->av_num[i]; j++) {
                app_t *a = app_ctx->av_list[i][j];
                if (a->status == APP_ENABLED) {
                    cli_buffer(buf, "%s ", a->name);
                } else {
                    cli_buffer(buf, "(%s) ", a->name);
                }
            }

            cli_bufprt(cli, buf);
        }
    }

    return 0;
}

/**
 * \brief Function to print the configuration of an application
 * \param cli CLI context
 * \param a Application context
 * \param details The flag to enable the detailed description
 */
static int application_print(cli_t *cli, app_t *a, int details)
{
    char buf[__CONF_STR_LEN];

    cli_bufcls(buf);
    cli_buffer(buf, "%2d: %s", a->id+1, a->name);

    if (details == FALSE) {
        cli_buffer(buf, " (");
        if (a->status == APP_DISABLED)
            cli_buffer(buf, "disabled");
        else if (a->activated && (a->site == APP_INTERNAL))
            cli_buffer(buf, ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
        else if (a->activated && (a->site == APP_EXTERNAL))
            cli_buffer(buf, ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
        else if (a->site == APP_EXTERNAL)
            cli_buffer(buf, ANSI_COLOR_MAGENTA "ready to talk" ANSI_COLOR_RESET);
        else
            cli_buffer(buf, ANSI_COLOR_BLUE "enabled" ANSI_COLOR_RESET);
        cli_buffer(buf, ")");

        cli_bufprt(cli, buf);

        return 0;
    } else {
        cli_bufprt(cli, buf);
    }

    cli_bufcls(buf);
    cli_buffer(buf, "    Arguments: ");

    int i;
    for (i=0; i<a->argc; i++)
        cli_buffer(buf, "%s ", a->argv[i]);

    cli_bufprt(cli, buf);

    if (a->type == APP_AUTO)
        cli_print(cli, "    Type: autonomous");
    else
        cli_print(cli, "    Type: general");

    if (a->site == APP_INTERNAL)
        cli_print(cli, "    Site: internal");
    else
        cli_print(cli, "    Site: external");

    if (a->role == APP_ADMIN)
        cli_print(cli, "    Role: admin");
    else if (a->role == APP_SECURITY)
        cli_print(cli, "    Role: security");
    else if (a->role == APP_MANAGEMENT)
        cli_print(cli, "    Role: management");
    else if (a->role == APP_NETWORK)
        cli_print(cli, "    Role: network");

    cli_bufcls(buf);
    cli_buffer(buf, "    Permission: ");

    if (a->perm & APP_READ) cli_buffer(buf, "r");
    if (a->perm & APP_WRITE) cli_buffer(buf, "w");
    if (a->perm & APP_EXECUTE) cli_buffer(buf, "x");

    cli_bufprt(cli, buf);

    if (a->status == APP_DISABLED)
        cli_print(cli, "    Status: disabled");
    else if (a->activated == TRUE)
        cli_print(cli, "    Status: " ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
    else if (a->status == APP_ENABLED)
        cli_print(cli, "    Status: " ANSI_COLOR_BLUE "enabled" ANSI_COLOR_RESET);

    cli_bufcls(buf);
    cli_buffer(buf, "    Inbounds: ");

    for (i=0; i<a->in_num; i++) {
        cli_buffer(buf, "%s(%d) ", app_event_string[a->in_list[i]], a->in_list[i]);
    }

    cli_bufprt(cli, buf);

    cli_bufcls(buf);
    cli_buffer(buf, "    Outbounds: ");

    for (i=0; i<a->out_num; i++) {
        cli_buffer(buf, "%s(%d) ", app_event_string[a->out_list[i]], a->out_list[i]);
    }

    cli_bufprt(cli, buf);

    return 0;
}

/**
 * \brief Function to print an application configuration
 * \param cli CLI context
 * \param name Application name
 */
int application_show(cli_t *cli, char *name)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            cli_print(cli, "<Application Information>");
            application_print(cli, app_ctx->app_list[i], TRUE);
            return 0;
        }
    }

    cli_print(cli, "No application whose name is %s", name);

    return -1;
}

/**
 * \brief Function to print all application configurations
 * \param cli CLI context
 */
int application_list(cli_t *cli)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    cli_print(cli, "<Application List>");

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        application_print(cli, app_ctx->app_list[i], FALSE);
    }

    return 0;
}

/**
 * \brief Function to enable an application
 * \param cli CLI context
 * \param name Application name
 */
int application_enable(cli_t *cli, char *name)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            if (app_ctx->app_list[i]->status != APP_ENABLED) {
                app_ctx->app_list[i]->status = APP_ENABLED;
                cli_print(cli, "%s is enabled", name);
            } else {
                cli_print(cli, "%s is already enabled", name);
            }
            
            return 0;
        }
    }

    return -1;
}

/**
 * \brief Function to disable an application
 * \param cli CLI context
 * \param name Application name
 */
int application_disable(cli_t *cli, char *name)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            if (app_ctx->app_list[i]->status != APP_DISABLED) {
                app_ctx->app_list[i]->status = APP_DISABLED;
                cli_print(cli, "%s is disabled", name);
            } else {
                cli_print(cli, "%s is already disabled", name);
            }

            return 0;
        }
    }

    return -1;
}

/**
 * \brief The thread for an autonomous application
 * \param a_id Application ID
 */
static void *app_thread_main(void *a_id)
{
    int id = *(int *)a_id;

    FREE(a_id);

    app_t *a = app_ctx->app_list[id];

    if (a->main(&a->activated, a->argc, a->argv) < 0) {
        a->activated = FALSE;
        return NULL;
    } else {
        return a;
    }
}

/**
 * \brief Function to activate an application
 * \param cli CLI context
 * \param name Application name
 */
int application_activate(cli_t *cli, char *name)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        // check application name
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            // enabled application?
            if (app_ctx->app_list[i]->status == APP_ENABLED) {
                // not activated yet?
                if (app_ctx->app_list[i]->activated == FALSE) {
                    // internal?
                    if (app_ctx->app_list[i]->site == APP_INTERNAL) {
                        // general type?
                        if (app_ctx->app_list[i]->type == APP_GENERAL) {
                            int *id = (int *)MALLOC(sizeof(int));
                            *id = app_ctx->app_list[i]->id;
                            if (app_thread_main(id) == NULL) {
                                cli_print(cli, "Failed to activate %s", app_ctx->app_list[i]->name);
                                return -1;
                            } else {
                                cli_print(cli, "%s is activated", app_ctx->app_list[i]->name);
                                return 0;
                            }
                        // autonomous type?
                        } else {
                            pthread_t thread;
                            int *id = (int *)MALLOC(sizeof(int));
                            *id = app_ctx->app_list[i]->id;
                            if (pthread_create(&thread, NULL, &app_thread_main, (void *)id)) {
                                cli_print(cli, "Failed to activate %s", app_ctx->app_list[i]->name);
                                return -1;
                            } else {
                                cli_print(cli, "%s is activated", app_ctx->app_list[i]->name);
                                return 0;
                            }
                        }
                    // external?
                    } else {
                        int j;
                        for (j=0; j<__NUM_PULL_THREADS; j++) {
                            app_ctx->app_list[i]->push_sock[j] = 0;
                            pthread_spin_init(&app_ctx->app_list[i]->push_lock[j], PTHREAD_PROCESS_PRIVATE);
                        }

                        app_ctx->app_list[i]->req_ctx = zmq_ctx_new();

                        cli_print(cli, "%s is ready to talk", app_ctx->app_list[i]->name);
                    }
                } else {
                    cli_print(cli, "%s is already activated", name);
                    return 0;
                }
            } else {
                cli_print(cli, "%s is not enabled", name);
                return -1;
            }
            break;
        }
    }

    return 0;
}

/**
 * \brief Function to deactivate an application
 * \param cli CLI context
 * \param name Application name
 */
int application_deactivate(cli_t *cli, char *name)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        // check application name
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            // enabled application?
            if (app_ctx->app_list[i]->status == APP_ENABLED) {
                // already activated?
                if (app_ctx->app_list[i]->activated) {
                    // internal?
                    if (app_ctx->app_list[i]->site == APP_INTERNAL) {
                        if (app_ctx->app_list[i]->cleanup(&app_ctx->app_list[i]->activated) < 0) {
                            cli_print(cli, "Failed to deactivate %s", app_ctx->app_list[i]->name);
                            return -1;
                        } else {
                            app_ctx->app_list[i]->activated = FALSE;
                            cli_print(cli, "%s is deactivated", app_ctx->app_list[i]->name);
                            return 0;
                        }
                    // external?
                    } else {
                        int j;
                        for (j=0; j<__NUM_PULL_THREADS; j++) {
                            pthread_spin_destroy(&app_ctx->app_list[i]->push_lock[j]);
                            close(app_ctx->app_list[i]->push_sock[j]);
                            app_ctx->app_list[i]->push_sock[j] = 0;
                        }

                        zmq_ctx_destroy(app_ctx->app_list[i]->req_ctx);
                        app_ctx->app_list[i]->req_ctx = NULL;

                        app_ctx->app_list[i]->activated = FALSE;
                        cli_print(cli, "%s is deactivated", app_ctx->app_list[i]->name);
                    }
                } else {
                    cli_print(cli, "%s is not activated yet", name);
                    return -1;
                }
            } else {
                cli_print(cli, "%s is not enabled", name);
                return -1;
            }
            break;
        }
    }

    return -1;
}

/**
 * \brief Function to activate all enabled applications
 * \param cli CLI context
 */
int application_start(cli_t *cli)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    if (app_ctx->app_on == TRUE) {
        cli_print(cli, "Applications are already started");
        return -1;
    }

    int i, appint = -1;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, "appint") == 0) {
            if (app_ctx->app_list[i]->status == APP_ENABLED)
                appint = i;
            continue;
        }
    }

    // activate appint, enabled application?
    if (appint != -1 && app_ctx->app_list[appint]->status == APP_ENABLED) {
        // not activated yet?
        if (app_ctx->app_list[appint]->activated == FALSE) {
            // internal?
            if (app_ctx->app_list[appint]->site == APP_INTERNAL) {
                // general type?
                if (app_ctx->app_list[appint]->type == APP_GENERAL) {
                    int *id = (int *)MALLOC(sizeof(int));
                    *id = app_ctx->app_list[appint]->id;
                    if (app_thread_main(id) == NULL) {
                        cli_print(cli, "Failed to activate %s", app_ctx->app_list[appint]->name);
                        return -1;
                    } else {
                        cli_print(cli, "%s is activated", app_ctx->app_list[appint]->name);
                    }
                // autonomous type?
                } else {
                    pthread_t thread;
                    int *id = (int *)MALLOC(sizeof(int));
                    *id = app_ctx->app_list[appint]->id;
                    if (pthread_create(&thread, NULL, &app_thread_main, (void *)id)) {
                        cli_print(cli, "Failed to activate %s", app_ctx->app_list[appint]->name);
                        return -1;
                    } else {
                        cli_print(cli, "%s is activated", app_ctx->app_list[appint]->name);
                    }
                }
            // external?
            } else {
                int j;
                for (j=0; j<__NUM_PULL_THREADS; j++) {
                    app_ctx->app_list[appint]->push_sock[j] = 0;
                    pthread_spin_init(&app_ctx->app_list[appint]->push_lock[j], PTHREAD_PROCESS_PRIVATE);
                }

                app_ctx->app_list[appint]->req_ctx = zmq_ctx_new();

                cli_print(cli, "%s is ready to talk", app_ctx->app_list[appint]->name);
            }
        }
    }

    // activate others
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, "appint") == 0)
            continue;

        // enabled application?
        if (app_ctx->app_list[i]->status == APP_ENABLED) {
            // not activated yet?
            if (app_ctx->app_list[i]->activated == FALSE) {
                // internal?
                if (app_ctx->app_list[i]->site == APP_INTERNAL) {
                    // general type?
                    if (app_ctx->app_list[i]->type == APP_GENERAL) {
                        int *id = (int *)MALLOC(sizeof(int));
                        *id = app_ctx->app_list[i]->id;
                        if (app_thread_main(id) == NULL) {
                            cli_print(cli, "Failed to activate %s", app_ctx->app_list[i]->name);
                            return -1;
                        } else {
                            cli_print(cli, "%s is activated", app_ctx->app_list[i]->name);
                        }
                    // autonomous type?
                    } else {
                        pthread_t thread;
                        int *id = (int *)MALLOC(sizeof(int));
                        *id = app_ctx->app_list[i]->id;
                        if (pthread_create(&thread, NULL, &app_thread_main, (void *)id)) {
                            cli_print(cli, "Failed to activate %s", app_ctx->app_list[i]->name);
                            return -1;
                        } else {
                            cli_print(cli, "%s is activated", app_ctx->app_list[i]->name);
                        }
                    }
                // external?
                } else {
                    int j;
                    for (j=0; j<__NUM_PULL_THREADS; j++) {
                        app_ctx->app_list[i]->push_sock[j] = 0;
                        pthread_spin_init(&app_ctx->app_list[i]->push_lock[j], PTHREAD_PROCESS_PRIVATE);
                    }

                    app_ctx->app_list[i]->req_ctx = zmq_ctx_new();

                    cli_print(cli, "%s is ready to talk", app_ctx->app_list[i]->name);
                }
            }
        }
    }

    app_ctx->app_on = TRUE;

    return 0;
}

/**
 * \brief Function to deactivate all activated applications
 * \param cli CLI context
 */
int application_stop(cli_t *cli)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    if (app_ctx->app_on == FALSE) {
        cli_print(cli, "Applications are not started yet");
        return -1;
    }

    // deactivate others except appint
    int i, appint = -1;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, "appint") == 0) {
            if (app_ctx->app_list[i]->activated)
                appint = i;
            continue;
        }

        // enabled application?
        if (app_ctx->app_list[i]->status == APP_ENABLED) {
            // already activated?
            if (app_ctx->app_list[i]->activated) {
                // internal?
                if (app_ctx->app_list[i]->site == APP_INTERNAL) {
                    if (app_ctx->app_list[i]->cleanup(&app_ctx->app_list[i]->activated) < 0) {
                        cli_print(cli, "Failed to deactivate %s", app_ctx->app_list[i]->name);
                        return -1;
                    } else {
                        app_ctx->app_list[i]->activated = FALSE;
                        cli_print(cli, "%s is deactivated", app_ctx->app_list[i]->name);
                    }
                // external?
                } else {
                    int j;
                    for (j=0; j<__NUM_PULL_THREADS; j++) {
                        pthread_spin_destroy(&app_ctx->app_list[i]->push_lock[j]);
                        close(app_ctx->app_list[i]->push_sock[j]);
                        app_ctx->app_list[i]->push_sock[j] = 0;
                    }

                    zmq_ctx_destroy(app_ctx->app_list[i]->req_ctx);
                    app_ctx->app_list[i]->req_ctx = NULL;

                    app_ctx->app_list[i]->activated = FALSE;
                    cli_print(cli, "%s is deactivated", app_ctx->app_list[i]->name);
                }
            }
        }
    }

    // deactivate appint
    if (appint != -1 && app_ctx->app_list[appint]->activated) {
        // internal?
        if (app_ctx->app_list[appint]->site == APP_INTERNAL) {
            if (app_ctx->app_list[appint]->cleanup(&app_ctx->app_list[appint]->activated) < 0) {
                cli_print(cli, "Failed to deactivate %s", app_ctx->app_list[appint]->name);
                return -1;
            } else {
                app_ctx->app_list[appint]->activated = FALSE;
                cli_print(cli, "%s is deactivated", app_ctx->app_list[appint]->name);
            }
        // external?
        } else {
            int j;
            for (j=0; j<__NUM_PULL_THREADS; j++) {
                pthread_spin_destroy(&app_ctx->app_list[appint]->push_lock[j]);
                close(app_ctx->app_list[appint]->push_sock[j]);
                app_ctx->app_list[appint]->push_sock[j] = 0;
            }

            zmq_ctx_destroy(app_ctx->app_list[appint]->req_ctx);
            app_ctx->app_list[appint]->req_ctx = NULL;

            app_ctx->app_list[appint]->activated = FALSE;
            cli_print(cli, "%s is deactivated", app_ctx->app_list[appint]->name);
        }
    }

    app_ctx->app_on = FALSE;

    return 0;
}

/**
 * \brief Function to add a policy to an application
 * \param cli CLI context
 * \param name Application name
 * \param odp Operator-defined policy
 */
int application_add_policy(cli_t *cli, char *name, char *odp)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    app_t *a = NULL;

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            a = app_ctx->app_list[i];
            break;
        }
    }

    if (a == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    int cnt = 0;
    char parm[__NUM_OF_ODP_FIELDS][__CONF_WORD_LEN] = {{0}};
    char val[__NUM_OF_ODP_FIELDS][__CONF_WORD_LEN] = {{0}};

    cli_print(cli, "Policy: %s, %s", a->name, odp);

    char *token = strtok(odp, ";");
    while (token != NULL) {
        if (sscanf(token, "%[^':']:%[^':']", parm[cnt], val[cnt]) != 2) {
            return -1;
        }
        cnt++;

        token = strtok(NULL, ";");
    }

    for (i=0; i<cnt; i++) {
        if (strcmp(parm[i], "dpid") == 0) {
            int idx = 0;
            char *v = strtok(val[i], ",");

            while (v != NULL) {
                if (idx < __MAX_POLICY_ENTRIES) {
                    a->odp[a->num_policies].flag |= ODP_DPID;
                    a->odp[a->num_policies].dpid[idx] = atoi(v);
                    cli_print(cli, "\tDPID: %lu", a->odp[a->num_policies].dpid[idx]);
                    idx++;
                }
                v = strtok(NULL, ",");
            }
        } else if (strcmp(parm[i], "port") == 0) {
            int idx = 0;
            char *v = strtok(val[i], ",");

            while (v != NULL) {
                if (idx < __MAX_POLICY_ENTRIES) {
                    if (atoi(v) <= 0 || atoi(v) >= __MAX_NUM_PORTS) {
                        cli_print(cli, "\tPport: %s (wrong)", v);
                    } else {
                        a->odp[a->num_policies].flag |= ODP_PORT;
                        a->odp[a->num_policies].port[idx] = atoi(v);
                        cli_print(cli, "\tPort: %u", a->odp[a->num_policies].port[idx]);
                        idx++;
                    }
                }
                v = strtok(NULL, ",");
            }
        } else if (strcmp(parm[i], "proto") == 0) {
            int idx = 0;
            char *v = strtok(val[i], ",");

            while (v != NULL) {
                if (idx < __MAX_POLICY_ENTRIES) {
                    if (strcmp(v, "arp") == 0) {
                        a->odp[a->num_policies].flag |= ODP_PROTO;
                        a->odp[a->num_policies].proto |= PROTO_ARP;
                        cli_print(cli, "\tProtocol: ARP");
                    } else if (strcmp(v, "lldp") == 0) {
                        a->odp[a->num_policies].flag |= ODP_PROTO;
                        a->odp[a->num_policies].proto |= PROTO_LLDP;
                        cli_print(cli, "\tProtocol: LLDP");
                    } else if (strcmp(v, "dhcp") == 0) {
                        a->odp[a->num_policies].flag |= ODP_PROTO;
                        a->odp[a->num_policies].proto |= PROTO_DHCP;
                        cli_print(cli, "\tProtocol: DHCP");
                    } else if (strcmp(v, "ipv4") == 0) {
                        a->odp[a->num_policies].flag |= ODP_PROTO;
                        a->odp[a->num_policies].proto |= PROTO_IPV4;
                        cli_print(cli, "\tProtocol: IPv4");
                    } else if (strcmp(v, "tcp") == 0) {
                        a->odp[a->num_policies].flag |= ODP_PROTO;
                        a->odp[a->num_policies].proto |= PROTO_TCP;
                        cli_print(cli, "\tProtocol: TCP");
                    } else if (strcmp(v, "udp") == 0) {
                        a->odp[a->num_policies].flag |= ODP_PROTO;
                        a->odp[a->num_policies].proto |= PROTO_UDP;
                        cli_print(cli, "\tProtocol: UDP");
                    } else if (strcmp(v, "icmp") == 0) {
                        a->odp[a->num_policies].flag |= ODP_PROTO;
                        a->odp[a->num_policies].proto |= PROTO_ICMP;
                        cli_print(cli, "\tProtocol: ICMP");
                    } else {
                        cli_print(cli, "\tProtocol: %s (wrong)", v);
                    }
                }
                v = strtok(NULL, ",");
            }
        } else if (strcmp(parm[i], "srcip") == 0) {
            int idx = 0;
            char *v = strtok(val[i], ",");

            while (v != NULL) {
                if (idx < __MAX_POLICY_ENTRIES) {
                    struct in_addr input;
                    if (inet_aton(v, &input)) {
                        a->odp[a->num_policies].flag |= ODP_SRCIP;
                        a->odp[a->num_policies].srcip[idx] = ip_addr_int(v);
                        cli_print(cli, "\tSource IP: %s", v);
                        idx++;
                    } else {
                        cli_print(cli, "\tSource IP: %s (wrong)", v);
                    }
                }
                v = strtok(NULL, ",");
            }
        } else if (strcmp(parm[i], "dstip") == 0) {
            int idx = 0;
            char *v = strtok(val[i], ",");

            while (v != NULL) {
                if (idx < __MAX_POLICY_ENTRIES) {
                    struct in_addr input;
                    if (inet_aton(v, &input)) {
                        a->odp[a->num_policies].flag |= ODP_DSTIP;
                        a->odp[a->num_policies].dstip[idx] = ip_addr_int(v);
                        cli_print(cli, "\tDestination IP: %s", v);
                        idx++;
                    } else {
                        cli_print(cli, "\tDestination IP: %s (wrong)", v);
                    }
                }
                v = strtok(NULL, ",");
            }
        } else if (strcmp(parm[i], "sport") == 0) {
            int idx = 0;
            char *v = strtok(val[i], ",");

            while (v != NULL) {
                if (idx < __MAX_POLICY_ENTRIES) {
                    uint32_t port = atoi(v);
                    if (port == 0 || port >= 65536) {
                        cli_print(cli, "\tSource port: %s (wrong)", v);
                    } else {
                        a->odp[a->num_policies].flag |= ODP_SPORT;
                        a->odp[a->num_policies].sport[idx] = port;
                        cli_print(cli, "\tSource port: %u", port);
                        idx++;
                    }
                }
                v = strtok(NULL, ",");
            }
        } else if (strcmp(parm[i], "dport") == 0) {
            int idx = 0;
            char *v = strtok(val[i], ",");

            while (v != NULL) {
                if (idx < __MAX_POLICY_ENTRIES) {
                    uint32_t port = atoi(v);
                    if (port == 0 || port >= 65536) {
                        cli_print(cli, "\tDestination port: %s (wrong)", v);
                    } else {
                        a->odp[a->num_policies].flag |= ODP_DPORT;
                        a->odp[a->num_policies].dport[idx] = port;
                        cli_print(cli, "\tDestination port: %u", port);
                        idx++;
                    }
                }
                v = strtok(NULL, ",");
            }
        } else {
            cli_print(cli, "Skipped argument: %s", parm[i]);
        }
    }

    a->num_policies++;

    return 0;
}

/**
 * \brief Function to delete a policy from an application
 * \param cli CLI context
 * \param name Application name
 * \param idx The index of the target operator-defined policy
 */
int application_del_policy(cli_t *cli, char *name, int idx)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    app_t *a = NULL;

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            a = app_ctx->app_list[i];
            break;
        }
    }

    if (a == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    if (a->num_policies < idx) {
        if (a->num_policies == 0) {
            cli_print(cli, "There is no policy in %s", a->name);
            return -1;
        } else {
            cli_print(cli, "%s has only %u policies", a->name, a->num_policies);
            return -1;
        }
    }

    memset(&a->odp[idx-1], 0, sizeof(odp_t));

    for (i=idx; i<__MAX_POLICY_ENTRIES; i++) {
        memmove(&a->odp[i-1], &a->odp[i], sizeof(odp_t));
    }

    memset(&a->odp[i-1], 0, sizeof(odp_t));

    a->num_policies--;

    cli_print(cli, "Deleted policy #%u in %s", idx, a->name);

    return 0;
}

/**
 * \brief Function to show all policies applied to an application
 * \param cli CLI context
 * \param name Application name
 */
int application_show_policy(cli_t *cli, char *name)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    app_t *a = NULL;

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, name) == 0) {
            a = app_ctx->app_list[i];
            break;
        }
    }

    if (a == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    cli_print(cli, "<Operator-Defined Policies for %s>", a->name);

    for (i=0; i<a->num_policies; i++) {
        cli_print(cli, "  Policy #%02u:", i+1);

        if (a->odp[i].dpid[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Datapath ID: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (a->odp[i].dpid[j] == 0)
                    break;
                cli_buffer(buf, "%lu ", a->odp[i].dpid[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (a->odp[i].port[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (a->odp[i].port[j] == 0)
                    break;
                cli_buffer(buf, "%u ", a->odp[i].port[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (a->odp[i].proto) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Protocol: ");

            if (a->odp[i].proto & PROTO_ARP)
                cli_buffer(buf, "ARP ");
            if (a->odp[i].proto & PROTO_LLDP)
                cli_buffer(buf, "LLDP ");
            if (a->odp[i].proto & PROTO_DHCP)
                cli_buffer(buf, "DHCP ");
            if (a->odp[i].proto & PROTO_IPV4)
                cli_buffer(buf, "IPv4 ");
            if (a->odp[i].proto & PROTO_TCP)
                cli_buffer(buf, "TCP ");
            if (a->odp[i].proto & PROTO_UDP)
                cli_buffer(buf, "UDP ");
            if (a->odp[i].proto & PROTO_ICMP)
                cli_buffer(buf, "ICMP ");

            cli_bufprt(cli, buf);
        }

        if (a->odp[i].srcip[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Source IP: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (a->odp[i].srcip[j] == 0)
                    break;
                cli_buffer(buf, "%s ", ip_addr_str(a->odp[i].srcip[j]));
            }

            cli_bufprt(cli, buf);
        }

        if (a->odp[i].dstip[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Destination IP: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (a->odp[i].dstip[j] == 0)
                    break;
                cli_buffer(buf, "%s ", ip_addr_str(a->odp[i].dstip[j]));
            }

            cli_bufprt(cli, buf);
        }

        if (a->odp[i].sport[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Source port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (a->odp[i].sport[j] == 0)
                    break;
                cli_buffer(buf, "%u ", a->odp[i].sport[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (a->odp[i].dport[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Destination port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (a->odp[i].dport[j] == 0)
                    break;
                cli_buffer(buf, "%u ", a->odp[i].dport[j]);
            }

            cli_bufprt(cli, buf);
        }
    }

    if (!a->num_policies)
        cli_print(cli, "  No policy");

    return 0;
}

/**
 * \brief Function to deliver a command to the corresponding application
 * \param cli CLI context
 * \param args Arguments
 */
int application_cli(cli_t *cli, char **args)
{
    if (app_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<app_ctx->num_apps; i++) {
        if (strcmp(app_ctx->app_list[i]->name, args[0]) == 0) {
            if (app_ctx->app_list[i]->activated == TRUE) {
                if (app_ctx->app_list[i]->cli != NULL) {
                    app_ctx->app_list[i]->cli(cli, &args[1]);
                }
            }
            return 0;
        }
    }

    return 0;
}

/**
 * \brief Function to deallocate all configuration structures
 * \param num_apps The number of registered applications
 * \param app_list The list of the applications
 * \param av_num The number of applications for each app event
 * \param av_list The applications listening to each app event
 */
static int clean_structs(int num_apps, app_t **app_list, int *av_num, app_t ***av_list)
{
    if (app_list != NULL) {
        int i;
        for (i=0; i<num_apps; i++) {
            if (app_list[i] != NULL)
                FREE(app_list[i]);
        }
        FREE(app_list);
    }

    if (av_num != NULL) {
        FREE(av_num);
    }

    if (av_list != NULL) {
        int i;
        for (i=0; i<__MAX_APP_EVENTS; i++) {
            if (av_list[i] != NULL)
                FREE(av_list[i]);
        }
        FREE(av_list);
    }

    return 0;
}

/**
 * \brief Function to load application configurations from a file
 * \param cli CLI context
 * \param ctx The context of the Barista NOS
 */
int application_load(cli_t *cli, ctx_t *ctx)
{
    json_t *json;
    json_error_t error;

    if (app_ctx == NULL) {
        app_ctx = ctx;
    }

    cli_print(cli, "Application configuration file: %s", ctx->app_conf_file);

    if (access(ctx->app_conf_file, F_OK)) {
        cli_print(cli, "No file whose name is '%s'", ctx->app_conf_file);
        return -1;
    }

    char *raw = str_read(ctx->app_conf_file);
    char *conf = str_preproc(raw);

    json = json_loads(conf, 0, &error);
    if (!json) {
        PERROR("json_loads");
        return -1;
    }

    if (!json_is_array(json)) {
        PERROR("json_is_array");
        json_decref(json);
        return -1;
    }

    int n_apps = 0;
    app_t **app_list = (app_t **)MALLOC(sizeof(app_t *) * __MAX_APPLICATIONS);
    if (app_list == NULL) {
        PERROR("malloc");
        json_decref(json);
        return -1;
    } else {
        memset(app_list, 0, sizeof(app_t *) * __MAX_APPLICATIONS);
    }

    int *av_num = (int *)MALLOC(sizeof(int) * __MAX_APP_EVENTS);
    if (av_num == NULL) {
        PERROR("malloc");
        clean_structs(n_apps, app_list, NULL, NULL);
        json_decref(json);
        return -1;
    } else {
        memset(av_num, 0, sizeof(int) * __MAX_APP_EVENTS);
    }

    app_t ***av_list = (app_t ***)MALLOC(sizeof(app_t **) * __MAX_APP_EVENTS);
    if (av_list == NULL) {
        PERROR("malloc");
        clean_structs(n_apps, app_list, av_num, NULL);
        json_decref(json);
        return -1;
    } else {
        int i;
        for (i=0; i<__MAX_APP_EVENTS; i++) {
            av_list[i] = (app_t **)MALLOC(sizeof(app_t *) * __MAX_APPLICATIONS);
            if (av_list[i] == NULL) {
                PERROR("malloc");
                clean_structs(n_apps, app_list, av_num, av_list);
                json_decref(json);
                return -1;
            } else {
                memset(av_list[i], 0, sizeof(app_t *) * __MAX_APPLICATIONS);
            }
        }
    }

    int i;
    for (i=0; i<json_array_size(json); i++) {
        json_t *data = json_array_get(json, i);

        char name[__CONF_WORD_LEN] = {0};
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        char args[__CONF_WORD_LEN] = {0};
        json_t *j_args = json_object_get(data, "args");
        if (json_is_string(j_args)) {
            strcpy(args, json_string_value(j_args));
        }

        char type[__CONF_WORD_LEN] = {0};
        json_t *j_type = json_object_get(data, "type");
        if (json_is_string(j_type)) {
            strcpy(type, json_string_value(j_type));
        }

        char site[__CONF_WORD_LEN] = {0};
        json_t *j_site = json_object_get(data, "site");
        if (json_is_string(j_site)) {
            strcpy(site, json_string_value(j_site));
        }
        
	char role[__CONF_WORD_LEN] = {0};
        json_t *j_role = json_object_get(data, "role");
        if (json_is_string(j_role)) {
            strcpy(role, json_string_value(j_role));
        }

        char perm[__CONF_WORD_LEN] = {0};
        json_t *j_perm = json_object_get(data, "perm");
        if (json_is_string(j_perm)) {
            strcpy(perm, json_string_value(j_perm));
        }

        char priority[__CONF_WORD_LEN] = {0};
        json_t *j_pri = json_object_get(data, "priority");
        if (json_is_string(j_pri)) {
            strcpy(priority, json_string_value(j_pri));
        }

        char status[__CONF_WORD_LEN] = {0};
        json_t *j_status = json_object_get(data, "status");
        if (json_is_string(j_status)) {
            strcpy(status, json_string_value(j_status));
        }

        char pull_addr[__CONF_WORD_LEN] = {0};
        json_t *j_pull_addr = json_object_get(data, "pull_addr");
        if (json_is_string(j_pull_addr)) {
            strcpy(pull_addr, json_string_value(j_pull_addr));
        }

        char reply_addr[__CONF_WORD_LEN] = {0};
        json_t *j_reply_addr = json_object_get(data, "reply_addr");
        if (json_is_string(j_reply_addr)) {
            strcpy(reply_addr, json_string_value(j_reply_addr));
        }

        // find the index of an application to link the corresponding functions
        const int num_apps = sizeof(g_applications) / sizeof(app_func_t);
        int k;
        for (k=0; k<num_apps; k++) {
            if (strcmp(g_applications[k].name, name))
                continue;
            else
                break;
        }

        // allocate a new application structure
        app_t *a = calloc(1, sizeof(app_t));
        if (!a) {
            PERROR("calloc");
            clean_structs(n_apps, app_list, av_num, av_list);
            json_decref(json);
            return -1;
        }

        // set an internal ID (simply for order)
        a->id = n_apps;

        // set the application name
        if (strlen(name) == 0) {
            cli_print(cli, "No application name");
            FREE(a);
            clean_structs(n_apps, app_list, av_num, av_list);
            json_decref(json);
            return -1;
        } else {
            strcpy(a->name, name);
            cli_print(cli, "%3d: %s", a->id+1, name);

            // set the actual ID
            a->app_id = hash_func((uint32_t *)&a->name, __HASHING_NAME_LENGTH);

#ifdef __SHOW_COMPONENT_ID
            cli_print(cli, "     ID: %u", a->app_id);
#endif /* __SHOW_COMPONENT_ID */
        }

        // set arguments
        strcpy(a->args, args);
        str2args(a->args, &a->argc, &a->argv[1], __CONF_ARGC);
        a->argc++;
        a->argv[0] = a->name;
        if (strlen(a->args))
            cli_print(cli, "     Arguments: %s", a->args);

        // set a type
        if (strlen(type) == 0) {
            a->type = APP_GENERAL;
        } else {
            a->type = (strcmp(type, "autonomous") == 0) ? APP_AUTO : APP_GENERAL;
        }
        cli_print(cli, "     Type: %s", type);

        // set a site
        if (strlen(site) == 0) {
            a->site = APP_INTERNAL;
        } else {
            if (strcmp(site, "external") == 0)
                a->site = APP_EXTERNAL;
            else
                a->site = APP_INTERNAL;
        }
        cli_print(cli, "     Site: %s", site);

        // set functions
        if (a->site == APP_INTERNAL) { // internal
            if (k == num_apps) {
                FREE(a);
                continue;
            }

            a->main = g_applications[k].main;
            a->handler = g_applications[k].handler;
            a->cleanup = g_applications[k].cleanup;
            a->cli = g_applications[k].cli;
        } else { // external
            a->main = NULL;
            a->handler = NULL;
            a->cleanup = NULL;
            a->cli = NULL;

            if (strlen(pull_addr) > 0) {
                strcpy(a->pull_addr, pull_addr);
            } else {
                 cli_print(cli, "No pulling address");
                 FREE(a);
                 clean_structs(n_apps, app_list, av_num, av_list);
                 json_decref(json);
                 return -1;
            }

            if (strlen(reply_addr) > 0) {
                strcpy(a->reply_addr, reply_addr);
            } else {
                 cli_print(cli, "No replying address");
                 FREE(a);
                 clean_structs(n_apps, app_list, av_num, av_list);
                 json_decref(json);
                 return -1;
            }
        }
        
	// set a role
        if (strlen(role) == 0) {
            a->role = APP_NETWORK;
        } else {
            if (strcmp(role, "base") == 0)
                a->role = APP_BASE;
            else if (strcmp(role, "network") == 0)
                a->role = APP_NETWORK;
            else if (strcmp(role, "management") == 0)
                a->role = APP_MANAGEMENT;
            else if (strcmp(role, "security") == 0)
                a->role = APP_SECURITY;
            else if (strcmp(role, "admin") == 0)
                a->role = APP_ADMIN;
            else
                a->role = APP_NETWORK;
        }

        // check base applications
        if (a->role == APP_BASE) {
            if (strcmp(a->name, base[0]) != 0) {
                 cli_print(cli, "Unauthorized base application");
                 FREE(a);
                 clean_structs(n_apps, app_list, av_num, av_list);
                 json_decref(json);
                 return -1;
            }
        }
        cli_print(cli, "     Role: %s", role);

        // set permissions
        int size = (strlen(perm) > APP_MAX_PERM) ? APP_MAX_PERM : strlen(perm);
        int l;
        for (l=0; l<size; l++) {
            if (perm[l] == 'r') {
                a->perm |= APP_READ;
            } else if (perm[l] == 'w') {
                a->perm |= APP_WRITE;
            } else if (perm[l] == 'x') {
                a->perm |= APP_EXECUTE;
            }
        }
        if (a->perm == 0) a->perm |= APP_READ;
        cli_print(cli, "     Permission: %s", perm);

        // set priority
        if (strlen(priority) == 0)
            a->priority = 0;
        else
            a->priority = atoi(priority);
        cli_print(cli, "    Priority: %s", priority);

        // set a status
        if (strlen(status) == 0) {
            a->status = APP_DISABLED;
        } else {
            a->status = (strcmp(status, "enabled") == 0) ? APP_ENABLED : APP_DISABLED;
        }
        cli_print(cli, "     Status: %s", status);

        int real_event_cnt = 0;

        // set inbound events
        json_t *events = json_object_get(data, "inbounds");
        if (json_is_array(events)) {
            int j;

            // first, check whether there is a wrong event name or not
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                if (app_event_type(json_string_value(event)) == AV_NUM_EVENTS) {
                    cli_print(cli, "     Inbounds: wrong app event name");
                    FREE(a);
                    clean_structs(n_apps, app_list, av_num, av_list);
                    json_decref(json);
                    return -1;
                }
            }

            // then, set the inbound events
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                if (app_event_type(json_string_value(event)) == AV_NONE) {
                    break;
                } else if (app_event_type(json_string_value(event)) == AV_ALL_UPSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_NONE+1; m<AV_ALL_UPSTREAM; m++) {
                        real_event_cnt++;

                        a->in_list[a->in_num] = m;
                        a->in_num++;

                        av_list[m][av_num[m]] = a;
	                av_num[m]++;
                    }
                } else if (app_event_type(json_string_value(event)) == AV_ALL_DOWNSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_ALL_UPSTREAM+1; m<AV_ALL_DOWNSTREAM; m++) {
                        real_event_cnt++;

                        a->in_list[a->in_num] = m;
                        a->in_num++;

			av_list[m][av_num[m]] = a;
			av_num[m]++;
                    }
                } else if (app_event_type(json_string_value(event)) == AV_WRT_INTSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_ALL_DOWNSTREAM+1; m<AV_WRT_INTSTREAM; m++) {
                        real_event_cnt++;

                        a->in_list[a->in_num] = m;
                        a->in_num++;

                        av_list[m][av_num[m]] = a;
                        av_num[m]++;
                    }
                } else if (app_event_type(json_string_value(event)) == AV_ALL_INTSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_WRT_INTSTREAM+1; m<AV_ALL_INTSTREAM; m++) {
                        real_event_cnt++;

                        a->in_list[a->in_num] = m;
                        a->in_num++;

			av_list[m][av_num[m]] = a;
                        av_num[m]++;
                    }
                } else {
                    real_event_cnt++;

                    a->in_list[a->in_num] = app_event_type(json_string_value(event));
                    a->in_num++;

                    av_list[app_event_type(json_string_value(event))][av_num[app_event_type(json_string_value(event))]] = a;
	            av_num[app_event_type(json_string_value(event))]++;
                }
            }

            if (real_event_cnt == 0) cli_print(cli, "     Inbounds: no event");
            else if (real_event_cnt == 1) cli_print(cli, "     Inbounds: 1 event");
            else cli_print(cli, "     Inbounds: %d events", a->in_num);
        }

        real_event_cnt = 0;

        // set outbound events
        json_t *out_events = json_object_get(data, "outbounds");
        if (json_is_array(out_events)) {
            int j;

            // first, check whether there is a wrong event name or not
            for (j=0; j<json_array_size(out_events); j++) {
                json_t *out_event = json_array_get(out_events, j);

                if (app_event_type(json_string_value(out_event)) == AV_NUM_EVENTS) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(a);
                    clean_structs(n_apps, app_list, av_num, av_list);
                    json_decref(json);
                    return -1;
                }
            }

            // then, set the outbound events
            for (j=0; j<json_array_size(out_events); j++) {
                json_t *out_event = json_array_get(out_events, j);

                if (app_event_type(json_string_value(out_event)) == AV_NONE) {
                    break;
                } else if (app_event_type(json_string_value(out_event)) == AV_ALL_UPSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Outbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_NONE+1; m<AV_ALL_UPSTREAM; m++) {
                        real_event_cnt++;

                        a->out_list[a->out_num] = m;
                        a->out_num++;
                    }
                } else if (app_event_type(json_string_value(out_event)) == AV_ALL_DOWNSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Outbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_ALL_UPSTREAM+1; m<AV_ALL_DOWNSTREAM; m++) {
                        real_event_cnt++;

                        a->out_list[a->out_num] = m;
                        a->out_num++;
                    }
                } else if (app_event_type(json_string_value(out_event)) == AV_WRT_INTSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Outbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_ALL_DOWNSTREAM+1; m<AV_WRT_INTSTREAM; m++) {
                        real_event_cnt++;

                        a->out_list[a->out_num] = m;
                        a->out_num++;
                    }
                } else if (app_event_type(json_string_value(out_event)) == AV_ALL_INTSTREAM) {
                    if (APP_BASE < a->role && a->role < APP_SECURITY) {
                        cli_print(cli, "     Outbounds: lower-level role");
                        FREE(a);
                        clean_structs(n_apps, app_list, av_num, av_list);
                        json_decref(json);
                        return -1;
                    }

                    int m;
                    for (m=AV_WRT_INTSTREAM+1; m<AV_ALL_INTSTREAM; m++) {
                        real_event_cnt++;

                        a->out_list[a->out_num] = m;
                        a->out_num++;
                    }
                } else {
                    real_event_cnt++;

                    a->out_list[a->out_num] = app_event_type(json_string_value(out_event));
                    a->out_num++;
                }
            }

            if (real_event_cnt == 0) cli_print(cli, "     Outbounds: no event");
            else if (real_event_cnt == 1) cli_print(cli, "     Outbounds: 1 event");
            else cli_print(cli, "     Outbounds: %d events", a->out_num);
        }

        app_list[n_apps] = a;
        n_apps++;
    }

    json_decref(json);

    // sort applications based on the ordering policy
    for (i=1; i<AV_NUM_EVENTS; i++) {
        app_t **list = av_list[i];

        if (av_num[i] == 0)
            continue;

        // first, based on priority
        int j;
        for (j=0; j<av_num[i]-1; j++) {
            int k;
            for (k=0; k<av_num[i]-j-1; k++) {
                if (list[k]->priority < list[k+1]->priority) {
                    app_t *temp = list[k];
                    list[k] = list[k+1];
                    list[k+1] = temp;
                }
            }
        }

        // second, based on role
        for (j=0; j<av_num[i]-1; j++) {
            int k;
            for (k=0; k<av_num[i]-j-1; k++) {
                if (list[k]->role < list[k+1]->role) {
                    app_t *temp = list[k];
                    list[k] = list[k+1];
                    list[k+1] = temp;
                }
            }
        }

        // third, based on perm while keeping the order of roles
        for (j=0; j<av_num[i]-1; j++) {
            int k;
            for (k=0; k<av_num[i]-j-1; k++) {
                if (list[k]->role == list[k+1]->role && list[k]->perm < list[k+1]->perm) {
                    app_t *temp = list[k];
                    list[k] = list[k+1];
                    list[k+1] = temp;
                }
            }
        }
    }

    app_ctx->app_on = FALSE;

    // back up previous pointers
    int temp_num_apps = app_ctx->num_apps;
    app_t **temp_app_list = app_ctx->app_list;
    int *temp_av_num = app_ctx->av_num;
    app_t ***temp_av_list = app_ctx->av_list;

    // recover previous status and metadata
    for (i=0; i<n_apps; i++) {
        app_t *new = app_list[i];

        int j;
        for (j=0; j<temp_num_apps; j++) {
            app_t *old = temp_app_list[j];

            if (strcmp(new->name, old->name) == 0) {
                if (new->site == old->site) {
                    new->status = old->status;
                    new->activated = old->activated;

                    int k;
                    for (k=0; k<__NUM_PULL_THREADS; k++) {
                        new->push_sock[k] = old->push_sock[k];
                        new->push_lock[k] = old->push_lock[k];
                    }

                    new->req_ctx = old->req_ctx;

                    break;
                }
            }
        }
    }

    // deactivate out-dated applications
    for (i=0; i<temp_num_apps; i++) {
        app_t *old = temp_app_list[i];

        int j, pass = FALSE;
        for (j=0; j<n_apps; j++) {
            app_t *new = app_list[j];

            if (strcmp(old->name, new->name) == 0) {
                if (old->site == new->site) {
                    pass = TRUE;
                    break;
                }
            }
        }

        if (pass == FALSE) {
            application_deactivate(cli, old->name);
        }
    }

    // replace previous pointers to new ones
    app_ctx->num_apps = n_apps;
    app_ctx->app_list = app_list;
    app_ctx->av_num = av_num;
    app_ctx->av_list = av_list;

    // deallocate previous pointers
    clean_structs(temp_num_apps, temp_app_list, temp_av_num, temp_av_list);

    return 0;
}

/**
 * @}
 *
 * @}
 */
