/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup host_mgmt Host Management
 * \brief (Management) host management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "host_mgmt.h"

/** \brief Host management ID */
#define HOST_MGMT_ID 367856965

/////////////////////////////////////////////////////////////////////

#include "host_queue.h"

/////////////////////////////////////////////////////////////////////

/** \brief Host tables */
host_table_t *host_table;

/** \brief The number of hosts */
int num_hosts;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to clean up host entries
 * \param idx The index of a host table
 * \param tmp_list The list of host entries to be removed
 * \param remote The origin of hosts in terms of management
 */
static int clean_up_tmp_list(int idx, host_table_t *tmp_list, int remote)
{
    // locked

    host_t *curr = tmp_list->head;

    while (curr != NULL) {
        host_t *tmp = curr;

        curr = curr->r_next;

        if (tmp->prev != NULL && tmp->next != NULL) {
            tmp->prev->next = tmp->next;
            tmp->next->prev = tmp->prev;
        } else if (tmp->prev == NULL && tmp->next != NULL) {
            host_table[idx].head = tmp->next;
            tmp->next->prev = NULL;
        } else if (tmp->prev != NULL && tmp->next == NULL) {
            host_table[idx].tail = tmp->prev;
            tmp->prev->next = NULL;
        } else if (tmp->prev == NULL && tmp->next == NULL) {
            host_table[idx].head = NULL;
            host_table[idx].tail = NULL;
        }

        host_t out = {0};

        out.dpid = tmp->dpid;
        out.port = tmp->port;

        out.mac = tmp->mac;
        out.ip = tmp->ip;

        if (remote == FALSE) {
            ev_host_deleted(HOST_MGMT_ID, &out);
        }

        uint8_t macaddr[6];
        int2mac(out.mac, macaddr);

        LOG_INFO(HOST_MGMT_ID, "Deleted a device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                 out.dpid, ip_addr_str(out.ip),
                 macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], out.port);

        host_enqueue(tmp);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to look up a host (locked)
 * \param list Host table mapped to a MAC address
 * \param ip IP address
 * \param mac MAC address
 * \return 0: new, 1: exist, 2: MAC conflict, 3: IP conflict
 */
static int check_host(host_table_t *list, uint32_t ip, uint64_t mac)
{
    // locked

    host_t *curr = NULL; 

    for (curr = list->head; curr != NULL; curr = curr->next) {
        if (curr->ip == ip && curr->mac == mac) {
            return 1;
        } else if (curr->ip == ip && curr->mac != mac) {
            uint8_t orig[ETH_ALEN], in[ETH_ALEN];

            int2mac(curr->mac, orig);
            int2mac(mac, in);

            LOG_WARN(HOST_MGMT_ID, "Wrong MAC address (IP: %s, original MAC: %02x:%02x:%02x:%02x:%02x:%02x, "
                                   "incoming MAC: %02x:%02x:%02x:%02x:%02x:%02x)",
                                   ip_addr_str(curr->ip), orig[0], orig[1], orig[2], orig[3], orig[4], orig[5],
                                   in[0], in[1], in[2], in[3], in[4], in[5]);

            return 2;
        } else if (curr->mac == mac && curr->ip != ip) {
            uint8_t m[ETH_ALEN];

            int2mac(curr->mac, m);

            if (strcmp(ip_addr_str(curr->ip), ip_addr_str(ip)) != 0) {
                LOG_WARN(HOST_MGMT_ID, "Wrong IP address (MAC: %02x:%02x:%02x:%02x:%02x:%02x, original IP: %s, incoming IP: %s)", 
                                       m[0], m[1], m[2], m[3], m[4], m[5], ip_addr_str(curr->ip), ip_addr_str(ip));
            } else {
                curr->ip = ip;
            }

            return 3;
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to add a new host
 * \param pktin PACKET_IN message
 */
static int add_new_host(const pktin_t *pktin)
{
    uint64_t mac = mac2int(pktin->src_mac);
    uint32_t mkey = hash_func((uint32_t *)&mac, 2) % __DEFAULT_TABLE_SIZE;

    pthread_rwlock_rdlock(&host_table[mkey].lock);
    int ret = check_host(&host_table[mkey], pktin->src_ip, mac);
    pthread_rwlock_unlock(&host_table[mkey].lock);

    if (ret == 0) {
        host_t *new = host_dequeue();
        if (new == NULL) return -1;

        new->dpid = pktin->dpid;
        new->port = pktin->port;

        new->ip = pktin->src_ip;
        new->mac = mac;

        pthread_rwlock_wrlock(&host_table[mkey].lock);

        if (host_table[mkey].head == NULL) {
            host_table[mkey].head = new;
            host_table[mkey].tail = new;
        } else {
            new->prev = host_table[mkey].tail;
            host_table[mkey].tail->next = new;
            host_table[mkey].tail = new;
        }

        num_hosts++;

        pthread_rwlock_unlock(&host_table[mkey].lock);

        ev_host_added(HOST_MGMT_ID, new);

        LOG_INFO(HOST_MGMT_ID, "Detected a new device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                 pktin->dpid, ip_addr_str(pktin->src_ip),
                 pktin->src_mac[0], pktin->src_mac[1], pktin->src_mac[2], pktin->src_mac[3], pktin->src_mac[4], pktin->src_mac[5],
                 pktin->port);
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int host_mgmt_main(int *activated, int argc, char **argv)
{
    LOG_INFO(HOST_MGMT_ID, "Init - Host management");

    num_hosts = 0;

    host_table = (host_table_t *)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(host_table_t));
    if (host_table == NULL) {
        LOG_ERROR(HOST_MGMT_ID, "calloc() error");
        return -1;
    }

    int i;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_init(&host_table[i].lock, NULL);
    }

    host_q_init();

    activate();

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int host_mgmt_cleanup(int *activated)
{
    LOG_INFO(HOST_MGMT_ID, "Clean up - Host management");

    deactivate();

    int i;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_wrlock(&host_table[i].lock);

        host_t *curr = host_table[i].head;
        while (curr != NULL) {
            host_t *tmp = curr;
            curr = curr->next;
            FREE(tmp);
        }

        pthread_rwlock_unlock(&host_table[i].lock);
        pthread_rwlock_destroy(&host_table[i].lock);
    }

    host_q_destroy();

    FREE(host_table);

    return 0;
}

/**
 * \brief Function to print all hosts
 * \param cli The pointer of the Barista CLI
 */
static int host_listup(cli_t *cli)
{
    cli_print(cli, "<Host List>");
    cli_print(cli, "  The total number of hosts: %d", num_hosts);

    int i, cnt = 0;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_rdlock(&host_table[i].lock);

        host_t *curr = NULL;
        for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
            uint8_t macaddr[ETH_ALEN];
            int2mac(curr->mac, macaddr);

            cli_print(cli, "  Host #%d\n    DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u", 
                      ++cnt, curr->dpid, ip_addr_str(curr->ip),
                      macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], curr->port);
        }

        pthread_rwlock_unlock(&host_table[i].lock);
    }

    return 0;
}

/**
 * \brief Function to find hosts in the switch corresponding to a datapath ID
 * \param cli The pointer of the Barista CLI
 * \param dpid_str Datapath ID
 */
static int host_showup_switch(cli_t *cli, const char *dpid_str)
{
    uint64_t dpid = strtoull(dpid_str, NULL, 0);

    cli_print(cli, "<Host List [%lu]>", dpid);

    int i, cnt = 0;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_rdlock(&host_table[i].lock);

        host_t *curr = NULL;
        for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
            if (curr->dpid == dpid) {
                uint8_t mac[ETH_ALEN];
                int2mac(curr->mac, mac);

                cli_print(cli, "  Host #%d\n    DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                          ++cnt, curr->dpid, ip_addr_str(curr->ip),
                          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], curr->port);
            }
        }

        pthread_rwlock_unlock(&host_table[i].lock);
    }

    if (!cnt)
        cli_print(cli, "  No connected host");

    return 0;
}

