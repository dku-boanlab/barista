/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup app_shim External Application Handler
 * \brief Functions to glue the Barista NOS and an external application
 * @{
 * \defgroup app_load External Application Loader
 * \brief Functions to execute an external application
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "shim.h"

/////////////////////////////////////////////////////////////////////

/** \brief External application context */
app_t app;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize an external application
 * \return None
 */
static int init_app(void)
{
    json_t *json;
    json_error_t error;

    PRINTF("Application configuration file: %s\n", DEFAULT_APP_CONFIG_FILE);

    if (access(DEFAULT_APP_CONFIG_FILE, F_OK)) {
        PRINTF("No file whose name is '%s'\n", DEFAULT_APP_CONFIG_FILE);
        return -1;
    }

    char *raw = str_read(DEFAULT_APP_CONFIG_FILE);
    char *conf = str_preproc(raw);

    json = json_loads(conf, 0, &error);
    if (!json) {
        PERROR("json_loads");
        return -1;
    }

    if (!json_is_array(json)) {
        PERROR("json_is_array");
        json_decref(json);
    }

    int i, pass = FALSE;
    for (i=0; i<json_array_size(json); i++) {
        json_t *data = json_array_get(json, i);

        char name[__CONF_WORD_LEN] = {0};
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        if (strcmp(name, TARGET_APP) != 0) continue;

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

        // set the application name
        if (strlen(name) == 0) {
            PRINTF("No application name\n");
            json_decref(json);
            return -1;
        }

        strcpy(app.name, name);
        app.app_id = TARGET_APP_ID;

        // set arguments
        strcpy(app.args, args);
        str2args(app.args, &app.argc, &app.argv[1], __CONF_ARGC);
        app.argc++;
        app.argv[0] = app.name;

        // set a site
        if (strlen(site) == 0) {
            app.site = APP_INTERNAL;
        } else {
            if (strcmp(site, "external") == 0)
#ifndef __ENABLE_CBENCH
                app.site = APP_EXTERNAL;
#else /* __ENABLE_CBENCH */
                app.site = APP_INTERNAL;
#endif /* __ENABLE_CBENCH */
            else
                app.site = APP_INTERNAL;
        }

        // set functions
        app.main = g_applications[0].main;
        app.handler = g_applications[0].handler;
        app.cleanup = g_applications[0].cleanup;
        app.cli = g_applications[0].cli;

        // set a type
        app.type = (strcmp(type, "autonomous") == 0) ? APP_AUTO : APP_GENERAL;

        pass = TRUE;

        break;
    }

    json_decref(json);

    if (!pass) {
        PRINTF("No %s in the given configuration file\n", TARGET_APP);
        return -1;
    }

    app.activated = FALSE;

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to execute the main function of an external application
 * \return Application context
 */
static void *app_thread_main(void *null)
{
    if (app.main(&app.activated, app.argc, app.argv) < 0) {
        app.activated = FALSE;
        return NULL;
    } else {
        return &app;
    }
}

/**
 * \brief Function to activate an external application
 * \return None
 */
static int activate_app(void)
{
    // general type?
    if (app.type == APP_GENERAL) {
        if (app_thread_main(NULL) == NULL) {
            return -1;
        }
    // autonomous type?
    } else {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &app_thread_main, NULL)) {
            PERROR("pthread_create");
            return -1;
        }
    }

    return 0;
}

/**
 * \brief Function to deactivate an external application
 * \return None
 */
static int deactivate_app(void)
{
    if (app.cleanup(&app.activated) < 0) {
        return -1;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/** \brief The SIGINT handler definition */
void (*sigint_func)(int);

/** \brief Function to handle the SIGINT signal */
void sigint_handler(int sig)
{
    //PRINTF("Got a SIGINT signal\n");

    if (deactivate_app())
        PRINTF("Failed to destroy %s", app.name);

    destroy_app_event(NULL);
}

/** \brief The SIGKILL handler definition */
void (*sigkill_func)(int);

/** \brief Function to handle the SIGKILL signal */
void sigkill_handler(int sig)
{
    //PRINTF("Got a SIGKILL signal\n");

    if (deactivate_app())
        PRINTF("Failed to destroy %s", app.name);

    destroy_app_event(NULL);
}

/** \brief The SIGTERM handler definition */
void (*sigterm_func)(int);

/** \brief Function to handle the SIGTERM signal */
void sigterm_handler(int sig)
{
    //PRINTF("Got a SIGTERM signal\n");

    if (deactivate_app())
        PRINTF("Failed to destroy %s", app.name);

    destroy_app_event(NULL);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to launch an external application
 * \param argc The number of arguments
 * \param argv Arguments
 */
int main(int argc, char *argv[])
{
    // init app
    if (init_app()) {
        PRINTF("Failed to initialize %s\n", TARGET_APP);
        return -1;
    } else {
        PRINTF("Initialized %s\n", TARGET_APP);
    }

    if (app.site != APP_EXTERNAL) {
        if (argc == 2 && strcmp(argv[1], "docker") == 0) {
            while (1) waitsec(1, 0);
        }

        PRINTF("Terminated %s\n", TARGET_APP);
        return 0;
    }

    // init app event handler
    if (init_app_event(NULL)) {
        PRINTF("Failed to initialize the external app event handler\n");
        return -1;
    } else {
        PRINTF("Initialized the external app event handler\n");
    }

    // activate app
    if (activate_app()) {
        PRINTF("Failed to activate %s\n", TARGET_APP);
        return -1;
    } else {
        PRINTF("Activated %s\n", TARGET_APP);
    }

    // signal handlers
    sigint_func = signal(SIGINT, sigint_handler);
    sigkill_func = signal(SIGKILL, sigkill_handler);
    sigterm_func = signal(SIGTERM, sigterm_handler);

    waitsec(1, 0);

    while(app.activated) {
        waitsec(1, 0);
    }

    PRINTF("Terminated %s\n", TARGET_APP);

    return 0;
}

/**
 * @}
 *
 * @}
 *
 * @}
 */
