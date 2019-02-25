/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

// inside of either 'l2_learning.c' or 'l2_shortest.c'

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a MAC entry */
typedef struct _mac_entry_t {
    uint64_t dpid; /**< Datapath ID */
    uint16_t port; /**< Port */

    uint32_t ip; /**< IP address */
    uint64_t mac; /**< MAC address */

    struct _mac_entry_t *prev; /**< Previous entry */
    struct _mac_entry_t *next; /**< Next entry */

    struct _mac_entry_t *r_next; /**< Next entry for removal */
} mac_entry_t;

/** \brief The structure of a MAC table */
typedef struct _mac_table_t {
    mac_entry_t *head; /**< The head pointer */
    mac_entry_t *tail; /**< The tail pointer */

    pthread_rwlock_t lock; /**< The lock for management */
} mac_table_t;

/** \brief MAC table */
mac_table_t *mac_table;

/////////////////////////////////////////////////////////////////////

/** \brief The size of a MAC table */
#define MAC_HASH_SIZE 4096

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
    mac_entry_t *mac = NULL;

    pthread_spin_lock(&mac_q.lock);

    if (mac_q.head == NULL) {
        pthread_spin_unlock(&mac_q.lock);

        mac = (mac_entry_t *)CALLOC(1, sizeof(mac_entry_t));
        if (mac == NULL) {
            PERROR("calloc");
            return NULL;
        }

        pthread_spin_lock(&mac_q.lock);
        mac_q.size--;
        pthread_spin_unlock(&mac_q.lock);

        return mac;
    } else if (mac_q.head == mac_q.tail) {
        mac = mac_q.head;
        mac_q.head = mac_q.tail = NULL;
    } else {
        mac = mac_q.head;
        mac_q.head = mac_q.head->next;
        mac_q.head->prev = NULL;
    }

    mac_q.size--;

    pthread_spin_unlock(&mac_q.lock);

    memset(mac, 0, sizeof(mac_entry_t));

    return mac;
}

/**
 * \brief Function to push a used MAC entry into a MAC pool
 * \param old Used MAC entry
 */
static int mac_enqueue(mac_entry_t *old)
{
    if (old == NULL) return -1;

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

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to clean up mac entires
 * \param idx The index of a MAC table
 * \param tmp_list The list of MAC entries to be removed
 */
static int clean_up_tmp_list(int idx, mac_table_t *tmp_list)
{
    // locked

    mac_entry_t *curr = tmp_list->head;

    while (curr != NULL) {
        mac_entry_t *tmp = curr;

        curr = curr->next;

        if (tmp->prev != NULL && tmp->next != NULL) {
            tmp->prev->next = tmp->next;
            tmp->next->prev = tmp->prev;
        } else if (tmp->prev == NULL && tmp->next != NULL) {
            mac_table[idx].head = tmp->next;
            tmp->next->prev = NULL;
        } else if (tmp->prev != NULL && tmp->next == NULL) {
            mac_table[idx].tail = tmp->prev;
            tmp->prev->next = NULL;
        } else if (tmp->prev == NULL && tmp->next == NULL) {
            mac_table[idx].head = NULL;
            mac_table[idx].tail = NULL;
        }

        mac_enqueue(tmp);
    }

    return 0;
}

/**
 * \brief Function to initialize a MAC table and a MAC pool
 * \return None
 */
static int mac_init(void)
{
    mac_table = (mac_table_t *)CALLOC(MAC_HASH_SIZE, sizeof(mac_table_t));
    if (mac_table == NULL) {
        PERROR("calloc");
        return -1;
    }

    int i;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_init(&mac_table[i].lock, NULL);
    }

    memset(&mac_q, 0, sizeof(mac_queue_t));
    pthread_spin_init(&mac_q.lock, PTHREAD_PROCESS_PRIVATE);

    for (i=0; i<MAC_PRE_ALLOC; i++) {
        mac_entry_t *mac = (mac_entry_t *)MALLOC(sizeof(mac_entry_t));
        if (mac == NULL) {
            PERROR("malloc");
            return -1;
        }

        mac_enqueue(mac);
    }

    return 0;
}

/**
 * \brief Function to destroy a MAC table and a MAC pool
 * \return None
 */
static int mac_destroy(void)
{
    int i;
    for (i=0; i<MAC_HASH_SIZE; i++) {
        pthread_rwlock_wrlock(&mac_table[i].lock);

        mac_entry_t *curr = mac_table[i].head;
        while (curr != NULL) {
            mac_entry_t *tmp = curr;
            curr = curr->next;
            FREE(tmp);
        }

        pthread_rwlock_unlock(&mac_table[i].lock);
        pthread_rwlock_destroy(&mac_table[i].lock);
    }

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

    FREE(mac_table);

    return 0;
}

/////////////////////////////////////////////////////////////////////

