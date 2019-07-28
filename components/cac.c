/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup cac Component Access Control
 * \brief (Security) component access control
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "cac.h"

/** \brief Component access control ID */
#define CAC_ID 1316219185

/////////////////////////////////////////////////////////////////////

/** \brief The event list to convert an event string to an event ID */
const char ev_string[EV_NUM_EVENTS+1][__CONF_WORD_LEN] = {
    #include "event_string.h"
};

/**
 * \brief Function to convert an event string to an event ID
 * \param event Event string
 * \return Event ID
 */
static int ev_type(const char *event)
{
    int i;
    for (i=0; i<EV_NUM_EVENTS+1; i++) {
        if (strcmp(event, ev_string[i]) == 0)
            return i;
    }
    return EV_NUM_EVENTS;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to load component configurations
 * \param conf_file Component configuration file
 */
static int config_load(char *conf_file)
{
    json_t *json;
    json_error_t error;

    LOG_INFO(CAC_ID, "Component configuration file: %s", conf_file);

    if (access(conf_file, F_OK)) {
        LOG_ERROR(CAC_ID, "No configuration file (%s)", conf_file);
        return -1;
    }

    char *raw = str_read(conf_file);
    char *conf = str_preproc(raw);

    json = json_loads(conf, 0, &error);
    if (!json) {
        LOG_ERROR(CAC_ID, "json_loads() failed");
        return -1;
    } else if (!json_is_array(json)) {
        LOG_ERROR(CAC_ID, "json_is_array() failed");
        json_decref(json);
        return -1;
    }

    int i, j, k;
    for (i=0; i<json_array_size(json); i++) {
        json_t *data = json_array_get(json, i);

        char name[__CONF_WORD_LEN];
        json_t *j_name = json_object_get(data, "name");
        if (json_is_string(j_name)) {
            strcpy(name, json_string_value(j_name));
        }

        // set a component name
        if (strlen(name) == 0) {
            LOG_ERROR(CAC_ID, "No component name");
            json_decref(json);
            return -1;
        } else {
            strcpy(compnt_to_event[compnt_to_event_cnt].name, name);
            compnt_to_event[compnt_to_event_cnt].id = 
                hash_func((uint32_t *)&compnt_to_event[compnt_to_event_cnt].name, __HASHING_NAME_LENGTH);
        }

        // set inbound events
        json_t *events = json_object_get(data, "inbounds");
        if (json_is_array(events)) {
            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                char in_name[__CONF_WORD_LEN] = {0};
                char in_perm[__CONF_SHORT_LEN] = {0};

                sscanf(json_string_value(event), "%[^,],%[^,]", in_name, in_perm);

                if (ev_type(in_name) == EV_NUM_EVENTS) {
                    LOG_ERROR(CAC_ID, "Wrong event name (%s)", in_name);
                    json_decref(json);
                    return -1;
                }
            }

            for (j=0; j<json_array_size(events); j++) {
                json_t *event = json_array_get(events, j);

                char in_name[__CONF_WORD_LEN] = {0};
                char in_perm[__CONF_SHORT_LEN] = {0};

                sscanf(json_string_value(event), "%[^,],%[^,]", in_name, in_perm);

                if (ev_type(in_name) == EV_NONE) {
                    break;
                } else if (ev_type(in_name) == EV_ALL_UPSTREAM) {
                    for (k=EV_NONE+1; k<EV_ALL_UPSTREAM; k++) {
                        compnt_to_event[compnt_to_event_cnt].in_list[compnt_to_event[compnt_to_event_cnt].in_num] = k;
                        compnt_to_event[compnt_to_event_cnt].in_num++;
                    }
                } else if (ev_type(in_name) == EV_ALL_DOWNSTREAM) {
                    for (k=EV_ALL_UPSTREAM+1; k<EV_ALL_DOWNSTREAM; k++) {
                        compnt_to_event[compnt_to_event_cnt].in_list[compnt_to_event[compnt_to_event_cnt].in_num] = k;
                        compnt_to_event[compnt_to_event_cnt].in_num++;
                    }
                } else if (ev_type(in_name) == EV_WRT_INTSTREAM) {
                    for (k=EV_ALL_DOWNSTREAM+1; k<EV_WRT_INTSTREAM; k++) {
                        compnt_to_event[compnt_to_event_cnt].in_list[compnt_to_event[compnt_to_event_cnt].in_num] = k;
                        compnt_to_event[compnt_to_event_cnt].in_num++;
                    }
                } else if (ev_type(in_name) == EV_ALL_INTSTREAM) {
                    for (k=EV_WRT_INTSTREAM+1; k<EV_ALL_INTSTREAM; k++) {
                        compnt_to_event[compnt_to_event_cnt].in_list[compnt_to_event[compnt_to_event_cnt].in_num] = k;
                        compnt_to_event[compnt_to_event_cnt].in_num++;
                    }
                } else {
                    compnt_to_event[compnt_to_event_cnt].in_list[compnt_to_event[compnt_to_event_cnt].in_num] = ev_type(in_name);
                    compnt_to_event[compnt_to_event_cnt].in_num++;
                }
            }
        }

        // set outbound events
        json_t *out_events = json_object_get(data, "outbounds");
        if (json_is_array(out_events)) {
            for (j=0; j<json_array_size(out_events); j++) {
                json_t *out_event = json_array_get(out_events, j);

                if (ev_type(json_string_value(out_event)) == EV_NUM_EVENTS) {
                    LOG_ERROR(CAC_ID, "Wrong event name (%s)", json_string_value(out_event));
                    json_decref(json);
                    return -1;
                }
            }

            for (j=0; j<json_array_size(out_events); j++) {
                json_t *out_event = json_array_get(out_events, j);

                compnt_to_event[compnt_to_event_cnt].out_list[compnt_to_event[compnt_to_event_cnt].out_num] = 
                    ev_type(json_string_value(out_event));
                compnt_to_event[compnt_to_event_cnt].out_num++;
            }
        }

        compnt_to_event_cnt++;
    }

    json_decref(json);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to verity the permission of a component
 * \param id Component ID
 * \param type Event type
 */
static int verify_component_event(uint32_t id, int type)
{
    int i;
    for (i=0; i<compnt_to_event_cnt; i++) {
        if (compnt_to_event[i].id == id) {
            int j;
            for (j=0; j<compnt_to_event[i].out_num; j++)
                if (compnt_to_event[i].out_list[j] == type) return 0;
            return 1;
        }
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int cac_main(int *activated, int argc, char **argv)
{
    LOG_INFO(CAC_ID, "Init - Component access control");

    compnt_to_event_cnt = 0;
    compnt_to_event = (compnt_to_event_t *)CALLOC(__MAX_COMPONENTS, sizeof(compnt_to_event_t));
    if (compnt_to_event == NULL) {
        LOG_ERROR(CAC_ID, "calloc() failed");
        return -1;
    }

    if (config_load(DEFAULT_COMPNT_CONFIG_FILE) < 0) {
        FREE(compnt_to_event);
        return -1;
    }

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int cac_cleanup(int *activated)
{
    LOG_INFO(CAC_ID, "Clean up - Component access control");

    deactivate();

    FREE(compnt_to_event);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int cac_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int cac_handler(const event_t *ev, event_out_t *ev_out)
{
    int res = verify_component_event(ev->id, ev->type);

    if (res < 0) {
        LOG_WARN(CAC_ID, "Wrong component ID (caller: %u, event: %s)", ev->id, ev_string[ev->type]);
        return -1;
    } else if (res > 0) {
        int i;
        for (i=0; i<compnt_to_event_cnt; i++) {
            if (compnt_to_event[i].id == ev->id) {
                LOG_WARN(CAC_ID, "Unexcepted access (caller: %s, event: %s)", compnt_to_event[i].name, ev_string[ev->type]);
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
