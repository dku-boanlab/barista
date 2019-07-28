/*
 * Copyright 2015-2019 NSSLab, KAIST
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

/**
 * \brief Function to push a log message into a log queue
 * \param msg Log message
 */
static int push_msg_into_queue(char *msg)
{
    pthread_spin_lock(&log_lock);

    if (num_msgs >= __LOG_BATCH_SIZE-1) {
        FILE *fp = fopen(__DEFAULT_LOG_FILE, "a");
        if (fp != NULL) {
            int i;
            for (i=0; i<num_msgs; i++) {
                fputs(msgs[i], fp);
                fputs("\n", fp);

                char values[__CONF_STR_LEN];
                sprintf(values, "'%s'", msgs[i]);

                if (insert_data(&log_db, "logs", "MESSAGE", values)) {
                    LOG_ERROR(LOG_ID, "insert_data() failed");
                }

                memset(msgs[i], 0, __CONF_STR_LEN);
            }

            num_msgs = 0;

            fclose(fp);
        } else {
            pthread_spin_unlock(&log_lock);

            return -1;
        }
    }

    snprintf(msgs[num_msgs++], __CONF_STR_LEN-1, "%s", msg);

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

    FILE *fp = fopen(__DEFAULT_LOG_FILE, "a");
    if (fp != NULL) {
        int i;
        for (i=0; i<num_msgs; i++) {
            fputs(msgs[i], fp);
            fputs("\n", fp);

            char values[__CONF_STR_LEN];
            sprintf(values, "'%s'", msgs[i]);

            if (insert_data(&log_db, "logs", "MESSAGE", values)) {
                LOG_ERROR(LOG_ID, "insert_data() failed");
            }

            memset(msgs[i], 0, __CONF_STR_LEN);
        }

        fputs(msg, fp);

        num_msgs = 0;

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
    if (init_database(&log_db, "barista_mgmt")) {
        PRINTF("Failed to connect a log database\n");
        return -1;
    } else {
        DEBUG("Connected to a log database\n");
    }

    num_msgs = 0;
    msgs = (char **)MALLOC(sizeof(char *) * __LOG_BATCH_SIZE);
    if (msgs == NULL) {
        PERROR("malloc");
        return -1;
    }

    int i;
    for (i=0; i<__LOG_BATCH_SIZE; i++) {
        char *m = (char *)MALLOC(sizeof(char) * __CONF_STR_LEN);
        if (m == NULL) {
            PERROR("malloc");
            return -1;
        }

        msgs[i] = m;

        memset(msgs[i], 0, __CONF_STR_LEN);
    }

    pthread_spin_init(&log_lock, PTHREAD_PROCESS_PRIVATE);

    time_t start_time;
    start_time = time(NULL);
    struct tm *t = localtime(&start_time);

    char start_time_string[32];
    sprintf(start_time_string, "== %04d-%02d-%02d %02d:%02d:%02d ==\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    FILE *fp = fopen(__DEFAULT_LOG_FILE, "a");
    if (fp != NULL) {
        fputs(start_time_string, fp);
        fclose(fp);
    } else {
        PRINTF("%s does not exist\n", __DEFAULT_LOG_FILE);
        return -1;
    }

    activate();

    LOG_INFO(LOG_ID, "Init - Logging mechanism");

    while (*activated) {
        pthread_spin_lock(&log_lock);

        if (num_msgs) {
            FILE *fp = fopen(__DEFAULT_LOG_FILE, "a");
            if (fp != NULL) {
                for (i=0; i<num_msgs; i++) {
                    fputs(msgs[i], fp);
                    fputs("\n", fp);

                    char values[__CONF_STR_LEN];
                    sprintf(values, "'%s'", msgs[i]);

                    if (insert_data(&log_db, "logs", "MESSAGE", values)) {
                        LOG_ERROR(LOG_ID, "insert_data() failed");
                    }

                    memset(msgs[i], 0, __CONF_STR_LEN);
                }

                num_msgs = 0;

                fclose(fp);
            } else {
                pthread_spin_unlock(&log_lock);

                continue;
            }
        }

        pthread_spin_unlock(&log_lock);

        for (i=0; i<__LOG_UPDATE_TIME; i++) {
            if (*activated == FALSE) break;
            else waitsec(1, 0);
        }
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

    if (num_msgs) {
        FILE *fp = fopen(__DEFAULT_LOG_FILE, "a");
        if (fp != NULL) {
            int i;
            for (i=0; i<num_msgs; i++) {
                fputs(msgs[i], fp);
                fputs("\n", fp);

                char values[__CONF_STR_LEN];
                sprintf(values, "'%s'", msgs[i]);

                if (insert_data(&log_db, "logs", "MESSAGE", values)) {
                    LOG_ERROR(LOG_ID, "insert_data() failed");
                }
            }

            fclose(fp);
        }
    }

    pthread_spin_unlock(&log_lock);
    pthread_spin_destroy(&log_lock);

    int i;
    for (i=0; i<__LOG_BATCH_SIZE; i++) {
        FREE(msgs[i]);
    }

    FREE(msgs);

    if (destroy_database(&log_db)) {
        PRINTF("Failed to disconnect a log database\n");
        return -1;
    } else {
        DEBUG("Disconnected from a log database\n");
    }

    return 0;
}

/**
 * \brief Function to print the last n messages
 * \param cli The pointer of the Barista CLI
 * \param num_lines The number of lines
 */
static int log_print_messages(cli_t *cli, char *num_lines)
{
    int nlines = atoi(num_lines);

    char query[__CONF_STR_LEN];
    sprintf(query, "select MESSAGE from logs order by id desc limit %d", nlines);

    if (execute_query(&log_db, query)) {
        cli_print(cli, "Failed to read log messages");
        return -1;
    }

    query_result_t *result = get_query_result(&log_db);
    query_row_t row;

    while ((row = fetch_query_row(result)) != NULL) {
        cli_print(cli, "%s", row[0]);
    }

    release_query_result(result);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int log_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && args[1] == NULL) {
        log_print_messages(cli, args[0]);
        return 0;
    }

    cli_print(cli, "< Available Commands >");
    cli_print(cli, "  log [N line(s)]");

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
            snprintf(out, __CONF_STR_LEN-1, "%s <DEBUG> (%010u) %s", now, ev->id, ev->log);
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
            snprintf(out, __CONF_STR_LEN-1, "%s <INFO> (%010u) %s", now, ev->id, ev->log);
            PRINTF("%s <INFO> (%010u) %s\n", now, ev->id, ev->log);
            push_msg_into_queue(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    case EV_LOG_WARN:
        PRINT_EV("EV_LOG_WARN\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <WARN> (%010u) %s", now, ev->id, ev->log);
            PRINTF(ANSI_COLOR_MAGENTA "%s <WARN> (%010u) %s" ANSI_COLOR_RESET "\n", now, ev->id, ev->log);
            push_msgs_into_file(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    case EV_LOG_ERROR:
        PRINT_EV("EV_LOG_ERROR\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <ERROR> (%010u) %s", now, ev->id, ev->log);
            PRINTF(ANSI_COLOR_MAGENTA "%s <ERROR> (%010u) %s" ANSI_COLOR_RESET "\n", now, ev->id, ev->log);
            push_msgs_into_file(out);
            ev_log_update_msgs(LOG_ID, out);
        }
        break;
    case EV_LOG_FATAL:
        PRINT_EV("EV_LOG_FATAL\n");
        {
            snprintf(out, __CONF_STR_LEN-1, "%s <FATAL> (%010u) %s", now, ev->id, ev->log);
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
