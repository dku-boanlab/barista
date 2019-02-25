/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/socket.h>

/////////////////////////////////////////////////////////////////////

/** \brief The running flag for listening connections */
int ep_listening;

/////////////////////////////////////////////////////////////////////

/** \brief The size of a network socket queue */
#define EP_MAXQLEN 1024

/** \brief The pointer of the first entry in a network socket queue */
#define EP_FIRST &ep_queue[0]

/** \brief The pointer of the last entry in a network socket queue */
#define EP_LAST &ep_queue[EP_MAXQLEN - 1]

/** \brief Network socket queue */
int ep_queue[EP_MAXQLEN];

/** \brief The head pointer of a network socket queue */
int *ep_head;

/** \brief The tail pointer of a network socket queue */
int *ep_tail;

/** \brief The number of entries in a network socket queue */
int ep_size;

/**
 * \brief Function to initialize a network socket queue
 * \return None
 */
static void init_queue(void)
{
    memset(ep_queue, 0, sizeof(int) * EP_MAXQLEN);
    ep_head = ep_tail = NULL;
    ep_size = 0;
}

/**
 * \brief Function to push a socket into a network socket queue
 * \param v Network socket
 */
static void push_back(int v)
{
    if (ep_size == 0) {
        ep_head = ep_tail = EP_FIRST;
    } else {
        ep_tail = (EP_LAST - ep_tail > 0) ? ep_tail + 1 : EP_FIRST;
    }

    *ep_tail = v;

    ep_size++;
}

/**
 * \brief Function to pop a socket from a network socket queue
 * \return Network socket
 */
static int pop_front(void)
{
    int ret = -1;

    if (ep_size > 0) {
        ret = *ep_head;
        ep_head = (EP_LAST - ep_head > 0) ? ep_head + 1 : EP_FIRST;
        ep_size--;
    }

    return ret;
}

/**
 * \brief Function to get the number of network sockets in a network socket queue
 * \return The number of network sockets in a network socket queue
 */
static int get_qsize(void)
{
    return ep_size;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle incoming messages from external components
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes);

/**
 * \brief Function to do something when a new component is connected
 * \return None
 */
static int new_connection(int sock);

/**
 * \brief Function to do something when a component is disconnected
 * \return None
 */
static int closed_connection(int sock);

/////////////////////////////////////////////////////////////////////

/** \brief The maximum number of epoll events */
#define EP_MAXEVENTS 1024

/** \brief Epoll for network sockets */
int ep_epoll;

/** \brief The event list for network sockets */
struct epoll_event ep_events[EP_MAXEVENTS];

/**
 * \brief Function to create an epoll
 * \return New epoll
 */
static int init_epoll(void)
{
    memset(ep_events, 0, sizeof(struct epoll_event) * EP_MAXEVENTS);

    if ((ep_epoll = epoll_create1(0)) < 0) {
        PERROR("epoll_create1");
        return -1;
    }

    return ep_epoll;
}

/**
 * \brief Function to add a new socket into an epoll
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
 * \brief Function to add a used socket into an epoll
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

/////////////////////////////////////////////////////////////////////

/** \brief The size of a recv buffer */
#define BUFFER_SIZE 8192

/** \brief The callback definition for network sockets */
typedef int (*ep_recv_cb_t)(int sock, uint8_t *rx_buf, int bytes);

/** \brief The callback initialization for network sockets */
ep_recv_cb_t ep_recv_cb;

/** \brief Mutexlock for workers */
pthread_mutex_t ep_queue_mutex;

/** \brief Condition for workers */
pthread_cond_t ep_queue_cond;

/**
 * \brief Function to register a callback function for network sockets
 * \param cb The callback function for network sockets
 */
static void recv_cb_register(ep_recv_cb_t cb)
{
    ep_recv_cb = cb;
}

/**
 * \brief Function to receive raw messages from network sockets
 * \return NULL
 */
