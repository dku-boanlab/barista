/*
 * Copyright 2015-2018 NSSLab, KAIST
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

/** \brief The header of OpenFlow protocol */
struct of_header {
    uint8_t version;
    uint8_t type;
    uint16_t length;
    uint32_t xid;
};

/////////////////////////////////////////////////////////////////////

/** \brief The running flag for packet I/O engine */
int conn_on;

// socket queue handling part ///////////////////////////////////////

/** \brief The size of a network socket queue */
#define MAXQLEN 1024

/** \brief The pointer of the first entry in a network socket queue */
#define FIRST &queue[0]

/** \brief The pointer of the last entry in a network socket queue */
#define LAST  &queue[MAXQLEN - 1]

/** \brief Network socket queue */
int queue[MAXQLEN];

/** \brief The head and tail pointers of a network socket queue */
int *head, *tail;

/** \brief The number of entries in a network socket queue */
int size = 0;

/**
 * \brief Function to initialize a network socket queue
 * \return None
 */
static void init_queue(void)
{
    memset(queue, 0, sizeof(int) * MAXQLEN);
    head = tail = NULL;
    size = 0;
}

/**
 * \brief Function to push a socket into a network socket queue
 * \param v Network socket
 */
static void push_back(int v)
{
    if (size == 0) {
        head = tail = FIRST;
    } else {
        tail = (LAST - tail > 0) ? tail + 1 : FIRST;
    }

    *tail = v;

    size++;
}

/**
 * \brief Function to pop a socket from a network socket queue
 * \return Network socket
 */
static int pop_front(void)
{
    int ret = -1;

    if (size > 0) {
        ret = *head;
        head = (LAST - head > 0) ? head + 1 : FIRST;
        size--;
    }

    return ret;
}

/**
 * \brief Function to get the number of network sockets in a network socket queue
 * \return The number of network sockets in a network socket queue
 */
static int get_qsize(void)
{
    return size;
}

// segmented context handling part //////////////////////////////////

/** \brief The structure to keep the remaining part of a raw message */
typedef struct _context_t {
    int need; /**< The bytes it needs to read */
    int done; /**< The bytes it has */
    uint8_t temp[__MAX_RAW_DATA_LEN]; /**< Temporary data */
} context_t;

/** \brief Context buffers for all possible sockets */
context_t *context;

/**
 * \brief Function to initialize all context buffers
 * \return None
 */
static void init_context(void)
{
    context = (context_t *)CALLOC(__DEFAULT_TABLE_SIZE, sizeof(context_t));
    if (context == NULL) {
        PERROR("calloc");
        return;
    }
}

/**
 * \brief Function to clean up a context buffer
 * \param sock Network socket
 */
static void clean_context(int sock)
{
    if (context)
        memset(&context[sock], 0, sizeof(context_t));
}

// message handling part ////////////////////////////////////////////

/**
 * \brief Function to handle incoming messages from network sockets
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes)
{
    context_t *c = &context[sock];

    uint8_t *temp = c->temp;
    int need = c->need;
    int done = c->done;

    int buf_ptr = 0;
    while (bytes > 0) {
        if (0 < done && done < 4) {
            if (done + bytes < 4) {
                memmove(temp + done, rx_buf + buf_ptr, bytes);

                done += bytes;
                bytes = 0;

                break;
            } else {
                memmove(temp + done, rx_buf + buf_ptr, (4 - done));

                bytes -= (4 - done);
                buf_ptr += (4 - done);

                need = ntohs(((struct of_header *)temp)->length) - 4;
                done = 4;
            }
        }

        if (need == 0) {
            struct of_header *ofph = (struct of_header *)(rx_buf + buf_ptr);

            if (bytes < 4) {
                memmove(temp, rx_buf + buf_ptr, bytes);

                need = 0;
                done = bytes;

                bytes = 0;
            } else {
                uint16_t len = ntohs(ofph->length);

                if (bytes < len) {
                    memmove(temp, rx_buf + buf_ptr, bytes);

                    need = len - bytes;
                    done = bytes;

                    bytes = 0;
                } else {
                    raw_msg_t msg = {0};

                    msg.fd = sock;
                    msg.length = ntohs(ofph->length);

                    if (msg.length == 0)
                        return -1;

                    msg.data = (uint8_t *)(rx_buf + buf_ptr);

                    ev_ofp_msg_in(CONN_ID, &msg);

                    bytes -= len;
                    buf_ptr += len;

                    need = 0;
                    done = 0;
                }
            }
        } else {
            struct of_header *ofph = (struct of_header *)temp;

            if (need > bytes) {
                memmove(temp + done, rx_buf + buf_ptr, bytes);

                need -= bytes;
                done += bytes;

                bytes = 0;
            } else {
                raw_msg_t msg = {0};

                msg.fd = sock;
                msg.length = ntohs(ofph->length);

                if (msg.length == 0)
                    return -1;

                memmove(temp + done, rx_buf + buf_ptr, need);

                msg.data = (uint8_t *)temp;

                ev_ofp_msg_in(CONN_ID, &msg);

                bytes -= need;
                buf_ptr += need;

                need = 0;
                done = 0;
            }
        }
    }

    c->need = need;
    c->done = done;

    return bytes;
}

// epoll handling part //////////////////////////////////////////////

/** \brief The maximum number of events */
#define MAXEVENTS 1024

