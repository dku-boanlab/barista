/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup compnt
 * @{
 * \defgroup conn Packet I/O Engine
 * \brief (Base) packet I/O engine
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 * \author Yeonkeun Kim <yeonk@kaist.ac.kr>
 */

#include "conn.h"

/** \brief Packet I/O engine ID */
#define CONN_ID 1651030346

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize all buffers
 * \return None
 */
static void init_buffers(void)
{
    buffer = (buffer_t *)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(buffer_t));
    if (buffer == NULL) {
        PERROR("calloc");
        return;
    }
}

/**
 * \brief Function to clean up a buffer
 * \param sock Network socket
 */
static void clean_buffer(int sock)
{
    if (buffer) {
        buffer[sock].need = 0;
        buffer[sock].done = 0;

        memset(buffer[sock].head, 0, 4);

        if (buffer[sock].data)
            FREE(buffer[sock].data);
    }
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle incoming messages from network sockets
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes)
{
    buffer_t *b = &buffer[sock];

    int need = b->need;
    int done = b->done;
    uint8_t *head = b->head;
    uint8_t *data = b->data;

    int buf_ptr = 0;
    while (bytes > 0) {
        if (0 < done && done < 4) {
            if (done + bytes < 4) { // keep a partial message which is not enough to get the actual length in head (2-1)
                memmove(head + done, rx_buf + buf_ptr, bytes);

                done += bytes;
                bytes = 0;

                break;
            } else { // now, we know the actual length of a message (2-2)
                memmove(head + done, rx_buf + buf_ptr, (4 - done));

                bytes -= (4 - done);
                buf_ptr += (4 - done);

                struct ofp_header *ofph = (struct ofp_header *)head;
                uint16_t len = ntohs(ofph->length);

                data = (uint8_t *)CALLOC(len+1, sizeof(uint8_t));
                memmove(data, head, 4);

                need = len - 4;
                done = 4;
            }
        }

        if (need == 0) { // either not enough to know the actual size or full packet
            if (bytes < 4) { // keep a partial message (less than 4 bytes) in head (1)
                memmove(head, rx_buf + buf_ptr, bytes);

                need = 0;
                done = bytes;

                bytes = 0;
            } else { // >= 4
                struct ofp_header *ofph = (struct ofp_header *)(rx_buf + buf_ptr);
                uint16_t len = ntohs(ofph->length);

                if (bytes < len) { // partial packet
                    data = (uint8_t *)CALLOC(len+1, sizeof(uint8_t));
                    memmove(data, rx_buf + buf_ptr, bytes);

                    need = len - bytes;
                    done = bytes;

                    bytes = 0;
                } else { // full packet
                    msg_t msg = {0};

                    msg.fd = sock;
                    msg.length = len;

                    if (msg.length == 0) return -1;

                    msg.data = (uint8_t *)CALLOC(len+1, sizeof(uint8_t));
                    memmove(msg.data, rx_buf + buf_ptr, len);

                    ev_ofp_msg_in(CONN_ID, &msg);

                    FREE(msg.data);

                    bytes -= len;
                    buf_ptr += len;

                    need = 0;
                    done = 0;
                }
            }
        } else { // append a subsequent message
            struct ofp_header *ofph = (struct ofp_header *)data;
            uint16_t len = ntohs(ofph->length);

            if (need > bytes) { // still not enough
                memmove(data + done, rx_buf + buf_ptr, bytes);

                need -= bytes;
                done += bytes;

                bytes = 0;
            } else {
                msg_t msg = {0};

                msg.fd = sock;
                msg.length = len; 

                if (msg.length == 0) return -1;

                msg.data = data;
                memmove(msg.data + done, rx_buf + buf_ptr, need);
                
                ev_ofp_msg_in(CONN_ID, &msg);

                FREE(msg.data);

                bytes -= need;
                buf_ptr += need;

                need = 0;
                done = 0;
            }
        }
    }

    b->need = need;
    b->done = done;

    return bytes;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to do something when a new device is connected
 * \return None
 */
static int new_connection(int sock)
{
    // new connection
    switch_t sw = {0};
    sw.conn.fd = sock;
    ev_sw_new_conn(CONN_ID, &sw);

    clean_buffer(sock);

    return 0;
}

/**
 * \brief Function to do something when a device is disconnected
 * \return None
 */
static int closed_connection(int sock)
{
    // closed connection
    switch_t sw = {0};
    sw.conn.fd = sock;
    ev_sw_expired_conn(CONN_ID, &sw);

    clean_buffer(sock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief The main function
 * \param activated The activation flag of this component
 * \param argc The number of arguments
 * \param argv Arguments
 */
int conn_main(int *activated, int argc, char **argv)
{
    LOG_INFO(CONN_ID, "Init - Packet I/O engine");

    init_buffers();

    create_epoll_env(INADDR_ANY, __DEFAULT_PORT);

    activate();

    socket_listen(NULL);

    return 0;
}

/**
 * \brief The cleanup function
 * \param activated The activation flag of this component
 */
int conn_cleanup(int *activated)
{
    LOG_INFO(CONN_ID, "Clean up - Packet I/O engine");

    deactivate();

    destroy_epoll_env();

    int sock;
    for (sock=0; sock<__DEFAULT_TABLE_SIZE; sock++) {
        clean_buffer(sock);
    }

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The pointer of the Barista CLI
 * \param args Arguments
 */
int conn_cli(cli_t *cli, char **args)
{
    cli_print(cli, "No CLI support");

    return 0;
}

/**
 * \brief The handler function
 * \param ev Read-only event
 * \param ev_out Read-write event (if this component has the write permission)
 */
int conn_handler(const event_t *ev, event_out_t *ev_out)
{
    switch (ev->type) {
    case EV_OFP_MSG_OUT:
        PRINT_EV("EV_OFP_MSG_OUT\n");
        {
            const msg_t *msg = ev->msg;
            int fd = msg->fd;

            if (fd == 0) break;

            int remain = msg->length;
            int bytes = 0;
            int done = 0;

            while (remain > 0) {
                bytes = write(fd, msg->data + done, remain);
                if (bytes < 0 && errno != EAGAIN) {
                    break;
                } else if (bytes < 0) {
                    continue;
                }

                remain -= bytes;
                done += bytes;
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
