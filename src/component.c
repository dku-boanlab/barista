/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup compnt_mgmt Component Management
 * \brief Functions to configure and manage components
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "component.h"
#include "component_list.h"
#include "event.h"

/////////////////////////////////////////////////////////////////////

/** \brief The context of the Barista NOS */
ctx_t *compnt_ctx;

/////////////////////////////////////////////////////////////////////

/** \brief The event list to convert an event string to an event ID */
const char event_string[__MAX_EVENTS][__CONF_WORD_LEN] = {
    #include "event_string.h"
};

/**
 * \brief Function to convert an event string to an event ID
 * \param event Event name
 * \return Event ID
 */
static int event_type(const char *event)
{
    int i;
    for (i=0; i<EV_NUM_EVENTS; i++) {
        if (strcmp(event, event_string[i]) == 0) {
            return i;
        }
    }

    return EV_NUM_EVENTS;
}

/**
 * \brief Function to print components that listen to an event
 * \param cli CLI context
 * \param id Event ID
 */
static int event_print(cli_t *cli, int id)
{
    char buf[__CONF_STR_LEN] ={0};

    cli_buffer(buf, "  ");

    int i;
    for (i=0; i<compnt_ctx->ev_num[id]; i++) {
        if (compnt_ctx->ev_list[id][i]->status == COMPNT_ENABLED) {
            cli_buffer(buf, "%s ", compnt_ctx->ev_list[id][i]->name);
        } else {
            cli_buffer(buf, "(%s) ", compnt_ctx->ev_list[id][i]->name);
        }
    }

    cli_bufprt(cli, buf);

    return 0;
}

/**
 * \brief Function to print an event and all components that listen to the event
 * \param cli CLI context
 * \param name Event name
 */
int event_show(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    if (strcmp(name, "EV_NONE") == 0) {
        return -1;
    } else if (strcmp(name, "EV_ALL_UPSTREAM") == 0) {
        cli_print(cli, "< Event - EV_ALL_UPSTREAM >");
        int i;
        for (i=EV_NONE+1; i<EV_ALL_UPSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else if (strcmp(name, "EV_ALL_DOWNSTREAM") == 0) {
        cli_print(cli, "< Event - EV_ALL_DOWNSTREAM >");
        int i;
        for (i=EV_ALL_UPSTREAM+1; i<EV_ALL_DOWNSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else if (strcmp(name, "EV_WRT_INTSTREAM") == 0) {
        cli_print(cli, "< Event - EV_WRT_INTSTREAM >");
        int i;
        for (i=EV_ALL_DOWNSTREAM+1; i<EV_WRT_INTSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else if (strcmp(name, "EV_ALL_INTSTREAM") == 0) {
        cli_print(cli, "< Event - EV_ALL_INTSTREAM >");
        int i;
        for (i=EV_WRT_INTSTREAM+1; i<EV_ALL_INTSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else {
        int i;
        for (i=0; i<EV_ALL_INTSTREAM; i++) {
            if (strcmp(name, event_string[i]) == 0) {
                cli_print(cli, "< Event - %s >", name);
                event_print(cli, i);
                return 0;
            }
        }
    }

    cli_print(cli, "No event whose name is %s", name);

    return -1;
}

/**
 * \brief Function to print all events
 * \param cli CLI context
 */
int event_list(cli_t *cli)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    cli_print(cli, "< Event List >");

    int i;
    for (i=0; i<EV_ALL_INTSTREAM; i++) {
        if (strcmp(event_string[i], "EV_NONE") == 0)
            continue;
        else if (strcmp(event_string[i], "EV_ALL_UPSTREAM") == 0)
            continue;
        else if (strcmp(event_string[i], "EV_ALL_DOWNSTREAM") == 0)
            continue;
        else if (strcmp(event_string[i], "EV_WRT_INTSTREAM") == 0)
            continue;
        else if (strcmp(event_string[i], "EV_ALL_INTSTREAM") == 0)
            continue;
        else {
            char buf[__CONF_STR_LEN] = {0};

            cli_buffer(buf, "  %s: ", event_string[i]);

            int j;
            for (j=0; j<compnt_ctx->ev_num[i]; j++) {
                compnt_t *compnt = compnt_ctx->ev_list[i][j];
                if (compnt->status == COMPNT_ENABLED) {
                    cli_buffer(buf, "%s ", compnt->name);
                } else {
                    cli_buffer(buf, "(%s) ", compnt->name);
                }
            }

            cli_bufprt(cli, buf);
        }
    }

    return 0;
}

/**
 * \brief Function to print the configuration of a component
 * \param cli CLI context
 * \param compnt Component context
 * \param details The flag to print the detailed description
 */
static int component_print(cli_t *cli, compnt_t *compnt, int details)
{
    char buf[__CONF_STR_LEN] = {0};

    cli_buffer(buf, "%2d: %s", compnt->id+1, compnt->name);

    if (details == FALSE) {
        cli_buffer(buf, " (");
        if (compnt->status == COMPNT_DISABLED)
            cli_buffer(buf, "disabled");
        else if (compnt->activated && (compnt->site == COMPNT_INTERNAL))
            cli_buffer(buf, ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
        else if (compnt->activated && (compnt->site == COMPNT_EXTERNAL))
            cli_buffer(buf, ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
        else if (compnt->site == COMPNT_EXTERNAL)
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
    for (i=0; i<compnt->argc; i++)
        cli_buffer(buf, "%s ", compnt->argv[i]);

    cli_bufprt(cli, buf);

    if (compnt->type == COMPNT_AUTO)
        cli_print(cli, "    Type: autonomous");
    else
        cli_print(cli, "    Type: general");

    if (compnt->site == COMPNT_INTERNAL)
        cli_print(cli, "    Site: internal");
    else
        cli_print(cli, "    Site: external");

    if (compnt->role == COMPNT_ADMIN)
        cli_print(cli, "    Role: admin");
    else if (compnt->role == COMPNT_SECURITY)
        cli_print(cli, "    Role: security");
    else if (compnt->role == COMPNT_SECURITY_V2)
        cli_print(cli, "    Role: security (v2)");
    else if (compnt->role == COMPNT_MANAGEMENT)
        cli_print(cli, "    Role: management");
    else if (compnt->role == COMPNT_NETWORK)
        cli_print(cli, "    Role: network");
    else if (compnt->role == COMPNT_BASE)
        cli_print(cli, "    Role: base");

    cli_bufcls(buf);
    cli_buffer(buf, "    Permission: ");

    if (compnt->perm & COMPNT_READ) cli_buffer(buf, "r");
    if (compnt->perm & COMPNT_WRITE) cli_buffer(buf, "w");
    if (compnt->perm & COMPNT_EXECUTE) cli_buffer(buf, "x");

    cli_bufprt(cli, buf);

    if (compnt->status == COMPNT_DISABLED)
        cli_print(cli, "    Status: disabled");
    else if (compnt->activated == TRUE)
        cli_print(cli, "    Status: " ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
    else if (compnt->status == COMPNT_ENABLED)
        cli_print(cli, "    Status: " ANSI_COLOR_BLUE "enabled" ANSI_COLOR_RESET);

    cli_bufcls(buf);
    cli_buffer(buf, "    Inbounds: ");

    for (i=0; i<compnt->in_num; i++) {
        cli_buffer(buf, "%s(%d) ", event_string[compnt->in_list[i]], compnt->in_list[i]);
    }

    cli_bufprt(cli, buf);

    cli_bufcls(buf);
    cli_buffer(buf, "    Outbounds: ");

    for (i=0; i<compnt->out_num; i++) {
        cli_buffer(buf, "%s(%d) ", event_string[compnt->out_list[i]], compnt->out_list[i]);
    }

    cli_bufprt(cli, buf);

    return 0;
}

/**
 * \brief Function to print a component configuration
 * \param cli CLI context
 * \param name Component name
 */
int component_show(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            cli_print(cli, "< Component Information >");
            component_print(cli, compnt_ctx->compnt_list[i], TRUE);
            return 0;
        }
    }

    cli_print(cli, "No component whose name is %s", name);

    return -1;
}

/**
 * \brief Function to print all component configurations
 * \param cli CLI context
 */
int component_list(cli_t *cli)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    cli_print(cli, "< Component List >");

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        component_print(cli, compnt_ctx->compnt_list[i], FALSE);
    }

    return 0;
}

/**
 * \brief Function to enable a component
 * \param cli CLI context
 * \param name Component name
 */
int component_enable(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            if (compnt_ctx->compnt_list[i]->status != COMPNT_ENABLED) {
                compnt_ctx->compnt_list[i]->status = COMPNT_ENABLED;
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
 * \brief Function to disable a component
 * \param cli CLI context
 * \param name Component name
 */
int component_disable(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            if (compnt_ctx->compnt_list[i]->status != COMPNT_DISABLED) {
                compnt_ctx->compnt_list[i]->status = COMPNT_DISABLED;
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
 * \brief The thread for an autonomous component
 * \param c_id Component ID
 */
static void *thread_main(void *compnt_id)
{
    int id = *(int *)compnt_id;
    compnt_t *compnt = compnt_ctx->compnt_list[id];

    FREE(compnt_id);

    if (compnt->main(&compnt->activated, compnt->argc, compnt->argv) < 0) {
        compnt->activated = FALSE;
        return NULL;
    } else {
        return compnt;
    }
}

/**
 * \brief Function to activate a component
 * \param cli CLI context
 * \param name Component name
 */
int component_activate(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        // check component name
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            // enabled component?
            if (compnt_ctx->compnt_list[i]->status == COMPNT_ENABLED) {
                // not activated yet?
                if (compnt_ctx->compnt_list[i]->activated == FALSE) {
                    // internal?
                    if (compnt_ctx->compnt_list[i]->site == COMPNT_INTERNAL) {
                        // general type?
                        if (compnt_ctx->compnt_list[i]->type == COMPNT_GENERAL) {
                            int *id = (int *)MALLOC(sizeof(int));
                            *id = compnt_ctx->compnt_list[i]->id;
                            if (thread_main(id) == NULL) {
                                cli_print(cli, "Failed to activate %s", name);
                                return -1;
                            } else {
                                cli_print(cli, "%s is activated", compnt_ctx->compnt_list[i]->name);
                                return 0;
                            }
                        // autonomous type?
                        } else {
                            pthread_t thread;
                            int *id = (int *)MALLOC(sizeof(int));
                            *id = compnt_ctx->compnt_list[i]->id;
                            if (pthread_create(&thread, NULL, &thread_main, (void *)id)) {
                                cli_print(cli, "Failed to activate %s", name);
                                return -1;
                            } else {
                                cli_print(cli, "%s is activated", compnt_ctx->compnt_list[i]->name);
                                return 0;
                            }
                        }
                    // external?
                    } else {
                        compnt_ctx->compnt_list[i]->push_ctx = zmq_ctx_new();
                        compnt_ctx->compnt_list[i]->req_ctx = zmq_ctx_new();

                        cli_print(cli, "%s is ready to talk", compnt_ctx->compnt_list[i]->name);
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
 * \brief Function to deactivate a component
 * \param cli CLI context
 * \param name Component name
 */
int component_deactivate(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        // check component name
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            // enabled component?
            if (compnt_ctx->compnt_list[i]->status == COMPNT_ENABLED) {
                // already activated?
                if (compnt_ctx->compnt_list[i]->activated == TRUE) {
                    // internal?
                    if (compnt_ctx->compnt_list[i]->site == COMPNT_INTERNAL) {
                        if (compnt_ctx->compnt_list[i]->cleanup(&compnt_ctx->compnt_list[i]->activated) < 0) {
                            cli_print(cli, "Failed to deactivate %s", compnt_ctx->compnt_list[i]->name);
                            return -1;
                        } else {
                            compnt_ctx->compnt_list[i]->activated = FALSE;
                            cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[i]->name);
                            return 0;
                        }
                    // external?
                    } else {
                        if (compnt_ctx->compnt_list[i]->push_ctx)
                            zmq_ctx_destroy(compnt_ctx->compnt_list[i]->push_ctx);
                        compnt_ctx->compnt_list[i]->push_ctx = NULL;

                        if (compnt_ctx->compnt_list[i]->req_ctx)
                            zmq_ctx_destroy(compnt_ctx->compnt_list[i]->req_ctx);
                        compnt_ctx->compnt_list[i]->req_ctx = NULL;

                        compnt_ctx->compnt_list[i]->activated = FALSE;
                        cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[i]->name);
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
 * \brief Function to activate all enabled components
 * \param cli CLI context
 */
int component_start(cli_t *cli)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    if (compnt_ctx->compnt_on == TRUE) {
        cli_print(cli, "Components are already started");
        return -1;
    }

    int i, conn = -1, cluster = -1;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strncmp(compnt_ctx->compnt_list[i]->name, "conn", 4) == 0) {
            if (compnt_ctx->compnt_list[i]->status == COMPNT_ENABLED)
                conn = i;
            continue;
        } else if (strcmp(compnt_ctx->compnt_list[i]->name, "cluster") == 0) {
            if (compnt_ctx->compnt_list[i]->status == COMPNT_ENABLED)
                cluster = i;
            continue;
        }
    }

    // activate cluster, enabled component?
    if (cluster != -1 && compnt_ctx->compnt_list[cluster]->status == COMPNT_ENABLED) {
        // not activated yet?
        if (compnt_ctx->compnt_list[cluster]->activated == FALSE) {
            // internal?
            if (compnt_ctx->compnt_list[cluster]->site == COMPNT_INTERNAL) {
                // general type?
                if (compnt_ctx->compnt_list[cluster]->type == COMPNT_GENERAL) {
                    int *id = (int *)MALLOC(sizeof(int));
                    *id = compnt_ctx->compnt_list[cluster]->id;
                    if (thread_main(id) == NULL) {
                        cli_print(cli, "Failed to activate %s", compnt_ctx->compnt_list[cluster]->name);
                        return -1;
                    } else {
                        cli_print(cli, "%s is activated", compnt_ctx->compnt_list[cluster]->name);
                    }
                // autonomous type?
                } else {
                    pthread_t thread;
                    int *id = (int *)MALLOC(sizeof(int));
                    *id = compnt_ctx->compnt_list[cluster]->id;
                    if (pthread_create(&thread, NULL, &thread_main, (void *)id)) {
                        cli_print(cli, "Failed to activate %s", compnt_ctx->compnt_list[cluster]->name);
                        return -1;
                    } else {
                        cli_print(cli, "%s is activated", compnt_ctx->compnt_list[cluster]->name);
                    }
                }
            // external?
            } else {
                compnt_ctx->compnt_list[cluster]->push_ctx = zmq_ctx_new();
                compnt_ctx->compnt_list[cluster]->req_ctx = zmq_ctx_new();

                cli_print(cli, "%s is ready to talk", compnt_ctx->compnt_list[cluster]->name);
            }
        }
    }

    // activate others
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, "log") == 0)
            continue;
        else if (strncmp(compnt_ctx->compnt_list[i]->name, "conn", 4) == 0)
            continue;
        else if (strcmp(compnt_ctx->compnt_list[i]->name, "cluster") == 0)
            continue;

        // enabled component?
        if (compnt_ctx->compnt_list[i]->status == COMPNT_ENABLED) {
            // not activated yet?
            if (compnt_ctx->compnt_list[i]->activated == FALSE) {
                // internal?
                if (compnt_ctx->compnt_list[i]->site == COMPNT_INTERNAL) {
                    // general type?
                    if (compnt_ctx->compnt_list[i]->type == COMPNT_GENERAL) {
                        int *id = (int *)MALLOC(sizeof(int));
                        *id = compnt_ctx->compnt_list[i]->id;
                        if (thread_main(id) == NULL) {
                            cli_print(cli, "Failed to activate %s", compnt_ctx->compnt_list[i]->name);
                            return -1;
                        } else {
                            cli_print(cli, "%s is activated", compnt_ctx->compnt_list[i]->name);
                        }
                    // autonomous type?
                    } else {
                        pthread_t thread;
                        int *id = (int *)MALLOC(sizeof(int));
                        *id = compnt_ctx->compnt_list[i]->id;
                        if (pthread_create(&thread, NULL, &thread_main, (void *)id)) {
                            cli_print(cli, "Failed to activate %s", compnt_ctx->compnt_list[i]->name);
                            return -1;
                        } else {
                            cli_print(cli, "%s is activated", compnt_ctx->compnt_list[i]->name);
                        }
                    }
                // external?
                } else {
                    compnt_ctx->compnt_list[i]->push_ctx = zmq_ctx_new();
                    compnt_ctx->compnt_list[i]->req_ctx = zmq_ctx_new();

                    cli_print(cli, "%s is ready to talk", compnt_ctx->compnt_list[i]->name);
                }
            }
        }
    }

    // activate conn*, enabled component?
    if (conn != -1 && compnt_ctx->compnt_list[conn]->status == COMPNT_ENABLED) {
        // not activated yet?
        if (compnt_ctx->compnt_list[conn]->activated == FALSE) {
            // internal?
            if (compnt_ctx->compnt_list[conn]->site == COMPNT_INTERNAL) {
                // general type?
                if (compnt_ctx->compnt_list[conn]->type == COMPNT_GENERAL) {
                    int *id = (int *)MALLOC(sizeof(int));
                    *id = compnt_ctx->compnt_list[conn]->id;
                    if (thread_main(id) == NULL) {
                        cli_print(cli, "Failed to activate %s", compnt_ctx->compnt_list[conn]->name);
                        return -1;
                    } else {
                        cli_print(cli, "%s is activated", compnt_ctx->compnt_list[conn]->name);
                    }
                // autonomous type?
                } else {
                    pthread_t thread;
                    int *id = (int *)MALLOC(sizeof(int));
                    *id = compnt_ctx->compnt_list[conn]->id;
                    if (pthread_create(&thread, NULL, &thread_main, (void *)id)) {
                        cli_print(cli, "Failed to activate %s", compnt_ctx->compnt_list[conn]->name);
                        return -1;
                    } else {
                        cli_print(cli, "%s is activated", compnt_ctx->compnt_list[conn]->name);
                    }
                }
            // external?
            } else {
                compnt_ctx->compnt_list[conn]->push_ctx = zmq_ctx_new();
                compnt_ctx->compnt_list[conn]->req_ctx = zmq_ctx_new();

                cli_print(cli, "%s is ready to talk", compnt_ctx->compnt_list[conn]->name);
            }
        }
    }

    compnt_ctx->compnt_on = TRUE;

    waitsec(0, 1000 * 1000);

    return 0;
}

/**
 * \brief Function to deactivate all activated components
 * \param cli CLI context
 */
int component_stop(cli_t *cli)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    if (compnt_ctx->compnt_on == FALSE) {
        cli_print(cli, "Components are not started yet");
        return -1;
    }

    int i, conn = -1, cluster = -1;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strncmp(compnt_ctx->compnt_list[i]->name, "conn", 4) == 0) {
            if (compnt_ctx->compnt_list[i]->activated) {
                conn = i;
                continue;
            }
        } else if (strcmp(compnt_ctx->compnt_list[i]->name, "cluster") == 0) {
            if (compnt_ctx->compnt_list[i]->activated) {
                cluster = i;
                continue;
            }
        }
    }

    // deactivate conn
    if (conn != -1 && compnt_ctx->compnt_list[conn]->activated) {
        // internal?
        if (compnt_ctx->compnt_list[conn]->site == COMPNT_INTERNAL) {
            if (compnt_ctx->compnt_list[conn]->cleanup(&compnt_ctx->compnt_list[conn]->activated) < 0) {
                cli_print(cli, "Failed to deactivate %s", compnt_ctx->compnt_list[conn]->name);
                return -1;
            } else {
                compnt_ctx->compnt_list[conn]->activated = FALSE;
                cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[conn]->name);
            }
        // external?
        } else {
            if (compnt_ctx->compnt_list[conn]->push_ctx)
                zmq_ctx_destroy(compnt_ctx->compnt_list[conn]->push_ctx);
            compnt_ctx->compnt_list[conn]->push_ctx = NULL;

            if (compnt_ctx->compnt_list[conn]->req_ctx)
                zmq_ctx_destroy(compnt_ctx->compnt_list[conn]->req_ctx);
            compnt_ctx->compnt_list[conn]->req_ctx = NULL;

            compnt_ctx->compnt_list[conn]->activated = FALSE;
            cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[conn]->name);
        }
    }

    // deactivate others except log and cluster
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, "log") == 0)
            continue;
        if (strncmp(compnt_ctx->compnt_list[i]->name, "conn", 4) == 0)
            continue;
        if (strcmp(compnt_ctx->compnt_list[i]->name, "cluster") == 0)
            continue;

        // enabled component?
        if (compnt_ctx->compnt_list[i]->status == COMPNT_ENABLED) {
            // already activated?
            if (compnt_ctx->compnt_list[i]->activated) {
                // internal?
                if (compnt_ctx->compnt_list[i]->site == COMPNT_INTERNAL) {
                    if (compnt_ctx->compnt_list[i]->cleanup(&compnt_ctx->compnt_list[i]->activated) < 0) {
                        cli_print(cli, "Failed to deactivate %s", compnt_ctx->compnt_list[i]->name);
                        return -1;
                    } else {
                        compnt_ctx->compnt_list[i]->activated = FALSE;
                        cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[i]->name);
                    }
                // external?
                } else {
                    if (compnt_ctx->compnt_list[i]->push_ctx)
                        zmq_ctx_destroy(compnt_ctx->compnt_list[i]->push_ctx);
                    compnt_ctx->compnt_list[i]->push_ctx = NULL;

                    if (compnt_ctx->compnt_list[i]->req_ctx)
                        zmq_ctx_destroy(compnt_ctx->compnt_list[i]->req_ctx);
                    compnt_ctx->compnt_list[i]->req_ctx = NULL;

                    compnt_ctx->compnt_list[i]->activated = FALSE;
                    cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[i]->name);
                }
            }
        }
    }

    // deactivate cluster
    if (cluster != -1 && compnt_ctx->compnt_list[cluster]->activated) {
        // internal?
        if (compnt_ctx->compnt_list[cluster]->site == COMPNT_INTERNAL) {
            if (compnt_ctx->compnt_list[cluster]->cleanup(&compnt_ctx->compnt_list[cluster]->activated) < 0) {
                cli_print(cli, "Failed to deactivate %s", compnt_ctx->compnt_list[cluster]->name);
                return -1;
            } else {
                compnt_ctx->compnt_list[cluster]->activated = FALSE;
                cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[cluster]->name);
            }
        // external?
        } else {
            if (compnt_ctx->compnt_list[cluster]->push_ctx)
                zmq_ctx_destroy(compnt_ctx->compnt_list[cluster]->push_ctx);
            compnt_ctx->compnt_list[cluster]->push_ctx = NULL;

            if (compnt_ctx->compnt_list[cluster]->req_ctx)
                zmq_ctx_destroy(compnt_ctx->compnt_list[cluster]->req_ctx);
            compnt_ctx->compnt_list[cluster]->req_ctx = NULL;

            compnt_ctx->compnt_list[cluster]->activated = FALSE;
            cli_print(cli, "%s is deactivated", compnt_ctx->compnt_list[cluster]->name);
        }
    }

    compnt_ctx->compnt_on = FALSE;

    waitsec(0, 1000 * 1000);

    return 0;
}

/**
 * \brief Function to add a policy to a component
 * \param cli CLI context
 * \param name Component name
 * \param odp Operator-defined policy
 */
int component_add_policy(cli_t *cli, char *name, char *odp)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    compnt_t *compnt = NULL;

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            compnt = compnt_ctx->compnt_list[i];
            break;
        }
    }

    if (compnt == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    int cnt = 0;
    char parm[__NUM_OF_ODP_FIELDS][__CONF_WORD_LEN] = {{0}};
    char val[__NUM_OF_ODP_FIELDS][__CONF_WORD_LEN] = {{0}};

    cli_print(cli, "Policy: %s, %s", compnt->name, odp);

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
                    compnt->odp[compnt->num_policies].flag |= ODP_DPID;
                    compnt->odp[compnt->num_policies].dpid[idx] = atoi(v);
                    cli_print(cli, "\tDPID: %lu", compnt->odp[compnt->num_policies].dpid[idx]);
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
                        cli_print(cli, "\tPort: %s (wrong)", v);
                    } else {
                        compnt->odp[compnt->num_policies].flag |= ODP_PORT;
                        compnt->odp[compnt->num_policies].port[idx] = atoi(v);
                        cli_print(cli, "\tPort: %u", compnt->odp[compnt->num_policies].port[idx]);
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
                        compnt->odp[compnt->num_policies].flag |= ODP_PROTO;
                        compnt->odp[compnt->num_policies].proto |= PROTO_ARP;
                        cli_print(cli, "\tProtocol: ARP");
                    } else if (strcmp(v, "lldp") == 0) {
                        compnt->odp[compnt->num_policies].flag |= ODP_PROTO;
                        compnt->odp[compnt->num_policies].proto |= PROTO_LLDP;
                        cli_print(cli, "\tProtocol: LLDP");
                    } else if (strcmp(v, "dhcp") == 0) {
                        compnt->odp[compnt->num_policies].flag |= ODP_PROTO;
                        compnt->odp[compnt->num_policies].proto |= PROTO_DHCP;
                        cli_print(cli, "\tProtocol: DHCP");
                    } else if (strcmp(v, "tcp") == 0) {
                        compnt->odp[compnt->num_policies].flag |= ODP_PROTO;
                        compnt->odp[compnt->num_policies].proto |= PROTO_TCP;
                        cli_print(cli, "\tProtocol: TCP");
                    } else if (strcmp(v, "udp") == 0) {
                        compnt->odp[compnt->num_policies].flag |= ODP_PROTO;
                        compnt->odp[compnt->num_policies].proto |= PROTO_UDP;
                        cli_print(cli, "\tProtocol: UDP");
                    } else if (strcmp(v, "icmp") == 0) {
                        compnt->odp[compnt->num_policies].flag |= ODP_PROTO;
                        compnt->odp[compnt->num_policies].proto |= PROTO_ICMP;
                        cli_print(cli, "\tProtocol: ICMP");
                    } else if (strcmp(v, "ipv4") == 0) {
                        compnt->odp[compnt->num_policies].flag |= ODP_PROTO;
                        compnt->odp[compnt->num_policies].proto |= PROTO_IPV4;
                        cli_print(cli, "\tProtocol: IPv4");
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
                        compnt->odp[compnt->num_policies].flag |= ODP_SRCIP;
                        compnt->odp[compnt->num_policies].srcip[idx] = ip_addr_int(v);
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
                        compnt->odp[compnt->num_policies].flag |= ODP_DSTIP;
                        compnt->odp[compnt->num_policies].dstip[idx] = ip_addr_int(v);
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
                        compnt->odp[compnt->num_policies].flag |= ODP_SPORT;
                        compnt->odp[compnt->num_policies].sport[idx] = port;
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
                        compnt->odp[compnt->num_policies].flag |= ODP_DPORT;
                        compnt->odp[compnt->num_policies].dport[idx] = port;
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

    compnt->num_policies++;

    return 0;
}

