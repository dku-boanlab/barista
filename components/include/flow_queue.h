/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

// inside of 'flow_mgmt.c'

/////////////////////////////////////////////////////////////////////

/** \brief The structure of a flow table */
typedef struct _flow_table_t {
    flow_t *head; /**< The head pointer */
    flow_t *tail; /**< The tail pointer */

    pthread_spinlock_t lock; /**< The lock for management */
} flow_table_t;

/** \brief The structure of a flow pool */
typedef struct _flow_queue_t {
    int size; /**< The size of entries */

    flow_t *head; /**< The head pointer */
    flow_t *tail; /**< The tail pointer */

    pthread_spinlock_t lock; /**< The lock for management */
} flow_queue_t;

/** \brief Flow pool */
flow_queue_t flow_q;

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to pop a flow from a flow pool
 * \return Empty flow
 */
static flow_t *flow_dequeue(void)
{
    flow_t *flow = NULL;

    pthread_spin_lock(&flow_q.lock);

    if (flow_q.head == NULL) {
        pthread_spin_unlock(&flow_q.lock);

        flow = (flow_t *)CALLOC(1, sizeof(flow_t));
        if (flow == NULL) {
            PERROR("calloc");
            return NULL;
        }

        pthread_spin_lock(&flow_q.lock);
        flow_q.size--;
        pthread_spin_unlock(&flow_q.lock);

        return flow;
    } else if (flow_q.head == flow_q.tail) {
        flow = flow_q.head;
        flow_q.head = flow_q.tail = NULL;
    } else {
        flow = flow_q.head;
        flow_q.head = flow_q.head->next;
        flow_q.head->prev = NULL;
    }

    flow_q.size--;

    pthread_spin_unlock(&flow_q.lock);

    memset(flow, 0, sizeof(flow_t));

    return flow;
}

/**
 * \brief Function to push a used flow into a flow pool
 * \param flow Used flow
 */
static int flow_enqueue(flow_t *flow)
{
    if (flow == NULL) return -1;

    memset(flow, 0, sizeof(flow_t));

    pthread_spin_lock(&flow_q.lock);

    if (flow_q.size < 0) {
        flow_q.size++;

        pthread_spin_unlock(&flow_q.lock);

        FREE(flow);

        return 0;
    }

    if (flow_q.head == NULL && flow_q.tail == NULL) {
        flow_q.head = flow_q.tail = flow;
    } else {
        flow_q.tail->next = flow;
        flow->prev = flow_q.tail;
        flow_q.tail = flow;
    }

    flow_q.size++;

    pthread_spin_unlock(&flow_q.lock);

    return 0;
}

/**
 * \brief Function to initialize a flow pool
 * \return None
 */
static int flow_q_init(void)
{
    memset(&flow_q, 0, sizeof(flow_queue_t));

    pthread_spin_init(&flow_q.lock, PTHREAD_PROCESS_PRIVATE);

    int i;
    for (i=0; i<FLOW_PRE_ALLOC; i++) {
        flow_t *flow = (flow_t *)MALLOC(sizeof(flow_t));
        if (flow == NULL) {
            PERROR("malloc");
            return -1;
        }

        flow_enqueue(flow);
    }

    return 0;
}

/**
 * \brief Function to destroy a flow pool
 * \return None
 */
static int flow_q_destroy(void)
{
    pthread_spin_lock(&flow_q.lock);

    flow_t *curr = flow_q.head;
    while (curr != NULL) {
        flow_t *tmp = curr;
        curr = curr->next;
        if (tmp)
            FREE(tmp);
    }

    pthread_spin_unlock(&flow_q.lock);
    pthread_spin_destroy(&flow_q.lock);

    return 0;
}
