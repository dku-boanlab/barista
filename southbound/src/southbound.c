/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup extensions
 * @{
 * \defgroup southbound External Packet I/O Engine
 * \brief Packet I/O engine located outside of the Barista NOS
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "southbound.h"

/////////////////////////////////////////////////////////////////////

/** \brief The running flag for external packet I/O engine */
int conn_sb_on;

/////////////////////////////////////////////////////////////////////

/** \brief Socket mapping table (network socket -> domain socket) */
int sb2cr[__DEFAULT_TABLE_SIZE];

/** \brief Socket mapping table (domain socket -> network socket) */
int cr2sb[__DEFAULT_TABLE_SIZE];

// socket queue handling part (domain socket) ///////////////////////

/** \brief The size of a domain socket queue */
#define DS_MAXQLEN 1024

/** \brief The pointer of the first entry in a domain socket queue */
#define DS_FIRST &ds_queue[0]

/** \brief The pointer of the last entry in a domain socket queue */
#define DS_LAST  &ds_queue[DS_MAXQLEN - 1]

/** \brief Domain socket queue */
int ds_queue[DS_MAXQLEN];

/** \brief The head and tail pointers of a domain socket queue */
int *ds_head, *ds_tail;

/** \brief The number of entries in a domain socket queue */
int ds_size = 0;

/**
 * \brief Function to initialize a domain socket queue
 * \return None
 */
void init_ds_queue(void)
{
    memset(ds_queue, 0, sizeof(int) * DS_MAXQLEN);
    ds_head = ds_tail = NULL;
    ds_size = 0;
}

/**
 * \brief Function to push a domain socket into a domain socket queue
 * \param v Domain socket
 */
void ds_push_back(int v)
{
    if (ds_size == 0) {
        ds_head = ds_tail = DS_FIRST;
    } else {
        ds_tail = (DS_LAST - ds_tail > 0) ? ds_tail + 1 : DS_FIRST;
    }

    *ds_tail = v;

    ds_size++;
}

/**
 * \brief Function to pop a domain socket from a domain socket queue
 * \return Domain socket
 */
int ds_pop_front(void)
{
    int ret = -1;

    if (ds_size > 0) {
        ret = *ds_head;
        ds_head = (DS_LAST - ds_head > 0) ? ds_head + 1 : DS_FIRST;
        ds_size--;
    }

    return ret;
}

/**
 * \brief Function to get the number of domain sockets in a domain socket queue
 * \return The number of domain sockets in a domain socket queue
 */
int ds_get_qsize(void)
{
    return ds_size;
}

// message handling part (domain socket) ////////////////////////////

/** \brief The size of a recv buffer */
#define BUFFER_SIZE 8192

/**
 * \brief Function to handle incoming messages from domain sockets (to southbound)
 * \param sock Domain socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 * \return The size of remaining bytes in the buffer
 */
int ds_msg_proc(int sock, uint8_t *rx_buf, int bytes)
{
    int remain = bytes;
    int done = 0;

    while (remain > 0) {
        bytes = write(cr2sb[sock], rx_buf + done, remain);
        if (bytes < 0 && errno != EAGAIN) {
            break;
        } else if (bytes < 0) {
            continue;
        }

        done += bytes;
        remain -= bytes;
    }

    return remain;
}

// socket queue handling part (network socket) //////////////////////

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
void init_queue(void)
{
    memset(queue, 0, sizeof(int) * MAXQLEN);
    head = tail = NULL;
    size = 0;
}

/**
 * \brief Function to push a network socket into a network socket queue
 * \param v Network socket
 */
void push_back(int v)
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
 * \brief Function to pop a network socket from a network socket queue
 * \return Network socket
 */
int pop_front(void)
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
int get_qsize(void)
{
    return size;
}

// message handling part (network socket) ///////////////////////////

/**
 * \brief Function to handle incoming messages from network sockets (to core)
 * \param sock Network socket
 * \param rx_buf Input buffer
 * \param bytes The size of the input buffer
 * \return The size of remaining bytes in the buffer
 */
int msg_proc(int sock, uint8_t *rx_buf, int bytes)
{
    int remain = bytes;
    int done = 0;

    while (remain > 0) {
        bytes = write(sb2cr[sock], rx_buf + done, remain);
        if (bytes < 0 && errno != EAGAIN) {
            break;
        } else if (bytes < 0) {
            continue;
        }

        done += bytes;
        remain -= bytes;
    }

    return remain;
}

// epoll handling part //////////////////////////////////////////////

/** \brief The maximum number of events */
#define MAXEVENTS 1024

/** \brief Epolls for domain and network sockets */
int ds_epoll, epoll;

/** \brief Event list for domain sockets */
struct epoll_event ds_events[MAXEVENTS];

/** \brief Event list for network sockets */
struct epoll_event events[MAXEVENTS];

/**
 * \brief Function to create an epoll
 * \return New epoll
 */
int init_epoll(void)
{
    int epol;

    if ((epol = epoll_create1(0)) < 0) {
        PERROR("epoll_create1");
        return -1;
    }

    return epol;
}

/**
 * \brief Function to add a new socket into an epoll
 * \param epol Epoll
 * \param fd Socket
 * \param flags Epoll flags
 */
int link_epoll(int epol, int fd, int flags)
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
int relink_epoll(int epol, int fd, int flags)
{
    struct epoll_event event;

    event.data.fd = fd;
    event.events = flags;

    epoll_ctl(epol, EPOLL_CTL_MOD, fd, &event);

    return 0;
}

// worker thread handling part (domain socket) //////////////////////

/** \brief The callback definition for domain sockets */
typedef int (*ds_recv_cb_t)(int sock, uint8_t *rx_buf, int bytes);

/** \brief The callback initialization for domain sockets */
ds_recv_cb_t ds_recv_cb = NULL;

/** \brief Mutexlock for workers (domain socket) */
pthread_mutex_t ds_queue_mutex;

/** \brief Condition for workers (domain socket) */
pthread_cond_t ds_queue_cond;

/**
 * \brief Function to register a callback function for domain sockets
 * \param cb The callback function for domain sockets
 */
void ds_recv_cb_register(ds_recv_cb_t cb)
{
    ds_recv_cb = cb;
}

/**
 * \brief Function to handle messages from domain sockets
 * \return NULL
 */
void *do_ds_tasks(void *null)
{
    int wsock, bytes, done = 0;
    uint8_t rx_buf[BUFFER_SIZE] = {0};

    pthread_mutex_lock(&ds_queue_mutex);

    while (conn_sb_on) {
        pthread_cond_wait(&ds_queue_cond, &ds_queue_mutex);

DS_FETCH_ONE_FD:
        if (conn_sb_on == FALSE) {
            pthread_mutex_unlock(&ds_queue_mutex);
            break;
        }

        wsock = ds_pop_front();
        if (wsock < 0) continue;
        pthread_mutex_unlock(&ds_queue_mutex);

        done = 0;
        bytes = 1;
        while (bytes > 0) {
            if (fcntl(wsock, F_GETFD) < 0) {
                done = 1;
                break;
            }
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

            bytes = ds_recv_cb(wsock, rx_buf, bytes);
        }

        if (done) {
            close(cr2sb[wsock]);

            sb2cr[cr2sb[wsock]] = 0;
            cr2sb[wsock] = 0;

            close(wsock);
        } else {
            if (relink_epoll(ds_epoll, wsock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                close(cr2sb[wsock]);

                sb2cr[cr2sb[wsock]] = 0;
                cr2sb[wsock] = 0;

                close(wsock);
            }
        }

        pthread_mutex_lock(&ds_queue_mutex);

        if (ds_get_qsize() > 0) {
            goto DS_FETCH_ONE_FD;
        }
    }

    return NULL;
}

/**
 * \brief Function to initialize domain socket workers
 * \return None
 */
void init_ds_workers(void)
{
    init_ds_queue();

    pthread_mutex_init(&ds_queue_mutex, NULL);
    pthread_cond_init(&ds_queue_cond, NULL);

    int i;
    pthread_t thread;
    for (i=0; i<__NUM_THREADS; i++) {
        if (pthread_create(&thread, NULL, &do_ds_tasks, NULL) < 0) {
            PERROR("pthread_create");
        }
    }
}

// worker thread handling part (network socket) /////////////////////

/** \brief The callback definition for network sockets */
typedef int (*recv_cb_t)(int sock, uint8_t *rx_buf, int bytes);

/** \brief The callback initialization for network sockets */
recv_cb_t recv_cb = NULL;

/** \brief Mutexlock for workers (network socket) */
pthread_mutex_t queue_mutex;

/** \brief Condition for workers (network socket) */
pthread_cond_t queue_cond;

/**
 * \brief Function to register a callback function for network sockets
 * \param cb The callback function for network sockets
 */
void recv_cb_register(recv_cb_t cb)
{
    recv_cb = cb;
}

/**
 * \brief Function to handle messages from network sockets
 * \return NULL
 */
void *do_tasks(void *null)
{
    int wsock, bytes, done = 0;
    uint8_t rx_buf[BUFFER_SIZE] = {0};

    pthread_mutex_lock(&queue_mutex);

    while (conn_sb_on) {
        pthread_cond_wait(&queue_cond, &queue_mutex);

FETCH_ONE_FD:
        if (conn_sb_on == FALSE) {
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
        }

        if (done) {
            close(sb2cr[wsock]);

            cr2sb[sb2cr[wsock]] = 0;
            sb2cr[wsock] = 0;

            close(wsock);
        } else {
            if (relink_epoll(epoll, wsock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                close(sb2cr[wsock]);

                DEBUG("Closed (cfd = %2d, fd = %2d)\n", sb2cr[wsock], wsock);

                cr2sb[sb2cr[wsock]] = 0;
                sb2cr[wsock] = 0;

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
void init_workers(void)
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
int nonblocking_mode(const int fd)
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
int init_socket(uint16_t port)
{
    struct sockaddr_in server;
    int option = 1;

    if ((sc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        PERROR("socket");
        return -1;
    }

    if (setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        PERROR("setsockopt");
    }

    if (nonblocking_mode(sc) < 0) {
        close(sc);
        return -1;
    }

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

// domain socket listening part /////////////////////////////////////

/** \brief Domain socket path */
#define DS_PATH "../tmp/sb_comm"

/**
 * \brief Function to receive connections from domain sockets
 * \return NULL
 */
void *ds_socket_listen(void *arg)
{
    int nums = 0;

    while (conn_sb_on) {
        nums = epoll_wait(ds_epoll, ds_events, MAXEVENTS, 100);

        int i;
        for (i=0; i<nums; i++) {
            pthread_mutex_lock(&ds_queue_mutex);
            ds_push_back(ds_events[i].data.fd);
            pthread_mutex_unlock(&ds_queue_mutex);
            pthread_cond_signal(&ds_queue_cond);
        }

        if (nums < 0 && errno != EINTR)
            break;
    }

    if (nums < 0)
        PERROR("epoll_wait");

    close(ds_epoll);

    return NULL;
}

// network socket listening part ////////////////////////////////////

/**
 * \brief Function to receive connections from network sockets
 * \return NULL
 */
void *socket_listen(void *arg)
{
    int csock;
    int nums = 0;
    struct sockaddr_in client;
    socklen_t len = sizeof(struct sockaddr);

    while (conn_sb_on) {
        nums = epoll_wait(epoll, events, MAXEVENTS, 100);

        if (conn_sb_on == FALSE) break;

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
                    int sock;
                    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
                        PERROR("socket");
                    }

                    struct sockaddr_un remote;
                    remote.sun_family = AF_UNIX;
                    strcpy(remote.sun_path, DS_PATH);
                    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);

                    if (connect(sock, (struct sockaddr *)&remote, len) == -1) {
                        PERROR("connect");
                    }

                    if (nonblocking_mode(sock) < 0) {
                        PERROR("nonblocking_mode");
                    }

                    if (link_epoll(ds_epoll, sock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                        PERROR("link_epoll");
                    }

                    sb2cr[csock] = sock;
                    cr2sb[sock] = csock;

                    DEBUG("Accepted (cfd=%2d, fd=%2d, host=%s, port=%d)\n", sock, csock, inet_ntoa(client.sin_addr), client.sin_port);

                    if (nonblocking_mode(csock) < 0) {
                        close(sock);

                        DEBUG("Closed (cfd = %2d, fd = %2d)\n", sock, csock);

                        cr2sb[sock] = 0;
                        sb2cr[csock] = 0;

                        close(csock);

                        break;
                    }

                    if (link_epoll(epoll, csock, EPOLLIN | EPOLLET | EPOLLONESHOT) < 0) {
                        close(sock);

                        DEBUG("Closed (cfd = %2d, fd = %2d\n", sock, csock);

                        cr2sb[sock] = 0;
                        sb2cr[csock] = 0;

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

    return NULL;
}

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to initialize the external packet I/O engine
 * \param port Port number
 */
void init_sb(char *port)
{
    PRINTF("Initializing external packet I/O engine (port: %s)\n", port);

    conn_sb_on = TRUE;

    memset(sb2cr, 0, sizeof(int) * __DEFAULT_TABLE_SIZE);
    memset(cr2sb, 0, sizeof(int) * __DEFAULT_TABLE_SIZE);

    ds_recv_cb_register(ds_msg_proc);
    ds_epoll = init_epoll();
    init_ds_workers();

    pthread_t thread;
    if (pthread_create(&thread, NULL, &ds_socket_listen, NULL) < 0) {
        PERROR("pthread_create");
    }

    waitsec(0, 1000);

    recv_cb_register(msg_proc);
    epoll = init_epoll();
    init_workers();

    init_socket(atoi(port));

    signal(SIGPIPE, SIG_IGN);
}

/**
 * \brief Function to clean up all socket queues
 * \return None
 */
int cleanup_sb(void)
{
    conn_sb_on = FALSE;

    // domain socket

    int i;
    for (i=0; i<__NUM_THREADS; i++) {
        pthread_mutex_lock(&ds_queue_mutex);
        ds_push_back(0);
        pthread_mutex_unlock(&ds_queue_mutex);
        pthread_cond_signal(&ds_queue_cond);
    }

    pthread_cond_destroy(&ds_queue_cond);
    pthread_mutex_destroy(&ds_queue_mutex);

    // network socket

    for (i=0; i<__NUM_THREADS; i++) {
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

/** \brief The SIGINT handler definition */
void (*sigint_func)(int);

/** \brief Function to handle the SIGINT signal */
void sigint_handler(int sig)
{
    PRINTF("Got a SIGINT signal\n");

    cleanup_sb();

    exit(0);
}

/** \brief The SIGKILL handler definition */
void (*sigkill_func)(int);

/** \brief Function to handle the SIGKILL signal */
void sigkill_handler(int sig)
{
    PRINTF("Got a SIGKILL signal\n");

    cleanup_sb();

    exit(0);
}

/** \brief The SIGTERM handler definition */
void (*sigterm_func)(int);

/** \brief Function to handle the SIGTERM signal */
void sigterm_handler(int sig)
{
    PRINTF("Got a SIGTERM signal\n");

    cleanup_sb();

    exit(0);
}

/////////////////////////////////////////////////////////////////////

/** \brief Function to print the Barista usage */
static void print_usage(char *name)
{
    PRINTF("Usage: %s -p [port number]\n", name);
}

/////////////////////////////////////////////////////////////////////

/** \brief The main function of the external packet I/O engine */
int main(int argc, char **argv)
{
    int opt;
    char port[__CONF_SHORT_LEN] = {0};

    if (argc == 1) {
        sprintf(port, "%d", __DEFAULT_PORT);
    } else if (argc < 2 || argc > 3) {
        print_usage(argv[0]);
        return -1;
    } else {
        while ((opt = getopt(argc, argv, "p:")) != -1) {
            switch (opt) {
            case 'p':
                if (0 < atoi(optarg) && atoi(optarg) < 65536) {
                    strcpy(port, optarg);
                } else {
                    print_usage(argv[0]);
                    return -1;
                }
                break;
            default:
                print_usage(argv[0]);
                return -1;
            }
        }
    }

    sigint_func = signal(SIGINT, sigint_handler);
    sigkill_func = signal(SIGKILL, sigkill_handler);
    sigterm_func = signal(SIGTERM, sigterm_handler);

    waitsec(0, 1000);

    init_sb(port);

    waitsec(0, 1000);

    socket_listen(NULL);

    return 0;
}

/**
 * @}
 *
 * @}
 */
