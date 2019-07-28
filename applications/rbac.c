/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup app
 * @{
 * \defgroup rbac Role-based Authorization
 * \brief (Security) role-based authorization
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "rbac.h"

/** \brief Role-based authorization ID */
#define RBAC_ID 3896669476

/////////////////////////////////////////////////////////////////////

/** \brief The app event list to convert an app event string to an app event ID */
const char av_string[AV_NUM_EVENTS+1][__CONF_WORD_LEN] = {
    #include "app_event_string.h"
};

/**
 * \brief Function to convert an app event string to an app event ID
 * \param event App event string
 * \return App event ID
 */
static int av_type(const char *event)
{
    int i;
    for (i=0; i<AV_NUM_EVENTS+1; i++) {
        if (strcmp(event, av_string[i]) == 0)
            return i;
    }
    return AV_NUM_EVENTS;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to load application roles
 * \param conf_file Application configuration file
 */
static int load_app_roles(char *conf_file)
{
    json_t *json;
    json_error_t error;

    ALOG_INFO(RBAC_ID, "Application configuration file: %s", conf_file);

    if (access(conf_file, F_OK)) {
        ALOG_ERROR(RBAC_ID, "No application configuration file whose name is '%s'", conf_file);
        return -1;
    }

    char *raw = str_read(conf_file);
    char *conf = str_preproc(raw);

    json = json_loads(conf, 0, &error);
    if (!json) {
        ALOG_ERROR(RBAC_ID, "json_loads() failed");
        return -1;
    } else if (!json_is_array(json)) {
        ALOG_ERROR(RBAC_ID, "json_is_array() failed");
        json_decref(json);
        return -1;
    }

    int i;
    for (i=0; i<json_array_size(json); i++) {
        json_t *data = json_array_get(json, i);

        char name[__CONF_WORD_LEN];
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        char role[__CONF_WORD_LEN];
        json_t *j_role = json_object_get(data, "role");
        if (json_is_string(j_role)) {
            strcpy(role, json_string_value(j_role));
        }

        if (strlen(name) == 0) {
            ALOG_ERROR(RBAC_ID, "No application name");
            json_decref(json);
            return -1;
        } else {
            app_to_role[app_to_role_cnt].id = hash_func((uint32_t *)&name, __HASHING_NAME_LENGTH);
            strcpy(app_to_role[app_to_role_cnt].name, name);
        }

        if (strlen(role) == 0) {
            app_to_role[app_to_role_cnt].role = APP_NETWORK;
        } else {
            if (strcmp(role, "base") == 0)
                app_to_role[app_to_role_cnt].role = APP_BASE;
            else if (strcmp(role, "network") == 0)
                app_to_role[app_to_role_cnt].role = APP_NETWORK;
            else if (strcmp(role, "management") == 0)
                app_to_role[app_to_role_cnt].role = APP_MANAGEMENT;
            else if (strcmp(role, "security") == 0)
                app_to_role[app_to_role_cnt].role = APP_SECURITY;
            else if (strcmp(role, "admin") == 0)
                app_to_role[app_to_role_cnt].role = APP_ADMIN;
            else {
                ALOG_ERROR(RBAC_ID, "Wrong role type: %s", role);
                json_decref(json);
                return -1;
            }
        }

        app_to_role_cnt++;
    }

    json_decref(json);

    return 0;
}

/**
 * \brief Function to load the events grouped by each role
 * \param role_file Application role file
 */
static int load_events_for_roles(char *role_file)
{
    json_t *json;
    json_error_t error;

    ALOG_INFO(RBAC_ID, "Application role file: %s", role_file);

    if (access(role_file, F_OK)) {
        ALOG_ERROR(RBAC_ID, "No application role file whose name is '%s'", role_file);
        return -1;
    }

    char *raw = str_read(role_file);
    char *role = str_preproc(raw);

    json = json_loads(role, 0, &error);
    if (!json) {
        ALOG_ERROR(RBAC_ID, "json_loads() failed");
        return -1;
    } else if (!json_is_array(json)) {
        ALOG_ERROR(RBAC_ID, "json_is_array() failed");
        json_decref(json);
        return -1;
    }

    int i, j;
    for (i=0; i<json_array_size(json); i++) {
        json_t *data = json_array_get(json, i);

        char name[__CONF_WORD_LEN];
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        int role = 0;
        if (strlen(name) == 0) {
            ALOG_ERROR(RBAC_ID, "No role type");
            json_decref(json);
            return -1;
        } else {
            if (strcmp(name, "network") == 0)
                role = APP_NETWORK;
            else if (strcmp(name, "management") == 0)
                role = APP_MANAGEMENT;
            else if (strcmp(name, "security") == 0)
                role = APP_SECURITY;
            else if (strcmp(name, "admin") == 0)
                role = APP_ADMIN;
            else {
                ALOG_ERROR(RBAC_ID, "Wrong role type: %s", role);
                json_decref(json);
                return -1;
            }
        }

        strcpy(role_to_event[role].name, name);

        // set inbound events
        json_t *events = json_object_get(data, "events");
        if (json_is_array(events)) {
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);
                if (av_type(json_string_value(event)) == AV_NUM_EVENTS) {
                    ALOG_ERROR(RBAC_ID, "Wrong app event name: %s", json_string_value(event));
                    json_decref(json);
                    return -1;
                }
            }

            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);
                role_to_event[role].event[av_type(json_string_value(event))] = 1;
            }
        }
    }

    json_decref(json);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to verify an incoming event with a triggering application
 * \param id Application ID
 * \param type App event
 */