/** \brief Epoll for network sockets */
int epoll;

/** \brief The event list for network sockets */
struct epoll_event events[MAXEVENTS];

/**
 * \brief Function to create an epoll
 * \return New epoll
 */
static int init_epoll(void)
{
    memset(events, 0, sizeof(struct epoll_event) * MAXEVENTS);

    if ((epoll = epoll_create1(0)) < 0) {
        PERROR("epoll_create1");
        return -1;
    }

    return epoll;
}

/**
 * \brief Function to add a new socket into an epoll structure
 * \param epol Epoll
 * \param fd Socket
 * \param flags Epoll flags
 */
static int link_epoll(int epol, int fd, int flags)
{
    struct epoll_event event;

    event.data.fd = fd;
    event.events = flags;

    if (epoll_ctl(epol, EPOLL_CTL_ADD, fd, &event) < 0) {
        PERROR("epoll_ctl");
        return -1;
    }

    return 0;
}

/**
 * \brief Function to add a used socket into an epoll structure
 * \param epol Epoll
 * \param fd Socket
 * \param flags Epoll flags
 */
static int relink_epoll(int epol, int fd, int flags)
{
    struct epoll_event event;

    event.data.fd = fd;
    event.events = flags;

    epoll_ctl(epol, EPOLL_CTL_MOD, fd, &event);

    return 0;
}

// worker thread handling part //////////////////////////////////////

/** \brief The size of a recv buffer */
#define BUFFER_SIZE 8192

/** \brief The callback definition for network sockets */
typedef int (*recv_cb_t)(int sock, uint8_t *rx_buf, int bytes);

/** \brief The callback initialization for network sockets */
recv_cb_t recv_cb = NULL;

/** \brief Mutexlock for workers */
pthread_mutex_t queue_mutex;

/** \brief Condition for workers */
pthread_cond_t queue_cond;

/**
 * \brief Function to register a callback function for network sockets
 * \param cb The callback function for network sockets
 */
static void recv_cb_register(recv_cb_t cb)
{
    recv_cb = cb;
}

/**
 * \brief Function to receive raw messages from network sockets
 * \return NULL
 */
static void *do_tasks(void *null)
{
    int wsock, bytes, done = 0;
    uint8_t rx_buf[BUFFER_SIZE] = {0};

    pthread_mutex_lock(&queue_mutex);

    while (conn_on) {
        struct timespec time_wait;
        time_wait.tv_sec = 1;
        time_wait.tv_nsec = 0;

        //pthread_cond_wait(&queue_cond, &queue_mutex);
        pthread_cond_timedwait(&queue_cond, &queue_mutex, &time_wait);

FETCH_ONE_FD:
        if (conn_on == FALSE) { // clean up
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        wsock = pop_front();
        if (wsock < 0) continue;
        pthread_mutex_unlock(&queue_mutex);

        done = 0;
        bytes = 1;
        while (bytes > 0) {
            bytes = read(wsock, rx_buf, BUFFER_SIZE);
            if (bytes < 0) {
                if (errno != EAGAIN) {
                    done = 1;
                }
                break;
            } else if (bytes == 0) {
                done = 1;
                break;
            }

            bytes = recv_cb(wsock, rx_buf, bytes);
            if (bytes == -1) {
                done = 1;
                break;
            }
        }

        if (done) {
            // closed connection
            switch_t sw = {0};
            sw.fd = wsock;
            ev_sw_expired_conn(CONN_ID, &sw);

            clean_context(wsock);
            close(wsock);
        } else {
            if (relink_epoll(epoll, wsock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                // closed connection
                switch_t sw = {0};
                sw.fd = wsock;
                ev_sw_expired_conn(CONN_ID, &sw);

                clean_context(wsock);
                close(wsock);
            }
        }

        pthread_mutex_lock(&queue_mutex);

        if (get_qsize() > 0) {
            goto FETCH_ONE_FD;
        }
    }

    return NULL;
}

/**
 * \brief Function to initialize network socket workers
 * \return None
 */
static void init_workers(void)
{
    init_queue();

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);

    int i;
    pthread_t thread;
    for (i=0; i<__NUM_THREADS; i++) {
        if (pthread_create(&thread, NULL, &do_tasks, NULL) < 0) {
            PERROR("pthread_create");
        }
    }
}

// network socket handling part /////////////////////////////////////

/** \brief Network socket */
int sc;

/**
 * \brief Function to set the non-blocking mode to a socket
 * \param fd Socket
 */
static int nonblocking_mode(const int fd)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        if (errno != EAGAIN) {
            PERROR("fcntl");
            return -1;
        }
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        if (errno != EAGAIN) {
            PERROR("fcntl");
            return -1;
        }
    }

    return 0;
}

