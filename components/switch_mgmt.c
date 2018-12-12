/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup switch_mgmt Switch Management
 * \brief (Management) switch management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "switch_mgmt.h"

/** \brief Switch management ID */
#define SWITCH_MGMT_ID 893048714

/////////////////////////////////////////////////////////////////////

#include "switch_queue.h"

/////////////////////////////////////////////////////////////////////

/** \brief The number of switches */
int num_switches;

/** \brief Switch table */
switch_table_t switches;

/** \brief Switch list */
switch_t **switch_table;

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int switch_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(SWITCH_MGMT_ID, "Init - Switch management");

    num_switches = 0;
    memset(&switches, 0, sizeof(switch_table_t));

    switch_table = (switch_t **)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(switch_t *));
    if (switch_table == NULL) {
        PERROR("calloc");
        return -1;
    }

    pthread_rwlock_init(&switches.lock, NULL);

    sw_q_init();

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int switch_mgmt_cleanup(int *activated)
{
    LOG_INFO(SWITCH_MGMT_ID, "Clean up - Switch management");

    deactivate();

    pthread_rwlock_wrlock(&switches.lock);

    switch_t *curr = switches.head;
    while (curr != NULL) {
        switch_t *tmp = curr;
        curr = curr->next;
        switch_table[tmp->fd] = NULL;
        FREE(tmp);
    }

    pthread_rwlock_unlock(&switches.lock);
    pthread_rwlock_destroy(&switches.lock);

    FREE(switch_table);

    sw_q_destroy();

    return 0;
}

/**
 * \brief Function to print all switches
 * \param cli The pointer of the Barista CLI
 */
static int switch_listup(cli_t *cli)
{
    int cnt = 0;

    cli_print(cli, "<Switch List>");
    cli_print(cli, "  The total number of switches: %d", num_switches);

    pthread_rwlock_rdlock(&switches.lock);

    switch_t *curr = switches.head;

    while (curr != NULL) {
        cli_print(cli, "  Switch #%d", ++cnt);
        cli_print(cli, "    Datapath ID: %lu (0x%lx)", curr->dpid, curr->dpid);
        cli_print(cli, "    Location: %s", (curr->remote) ? "remote" : "local");
        cli_print(cli, "    Number of tables: %u", curr->n_tables);
        cli_print(cli, "    Number of buffers: %u", curr->n_buffers);
        cli_print(cli, "    Capabilities: 0x%x", curr->capabilities);
        cli_print(cli, "    Actions: 0x%x", curr->actions);
        cli_print(cli, "    Manufacturer: %s", curr->mfr_desc);
        cli_print(cli, "    Hardware: %s", curr->hw_desc);
        cli_print(cli, "    Software: %s", curr->sw_desc);
        cli_print(cli, "    Serial number: %s", curr->serial_num);
        cli_print(cli, "    Datapath: %s", curr->dp_desc);
        if (curr->pkt_count) {
            cli_print(cli, "    # of packets: %lu", curr->pkt_count);
            cli_print(cli, "    # of bytes: %lu", curr->byte_count);
            cli_print(cli, "    # of flows: %u", curr->flow_count);
        } else {
            cli_print(cli, "    # of packets: 0");
            cli_print(cli, "    # of bytes: 0");
            cli_print(cli, "    # of flows: 0");
        }

        curr = curr->next;
    }

    pthread_rwlock_unlock(&switches.lock);

    return 0;
}

/**
 * \brief Function to print a switch
 * \param cli The pointer of the Barista CLI
 * \param dpid_str Datapath ID
 */
