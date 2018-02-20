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

/** \brief The default application configuration file */
char app_file[__CONF_WORD_LEN] = APP_DEFAULT_CONFIG_FILE;

/** \brief The default role file */
char role_file[__CONF_WORD_LEN] = DEFAULT_APP_ROLE_FILE;

/////////////////////////////////////////////////////////////////////

/** \brief The role of an application */
enum {
    APP_BASE,
    APP_NETWORK,
    APP_MANAGEMENT,
    APP_SECURITY,
    APP_ADMIN,
    NUM_APP_ROLES,
};

/////////////////////////////////////////////////////////////////////

/** \brief The number of applications */
int app_to_role_cnt;

/** \brief The structure for mapping app IDs and app roles */
typedef struct _app2role_t {
    int id; /**< Application ID */
    int role; /**< Application role */
    char name[__CONF_WORD_LEN]; /**< Application name */
} app2role_t;

/** \brief The mapping table of applications and their roles */
app2role_t *app_to_role;

/////////////////////////////////////////////////////////////////////

/** \brief The structure for mapping app roles and app events */
typedef struct _role2ev_t {
    char name[__CONF_WORD_LEN]; /**< Role */
    uint8_t event[__MAX_APP_EVENTS]; /**< Events for the role */
} role2ev_t;

/** \brief The mapping table of roles and their events */
role2ev_t *role_to_event;

/////////////////////////////////////////////////////////////////////

/** \brief The app event list to convert an app event string to an app event ID */
const char app_ev_list[AV_NUM_EVENTS+1][__CONF_WORD_LEN] = {
#include "app_event_string.h"
};

/**
 * \brief Function to convert an app event string to an app event ID
 * \param event App event string
 * \return App event ID
 */
static int app_event_type(const char *event)
{
    int i;
    for (i=0; i<AV_NUM_EVENTS+1; i++) {
        if (strcmp(event, app_ev_list[i]) == 0)
            return i;
    }
    return AV_NUM_EVENTS;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to load the roles of applications
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
        PERROR("json_loads");
        return -1;
    } else if (!json_is_array(json)) {
        PERROR("json_is_array");
        json_decref(json);
        return -1;
    }

    int i;
    for (i=0; i<json_array_size(json); i++) {
        json_t *data = json_array_get(json, i);

        char name[__CONF_WORD_LEN] = {0};
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        char role[__CONF_WORD_LEN] = {0};
        json_t *j_role = json_object_get(data, "role");
        if (json_is_string(j_role)) {
            strcpy(role, json_string_value(j_role));
        }

        if (strlen(name) == 0) {
            PRINTF("no application name\n");
            return -1;
        }

        app_to_role[app_to_role_cnt].id = hash_func((uint32_t *)&name, __HASHING_NAME_LENGTH);

        strcpy(app_to_role[app_to_role_cnt].name, name);

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
            else
                app_to_role[app_to_role_cnt].role = APP_NETWORK;
        }

        app_to_role_cnt++;
    }

    json_decref(json);

    int ext_app_cnt = 0;
    char ext_apps[__MAX_APPLICATIONS][__CONF_WORD_LEN];

    get_external_configurations(ext_apps, &ext_app_cnt, DEFAULT_EXT_APP_PATH);

    for (i=0; i<ext_app_cnt; i++) {
        ALOG_INFO(RBAC_ID, "External application configuration file: %s", ext_apps[i]);

        if (access(ext_apps[i], F_OK)) {
            ALOG_INFO(RBAC_ID, "No file whose name is '%s'", ext_apps[i]);
            return -1;
        }

        raw = str_read(ext_apps[i]);
        conf = str_preproc(raw);

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

        json_t *data = json_array_get(json, 0);

        char name[__CONF_WORD_LEN] = {0};
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        char role[__CONF_WORD_LEN] = {0};
        json_t *j_role = json_object_get(data, "role");
        if (json_is_string(j_role)) {
            strcpy(role, json_string_value(j_role));
        }

        if (strlen(name) == 0) {
            PRINTF("no application name\n");
            return -1;
        }

        app_to_role[app_to_role_cnt].id = hash_func((uint32_t *)&name, __HASHING_NAME_LENGTH);

        strcpy(app_to_role[app_to_role_cnt].name, name);

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
            else
                app_to_role[app_to_role_cnt].role = APP_NETWORK;
        }

        app_to_role_cnt++;

        json_decref(json);
    }

    return 0;
}

/**
 * \brief Function to load the available events of each role
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
        PERROR("json_loads");
        return -1;
    } else if (!json_is_array(json)) {
        PERROR("json_is_array");
        json_decref(json);
        return -1;
    }

    int i;
    for (i=0; i<json_array_size(json); i++) {
        json_t *data = json_array_get(json, i);

        char name[__CONF_WORD_LEN] = {0};
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        int role = 0;
        if (strlen(name) == 0) {
            PRINTF("no role type\n");
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
            else
                continue;
        }

        strcpy(role_to_event[role].name, name);

        // set inbound events
        json_t *events = json_object_get(data, "events");
        if (json_is_array(events)) {
            int j;
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                if (app_event_type(json_string_value(event)) == AV_NUM_EVENTS) {
                    PRINTF("wrong app event name\n");
                    return -1;
                }
            }

            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                role_to_event[role].event[app_event_type(json_string_value(event))] = 1;
            }
        }
    }

    return 0;
}

/**
 * \brief Function to initialize the mapping tables of applications and their roles
 * \return None
 */
static int reset_app_roles(void)
{
    app_to_role_cnt = 0;

    memset(app_to_role, 0, sizeof(app2role_t) * __MAX_APPLICATIONS);
    memset(role_to_event, 0, sizeof(role2ev_t) * NUM_APP_ROLES);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to check the incoming event of the corresponding application
 * \param id Application ID
 * \param type App event
 */
static int check_app_role(uint32_t id, uint16_t type)
{
    int i;
    for (i=0; i<app_to_role_cnt; i++) {
        if (app_to_role[i].id == id) { // found the matched app
            if (app_to_role[i].role == APP_BASE) // skip base
                return -1;
            else if (role_to_event[app_to_role[i].role].event[type] == 1) // allow or deny
                return 1;
            else
                return 0;
        }
    }

    return 0;
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

    app_to_role = (app2role_t *)MALLOC(sizeof(app2role_t) * __MAX_APPLICATIONS);
    if (app_to_role == NULL) {
        PERROR("malloc");
        return -1;
    }

    role_to_event = (role2ev_t *)MALLOC(sizeof(role2ev_t) * NUM_APP_ROLES);
    if (role_to_event == NULL) {
        PERROR("malloc");
        return -1;
    }

    reset_app_roles();
    
    // load app roles
    if (load_app_roles(app_file) < 0)
        return -1;

    // load app events for each role
    if (load_events_for_roles(role_file) < 0)
        return -1;

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
 * \param cli The CLI pointer
 * \param args Arguments
 */
int rbac_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param av Read-only app event
 * \param av_out Read-write app event (if this application has the write permission)
 */
int rbac_handler(const app_event_t *av, app_event_out_t *av_out)
{
    if (check_app_role(av->id, av->type)) {
        return 0;
    }

    int i;
    for (i=0; i<app_to_role_cnt; i++) {
        if (app_to_role[i].id == av->id) {
            ALOG_WARN(RBAC_ID, "Unauthorized access (app: %s (%u), type: %s)", app_to_role[i].name, av->id, app_ev_list[av->type]);
            break;
        }
    }

    return -1;
}

/**
 * @}
 *
 * @}
 */
