/*
 * Copyright 2015-2018 NSSLab, KAIST
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

/** \brief The pointer of the Barista context */
ctx_t *compnt_ctx;

/////////////////////////////////////////////////////////////////////

/** \brief The event list to convert an event string to an event ID */
const char event_string[__MAX_EVENTS][__CONF_WORD_LEN] = {
#include "event_string.h"
};

/**
 * \brief Function to convert an event string to an event ID
 * \param event Event string
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
 * \brief Function to print all components that listen to an event
 * \param cli the CLI pointer
 * \param id Event ID
 */
static int event_print(cli_t *cli, int id)
{
    char buf[__CONF_STR_LEN];

    cli_bufcls(buf);
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
 * \brief Function to print an event and its components
 * \param cli the CLI pointer
 * \param name Event string
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
        cli_print(cli, "<Event - EV_ALL_UPSTREAM>");
        int i;
        for (i=EV_NONE+1; i<EV_ALL_UPSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else if (strcmp(name, "EV_ALL_DOWNSTREAM") == 0) {
        cli_print(cli, "<Event - EV_ALL_DOWNSTREAM>");
        int i;
        for (i=EV_ALL_UPSTREAM+1; i<EV_ALL_DOWNSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else if (strcmp(name, "EV_WRT_INTSTREAM") == 0) {
        cli_print(cli, "<Event - EV_WRT_INTSTREAM");
        int i;
        for (i=EV_ALL_DOWNSTREAM+1; i<EV_WRT_INTSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else if (strcmp(name, "EV_ALL_INTSTREAM") == 0) {
        cli_print(cli, "<Event - EV_ALL_INTSTREAM>");
        int i;
        for (i=EV_WRT_INTSTREAM+1; i<EV_ALL_INTSTREAM; i++)
            event_print(cli, i);
        return 0;
    } else {
        int i;
        for (i=0; i<EV_ALL_INTSTREAM; i++) {
            if (strcmp(name, event_string[i]) == 0) {
                cli_print(cli, "<Event - %s>", name);
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
 * \param cli the CLI pointer
 */
int event_list(cli_t *cli)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    cli_print(cli, "<Event List>");

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
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "  %s: ", event_string[i]);

            int j;
            for (j=0; j<compnt_ctx->ev_num[i]; j++) {
                compnt_t *c = compnt_ctx->ev_list[i][j];
                if (c->status == COMPNT_ENABLED) {
                    cli_buffer(buf, "%s ", c->name);
                } else {
                    cli_buffer(buf, "(%s) ", c->name);
                }
            }

            cli_bufprt(cli, buf);
        }
    }

    return 0;
}

/**
 * \brief Function to print the configuration of a component
 * \param cli the CLI pointer
 * \param c Component configuration
 * \param details The flag to enable the detailed description
 */
static int component_print(cli_t *cli, compnt_t *c, int details)
{
    char buf[__CONF_STR_LEN];

    cli_bufcls(buf);
    cli_buffer(buf, "%2d: %s", c->id+1, c->name);

    if (details == FALSE) {
        cli_buffer(buf, " (");
        if (c->status == COMPNT_DISABLED)
            cli_buffer(buf, "disabled");
        else if (c->activated)
            cli_buffer(buf, ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
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
    for (i=0; i<c->argc; i++)
        cli_buffer(buf, "%s ", c->argv[i]);

    cli_bufprt(cli, buf);

    if (c->type == COMPNT_AUTO)
        cli_print(cli, "    Type: autonomous");
    else
        cli_print(cli, "    Type: general");

    if (c->site == COMPNT_INTERNAL)
        cli_print(cli, "    Site: internal");
    else
        cli_print(cli, "    Site: external");

    if (c->role == COMPNT_ADMIN)
        cli_print(cli, "    Role: admin");
    else if (c->role == COMPNT_SECURITY_V1)
        cli_print(cli, "    Role: security");
    else if (c->role == COMPNT_SECURITY_V2)
        cli_print(cli, "    Role: security (one by one)");
    else if (c->role == COMPNT_MANAGEMENT)
        cli_print(cli, "    Role: management");
    else if (c->role == COMPNT_NETWORK)
        cli_print(cli, "    Role: network");
    else if (c->role == COMPNT_BASE)
        cli_print(cli, "    Role: base");

    cli_bufcls(buf);
    cli_buffer(buf, "    Permission: ");

    if (c->perm & COMPNT_READ) cli_buffer(buf, "r");
    if (c->perm & COMPNT_WRITE) cli_buffer(buf, "w");
    if (c->perm & COMPNT_EXECUTE) cli_buffer(buf, "x");

    cli_bufprt(cli, buf);

    if (c->status == COMPNT_DISABLED)
        cli_print(cli, "    Status: disabled");
    else if (c->activated == TRUE)
        cli_print(cli, "    Status: " ANSI_COLOR_GREEN "activated" ANSI_COLOR_RESET);
    else if (c->status == COMPNT_ENABLED)
        cli_print(cli, "    Status: " ANSI_COLOR_BLUE "enabled" ANSI_COLOR_RESET);

    cli_bufcls(buf);
    cli_buffer(buf, "    Inbounds: ");

    for (i=0; i<c->in_num; i++) {
        cli_buffer(buf, "%s(%d) ", event_string[c->in_list[i]], c->in_list[i]);
    }

    cli_bufprt(cli, buf);

    cli_bufcls(buf);
    cli_buffer(buf, "    Outbounds: ");

    for (i=0; i<c->out_num; i++) {
        cli_buffer(buf, "%s(%d) ", event_string[c->out_list[i]], c->out_list[i]);
    }

    cli_bufprt(cli, buf);

    return 0;
}

/**
 * \brief Function to print a component configuration
 * \param cli the CLI pointer
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
            cli_print(cli, "<Component Information>");
            component_print(cli, compnt_ctx->compnt_list[i], TRUE);
            return 0;
        }
    }

    cli_print(cli, "No component whose name is %s", name);

    return -1;
}

/**
 * \brief Function to print all component configurations
 * \param cli the CLI pointer
 */
int component_list(cli_t *cli)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    cli_print(cli, "<Component List>");

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        component_print(cli, compnt_ctx->compnt_list[i], FALSE);
    }

    return 0;
}

/**
 * \brief Function to enable a component
 * \param cli the CLI pointer
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
 * \param cli the CLI pointer
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
 * \param arg Component ID
 */
static void *thread_main(void *c_id)
{
    int id = *(int *)c_id;

    compnt_t *c = compnt_ctx->compnt_list[id];

    if (c->main(&c->activated, c->argc, c->argv) < 0) {
        c->activated = FALSE;
        FREE(c_id);
        return NULL;
    } else {
        FREE(c_id);
        return c;
    }
}

/**
 * \brief Function to activate a component
 * \param cli the CLI pointer
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
 * \param cli the CLI pointer
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
 * \param cli the CLI pointer
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
            }
        }
    }

    // activate others
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strncmp(compnt_ctx->compnt_list[i]->name, "conn", 4) == 0)
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
            }
        }
    }

    compnt_ctx->compnt_on = TRUE;

    return 0;
}

/**
 * \brief Function to deactivate all activated components
 * \param cli the CLI pointer
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
        }
    }

    compnt_ctx->compnt_on = FALSE;

    return 0;
}

/**
 * \brief Function to add a policy to a component
 * \param cli the CLI pointer
 * \param name Component name
 * \param p Operator-defined policy
 */
int component_add_policy(cli_t *cli, char *name, char *p)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    compnt_t *c = NULL;

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            c = compnt_ctx->compnt_list[i];
            break;
        }
    }

    if (c == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    int cnt = 0;
    char parm[__NUM_OF_ODP_FIELDS][__CONF_WORD_LEN] = {{0}};
    char val[__NUM_OF_ODP_FIELDS][__CONF_WORD_LEN] = {{0}};

    cli_print(cli, "Policy: %s, %s", c->name, p);

    char *token = strtok(p, ";");

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
                    c->odp[c->num_policies].flag |= ODP_DPID;
                    c->odp[c->num_policies].dpid[idx] = atoi(v);
                    cli_print(cli, "\tDPID: %lu", c->odp[c->num_policies].dpid[idx]);
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
                        c->odp[c->num_policies].flag |= ODP_PORT;
                        c->odp[c->num_policies].port[idx] = atoi(v);
                        cli_print(cli, "\tPort: %u", c->odp[c->num_policies].port[idx]);
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
                        c->odp[c->num_policies].flag |= ODP_PROTO;
                        c->odp[c->num_policies].proto |= PROTO_ARP;
                        cli_print(cli, "\tProtocol: ARP");
                    } else if (strcmp(v, "dhcp") == 0) {
                        c->odp[c->num_policies].flag |= ODP_PROTO;
                        c->odp[c->num_policies].proto |= PROTO_DHCP;
                        cli_print(cli, "\tProtocol: DHCP");
                    } else if (strcmp(v, "lldp") == 0) {
                        c->odp[c->num_policies].flag |= ODP_PROTO;
                        c->odp[c->num_policies].proto |= PROTO_LLDP;
                        cli_print(cli, "\tProtocol: LLDP");
                    } else if (strcmp(v, "ipv4") == 0) {
                        c->odp[c->num_policies].flag |= ODP_PROTO;
                        c->odp[c->num_policies].proto |= PROTO_IPV4;
                        cli_print(cli, "\tProtocol: IPv4");
                    } else if (strcmp(v, "tcp") == 0) {
                        c->odp[c->num_policies].flag |= ODP_PROTO;
                        c->odp[c->num_policies].proto |= PROTO_TCP;
                        cli_print(cli, "\tProtocol: TCP");
                    } else if (strcmp(v, "udp") == 0) {
                        c->odp[c->num_policies].flag |= ODP_PROTO;
                        c->odp[c->num_policies].proto |= PROTO_UDP;
                        cli_print(cli, "\tProtocol: UDP");
                    } else if (strcmp(v, "icmp") == 0) {
                        c->odp[c->num_policies].flag |= ODP_PROTO;
                        c->odp[c->num_policies].proto |= PROTO_ICMP;
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
                        c->odp[c->num_policies].flag |= ODP_SRCIP;
                        c->odp[c->num_policies].srcip[idx] = input.s_addr;
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
                        c->odp[c->num_policies].flag |= ODP_DSTIP;
                        c->odp[c->num_policies].dstip[idx] = input.s_addr;
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
                        c->odp[c->num_policies].flag |= ODP_SPORT;
                        c->odp[c->num_policies].sport[idx] = port;
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
                        c->odp[c->num_policies].flag |= ODP_DPORT;
                        c->odp[c->num_policies].dport[idx] = port;
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

    c->num_policies++;

    return 0;
}

/**
 * \brief Function to delete a policy from a component
 * \param cli the CLI pointer
 * \param name Component name
 * \param idx The index of the target operator-defined policy
 */
int component_del_policy(cli_t *cli, char *name, int idx)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    compnt_t *c = NULL;

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            c = compnt_ctx->compnt_list[i];
            break;
        }
    }

    if (c == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    if (c->num_policies < idx) {
        if (c->num_policies == 0) {
            cli_print(cli, "There is no policy in %s", c->name);
            return -1;
        } else {
            cli_print(cli, "%s has only %u policies", c->name, c->num_policies);
            return -1;
        }
    }

    memset(&c->odp[idx-1], 0, sizeof(odp_t));

    for (i=idx; i<__MAX_POLICY_ENTRIES; i++) {
        memmove(&c->odp[i-1], &c->odp[i], sizeof(odp_t));
    }

    memset(&c->odp[i-1], 0, sizeof(odp_t));

    c->num_policies--;

    cli_print(cli, "Deleted policy #%u in %s", idx, c->name);

    return 0;
}

/**
 * \brief Function to show all policies of a component
 * \param cli the CLI pointer
 * \param name Component name
 */
int component_show_policy(cli_t *cli, char *name)
{
    if (compnt_ctx == NULL) {
        cli_print(cli, "Need to load configurations");
        return -1;
    }

    compnt_t *c = NULL;

    int i;
    for (i=0; i<compnt_ctx->num_compnts; i++) {
        if (strcmp(compnt_ctx->compnt_list[i]->name, name) == 0) {
            c = compnt_ctx->compnt_list[i];
            break;
        }
    }

    if (c == NULL) {
        cli_print(cli, "%s does not exist", name);
        return -1;
    }

    cli_print(cli, "<Operator-Defined Policies for %s>", c->name);

    for (i=0; i<c->num_policies; i++) {
        cli_print(cli, "  Policy #%02u:", i+1);

        if (c->odp[i].dpid[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Datapath ID: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (c->odp[i].dpid[j] == 0)
                    break;
                cli_buffer(buf, "%lu ", c->odp[i].dpid[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (c->odp[i].port[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (c->odp[i].port[j] == 0)
                    break;
                cli_buffer(buf, "%u ", c->odp[i].port[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (c->odp[i].proto) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Protocol: ");

            if (c->odp[i].proto & PROTO_IPV4)
                cli_buffer(buf, "IPv4 ");
            if (c->odp[i].proto & PROTO_TCP)
                cli_buffer(buf, "TCP ");
            if (c->odp[i].proto & PROTO_UDP)
                cli_buffer(buf, "UDP ");
            if (c->odp[i].proto & PROTO_ICMP)
                cli_buffer(buf, "ICMP ");
            if (c->odp[i].proto & PROTO_ARP)
                cli_buffer(buf, "ARP ");
            if (c->odp[i].proto & PROTO_DHCP)
                cli_buffer(buf, "DHCP ");
            if (c->odp[i].proto & PROTO_LLDP)
                cli_buffer(buf, "LLDP ");

            cli_bufprt(cli, buf);
        }

        if (c->odp[i].srcip[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Source IP: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (c->odp[i].srcip[j] == 0)
                    break;
                struct in_addr ip;
                ip.s_addr = c->odp[i].srcip[j];
                cli_buffer(buf, "%s ", inet_ntoa(ip));
            }

            cli_bufprt(cli, buf);
        }

        if (c->odp[i].dstip[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Destination IP: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (c->odp[i].dstip[j] == 0)
                    break;
                struct in_addr ip;
                ip.s_addr = c->odp[i].dstip[j];
                cli_buffer(buf, "%s ", inet_ntoa(ip));
            }

            cli_bufprt(cli, buf);
        }

        if (c->odp[i].sport[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Source port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (c->odp[i].sport[j] == 0)
                    break;
                cli_buffer(buf, "%u ", c->odp[i].sport[j]);
            }

            cli_bufprt(cli, buf);
        }

        if (c->odp[i].dport[0]) {
            char buf[__CONF_STR_LEN];

            cli_bufcls(buf);
            cli_buffer(buf, "    Destination port: ");

            int j;
            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (c->odp[i].dport[j] == 0)
                    break;
                cli_buffer(buf, "%u ", c->odp[i].dport[j]);
            }

            cli_bufprt(cli, buf);
        }
    }

    if (!c->num_policies)
        cli_print(cli, "  No policy");

    return 0;
}

/**
 * \brief Function to deliver a command to the corresponding component
 * \param cli the CLI pointer
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
 * \brief Function to free all configuration structures
 * \param num_compnts The number of registered components
 * \param compnt_list The list of the components
 * \param ev_num The number of components for each event
 * \param ev_list The components listening to each event
 */
static int clean_structures(int num_compnts, compnt_t **compnt_list, int *ev_num, compnt_t ***ev_list)
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
 * \param cli The CLI pointer
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
        return -1;
    } else {
        memset(compnt_list, 0, sizeof(compnt_t *) * __MAX_COMPONENTS);
    }

    int *ev_num = (int *)MALLOC(sizeof(int) * __MAX_EVENTS);
    if (ev_num == NULL) {
        PERROR("malloc");
        clean_structures(num_compnts, compnt_list, NULL, NULL);
        return -1;
    } else {
        memset(ev_num, 0, sizeof(int) * __MAX_EVENTS);
    }

    compnt_t ***ev_list = (compnt_t ***)MALLOC(sizeof(compnt_t **) * __MAX_EVENTS);
    if (ev_list == NULL) {
        PERROR("malloc");
        clean_structures(num_compnts, compnt_list, ev_num, NULL);
        return -1;
    } else {
        int i;
        for (i=0; i<__MAX_EVENTS; i++) {
            ev_list[i] = (compnt_t **)MALLOC(sizeof(compnt_t *) * __MAX_COMPONENTS);
            if (ev_list[i] == NULL) {
                PERROR("malloc");
                clean_structures(num_compnts, compnt_list, ev_num, ev_list);
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

        char status[__CONF_WORD_LEN] = {0};
        json_t *j_status = json_object_get(data, "status");
        if (json_is_string(j_status)) {
            strcpy(status, json_string_value(j_status));
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
        compnt_t *c = calloc(1, sizeof(compnt_t));
        if (!c) {
             PERROR("calloc");
             clean_structures(num_compnts, compnt_list, ev_num, ev_list);
             return -1;
        }

        // set an internal ID (simply for ordering)
        c->id = num_compnts;

        // set the component name
        if (strlen(name) == 0) {
            cli_print(cli, "No component name");
            FREE(c);
            clean_structures(num_compnts, compnt_list, ev_num, ev_list);
            return -1;
        } else {
            strcpy(c->name, name);
            cli_print(cli, "%3d: %s", c->id+1, name);

            // set the actual ID
            c->component_id = hash_func((uint32_t *)&c->name, __HASHING_NAME_LENGTH);

#ifdef __SHOW_COMPONENT_ID
            cli_print(cli, "     ID: %u", c->component_id);
#endif /* __SHOW_COMPONENT_ID */
        }

        // set arguments
        strcpy(c->args, args);
        str2args(c->args, &c->argc, &c->argv[1], __CONF_ARGC);
        c->argc++;
        c->argv[0] = c->name;
        if (strlen(c->args))
            cli_print(cli, "     Arguments: %s", c->args);

        // set a type
        if (strlen(type) == 0) {
            c->type = COMPNT_GENERAL;
        } else {
            c->type = (strcmp(type, "autonomous") == 0) ? COMPNT_AUTO : COMPNT_GENERAL;
        }
        cli_print(cli, "     Type: %s", type);

        // set a site
        if (strlen(site) == 0) {
            c->site = COMPNT_INTERNAL;
        } else {
            c->site = (strcmp(site, "internal") == 0) ? COMPNT_INTERNAL : COMPNT_EXTERNAL;
        }
        cli_print(cli, "     Site: %s", site);

        // set functions
        if (c->site == COMPNT_INTERNAL) { // internal
            if (k == num_components) {
                FREE(c);
                continue;
            }

            c->main = g_components[k].main;
            c->handler = g_components[k].handler;
            c->cleanup = g_components[k].cleanup;
            c->cli = g_components[k].cli;
        } else { // external
            c->main = NULL;
            c->handler = NULL;
            c->cleanup = NULL;
            c->cli = NULL;
        }

        // set a role
        if (strlen(role) == 0) {
            c->role = COMPNT_NETWORK;
        } else {
            if (strcmp(role, "base") == 0)
                c->role = COMPNT_BASE;
            else if (strcmp(role, "network") == 0)
                c->role = COMPNT_NETWORK;
            else if (strcmp(role, "management") == 0)
                c->role = COMPNT_MANAGEMENT;
            else if (strcmp(role, "security") == 0)
                c->role = COMPNT_SECURITY_V1;
            else if (strcmp(role, "security_v2") == 0)
                c->role = COMPNT_SECURITY_V2;
            else if (strcmp(role, "admin") == 0)
                c->role = COMPNT_ADMIN;
            else
                c->role = COMPNT_NETWORK;
        }
        cli_print(cli, "     Role: %s", role);

        // set permissions
        int size = (strlen(perm) > COMPNT_MAX_PERM) ? COMPNT_MAX_PERM : strlen(perm);
        int l;
        for (l=0; l<size; l++) {
            if (perm[l] == 'r') {
                c->perm |= COMPNT_READ;
            } else if (perm[l] == 'w') {
                c->perm |= COMPNT_WRITE;
            } else if (perm[l] == 'x') {
                c->perm |= COMPNT_EXECUTE;
            }
        }
        if (c->perm == 0) c->perm |= COMPNT_READ;
        cli_print(cli, "     Permission: %s", perm);

        // set a status
        if (strlen(status) == 0) {
            c->status = COMPNT_DISABLED;
        } else {
            c->status = (strcmp(status, "enabled") == 0) ? COMPNT_ENABLED : COMPNT_DISABLED;
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

                if (event_type(json_string_value(event)) == EV_NUM_EVENTS) {
                    cli_print(cli, "     Inbounds: wrong event name");
                    FREE(c);
                    clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                    return -1;
                }
            }

            // then, set the inbound events
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                if (event_type(json_string_value(event)) == EV_NONE) {
                    break;
                } else if (event_type(json_string_value(event)) == EV_ALL_UPSTREAM) {
                    if (c->role < COMPNT_SECURITY_V1) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(c);
                        clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                        return -1;
                    }

                    int k;
                    for (k=EV_NONE+1; k<EV_ALL_UPSTREAM; k++) {
                        real_event_cnt++;

                        c->in_list[c->in_num] = k;
                        c->in_num++;

                        ev_list[k][ev_num[k]] = c;
                        ev_num[k]++;
                    }
                } else if (event_type(json_string_value(event)) == EV_ALL_DOWNSTREAM) {
                    if (c->role < COMPNT_SECURITY_V1) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(c);
                        clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                        return -1;
                    }

                    int k;
                    for (k=EV_ALL_UPSTREAM+1; k<EV_ALL_DOWNSTREAM; k++) {
                        real_event_cnt++;

                        c->in_list[c->in_num] = k;
                        c->in_num++;

                        ev_list[k][ev_num[k]] = c;
                        ev_num[k]++;
                    }
                } else if (event_type(json_string_value(event)) == EV_WRT_INTSTREAM) {
                    if (c->role < COMPNT_SECURITY_V1) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(c);
                        clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                        return -1;
                    }

                    int k;
                    for (k=EV_ALL_DOWNSTREAM+1; k<EV_WRT_INTSTREAM; k++) {
                        real_event_cnt++;

                        c->in_list[c->in_num] = k;
                        c->in_num++;

                        ev_list[k][ev_num[k]] = c;
                        ev_num[k]++;
                    }
                } else if (event_type(json_string_value(event)) == EV_ALL_INTSTREAM) {
                    if (c->role < COMPNT_SECURITY_V1) {
                        cli_print(cli, "     Inbounds: lower-level role");
                        FREE(c);
                        clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                        return -1;
                    }

                    int k;
                    for (k=EV_WRT_INTSTREAM+1; k<EV_ALL_INTSTREAM; k++) {
                        real_event_cnt++;

                        c->in_list[c->in_num] = k;
                        c->in_num++;

                        ev_list[k][ev_num[k]] = c;
                        ev_num[k]++;
                    }
                } else {
                    real_event_cnt++;

                    c->in_list[c->in_num] = event_type(json_string_value(event));
                    c->in_num++;

                    ev_list[event_type(json_string_value(event))][ev_num[event_type(json_string_value(event))]] = c;
                    ev_num[event_type(json_string_value(event))]++;
                }
            }

            if (real_event_cnt == 0) cli_print(cli, "     Inbounds: no event");
            else if (real_event_cnt == 1) cli_print(cli, "     Inbounds: 1 event");
            else cli_print(cli, "     Inbounds: %d events", c->in_num);
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
                    FREE(c);
                    clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_ALL_DOWNSTREAM) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(c);
                    clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_WRT_INTSTREAM) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(c);
                    clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_ALL_INTSTREAM) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(c);
                    clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                    return -1;
                } else if (event_type(json_string_value(out_event)) == EV_NUM_EVENTS) {
                    cli_print(cli, "     Outbounds: wrong event name");
                    FREE(c);
                    clean_structures(num_compnts, compnt_list, ev_num, ev_list);
                    return -1;
                }
            }

            // then, set the outbound events
            for (j=0; j<json_array_size(out_events); j++) {
                json_t *out_event = json_array_get(out_events, j);

                if (event_type(json_string_value(out_event)) == EV_NONE)
                    break;

                real_event_cnt++;

                c->out_list[c->out_num] = event_type(json_string_value(out_event));
                c->out_num++;
            }
            if (real_event_cnt == 0) cli_print(cli, "     Outbounds: no event");
            else if (real_event_cnt == 1) cli_print(cli, "     Outbounds: 1 event");
            else cli_print(cli, "     Outbounds: %d events", c->out_num);
        }

        compnt_list[num_compnts] = c;
        num_compnts++;
    }

    json_decref(json);

    // sort components based on the ordering policy
    for (i=1; i<EV_NUM_EVENTS; i++) {
        compnt_t **list = ev_list[i];

        if (ev_num[i] == 0)
            continue;

        // first, based on role
        int j;
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

        // second, based on perm while keeping the order of roles
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
                new->status = old->status;
                new->activated = old->activated;

                break;
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
                pass = TRUE;

                break;
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
    clean_structures(temp_num_compnts, temp_compnt_list, temp_ev_num, temp_ev_list);

    return 0;
}

/**
 * @}
 *
 * @}
 */