static int switch_showup(cli_t *cli, char *dpid_str)
{
    int cnt = 0;
    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "<Switch Information>");

    pthread_rwlock_rdlock(&switches.lock);

    switch_t *curr = switches.head;

    while (curr != NULL) {
        if (curr->dpid == dpid) {
            cli_print(cli, "    Datapath ID: %lu (0x%lx)", curr->dpid, curr->dpid);
            cli_print(cli, "    Location: %s", (curr->remote) ? "remote" : "local");
            cli_print(cli, "    Number of tables: %u", curr->n_tables);
            cli_print(cli, "    Number of buffers: %u", curr->n_buffers);
            cli_print(cli, "    Capabilities: 0x%x", curr->capabilities);
            cli_print(cli, "    Actions: 0x%x", curr->actions);
            cli_print(cli, "    Manufacturer: %s", curr->mfr_desc);
            cli_print(cli, "    Hardware: %s", curr->hw_desc);
            cli_print(cli, "    Software: %s", curr->sw_desc);
            cli_print(cli, "    Serial number: %s", curr->serial_num);
            cli_print(cli, "    Datapath: %s", curr->dp_desc);
            if (curr->pkt_count) {
                cli_print(cli, "    # of packets: %lu", curr->pkt_count);
                cli_print(cli, "    # of bytes: %lu", curr->byte_count);
                cli_print(cli, "    # of flows: %u", curr->flow_count);
            } else {
                cli_print(cli, "    # of packets: 0");
                cli_print(cli, "    # of bytes: 0");
                cli_print(cli, "    # of flows: 0");
            }

            cnt++;

            break;
        }

        curr = curr->next;
    }

    pthread_rwlock_unlock(&switches.lock);

    if (!cnt)
        cli_print(cli, "  No switch whose datapath ID is %lu", dpid);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int switch_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0) {
        if (args[1] != NULL && strcmp(args[1], "switches") == 0 && args[2] == NULL) {
            switch_listup(cli);
            return 0;
        }
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0) {
        if (args[1] != NULL && args[2] == NULL) {
            switch_showup(cli, args[1]);
            return 0;
        }
    }

    cli_print(cli, "<Available Commands>");
    cli_print(cli, "  switch_mgmt list switches");
    cli_print(cli, "  switch_mgmt show [datapath ID]");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int switch_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_SW_NEW_CONN:
        PRINT_EV("EV_SW_NEW_CONN\n");
        {
            LOG_DEBUG(SWITCH_MGMT_ID, "Accepted (FD=%d)", ev->sw->fd);
        }
        break;
    case EV_SW_EXPIRED_CONN:
        PRINT_EV("EV_SW_EXPIRED_CONN\n");
        {
            const switch_t *sw = ev->sw;

            pthread_rwlock_wrlock(&switches.lock);

            if (switch_table[sw->fd] == NULL) {
                pthread_rwlock_unlock(&switches.lock);
                break;
            }

            switch_t *curr = switch_table[sw->fd];
            switch_t *tmp = curr;

            if (curr->prev != NULL && curr->next != NULL) {
                curr->prev->next = curr->next;
                curr->next->prev = curr->prev;
            } else if (curr->prev == NULL && curr->next != NULL) {
                switches.head = curr->next;
                curr->next->prev = NULL;
            } else if (curr->prev != NULL && curr->next == NULL) {
                switches.tail = curr->prev;
                curr->prev->next = NULL;
            } else if (curr->prev == NULL && curr->next == NULL) {
                switches.head = NULL;
                switches.tail = NULL;
            }

            switch_table[sw->fd] = NULL;

            pthread_rwlock_unlock(&switches.lock);

            if (tmp->dpid == 0) {
                LOG_DEBUG(SWITCH_MGMT_ID, "Closed (FD=%d)", sw->fd);
            } else {
                num_switches--;

                switch_t out = {0};

                out.dpid = tmp->dpid;
                out.fd = sw->fd;

                ev_sw_disconnected(SWITCH_MGMT_ID, &out);

                LOG_INFO(SWITCH_MGMT_ID, "Disconnected (FD=%d, DPID=%lu)", sw->fd, tmp->dpid);

                sw_enqueue(tmp);
            }
        }
        break;
    case EV_SW_UPDATE_CONFIG:
        PRINT_EV("EV_SW_UPDATE_CONFIG\n");
        {
            const switch_t *sw = ev->sw;

            pthread_rwlock_rdlock(&switches.lock);

            int i, pass = FALSE;
            for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
                if (switch_table[i] == NULL)
                    continue;

                if (switch_table[i]->dpid == sw->dpid) {
                    pass = TRUE;
                    break;
                }
            }

            pthread_rwlock_unlock(&switches.lock);

            if (pass == FALSE) {
                switch_t *new = sw_dequeue();
                if (new == NULL) return -1;

                new->dpid = sw->dpid;
                new->fd = sw->fd;

                new->xid = 0;
                new->remote = sw->remote;

                new->n_tables = sw->n_tables;
                new->n_buffers = sw->n_buffers;
                new->capabilities = sw->capabilities;
                new->actions = sw->actions;

                pthread_rwlock_wrlock(&switches.lock);

                if (switches.head == NULL) {
                    switches.head = new;
                    switches.tail = new;
                } else {
                    new->prev = switches.tail;
                    switches.tail->next = new;
                    switches.tail = new;
                }

                switch_table[sw->fd] = new;
                num_switches++;

                pthread_rwlock_unlock(&switches.lock);

                switch_t out = {0};

                out.dpid = sw->dpid;
                out.fd = sw->fd;

                out.remote = sw->remote;

                ev_sw_connected(SWITCH_MGMT_ID, &out);

                LOG_INFO(SWITCH_MGMT_ID, "Connected (FD=%d, DPID=%lu)", sw->fd, sw->dpid);
            }
        }
        break;
    case EV_SW_UPDATE_DESC:
        PRINT_EV("EV_SW_UPDATE_DESC\n");
        {
            const switch_t *sw = ev->sw;

            pthread_rwlock_wrlock(&switches.lock);

            if (switch_table[sw->fd] != NULL) {
                strncpy(switch_table[sw->fd]->mfr_desc, sw->mfr_desc, 256);
                strncpy(switch_table[sw->fd]->hw_desc, sw->hw_desc, 256);
                strncpy(switch_table[sw->fd]->sw_desc, sw->sw_desc, 256);
                strncpy(switch_table[sw->fd]->serial_num, sw->serial_num, 32);
                strncpy(switch_table[sw->fd]->dp_desc, sw->dp_desc, 256);
            }

            pthread_rwlock_unlock(&switches.lock);
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            if (sw->remote == FALSE) break;

            pthread_rwlock_wrlock(&switches.lock);

            if (switch_table[sw->fd] == NULL) {
                pthread_rwlock_unlock(&switches.lock);
                break;
            }

            switch_t *curr = switch_table[sw->fd];
            switch_t *tmp = curr;

            if (curr->prev != NULL && curr->next != NULL) {
                curr->prev->next = curr->next;
                curr->next->prev = curr->prev;
            } else if (curr->prev == NULL && curr->next != NULL) {
                switches.head = curr->next;
                curr->next->prev = NULL;
            } else if (curr->prev != NULL && curr->next == NULL) {
                switches.tail = curr->prev;
                curr->prev->next = NULL;
            } else if (curr->prev == NULL && curr->next == NULL) {
                switches.head = NULL;
                switches.tail = NULL;
            }

            switch_table[sw->fd] = NULL;
            num_switches--;

            pthread_rwlock_unlock(&switches.lock);

            LOG_INFO(SWITCH_MGMT_ID, "Disconnected (FD=%d, DPID=%lu)", sw->fd, tmp->dpid);

            sw_enqueue(tmp);
        }
        break;
    case EV_DP_AGGREGATE_STATS:
        PRINT_EV("EV_DP_AGGREGATE_STATS\n");
        {
            const flow_t *flow = ev->flow;

            pthread_rwlock_wrlock(&switches.lock);

            switch_t *curr = switches.head;
            while (curr != NULL) {
                if (curr->dpid == flow->dpid) {
                    curr->pkt_count = flow->pkt_count - curr->old_pkt_count;
                    curr->byte_count = flow->byte_count - curr->old_byte_count;
                    curr->flow_count = flow->flow_count - curr->old_flow_count;

                    curr->old_pkt_count = flow->pkt_count;
                    curr->old_byte_count = flow->byte_count;
                    curr->old_flow_count = flow->flow_count;

                    break;
                }
                curr = curr->next;
            }

            pthread_rwlock_unlock(&switches.lock);
        }
        break;
    case EV_SW_GET_DPID:
        PRINT_EV("EV_SW_GET_DPID\n");
        {
            switch_t *sw = ev_out->sw_data;

            pthread_rwlock_rdlock(&switches.lock);

            if (switch_table[sw->fd] != NULL) {
                sw->dpid = switch_table[sw->fd]->dpid;
            } else {
                switch_t *curr = switches.head;
                while (curr != NULL) {
                    if (curr->fd == sw->fd) {
                        sw->dpid = curr->dpid;
                        break;
                    }
                    curr = curr->next;
                }
            }

            pthread_rwlock_unlock(&switches.lock);
        }
        break;
    case EV_SW_GET_FD:
        PRINT_EV("EV_SW_GET_FD\n");
        {
            switch_t *sw = ev_out->sw_data;

            pthread_rwlock_rdlock(&switches.lock);

            switch_t *curr = switches.head;
            while (curr != NULL) {
                if (curr->dpid == sw->dpid) {
                    sw->fd = curr->fd;
                    break;
                }
                curr = curr->next;
            }

            pthread_rwlock_unlock(&switches.lock);
        }
        break;
    case EV_SW_GET_XID:
        PRINT_EV("EV_SW_GET_XID\n");
        {
            switch_t *sw = ev_out->sw_data;

            pthread_rwlock_rdlock(&switches.lock);

            if (sw->fd) {
                sw->xid = switch_table[sw->fd]->xid++;
            } else {
                switch_t *curr = switches.head;
                while (curr != NULL) {
                    if (curr->dpid == sw->dpid) {
                        sw->xid = curr->xid++;
                        break;
                    }
                    curr = curr->next;
                }
            }

            pthread_rwlock_unlock(&switches.lock);
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