/**
 * \brief Function to delete a policy from a component
 * \param cli CLI context
 * \param name Component name
 * \param idx The index of the target operator-defined policy
 */
int component_del_policy(cli_t *cli, char *name, int idx)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    compnt_t *compnt = NULL;

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            compnt = compnt_ctx->compnt_list[i];
            break;
        }
    }

    if (compnt == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    if (compnt->num_policies == 0) {
        cli_print(cli, "There is no policy in %s", compnt->name);
        return -1;
    } else if (compnt->num_policies < idx) {
        cli_print(cli, "%s has only %u policies", compnt->name, compnt->num_policies);
        return -1;
    }

    memset(&compnt->odp[idx-1], 0, sizeof(odp_t));

    for (i=idx; i<__MAX_POLICY_ENTRIES; i++) {
        memmove(&compnt->odp[i-1], &compnt->odp[i], sizeof(odp_t));
    }

    memset(&compnt->odp[i-1], 0, sizeof(odp_t));

    compnt->num_policies--;

    cli_print(cli, "Deleted policy #%u in %s", idx, compnt->name);

    return 0;
}

/**
 * \brief Function to show all policies applied to a component
 * \param cli CLI context
 * \param name Component name
 */
int component_show_policy(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    compnt_t *compnt = NULL;

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            compnt = compnt_ctx->compnt_list[i];
            break;
        }
    }

    if (compnt == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    cli_print(cli, "< Operator-Defined Policies for %s >", compnt->name);

    for (i=0; i<compnt->num_policies; i++) {
        cli_print(cli, "  Policy #%02u:", i+1);

        if (compnt->odp[i].dpid[0]) {
            char buf[__CONF_STR_LEN] = {0};

            cli_buffer(buf, "    Datapath ID: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (compnt->odp[i].dpid[j] == 0) break;
                cli_buffer(buf, "%lu ", compnt->odp[i].dpid[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (compnt->odp[i].port[0]) {
            char buf[__CONF_STR_LEN] = {0};

            cli_buffer(buf, "    Port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (compnt->odp[i].port[j] == 0) break;
                cli_buffer(buf, "%u ", compnt->odp[i].port[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (compnt->odp[i].proto) {
            char buf[__CONF_STR_LEN] = {0};

            cli_buffer(buf, "    Protocol: ");

            if (compnt->odp[i].proto & PROTO_ARP)
                cli_buffer(buf, "ARP ");
            if (compnt->odp[i].proto & PROTO_LLDP)
                cli_buffer(buf, "LLDP ");
            if (compnt->odp[i].proto & PROTO_DHCP)
                cli_buffer(buf, "DHCP ");
            if (compnt->odp[i].proto & PROTO_TCP)
                cli_buffer(buf, "TCP ");
            if (compnt->odp[i].proto & PROTO_UDP)
                cli_buffer(buf, "UDP ");
            if (compnt->odp[i].proto & PROTO_ICMP)
                cli_buffer(buf, "ICMP ");
            if (compnt->odp[i].proto & PROTO_IPV4)
                cli_buffer(buf, "IPv4 ");
            if (!compnt->odp[i].proto)
                cli_buffer(buf, "ALL ");

            cli_bufprt(cli, buf);
        }

        if (compnt->odp[i].srcip[0]) {
            char buf[__CONF_STR_LEN] ={0};

            cli_buffer(buf, "    Source IP: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (compnt->odp[i].srcip[j] == 0) break;
                cli_buffer(buf, "%s ", ip_addr_str(compnt->odp[i].srcip[j]));
            }

            cli_bufprt(cli, buf);
        }

        if (compnt->odp[i].dstip[0]) {
            char buf[__CONF_STR_LEN] = {0};

            cli_buffer(buf, "    Destination IP: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (compnt->odp[i].dstip[j] == 0) break;
                cli_buffer(buf, "%s ", ip_addr_str(compnt->odp[i].dstip[j]));
            }

            cli_bufprt(cli, buf);
        }

        if (compnt->odp[i].sport[0]) {
            char buf[__CONF_STR_LEN] = {0};

            cli_buffer(buf, "    Source port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (compnt->odp[i].sport[j] == 0) break;
                cli_buffer(buf, "%u ", compnt->odp[i].sport[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (compnt->odp[i].dport[0]) {
            char buf[__CONF_STR_LEN] = {0};

            cli_buffer(buf, "    Destination port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (compnt->odp[i].dport[j] == 0) break;
                cli_buffer(buf, "%u ", compnt->odp[i].dport[j]);
            }

            cli_bufprt(cli, buf);
        }
    }

    if (!compnt->num_policies)
        cli_print(cli, "  No policy");

    return 0;
}

/**
 * \brief Function to deliver a command to a component
 * \param cli CLI context
 * \param args Arguments
 */
int component_cli(cli_t *cli, char **args)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, args[0]) == 0) {
            if (compnt_ctx->compnt_list[i]->activated == TRUE) {
                if (compnt_ctx->compnt_list[i]->cli != NULL) {
                    compnt_ctx->compnt_list[i]->cli(cli, &args[1]);
                }
            }

            return 0;
        }
    }

    return 0;
}

/**
 * \brief Function to deallocate all configuration structures
 * \param num_compnts The number of registered components
 * \param compnt_list The list of the components
 * \param ev_num The number of components for each event
 * \param ev_list The components listening to each event
 */
static int clean_up_config(int num_compnts, compnt_t **compnt_list, int *ev_num, compnt_t ***ev_list)
{
    if (compnt_list != NULL) {
        int i;
        for (i=0; i<num_compnts; i++) {
            if (compnt_list[i] != NULL)
                FREE(compnt_list[i]);
        }
        FREE(compnt_list);
    }

    if (ev_num != NULL) {
        FREE(ev_num);
    }

    if (ev_list != NULL) {
        int i;
        for (i=0; i<__MAX_EVENTS; i++) {
            if (ev_list[i] != NULL)
                FREE(ev_list[i]);
        }
        FREE(ev_list);
    }

    return 0;
}

/**
 * \brief Function to load component configurations from a file
 * \param cli CLI context
 * \param ctx The context of the Barista NOS
 */
int component_load(cli_t *cli, ctx_t *ctx)
{
    json_t *json;
    json_error_t error;

    if (compnt_ctx == NULL) {
        compnt_ctx = ctx;
    }

    cli_print(cli, "Component configuration file: %s", ctx->conf_file);

    if (access(ctx->conf_file, F_OK)) {
        cli_print(cli, "No configuration file whose name is '%s'", ctx->conf_file);
        return -1;
    }

    char *raw = str_read(ctx->conf_file);
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

    int num_compnts = 0;
    compnt_t **compnt_list = (compnt_t **)MALLOC(sizeof(compnt_t *) * __MAX_COMPONENTS);
    if (compnt_list == NULL) {
        PERROR("malloc");
        json_decref(json);
        return -1;
    } else {
        memset(compnt_list, 0, sizeof(compnt_t *) * __MAX_COMPONENTS);
    }

    int *ev_num = (int *)MALLOC(sizeof(int) * __MAX_EVENTS);
    if (ev_num == NULL) {
        PERROR("malloc");
        clean_up_config(num_compnts, compnt_list, NULL, NULL);
        json_decref(json);
        return -1;
    } else {
        memset(ev_num, 0, sizeof(int) * __MAX_EVENTS);
    }

    compnt_t ***ev_list = (compnt_t ***)MALLOC(sizeof(compnt_t **) * __MAX_EVENTS);
    if (ev_list == NULL) {
        PERROR("malloc");
        clean_up_config(num_compnts, compnt_list, ev_num, NULL);
        json_decref(json);
        return -1;
    } else {
        int i;
        for (i=0; i<__MAX_EVENTS; i++) {
            ev_list[i] = (compnt_t **)MALLOC(sizeof(compnt_t *) * __MAX_COMPONENTS);
            if (ev_list[i] == NULL) {
                PERROR("malloc");
                clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                json_decref(json);
                return -1;
            } else {
                memset(ev_list[i], 0, sizeof(compnt_t *) * __MAX_COMPONENTS);
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

        char push_addr[__CONF_WORD_LEN] = {0};
        json_t *j_push_addr = json_object_get(data, "push_addr");
        if (json_is_string(j_push_addr)) {
            strcpy(push_addr, json_string_value(j_push_addr));
        }

        char req_addr[__CONF_WORD_LEN] = {0};
        json_t *j_req_addr = json_object_get(data, "request_addr");
        if (json_is_string(j_req_addr)) {
            strcpy(req_addr, json_string_value(j_req_addr));
        }

        // find the index of a component to link the corresponding functions
        const int num_components = sizeof(g_components) / sizeof(compnt_func_t);
        int k;
        for (k=0; k<num_components; k++) {
            if (strcmp(g_components[k].name, name))
                continue;
            else
                break;
        }

        // allocate a new component structure
        compnt_t *compnt = calloc(1, sizeof(compnt_t));
        if (!compnt) {
             PERROR("calloc");
             clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
             json_decref(json);
             return -1;
        }

        // set an internal ID (simply for ordering)
        compnt->id = num_compnts;

        // set the component name
        if (strlen(name) == 0) {
            cli_print(cli, "No component name");
            FREE(compnt);
            clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
            json_decref(json);
            return -1;
        } else {
            strcpy(compnt->name, name);
            cli_print(cli, "%3d: %s", compnt->id+1, name);

            // set the actual ID
            compnt->component_id = hash_func((uint32_t *)&compnt->name, __HASHING_NAME_LENGTH);
            cli_print(cli, "     ID: %u", compnt->component_id);
        }

        // set arguments
        strcpy(compnt->args, args);
        str2args(compnt->args, &compnt->argc, &compnt->argv[1], __CONF_ARGC);
        compnt->argc++;
        compnt->argv[0] = compnt->name;
        if (strlen(compnt->args))
            cli_print(cli, "     Arguments: %s", compnt->args);

        // set a type
        if (strlen(type) == 0) {
            compnt->type = COMPNT_GENERAL;
        } else {
            compnt->type = (strcmp(type, "autonomous") == 0) ? COMPNT_AUTO : COMPNT_GENERAL;
        }
        cli_print(cli, "     Type: %s", type);

        // set a site
        if (strlen(site) == 0) {
            compnt->site = COMPNT_INTERNAL;
        } else {
            if (strcmp(site, "external") == 0)
                compnt->site = COMPNT_EXTERNAL;
            else
                compnt->site = COMPNT_INTERNAL;
        }
        cli_print(cli, "     Site: %s", site);

        // set functions
        if (compnt->site == COMPNT_INTERNAL) { // internal
            if (k == num_components) {
                FREE(compnt);
                continue;
            }

            compnt->main = g_components[k].main;
            compnt->handler = g_components[k].handler;
            compnt->cleanup = g_components[k].cleanup;
            compnt->cli = g_components[k].cli;
        } else { // external
            compnt->main = NULL;
            compnt->handler = NULL;
            compnt->cleanup = NULL;
            compnt->cli = NULL;

            if (strlen(push_addr) > 0) {
                strcpy(compnt->push_addr, push_addr);
            } else {
                 cli_print(cli, "No push address");
                 FREE(compnt);
                 clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                 json_decref(json);
                 return -1;
            }

            if (strlen(req_addr) > 0) {
                strcpy(compnt->req_addr, req_addr);
            } else {
                 cli_print(cli, "No request address");
                 FREE(compnt);
                 clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                 json_decref(json);
                 return -1;
            }
        }

        // set a role
        if (strlen(role) == 0) {
            compnt->role = COMPNT_NETWORK;
        } else {
            if (strcmp(role, "base") == 0)
                compnt->role = COMPNT_BASE;
            else if (strcmp(role, "network") == 0)
                compnt->role = COMPNT_NETWORK;
            else if (strcmp(role, "management") == 0)
                compnt->role = COMPNT_MANAGEMENT;
            else if (strcmp(role, "security") == 0)
                compnt->role = COMPNT_SECURITY;
            else if (strcmp(role, "security_v2") == 0)
                compnt->role = COMPNT_SECURITY_V2;
            else if (strcmp(role, "admin") == 0)
                compnt->role = COMPNT_ADMIN;
            else // default
                compnt->role = COMPNT_NETWORK;
        }
        cli_print(cli, "     Role: %s", role);

        // set permissions
        int size = (strlen(perm) > 3) ? 3 : strlen(perm);
        int l;
        for (l=0; l<size; l++) {
            if (perm[l] == 'r') {
                compnt->perm |= COMPNT_READ;
            } else if (perm[l] == 'w') {
                compnt->perm |= COMPNT_WRITE;
            } else if (perm[l] == 'x') {
                compnt->perm |= COMPNT_EXECUTE;
            }
        }
        if (compnt->perm == 0) compnt->perm |= COMPNT_READ;
        cli_print(cli, "     Permission: %s", perm);

        // set priority
        if (strlen(priority) == 0)
            compnt->priority = 0;
        else
            compnt->priority = atoi(priority);
        if (strlen(priority))
            cli_print(cli, "     Priority: %s", priority);

        // set a status
        if (strlen(status) == 0) {
            compnt->status = COMPNT_DISABLED;
        } else {
            compnt->status = (strcmp(status, "enabled") == 0) ? COMPNT_ENABLED : COMPNT_DISABLED;
        }
        cli_print(cli, "     Status: %s", status);

        int real_event_cnt = 0;

        // set inbound events
        json_t *events = json_object_get(data, "inbounds");
        if (json_is_array(events)) {
            // first, check whether there is a wrong event name or not
            int j;
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                char in_name[__CONF_WORD_LEN] = {0};
                char in_perm[__CONF_SHORT_LEN] = {0};

                sscanf(json_string_value(event), "%[^,],%[^,]", in_name, in_perm);

                if (event_type(in_name) == EV_NUM_EVENTS) {
                    cli_print(cli, "     Inbounds: wrong event name");
                    FREE(compnt);
                    clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                    json_decref(json);
                    return -1;
                }
            }

            // then, set the inbound events
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                char in_name[__CONF_WORD_LEN] = {0};
                char in_perm[__CONF_SHORT_LEN] = {0};
                int in_perm_val = 0;

                sscanf(json_string_value(event), "%[^,],%[^,]", in_name, in_perm);

                int plen = (strlen(in_perm) > 3) ? 3 : strlen(in_perm);

                int n;
                for (n=0; n<plen; n++) {
                    if (in_perm[n] == 'r')
                        in_perm_val |= COMPNT_READ;
                    else if (in_perm[n] == 'w')
                        in_perm_val |= COMPNT_WRITE;
                    else if (perm[l] == 'x')
                        in_perm_val |= COMPNT_EXECUTE;
                }

                if (in_perm_val == 0) in_perm_val |= COMPNT_READ;
                else in_perm_val &= compnt->perm;

                if (event_type(in_name) == EV_NONE) {
                    break;
                } else if (event_type(in_name) == EV_ALL_UPSTREAM) {
                    if (compnt->role < COMPNT_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(compnt);
                        clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                        json_decref(json);
                        return -1;
                    }

                    int k;
                    for (k=EV_NONE+1; k<EV_ALL_UPSTREAM; k++) {
                        real_event_cnt++;

                        compnt->in_list[compnt->in_num] = k;
                        compnt->in_perm[k] = in_perm_val;
                        compnt->in_num++;

                        ev_list[k][ev_num[k]] = compnt;
                        ev_num[k]++;
                    }
                } else if (event_type(in_name) == EV_ALL_DOWNSTREAM) {
                    if (compnt->role < COMPNT_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(compnt);
                        clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                        json_decref(json);
                        return -1;
                    }

                    int k;
                    for (k=EV_ALL_UPSTREAM+1; k<EV_ALL_DOWNSTREAM; k++) {
                        real_event_cnt++;

                        compnt->in_list[compnt->in_num] = k;
                        compnt->in_perm[k] = in_perm_val;
                        compnt->in_num++;

                        ev_list[k][ev_num[k]] = compnt;
                        ev_num[k]++;
                    }
                } else if (event_type(in_name) == EV_WRT_INTSTREAM) {
                    if (compnt->role < COMPNT_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(compnt);
                        clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                        json_decref(json);
                        return -1;
                    }

                    int k;
                    for (k=EV_ALL_DOWNSTREAM+1; k<EV_WRT_INTSTREAM; k++) {
                        real_event_cnt++;

                        compnt->in_list[compnt->in_num] = k;
                        compnt->in_perm[k] = in_perm_val;
                        compnt->in_num++;

                        ev_list[k][ev_num[k]] = compnt;
                        ev_num[k]++;
                    }
                } else if (event_type(in_name) == EV_ALL_INTSTREAM) {
                    if (compnt->role < COMPNT_SECURITY) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(compnt);
                        clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                        json_decref(json);
                        return -1;
                    }

                    int k;
                    for (k=EV_WRT_INTSTREAM+1; k<EV_ALL_INTSTREAM; k++) {
                        real_event_cnt++;

                        compnt->in_list[compnt->in_num] = k;
                        compnt->in_perm[k] = in_perm_val;
                        compnt->in_num++;

                        ev_list[k][ev_num[k]] = compnt;
                        ev_num[k]++;
                    }
                } else {
                    real_event_cnt++;

                    compnt->in_list[compnt->in_num] = event_type(in_name);
                    compnt->in_perm[event_type(in_name)] = in_perm_val;
                    compnt->in_num++;

                    ev_list[event_type(in_name)][ev_num[event_type(in_name)]] = compnt;
                    ev_num[event_type(in_name)]++;
                }
            }

            if (real_event_cnt == 0) cli_print(cli, "     Inbounds: no event");
            else if (real_event_cnt == 1) cli_print(cli, "     Inbounds: 1 event");
            else cli_print(cli, "     Inbounds: %d events", compnt->in_num);
        }

        real_event_cnt = 0;

        // set outbound events
        json_t *out_events = json_object_get(data, "outbounds");
        if (json_is_array(out_events)) {
            int j;

            // first, check whether there is a wrong event name or not
            for (j=0; j<json_array_size(out_events); j++) {
                json_t *out_event = json_array_get(out_events, j);

                if (event_type(json_string_value(out_event)) == EV_ALL_UPSTREAM) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(compnt);
                    clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                    json_decref(json);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_ALL_DOWNSTREAM) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(compnt);
                    clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                    json_decref(json);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_WRT_INTSTREAM) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(compnt);
                    clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                    json_decref(json);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_ALL_INTSTREAM) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(compnt);
                    clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                    json_decref(json);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_NUM_EVENTS) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(compnt);
                    clean_up_config(num_compnts, compnt_list, ev_num, ev_list);
                    json_decref(json);
                    return -1;
                }
            }

            // then, set the outbound events
            for (j=0; j<json_array_size(out_events); j++) {
                json_t *out_event = json_array_get(out_events, j);

                if (event_type(json_string_value(out_event)) == EV_NONE)
                    break;

                real_event_cnt++;

                compnt->out_list[compnt->out_num] = event_type(json_string_value(out_event));
                compnt->out_num++;
            }

            if (real_event_cnt == 0) cli_print(cli, "     Outbounds: no event");
            else if (real_event_cnt == 1) cli_print(cli, "     Outbounds: 1 event");
            else cli_print(cli, "     Outbounds: %d events", compnt->out_num);
        }

        compnt_list[num_compnts] = compnt;
        num_compnts++;
    }

    json_decref(json);

    // sort components based on the ordering policy
    for (i=1; i<EV_NUM_EVENTS; i++) {
        compnt_t **list = ev_list[i];

        if (ev_num[i] == 0)
            continue;

        // first, based on priority
        int j;
        for (j=0; j<ev_num[i]-1; j++) {
            int k;
            for (k=0; k<ev_num[i]-j-1; k++) {
                if (list[k]->priority < list[k+1]->priority) {
                    compnt_t *temp = list[k];
                    list[k] = list[k+1];
                    list[k+1] = temp;
                }
            }
        }

        // second, based on role
        for (j=0; j<ev_num[i]-1; j++) {
            int k;
            for (k=0; k<ev_num[i]-j-1; k++) {
                if (list[k]->role < list[k+1]->role) {
                    compnt_t *temp = list[k];
                    list[k] = list[k+1];
                    list[k+1] = temp;
                }
            }
        }

        // third, based on perm while keeping the order of roles
        for (j=0; j<ev_num[i]-1; j++) {
            int k;
            for (k=0; k<ev_num[i]-j-1; k++) {
                if (list[k]->role == list[k+1]->role && list[k]->perm < list[k+1]->perm) {
                    compnt_t *temp = list[k];
                    list[k] = list[k+1];
                    list[k+1] = temp;
                }
            }
        }
    }

    compnt_ctx->compnt_on = FALSE;

    // back up previous pointers
    int temp_num_compnts = compnt_ctx->num_compnts;
    compnt_t **temp_compnt_list = compnt_ctx->compnt_list;
    int *temp_ev_num = compnt_ctx->ev_num;
    compnt_t ***temp_ev_list = compnt_ctx->ev_list;

    // recover previous status and metadata
    for (i=0; i<num_compnts; i++) {
        compnt_t *new = compnt_list[i];

        int j;
        for (j=0; j<temp_num_compnts; j++) {
            compnt_t *old = temp_compnt_list[j];

            if (strcmp(new->name, old->name) == 0) {
                if (new->site == old->site) {
                    new->status = old->status;
                    new->activated = old->activated;

                    new->push_ctx = old->push_ctx;
                    new->req_ctx = old->req_ctx;

                    break;
                }
            }
        }
    }

    // deactivate out-dated components
    for (i=0; i<temp_num_compnts; i++) {
        compnt_t *old = temp_compnt_list[i];

        int j, pass = FALSE;
        for (j=0; j<num_compnts; j++) {
            compnt_t *new = compnt_list[j];

            if (strcmp(old->name, new->name) == 0) {
                if (old->site == new->site) {
                    pass = TRUE;
                    break;
                }
            }
        }

        if (pass == FALSE) {
            component_deactivate(cli, old->name);
        }
    }

    // replace previous pointers to new ones
    compnt_ctx->num_compnts = num_compnts;
    compnt_ctx->compnt_list = compnt_list;
    compnt_ctx->ev_num = ev_num;
    compnt_ctx->ev_list = ev_list;

    // deallocate previous pointers
    clean_up_config(temp_num_compnts, temp_compnt_list, temp_ev_num, temp_ev_list);

    return 0;
}

/**
 * @}
 *
 * @}
 */