/**
 * \brief Function to initialize a socket
 * \param port Port number
 */
static int init_socket(uint16_t port)
{
    if ((sc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        PERROR("socket");
        return -1;
    }

    int option = 1;
    if (setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        PERROR("setsockopt");
    }

    if (nonblocking_mode(sc) < 0) {
        close(sc);
        return -1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(sc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        PERROR("bind");
        close(sc);
        return -1;
    }

    if (listen(sc, SOMAXCONN) < 0) {
        PERROR("listen");
        close(sc);
        return -1;
    }

    if (link_epoll(epoll, sc, EPOLLIN | EPOLLET) < 0) {
        close(sc);
        return -1;
    }

    return sc;
}

/**
 * \brief Function to receive connections from network sockets
 * \return NULL
 */
static void *socket_listen(void *arg)
{
    int csock;
    int nums = 0;
    struct sockaddr_in client;
    socklen_t len = sizeof(struct sockaddr);

    while (conn_on) {
        nums = epoll_wait(epoll, events, MAXEVENTS, 100);

        if (conn_on == FALSE) break;

        int i;
        for (i=0; i<nums; i++) {
            if (events[i].data.fd == sc) {
                while (1) {
                    if ((csock = accept(sc, (struct sockaddr *)&client, &len)) < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            PERROR("accept");
                        }
                        break;
                    }

                    // clean up the previous segments
                    clean_context(csock);

                    // new connection
                    switch_t sw = {0};
                    sw.fd = csock;
                    ev_sw_new_conn(CONN_ID, &sw);

                    if (nonblocking_mode(csock) < 0) {
                        // closed connection
                        switch_t sw = {0};
                        sw.fd = csock;
                        ev_sw_expired_conn(CONN_ID, &sw);

                        close(csock);
                        break;
                    }

                    if (link_epoll(epoll, csock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                        // closed connection
                        switch_t sw = {0};
                        sw.fd = csock;
                        ev_sw_expired_conn(CONN_ID, &sw);

                        close(csock);
                        break;
                    }
                }
            } else {
                pthread_mutex_lock(&queue_mutex);
                push_back(events[i].data.fd);
                pthread_mutex_unlock(&queue_mutex);
                pthread_cond_signal(&queue_cond);
            }
        }

        if (nums < 0 && errno != EINTR)
            break;
    }

    if (nums < 0)
        PERROR("epoll_wait");

    close(epoll);
    close(sc);

    LOG_INFO(CONN_ID, "Closed a socket");

    return NULL;
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

    conn_on = TRUE;

    init_context();
    recv_cb_register(msg_proc);

    init_epoll();
    init_workers();
    init_socket(__DEFAULT_PORT);

    signal(SIGPIPE, SIG_IGN);

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

    conn_on = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__NUM_THREADS; i++) {
        pthread_mutex_lock(&queue_mutex);
        push_back(0);
        pthread_mutex_unlock(&queue_mutex);
        pthread_cond_signal(&queue_cond);
    }

    pthread_cond_destroy(&queue_cond);
    pthread_mutex_destroy(&queue_mutex);

    FREE(context);

    signal(SIGPIPE, SIG_DFL);

    return 0;
}

/**
 * \brief The CLI function
 * \param cli The CLI pointer
 * \param args Arguments
 */
int conn_cli(cli_t *cli, char **args)
{
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
