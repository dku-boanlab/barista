/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

// inside of 'host_mgmt.c'

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a host table */
typedef struct _host_table_t {
    host_t *head; /**< The head pointer */
    host_t *tail; /**< The tail pointer */

    pthread_rwlock_t lock; /**< The lock for management */
} host_table_t;

/** \brief The structure of a host pool */
typedef struct _host_queue_t {
    int size; /**< the number of entries */

    host_t *head; /**< The head pointer */
    host_t *tail; /**< The tail pointer */

    pthread_spinlock_t lock; /**< The lock for management */
} host_queue_t;

/** \brief Host pool */
host_queue_t host_q;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to pop a host from a host pool
 * \return Empty host
 */
static host_t *host_dequeue(void)
{
    host_t *host = NULL;

    pthread_spin_lock(&host_q.lock);

    if (host_q.head == NULL) {
        pthread_spin_unlock(&host_q.lock);

        host = (host_t *)CALLOC(1, sizeof(host_t));
        if (host == NULL) {
            PERROR("calloc");
            return NULL;
        }

        pthread_spin_lock(&host_q.lock);
        host_q.size--;
        pthread_spin_unlock(&host_q.lock);

        return host;
    } else if (host_q.head == host_q.tail) {
        host = host_q.head;
        host_q.head = host_q.tail = NULL;
    } else {
        host = host_q.head;
        host_q.head = host_q.head->next;
        host_q.head->prev = NULL;
    }

    host_q.size--;

    pthread_spin_unlock(&host_q.lock);

    memset(host, 0, sizeof(host_t));

    return host;
}

/**
 * \brief Function to push a used host into a host pool
 * \param old Used host
 */
static int host_enqueue(host_t *old)
{
    if (old == NULL) return -1;

    memset(old, 0, sizeof(host_t));

    pthread_spin_lock(&host_q.lock);

    if (host_q.size < 0) {
        host_q.size++;

        pthread_spin_unlock(&host_q.lock);

        FREE(old);

        return 0;
    }

    if (host_q.head == NULL && host_q.tail == NULL) {
        host_q.head = host_q.tail = old;
    } else {
        host_q.tail->next = old;
        old->prev = host_q.tail;
        host_q.tail = old;
    }

    host_q.size++;

    pthread_spin_unlock(&host_q.lock);

    return 0;
}

/**
 * \brief Function to initialize a host pool
 * \return None
 */
static int host_q_init(void)
{
    memset(&host_q, 0, sizeof(host_queue_t));

    pthread_spin_init(&host_q.lock, PTHREAD_PROCESS_PRIVATE);

    int i;
    for (i=0; i<HOST_PRE_ALLOC; i++) {
        host_t *host = (host_t *)MALLOC(sizeof(host_t));
        if (host == NULL) {
            PERROR("malloc");
            return -1;
        }

        host_enqueue(host);
    }

    return 0;
}

/**
 * \brief Function to destroy a host pool
 * \return None
 */
static int host_q_destroy(void)
{
    pthread_spin_lock(&host_q.lock);

    host_t *curr = host_q.head;
    while (curr != NULL) {
        host_t *tmp = curr;
        curr = curr->next;
        FREE(tmp);
    }

    pthread_spin_unlock(&host_q.lock);
    pthread_spin_destroy(&host_q.lock);

    return 0;
}

/////////////////////////////////////////////////////////////////////

