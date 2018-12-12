/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup log Log Management
 * \brief (Base) log management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "log.h"

/** \brief Log ID */
#define LOG_ID 3234321643

/////////////////////////////////////////////////////////////////////

/** \brief The number of log messages */
int write_msg;

/** \brief Log queue */
char **msgs;

/** \brief The lock for log updates */
pthread_spinlock_t log_lock;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to push a log message into a log queue
 * \param msg Log message
 */
static int push_msg_into_queue(char *msg)
{
    pthread_spin_lock(&log_lock);

    if (write_msg >= BATCH_LOGS-1) {
        FILE *fp = fopen(DEFAULT_LOG_FILE, "a");
        if (fp != NULL) {
            int i;
            for (i=0; i<write_msg; i++) {
                fputs(msgs[i], fp);
                memset(msgs[i], 0, __CONF_STR_LEN);
            }

            write_msg = 0;

            fclose(fp);
        } else {
            pthread_spin_unlock(&log_lock);

            return -1;
        }
    }

    snprintf(msgs[write_msg++], __CONF_STR_LEN-1, "%s", msg);

    pthread_spin_unlock(&log_lock);

    return 0;
}

/**
 * \brief Function to directly push a message into a log file
 * \param msg Log message
 */
static int push_msgs_into_file(char *msg)
{
    pthread_spin_lock(&log_lock);

    FILE *fp = fopen(DEFAULT_LOG_FILE, "a");
    if (fp != NULL) {
        int i;
        for (i=0; i<write_msg; i++) {
            fputs(msgs[i], fp);
            memset(msgs[i], 0, __CONF_STR_LEN);
        }

        fputs(msg, fp);

        write_msg = 0;

        fclose(fp);
    }

    pthread_spin_unlock(&log_lock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int log_main(int *activated, int argc, char **argv)
{
    write_msg = 0;

    msgs = (char **)MALLOC(sizeof(char *) * BATCH_LOGS);
    if (msgs == NULL) {
        PERROR("malloc");
        return -1;
    }

    int k;
    for (k=0; k<BATCH_LOGS; k++) {
        char *m = (char *)MALLOC(sizeof(char) * __CONF_STR_LEN);
        if (m == NULL) {
            PERROR("malloc");
            return -1;
        }

        msgs[k] = m;

        memset(msgs[k], 0, __CONF_STR_LEN);
    }

    pthread_spin_init(&log_lock, PTHREAD_PROCESS_PRIVATE);

    activate();

    time_t start_time;
    start_time = time(NULL);
    struct tm *t = localtime(&start_time);

    char start_time_string[32];
    sprintf(start_time_string, "== %04d-%02d-%02d %02d:%02d:%02d ==\n",
                               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    push_msg_into_queue(start_time_string);

    LOG_INFO(LOG_ID, "Init - Logging mechanism");

    while (*activated) {
        pthread_spin_lock(&log_lock);

        if (write_msg) {
            FILE *fp = fopen(DEFAULT_LOG_FILE, "a");
            if (fp != NULL) {
                int i;
                for (i=0; i<write_msg; i++) {
                    fputs(msgs[i], fp);
                    memset(msgs[i], 0, __CONF_STR_LEN);
                }

                write_msg = 0;

                fclose(fp);
            } else {
                pthread_spin_unlock(&log_lock);

                continue;
            }
        }

        pthread_spin_unlock(&log_lock);

        waitsec(LOG_UPDATE_TIME, 0);
    }

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int log_cleanup(int *activated)
{
    LOG_INFO(LOG_ID, "Clean up - Logging mechanism");

    deactivate();

    pthread_spin_lock(&log_lock);

    if (write_msg) {
        FILE *fp = fopen(DEFAULT_LOG_FILE, "a");
        if (fp != NULL) {
            int i;
            for (i=0; i<write_msg; i++) {
                fputs(msgs[i], fp);
            }

            fclose(fp);
        }
    }

    pthread_spin_unlock(&log_lock);
    pthread_spin_destroy(&log_lock);

    int k;
    for (k=0; k<BATCH_LOGS; k++) {
        FREE(msgs[k]);
    }

    FREE(msgs);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int log_cli(cli_t *cli, char **args)
{
    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int log_handler(const event_t *ev, event_out_t *ev_out)
{
    time_t timer;
    struct tm *tm_info;
    char now[__CONF_WORD_LEN], out[__CONF_STR_LEN];

    time(&timer);
    tm_info = localtime(&timer);
    strftime(now, __CONF_WORD_LEN, "%Y:%m:%d %H:%M:%S", tm_info);

    switch (ev->type) {
    case EV_LOG_DEBUG:
        PRINT_EV("EV_LOG_DEBUG\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <DEBUG> (%010u) %s\n", now, ev->id, ev->log);
#ifdef __ENABLE_DEBUG
            PRINTF("%s <DEBUG> (%010u) %s\n", now, ev->id, ev->log);
#endif /* __ENABLE_DEBUG */
            push_msg_into_queue(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    case EV_LOG_INFO:
        PRINT_EV("EV_LOG_INFO\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <INFO> (%010u) %s\n", now, ev->id, ev->log);
            PRINTF("%s <INFO> (%010u) %s\n", now, ev->id, ev->log);
            push_msg_into_queue(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    case EV_LOG_WARN:
        PRINT_EV("EV_LOG_WARN\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <WARN> (%010u) %s\n", now, ev->id, ev->log);
            PRINTF(ANSI_COLOR_MAGENTA "%s <WARN> (%010u) %s" ANSI_COLOR_RESET "\n", now, ev->id, ev->log);
            push_msgs_into_file(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    case EV_LOG_ERROR:
        PRINT_EV("EV_LOG_ERROR\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <ERROR> (%010u) %s\n", now, ev->id, ev->log);
            PRINTF(ANSI_COLOR_MAGENTA "%s <ERROR> (%010u) %s" ANSI_COLOR_RESET "\n", now, ev->id, ev->log);
            push_msgs_into_file(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    case EV_LOG_FATAL:
        PRINT_EV("EV_LOG_FATAL\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <FATAL> (%010u) %s\n", now, ev->id, ev->log);
            PRINTF(ANSI_COLOR_RED "%s <FATAL> (%010u) %s" ANSI_COLOR_RESET "\n", now, ev->id, ev->log);
            push_msgs_into_file(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    default:
        break;
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */
