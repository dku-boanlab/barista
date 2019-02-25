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
int listening;

/////////////////////////////////////////////////////////////////////

/** \brief The size of a network socket queue */
#define MAXQLEN 1024

/** \brief The pointer of the first entry in a network socket queue */
#define FIRST &queue[0]

/** \brief The pointer of the last entry in a network socket queue */
#define LAST &queue[MAXQLEN - 1]

/** \brief Network socket queue */
int queue[MAXQLEN];

/** \brief The head pointer of a network socket queue */
int *head;

/** \brief The tail pointer of a network socket queue */
int *tail;

/** \brief The number of entries in a network socket queue */
int size;

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

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to handle incoming messages from network sockets
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 */
static int msg_proc(int sock, uint8_t *rx_buf, int bytes);

/**
 * \brief Function to do something when a new device is connected
 * \return None
 */
static int new_connection(int sock);

/**
 * \brief Function to do something when a device is disconnected
 * \return None
 */
static int closed_connection(int sock);

/////////////////////////////////////////////////////////////////////

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
typedef int (*recv_cb_t)(int sock, uint8_t *rx_buf, int bytes);

/** \brief The callback initialization for network sockets */
recv_cb_t recv_cb;

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
    uint8_t rx_buf[BUFFER_SIZE];

    pthread_mutex_lock(&queue_mutex);

    while (listening) {
        struct timeval now;
        gettimeofday(&now, NULL);

        struct timespec time_wait;
        time_wait.tv_sec = now.tv_sec+1;
        time_wait.tv_nsec = now.tv_usec;

        pthread_cond_timedwait(&queue_cond, &queue_mutex, &time_wait);

FETCH_ONE_FD:
        if (listening == FALSE) { // clean up
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
            closed_connection(wsock);
            close(wsock);
        } else {
            if (relink_epoll(epoll, wsock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                // closed connection
                closed_connection(wsock);
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
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        if (pthread_create(&thread, NULL, &do_tasks, NULL) < 0) {
            PERROR("pthread_create");
        }
    }
}

/////////////////////////////////////////////////////////////////////

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
 * \param addr Binding address
 * \param port Port number
 */
static int init_socket(uint32_t addr, uint16_t port)
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
    server.sin_addr.s_addr = htonl(addr);
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

    while (listening) {
        nums = epoll_wait(epoll, events, MAXEVENTS, 100);

        if (listening == FALSE) break;

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

                    // new connection
                    new_connection(csock);

                    if (nonblocking_mode(csock) < 0) {
                        // closed connection
                        closed_connection(csock);
                        close(csock);
                        break;
                    }

                    if (link_epoll(epoll, csock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                        // closed connection
                        closed_connection(csock);
                        close(csock);
                        break;
                    }
                }
            } else {
                pthread_mutex_lock(&queue_mutex);
                push_back(events[i].data.fd);
                pthread_cond_signal(&queue_cond);
                pthread_mutex_unlock(&queue_mutex);
            }
        }

        if (nums < 0 && errno != EINTR)
            break;
    }

    if (nums < 0)
        PERROR("epoll_wait");

    close(epoll);
    close(sc);

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize socket, epoll, and other things
 * \return None
 */
static int create_epoll_env(uint32_t addr, uint16_t port)
{
    listening = TRUE;

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
    listening = FALSE;

    waitsec(1, 0);

    int i;
    for (i=0; i<__NUM_PULL_THREADS; i++) {
        pthread_mutex_lock(&queue_mutex);
        push_back(0);
        pthread_mutex_unlock(&queue_mutex);
        pthread_cond_signal(&queue_cond);
    }

    pthread_cond_destroy(&queue_cond);
    pthread_mutex_destroy(&queue_mutex);

    signal(SIGPIPE, SIG_DFL);

    return 0;
}

/////////////////////////////////////////////////////////////////////