/**
 * \brief Function to find a host that has a specific IP address
 * \param cli The pointer of the Barista CLI
 * \param ipaddr IP address
 */
static int host_showup_ip(cli_t *cli, const char *ipaddr)
{
    int i, cnt = 0;

    cli_print(cli, "<Host [%s]>", ipaddr);

    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_rdlock(&host_table[i].lock);

        host_t *curr = NULL;
        for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
            if (curr->ip == ip_addr_int(ipaddr)) {
                uint8_t macaddr[ETH_ALEN];
                int2mac(curr->mac, macaddr);

                cli_print(cli, "  Host #%d\n    DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u",
                          ++cnt, curr->dpid, ipaddr,
                          macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], curr->port);
            }
        }

        pthread_rwlock_unlock(&host_table[i].lock);
    }

    if (!cnt)
        cli_print(cli, "  No connected host");

    return 0;
}

/**
 * \brief Function to find a host that has a specific MAC address
 * \param cli The pointer of the Barista CLI
 * \param macaddr MAC address
 */
static int host_showup_mac(cli_t *cli, const char *macaddr)
{
    uint8_t mac[ETH_ALEN];
    str2mac(macaddr, mac);

    uint64_t macval = mac2int(mac);

    cli_print(cli, "<Host [%s]>", macaddr);

    int i, cnt = 0;
    for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
        pthread_rwlock_rdlock(&host_table[i].lock);

        host_t *curr = NULL;
        for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
            if (curr->mac == macval) {
                cli_print(cli, "  Host #%d\n    DPID: %lu, IP: %s, Mac: %s, Port: %u",
                          ++cnt, curr->dpid, ip_addr_str(curr->ip), macaddr, curr->port);
            }
        }

        pthread_rwlock_unlock(&host_table[i].lock);
    }

    if (!cnt)
        cli_print(cli, "  No connected host");

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int host_mgmt_cli(cli_t *cli, char **args)
{
    if (args[0] != NULL && strcmp(args[0], "list") == 0 && args[1] != NULL && strcmp(args[1], "hosts") == 0 && args[2] == NULL) {
        host_listup(cli);
        return 0;
    } else if (args[0] != NULL && strcmp(args[0], "show") == 0) {
        if (args[1] != NULL && strcmp(args[1], "switch") == 0 && args[2] != NULL && args[3] == NULL) {
            host_showup_switch(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "ip") == 0 && args[2] != NULL && args[3] == NULL) {
            host_showup_ip(cli, args[2]);
            return 0;
        } else if (args[1] != NULL && strcmp(args[1], "mac") == 0 && args[2] != NULL && args[3] == NULL) {
            host_showup_mac(cli, args[2]);
            return 0;
        }
    }

    cli_print(cli, "<Available Commands>");
    cli_print(cli, "  host_mgmt list hosts");
    cli_print(cli, "  host_mgmt show switch [DPID]");
    cli_print(cli, "  host_mgmt show ip [IP address]");
    cli_print(cli, "  host_mgmt show mac [MAC address]");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int host_mgmt_handler(const event_t *ev, event_out_t *ev_out)
{
#ifdef __ENABLE_CBENCH
    return 0;
#endif /* __ENABLE_CBENCH */

    switch (ev->type) {
    case EV_DP_RECEIVE_PACKET:
        PRINT_EV("EV_DP_RECEIVE_PACKET\n");
        {
            const pktin_t *pktin = ev->pktin;

            add_new_host(pktin);
        }
        break;
    case EV_DP_PORT_ADDED:
        PRINT_EV("EV_DP_PORT_ADDED\n");
        {
            const port_t *port = ev->port;

            int i;
            for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
                host_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&host_table[i].lock);

                host_t *curr = NULL;
                for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
                    if (curr->dpid == port->dpid && curr->port == port->port) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }

                        num_hosts--;
                    }
                }

                clean_up_tmp_list(i, &tmp_list, FALSE);

                pthread_rwlock_unlock(&host_table[i].lock);
            }
        }
        break;
    case EV_DP_PORT_DELETED:
        PRINT_EV("EV_DP_PORT_DELETED\n");
        {
            const port_t *port = ev->port;

            int i;
            for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
                host_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&host_table[i].lock);

                host_t *curr = NULL;
                for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
                    if (curr->dpid == port->dpid && curr->port == port->port) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }

                        num_hosts--;
                    }
                }

                clean_up_tmp_list(i, &tmp_list, FALSE);

                pthread_rwlock_unlock(&host_table[i].lock);
            }
        }
        break;
    case EV_SW_CONNECTED:
        PRINT_EV("EV_SW_CONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            int i;
            for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
                host_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&host_table[i].lock);

                host_t *curr = NULL;
                for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
                    if (curr->dpid == sw->dpid) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }

                        num_hosts--;
                    }
                }

                clean_up_tmp_list(i, &tmp_list, sw->remote);

                pthread_rwlock_unlock(&host_table[i].lock);
            }
        }
        break;
    case EV_SW_DISCONNECTED:
        PRINT_EV("EV_SW_DISCONNECTED\n");
        {
            const switch_t *sw = ev->sw;

            int i;
            for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
                host_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&host_table[i].lock);

                host_t *curr = NULL;
                for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
                    if (curr->dpid == sw->dpid) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }

                        num_hosts--;
                    }
                }

                clean_up_tmp_list(i, &tmp_list, sw->remote);

                pthread_rwlock_unlock(&host_table[i].lock);
            }
        }
        break;
    case EV_HOST_ADDED:
        PRINT_EV("EV_HOST_ADDED\n");
        {
            const host_t *host = ev->host;

            if (host->remote == FALSE) break;

            uint32_t mkey = hash_func((uint32_t *)&host->mac, 2) % __DEFAULT_TABLE_SIZE;

            pthread_rwlock_rdlock(&host_table[mkey].lock);
            int res = check_host(&host_table[mkey], host->ip, host->mac);
            pthread_rwlock_unlock(&host_table[mkey].lock);

            if (res == 0) {
                host_t *new = host_dequeue();
                if (new == NULL) break;

                new->dpid = host->dpid;
                new->port = host->port;

                new->remote = host->remote;

                new->ip = host->ip;
                new->mac = host->mac;

                new->prev = new->next = NULL;

                pthread_rwlock_wrlock(&host_table[mkey].lock);

                if (host_table[mkey].head == NULL) {
                    host_table[mkey].head = new;
                    host_table[mkey].tail = new;
                } else {
                    new->prev = host_table[mkey].tail;
                    host_table[mkey].tail->next = new;
                    host_table[mkey].tail = new;
                }

                num_hosts++;

                pthread_rwlock_unlock(&host_table[mkey].lock);

                uint8_t macaddr[ETH_ALEN];
                int2mac(new->mac, macaddr);

                LOG_INFO(HOST_MGMT_ID, "Detected a new device (DPID: %lu, IP: %s, Mac: %02x:%02x:%02x:%02x:%02x:%02x, Port: %u)",
                         new->dpid, ip_addr_str(new->ip),
                         macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], new->port);
            }
        }
        break;
    case EV_HOST_DELETED:
        PRINT_EV("EV_HOST_DELETED\n");
        {
            const host_t *host = ev->host;

            if (host->remote == FALSE) break;

            int i;
            for (i=0; i<__DEFAULT_TABLE_SIZE; i++) {
                host_table_t tmp_list = {0};

                pthread_rwlock_wrlock(&host_table[i].lock);

                host_t *curr = NULL;
                for (curr = host_table[i].head; curr != NULL; curr = curr->next) {
                    if (curr->dpid == host->dpid && curr->port == host->port 
                        && curr->ip == host->ip && curr->mac == host->mac) {
                        if (tmp_list.head == NULL) {
                            tmp_list.head = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        } else {
                            tmp_list.tail->r_next = curr;
                            tmp_list.tail = curr;
                            curr->r_next = NULL;
                        }

                        num_hosts--;
                    }
                }

                clean_up_tmp_list(i, &tmp_list, TRUE);

                pthread_rwlock_unlock(&host_table[i].lock);
            }
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