static void *do_tasks(void *null)
{
    int wsock, bytes, done = 0;
    uint8_t rx_buf[BUFFER_SIZE];

    pthread_mutex_lock(&ep_queue_mutex);

    while (ep_listening) {
        struct timeval now;
        gettimeofday(&now, NULL);

        struct timespec time_wait;
        time_wait.tv_sec = now.tv_sec+1;
        time_wait.tv_nsec = now.tv_usec;

        pthread_cond_timedwait(&ep_queue_cond, &ep_queue_mutex, &time_wait);

FETCH_ONE_FD:
        if (ep_listening == FALSE) { // clean up
            pthread_mutex_unlock(&ep_queue_mutex);
            break;
        }

        wsock = pop_front();
        if (wsock < 0) continue;
        pthread_mutex_unlock(&ep_queue_mutex);

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

            bytes = ep_recv_cb(wsock, rx_buf, bytes);
            if (bytes == -1) {
                done = 1;
                break;
            }
        }

        if (done) {
            // closed connection
            closed_connection(wsock);
            close(wsock);
        } else {
            if (relink_epoll(ep_epoll, wsock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                // closed connection
                closed_connection(wsock);
                close(wsock);
            }
        }

        pthread_mutex_lock(&ep_queue_mutex);

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

    pthread_mutex_init(&ep_queue_mutex, NULL);
    pthread_cond_init(&ep_queue_cond, NULL);

    int i;
    pthread_t thread;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        if (pthread_create(&thread, NULL, &do_tasks, NULL) < 0) {
            PERROR("pthread_create");
        }
    }
}

/////////////////////////////////////////////////////////////////////

/** \brief Network socket */
int ep_sc;

/** \brief Network socket type (TRUE: tcp, FALSE: ipc) */
int ep_type;

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
 * \param type Socket type
 * \param addr Binding address
 * \param port Port number
 */
static int init_socket(char *type, char *addr, uint16_t port)
{
    if (strcmp(type, "tcp") == 0) {
        ep_type = TRUE;

        if ((ep_sc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            PERROR("socket");
            return -1;
        }

        int option = 1;
        if (setsockopt(ep_sc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
            PERROR("setsockopt");
        }

        if (nonblocking_mode(ep_sc) < 0) {
            close(ep_sc);
            return -1;
        }

        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(addr);
        server.sin_port = htons(port);

        if (bind(ep_sc, (struct sockaddr *)&server, sizeof(server)) < 0) {
            PERROR("bind");
            close(ep_sc);
            return -1;
        }

        if (listen(ep_sc, SOMAXCONN) < 0) {
            PERROR("listen");
            close(ep_sc);
            return -1;
        }

        if (link_epoll(ep_epoll, ep_sc, EPOLLIN | EPOLLET) < 0) {
            close(ep_sc);
            return -1;
        }
    } else { // ipc
        ep_type = FALSE;

        if ((ep_sc = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            PERROR("socket");
            return -1;
        }

        int option = 1;
        if (setsockopt(ep_sc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
            PERROR("setsockopt");
        }

        if (nonblocking_mode(ep_sc) < 0) {
            close(ep_sc);
            return -1;
        }

        struct sockaddr_un server;
        memset(&server, 0, sizeof(server));
        server.sun_family = AF_UNIX;
        strcpy(server.sun_path, addr);

        if (bind(ep_sc, (struct sockaddr *)&server, sizeof(server)) < 0) {
            PERROR("bind");
            close(ep_sc);
            return -1;
        }

        if (listen(ep_sc, SOMAXCONN) < 0) {
            PERROR("listen");
            close(ep_sc);
            return -1;
        }

        if (link_epoll(ep_epoll, ep_sc, EPOLLIN | EPOLLET) < 0) {
            close(ep_sc);
            return -1;
        }
    }

    return ep_sc;
}

/**
 * \brief Function to receive connections from network sockets
 * \return NULL
 */
static void *socket_listen(void *arg)
{
    if (ep_type == TRUE) { // tcp
        int csock;
        int nums = 0;
        struct sockaddr_in client;
        socklen_t len = sizeof(struct sockaddr);

        while (ep_listening) {
            nums = epoll_wait(ep_epoll, ep_events, EP_MAXEVENTS, 100);

            if (ep_listening == FALSE) break;

            int i;
            for (i=0; i<nums; i++) {
                if (ep_events[i].data.fd == ep_sc) {
                    while (1) {
                        if ((csock = accept(ep_sc, (struct sockaddr *)&client, &len)) < 0) {
                            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                                PERROR("accept");
                            }
                            break;
                        }

                        // new connection
                        new_connection(csock);

                        if (nonblocking_mode(csock) < 0) {
                            // closed connection
                            closed_connection(csock);
                            close(csock);
                            break;
                        }

                        if (link_epoll(ep_epoll, csock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                            // closed connection
                            closed_connection(csock);
                            close(csock);
                            break;
                        }
                    }
                } else {
                    pthread_mutex_lock(&ep_queue_mutex);
                    push_back(ep_events[i].data.fd);
                    pthread_cond_signal(&ep_queue_cond);
                    pthread_mutex_unlock(&ep_queue_mutex);
                }
            }

            if (nums < 0 && errno != EINTR)
                break;
        }

        if (nums < 0)
            PERROR("epoll_wait");

        close(ep_epoll);
        close(ep_sc);
    } else { // ipc
        int csock;
        int nums = 0;
        struct sockaddr_un client;
        socklen_t len = sizeof(struct sockaddr);

        while (ep_listening) {
            nums = epoll_wait(ep_epoll, ep_events, EP_MAXEVENTS, 100);

            if (ep_listening == FALSE) break;

            int i;
            for (i=0; i<nums; i++) {
                if (ep_events[i].data.fd == ep_sc) {
                    while (1) {
                        if ((csock = accept(ep_sc, (struct sockaddr *)&client, &len)) < 0) {
                            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                                PERROR("accept");
                            }
                            break;
                        }

                        // new connection
                        new_connection(csock);

                        if (nonblocking_mode(csock) < 0) {
                            // closed connection
                            closed_connection(csock);
                            close(csock);
                            break;
                        }

                        if (link_epoll(ep_epoll, csock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                            // closed connection
                            closed_connection(csock);
                            close(csock);
                            break;
                        }
                    }
                } else {
                    pthread_mutex_lock(&ep_queue_mutex);
                    push_back(ep_events[i].data.fd);
                    pthread_cond_signal(&ep_queue_cond);
                    pthread_mutex_unlock(&ep_queue_mutex);
                }
            }

            if (nums < 0 && errno != EINTR)
                break;
        }

        if (nums < 0)
            PERROR("epoll_wait");

        close(ep_epoll);
        close(ep_sc);
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize socket, epoll, and other things
 * \return None
 */
static int create_epoll_env(char *type, char *addr, uint16_t port)
{
    ep_listening = TRUE;

    recv_cb_register(msg_proc);

    init_epoll();
    init_workers();
    init_socket(type, addr, port);

    signal(SIGPIPE, SIG_IGN);

    return 0;
}

/**
 * \brief Function to destroy socket, epoll, and other things
 * \return None
 */
static int destroy_epoll_env(void)
{
    ep_listening = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        pthread_mutex_lock(&ep_queue_mutex);
        push_back(0);
        pthread_mutex_unlock(&ep_queue_mutex);
        pthread_cond_signal(&ep_queue_cond);
    }

    pthread_cond_destroy(&ep_queue_cond);
    pthread_mutex_destroy(&ep_queue_mutex);

    signal(SIGPIPE, SIG_DFL);

    return 0;
}

/////////////////////////////////////////////////////////////////////

