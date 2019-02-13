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
int ap_listening;

/////////////////////////////////////////////////////////////////////

/** \brief The size of a network socket queue */
#define AP_MAXQLEN 1024

/** \brief The pointer of the first entry in a network socket queue */
#define AP_FIRST &ap_queue[0]

/** \brief The pointer of the last entry in a network socket queue */
#define AP_LAST &ap_queue[AP_MAXQLEN - 1]

/** \brief Network socket queue */
int ap_queue[AP_MAXQLEN];

/** \brief The head and tail pointers of a network socket queue */
int *ap_head, *ap_tail;

/** \brief The number of entries in a network socket queue */
int ap_size;

/**
 * \brief Function to initialize a network socket queue
 * \return None
 */
static void init_queue(void)
{
    memset(ap_queue, 0, sizeof(int) * AP_MAXQLEN);
    ap_head = ap_tail = NULL;
    ap_size = 0;
}

/**
 * \brief Function to push a socket into a network socket queue
 * \param v Network socket
 */
static void push_back(int v)
{
    if (ap_size == 0) {
        ap_head = ap_tail = AP_FIRST;
    } else {
        ap_tail = (AP_LAST - ap_tail > 0) ? ap_tail + 1 : AP_FIRST;
    }

    *ap_tail = v;

    ap_size++;
}

/**
 * \brief Function to pop a socket from a network socket queue
 * \return Network socket
 */
static int pop_front(void)
{
    int ret = -1;

    if (ap_size > 0) {
        ret = *ap_head;
        ap_head = (AP_LAST - ap_head > 0) ? ap_head + 1 : AP_FIRST;
        ap_size--;
    }

    return ret;
}

/**
 * \brief Function to get the number of network sockets in a network socket queue
 * \return The number of network sockets in a network socket queue
 */
static int get_qsize(void)
{
    return ap_size;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle incoming messages from external applications
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes);

/**
 * \brief Function to do something when a new application is connected
 * \return None
 */
static int new_connection(int sock);

/**
 * \brief Function to do something when an application is disconnected
 * \return None
 */
static int closed_connection(int sock);

/////////////////////////////////////////////////////////////////////

/** \brief The maximum number of epoll events */
#define AP_MAXEVENTS 1024

/** \brief Epoll for network sockets */
int ap_epoll;

/** \brief The event list for network sockets */
struct epoll_event ap_events[AP_MAXEVENTS];

/**
 * \brief Function to create an epoll
 * \return New epoll
 */
static int init_epoll(void)
{
    memset(ap_events, 0, sizeof(struct epoll_event) * AP_MAXEVENTS);

    if ((ap_epoll = epoll_create1(0)) < 0) {
        PERROR("epoll_create1");
        return -1;
    }

    return ap_epoll;
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

/////////////////////////////////////////////////////////////////////

/** \brief The size of a recv buffer */
#define BUFFER_SIZE 8192

/** \brief The callback definition for network sockets */
typedef int (*ap_recv_cb_t)(int sock, uint8_t *rx_buf, int bytes);

/** \brief The callback initialization for network sockets */
ap_recv_cb_t ap_recv_cb;

/** \brief Mutexlock for workers */
pthread_mutex_t ap_queue_mutex;

/** \brief Condition for workers */
pthread_cond_t ap_queue_cond;

/**
 * \brief Function to register a callback function for network sockets
 * \param cb The callback function for network sockets
 */
static void recv_cb_register(ap_recv_cb_t cb)
{
    ap_recv_cb = cb;
}

/**
 * \brief Function to receive raw messages from network sockets
 * \return NULL
 */
static void *do_tasks(void *null)
{
    int wsock, bytes, done = 0;
    uint8_t rx_buf[BUFFER_SIZE];

    pthread_mutex_lock(&ap_queue_mutex);

    while (ap_listening) {
        struct timeval now;
        gettimeofday(&now, NULL);

        struct timespec time_wait;
        time_wait.tv_sec = now.tv_sec+1;
        time_wait.tv_nsec = now.tv_usec;

        pthread_cond_timedwait(&ap_queue_cond, &ap_queue_mutex, &time_wait);

FETCH_ONE_FD:
        if (ap_listening == FALSE) { // clean up
            pthread_mutex_unlock(&ap_queue_mutex);
            break;
        }

        wsock = pop_front();
        if (wsock < 0) continue;
        pthread_mutex_unlock(&ap_queue_mutex);

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

            bytes = ap_recv_cb(wsock, rx_buf, bytes);
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
            if (relink_epoll(ap_epoll, wsock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                // closed connection
                closed_connection(wsock);
                close(wsock);
            }
        }

        pthread_mutex_lock(&ap_queue_mutex);

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

    pthread_mutex_init(&ap_queue_mutex, NULL);
    pthread_cond_init(&ap_queue_cond, NULL);

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
int ap_sc;

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
 * \param addr Binding address
 * \param port Port number
 */
static int init_socket(char *addr, uint16_t port)
{
    if ((ap_sc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        PERROR("socket");
        return -1;
    }

    int option = 1;
    if (setsockopt(ap_sc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        PERROR("setsockopt");
    }

    if (nonblocking_mode(ap_sc) < 0) {
        close(ap_sc);
        return -1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_port = htons(port);

    if (bind(ap_sc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        PERROR("bind");
        close(ap_sc);
        return -1;
    }

    if (listen(ap_sc, SOMAXCONN) < 0) {
        PERROR("listen");
        close(ap_sc);
        return -1;
    }

    if (link_epoll(ap_epoll, ap_sc, EPOLLIN | EPOLLET) < 0) {
        close(ap_sc);
        return -1;
    }

    return ap_sc;
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

    while (ap_listening) {
        nums = epoll_wait(ap_epoll, ap_events, AP_MAXEVENTS, 100);

        if (ap_listening == FALSE) break;

        int i;
        for (i=0; i<nums; i++) {
            if (ap_events[i].data.fd == ap_sc) {
                while (1) {
                    if ((csock = accept(ap_sc, (struct sockaddr *)&client, &len)) < 0) {
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

                    if (link_epoll(ap_epoll, csock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                        // closed connection
                        closed_connection(csock);
                        close(csock);
                        break;
                    }
                }
            } else {
                pthread_mutex_lock(&ap_queue_mutex);
                push_back(ap_events[i].data.fd);
                pthread_cond_signal(&ap_queue_cond);
                pthread_mutex_unlock(&ap_queue_mutex);
            }
        }

        if (nums < 0 && errno != EINTR)
            break;
    }

    if (nums < 0)
        PERROR("epoll_wait");

    close(ap_epoll);
    close(ap_sc);

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize socket, epoll, and other things
 * \return None
 */
static int create_epoll_env(char *addr, uint16_t port)
{
    ap_listening = TRUE;

    recv_cb_register(msg_proc);

    init_epoll();
    init_workers();
    init_socket(addr, port);

    signal(SIGPIPE, SIG_IGN);

    return 0;
}

/**
 * \brief Function to destroy socket, epoll, and other things
 * \return None
 */
static int destroy_epoll_env(void)
{
    ap_listening = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        pthread_mutex_lock(&ap_queue_mutex);
        push_back(0);
        pthread_mutex_unlock(&ap_queue_mutex);
        pthread_cond_signal(&ap_queue_cond);
    }

    pthread_cond_destroy(&ap_queue_cond);
    pthread_mutex_destroy(&ap_queue_mutex);

    signal(SIGPIPE, SIG_DFL);

    return 0;
}
