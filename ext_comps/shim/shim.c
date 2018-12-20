/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup framework
 * @{
 * \defgroup compnt_shim External Component Handler
 * \brief Functions to glue the Barista NOS and external components
 * @{
 * \defgroup compnt_load External Component Loader
 * \brief Functions to execute an external component
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "shim.h"

/////////////////////////////////////////////////////////////////////

/** \brief External component context */
compnt_t compnt;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize an external component
 * \return None
 */
static int init_compnt(void)
{
    json_t *json;
    json_error_t error;

    PRINTF("Component configuration file: %s\n", DEFAULT_COMPNT_CONFIG_FILE);

    if (access(DEFAULT_COMPNT_CONFIG_FILE, F_OK)) {
        PRINTF("No file whose name is '%s'\n", DEFAULT_COMPNT_CONFIG_FILE);
        return -1;
    }

    char *raw = str_read(DEFAULT_COMPNT_CONFIG_FILE);
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

        if (strcmp(name, TARGET_COMPNT) != 0)
            continue;

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

        // set the component name
        if (strlen(name) == 0) {
            PRINTF("No component name\n");
            json_decref(json);
            return -1;
        } else {
            strcpy(compnt.name, name);
            compnt.component_id = hash_func((uint32_t *)&compnt.name, __HASHING_NAME_LENGTH);
        }

        // set arguments
        strcpy(compnt.args, args);
        str2args(compnt.args, &compnt.argc, &compnt.argv[1], __CONF_ARGC);
        compnt.argc++;
        compnt.argv[0] = compnt.name;

        // set functions
        compnt.main = g_components[0].main;
        compnt.handler = g_components[0].handler;
        compnt.cleanup = g_components[0].cleanup;
        compnt.cli = g_components[0].cli;

        // set a type
        if (strlen(type) == 0) {
            compnt.type = COMPNT_GENERAL;
        } else {
            compnt.type = (strcmp(type, "autonomous") == 0) ? COMPNT_AUTO : COMPNT_GENERAL;
        }

        pass = TRUE;

        break;
    }

    json_decref(json);

    if (pass) {
        compnt.activated = FALSE;
    } else {
        PRINTF("No %s in the given configuration file\n", TARGET_COMPNT);
        return -1;
    }

    return 0;
}

/**
 * \brief Function to execute the main function of an external component
 * \return Component context
 */
static void *thread_main(void *null)
{
    if (compnt.main(&compnt.activated, compnt.argc, compnt.argv) < 0) {
        compnt.activated = FALSE;
        return NULL;
    } else {
        return &compnt;
    }
}

/**
 * \brief Function to activate an external component
 * \return None
 */
static int activate_compnt(void)
{
    // general type?
    if (compnt.type == COMPNT_GENERAL) {
        if (thread_main(NULL) == NULL) {
            return -1;
        }
    // autonomous type?
    } else {
        pthread_t thread;
        if (pthread_create(&thread, NULL, &thread_main, NULL)) {
            PERROR("pthread_create");
            return -1;
        }
    }

    return 0;
}

/**
 * \brief Function to deactivate an external component
 * \return None
 */
static int deactivate_compnt(void)
{
    if (compnt.cleanup(&compnt.activated) < 0) {
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
    PRINTF("Got a SIGINT signal\n");

    deactivate_compnt();
    destroy_ev_workers(NULL);
}

/** \brief The SIGKILL handler definition */
void (*sigkill_func)(int);

/** \brief Function to handle the SIGKILL signal */
void sigkill_handler(int sig)
{
    PRINTF("Got a SIGKILL signal\n");

    deactivate_compnt();
    destroy_ev_workers(NULL);
}

/** \brief The SIGTERM handler definition */
void (*sigterm_func)(int);

/** \brief Function to handle the SIGTERM signal */
void sigterm_handler(int sig)
{
    PRINTF("Got a SIGTERM signal\n");

    deactivate_compnt();
    destroy_ev_workers(NULL);
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to execute an external component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int main(int argc, char *argv[])
{
    // init component
    if (init_compnt()) {
        PRINTF("Failed to initialize %s\n", TARGET_COMPNT);
        return -1;
    } else {
        PRINTF("Initialized %s\n", TARGET_COMPNT);
    }

    // init event handler
    if (event_init(NULL)) {
        PRINTF("Failed to initialize the external event handler\n");
        return -1;
    } else {
        PRINTF("Initialized the external event handler\n");
    }

    // activate component
    if (activate_compnt()) {
        PRINTF("Failed to activate %s\n", TARGET_COMPNT);
        return -1;
    } else {
        PRINTF("Activated %s\n", TARGET_COMPNT);
    }

    // signal handlers
    sigint_func = signal(SIGINT, sigint_handler);
    sigkill_func = signal(SIGKILL, sigkill_handler);
    sigterm_func = signal(SIGTERM, sigterm_handler);

    waitsec(1, 0);

    while(compnt.activated) {
        waitsec(1, 0);
    }

    PRINTF("Terminated %s\n", TARGET_COMPNT);

    return 0;
}

/**
 * @}
 *
 * @}
 *
 * @}
 */