static int verify_app_role(uint32_t id, uint16_t type)
{
    int i;

    for (i=0; i<app_to_role_cnt; i++) {
        if (app_to_role[i].id == id) {
            if (app_to_role[i].role == APP_BASE) return 0;
            else if (role_to_event[app_to_role[i].role].event[type] == TRUE) return 0;
            else return -1;
        }
    }

    return -1;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int rbac_main(int *activated, int argc, char **argv)
{
    ALOG_INFO(RBAC_ID, "Init - Role-based access control");

    app_to_role_cnt = 0;
    app_to_role = (app_to_role_t *)CALLOC(__MAX_APPLICATIONS, sizeof(app_to_role_t));
    if (app_to_role == NULL) {
        ALOG_ERROR(RBAC_ID, "calloc() failed");
        return -1;
    }

    role_to_event = (role_to_event_t *)CALLOC(__NUM_APP_ROLES, sizeof(role_to_event_t));
    if (role_to_event == NULL) {
        ALOG_ERROR(RBAC_ID, "calloc() failed");
        return -1;
    }

    // load app roles
    if (load_app_roles(DEFAULT_APP_CONFIG_FILE) < 0) {
        FREE(role_to_event);
        FREE(app_to_role);
        return -1;
    }

    // load app events for each role
    if (load_events_for_roles(DEFAULT_APP_ROLE_FILE) < 0) {
        FREE(role_to_event);
        FREE(app_to_role);
        return -1;
    }

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this application
 */
int rbac_cleanup(int *activated)
{
    ALOG_INFO(RBAC_ID, "Clean up - Role-based access control");

    deactivate();

    FREE(role_to_event);
    FREE(app_to_role);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int rbac_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int rbac_handler(const app_event_t *av, app_event_out_t *av_out)
{
    int res = verify_app_role(av->id, av->type);

    if (res < 0) {
        ALOG_WARN(RBAC_ID, "Wrong application ID (caller: %u, event: %s)", av->id, av_string[av->type]);
        return -1;
    } else if (res > 0) {
        int i;
        for (i=0; i<app_to_role_cnt; i++) {
            if (app_to_role[i].id == av->id) {
                ALOG_WARN(RBAC_ID, "Unauthorized access (app: %s (%u), type: %s)", (strlen(app_to_role[i].name) > 0) ? app_to_role[i].name : "unknown", av->id, av_string[av->type]);
                break;
            }
        }
        return -1;
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */
