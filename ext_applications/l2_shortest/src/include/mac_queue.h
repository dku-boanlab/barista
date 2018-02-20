/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

// inside of either 'l2_learning.c' or 'l2_shortest.c'

/////////////////////////////////////////////////////////////////////

/** \brief The number of pre-allocated MAC entries */
#define MAC_PRE_ALLOC 1024

/** \brief The structure of a MAC pool */
typedef struct _mac_queue_t {
    int size; /**< The number of entries */

    mac_entry_t *head; /**< The head pointer */
    mac_entry_t *tail; /**< The tail pointer */

    pthread_spinlock_t lock; /**< The lock for management */
} mac_queue_t;

/** \brief MAC pool */
mac_queue_t mac_q;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to pop a MAC entry from a MAC pool
 * \return Empty MAC entry
 */
static mac_entry_t *mac_dequeue(void)
{
    mac_entry_t *new = NULL;

    pthread_spin_lock(&mac_q.lock);

    if (mac_q.head == NULL) {
        pthread_spin_unlock(&mac_q.lock);

        new = (mac_entry_t *)CALLOC(1, sizeof(mac_entry_t));
        if (new == NULL) {
            PERROR("calloc");
            return NULL;
        }

        pthread_spin_lock(&mac_q.lock);
        mac_q.size--;
        pthread_spin_unlock(&mac_q.lock);

        return new;
    } else if (mac_q.head == mac_q.tail) {
        new = mac_q.head;
        mac_q.head = mac_q.tail = NULL;
    } else {
        new = mac_q.head;
        mac_q.head = mac_q.head->next;
        mac_q.head->prev = NULL;
    }

    mac_q.size--;

    pthread_spin_unlock(&mac_q.lock);

    memset(new, 0, sizeof(mac_entry_t));

    return new;
}

/**
 * \brief Function to push a used MAC entry into a MAC pool
 * \param old Used MAC entry
 */
static int mac_enqueue(mac_entry_t *old)
{
    if (old == NULL)
        return -1;

    memset(old, 0, sizeof(mac_entry_t));

    pthread_spin_lock(&mac_q.lock);

    if (mac_q.size < 0) {
        mac_q.size++;

        pthread_spin_unlock(&mac_q.lock);

        FREE(old);

        return 0;
    }

    if (mac_q.head == NULL && mac_q.tail == NULL) {
        mac_q.head = mac_q.tail = old;
    } else {
        mac_q.tail->next = old;
        old->prev = mac_q.tail;
        mac_q.tail = old;
    }

    mac_q.size++;

    pthread_spin_unlock(&mac_q.lock);

    return 0;
}

/**
 * \brief Function to initialize a MAC pool
 * \return None
 */
static int mac_q_init(void)
{
    memset(&mac_q, 0, sizeof(mac_queue_t));
    pthread_spin_init(&mac_q.lock, PTHREAD_PROCESS_PRIVATE);

    int i;
    for (i=0; i<MAC_PRE_ALLOC; i++) {
        mac_entry_t *new = (mac_entry_t *)MALLOC(sizeof(mac_entry_t));
        if (new == NULL) {
            PERROR("malloc");
            return -1;
        }

        mac_enqueue(new);
    }

    return 0;
}

/**
 * \brief Function to destroy a MAC pool
 * \return None
 */
static int mac_q_destroy(void)
{
    pthread_spin_lock(&mac_q.lock);

    mac_entry_t *curr = mac_q.head;
    while (curr != NULL) {
        mac_entry_t *tmp = curr;
        curr = curr->next;
        FREE(tmp);
    }

    pthread_spin_unlock(&mac_q.lock);
    pthread_spin_destroy(&mac_q.lock);

    memset(&mac_q, 0, sizeof(mac_queue_t));

    return 0;
}

