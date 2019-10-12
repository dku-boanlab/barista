/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup event_monitor Core Event Monitor
 * \brief (Admin) core event monitor
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "event_monitor.h"

/** \brief Core Event Monitor ID */
#define EV_MONITOR_ID 2889962257

/////////////////////////////////////////////////////////////////////

/* \brief The flag to enable API monitoring */
int API_monitor_enabled;

/* \brief The file descriptor for logging */
FILE *ev_fp;

/////////////////////////////////////////////////////////////////////

/** \brief The event list to convert an event string to an event ID */
const char core_event_string[__MAX_EVENTS][__CONF_WORD_LEN] = {
    #include "event_string.h"
};

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int event_monitor_main(int *activated, int argc, char **argv)
{
    LOG_INFO(EV_MONITOR_ID, "Init - Core event monitor");

    const char *API_monitor = getenv("API_monitor");
    if (API_monitor != NULL && strcmp(API_monitor, "API_monitor") == 0)
        API_monitor_enabled = TRUE;

    ev_fp = fopen("log/event.log", "a");
    if (ev_fp == NULL) {
        LOG_ERROR(EV_MONITOR_ID, "Failed to open log/event.log");
        return -1;
    }

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int event_monitor_cleanup(int *activated)
{
    LOG_INFO(EV_MONITOR_ID, "Clean up - Core event monitor");

    deactivate();

    if (ev_fp != NULL)
        fclose(ev_fp);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int event_monitor_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int event_monitor_handler(const event_t *ev, event_out_t *ev_out)
{
    if (API_monitor_enabled) {
        struct timespec curr_time, time_diff;
        clock_gettime(CLOCK_REALTIME, &curr_time);

        time_diff.tv_sec = curr_time.tv_sec - ev->time.tv_sec;
        time_diff.tv_nsec = curr_time.tv_nsec - ev->time.tv_nsec;

        char message[__CONF_STR_LEN];
        sprintf(message, "%u\t%u\t%s\t%lld.%.9ld\t%lld.%9ld\t%lld.%ld\n",
            ev->id, ev->type, core_event_string[ev->type],
            (long long)ev->time.tv_sec, ev->time.tv_nsec,
            (long long)curr_time.tv_sec, curr_time.tv_nsec,
            (long long)time_diff.tv_sec, time_diff.tv_nsec);

        fputs(message, ev_fp);
        fflush(ev_fp);
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */
