#ifndef QUEUES_H
#define QUEUES_H

#include <float.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_QUEUE_NODES 1000

/**
 * @brief node for our queues. Linked list implementation that utilizes void pointers.
 *  We will be enqueueing items to the tail of the queue, and dequeueing items from the head of the queue.
 */

typedef struct q_node
{
    void *data;
    struct q_node *next;
} Q_NODE_t, *Q_NODE_p_t;

/**
 * @brief a struct to represent our queue
 */

typedef struct queue
{
    Q_NODE_p_t head;
    Q_NODE_p_t tail;
} QUEUE_t, *QUEUE_p_t;

/**
 * @brief a struct to represent our atomic queue
 */

typedef struct aqueue
{
    Q_NODE_p_t head;
    Q_NODE_p_t tail;
    atomic_int num_nodes;
    pthread_mutex_t lock;

} AQUEUE_t, *AQUEUE_p_t;

/**
 * @brief create a queue with items
 *
 * @param items items to initialize queue with (for empty queue pass NULL)
 * @param numItems number of items in items
 * @return pointer to beginning of queue or NULL on failure
 *
 */
QUEUE_p_t create_queue(void **items, int numItems);

/**
 * @brief create an atomic queue with items
 *
 * @param items items to initialize queue with (for empty queue pass NULL)
 * @param numItems number of items in items
 * @return pointer to beginning of queue or NULL on failure
 *
 */
AQUEUE_p_t create_aqueue(void **items, int numItems);

/**
 * @brief enqueue an item to the tail of a queue
 *
 * @param queue queue to add item to
 * @param item item to add to queue
 * @return returns 0 on success or -1 on failure
 */
int enqueue(QUEUE_p_t queue, void *item);

/**
 * @brief enqueue an item to the tail of a queue
 *
 * @param aqueue queue to add item to
 * @param item item to add to queue
 * @return returns 0 on success or -1 on failure
 */
int aenqueue(AQUEUE_p_t aqueue, void *item);

/**
 * @brief dequeue an item from the head of a queue
 *
 * @param queue queue to add item to
 * @param ret_address if set to 1, returns the address of dequeued item. Otherwise returns the item.
 * @return the item that was at the head of the queue, or NULL if the queue was empty
 *
 */
void *dequeue(QUEUE_p_t queue, int ret_address);

/**
 * @brief dequeue an item from the head of an atomic queue
 *
 * @param aqueue queue to add item to
 * @param ret_address if set to 1, returns the address of dequeued item. Otherwise returns the item.
 * @return the item that was at the head of the queue, or NULL if the queue was empty
 *
 */
void *adequeue(AQUEUE_p_t aqueue, int ret_address);

/**
 * @brief checks if an item is already in the queue
 *
 * @param queue queue to search through
 * @return returns 1 if item is in queue, otherwise returns -1
 *
 */
int check_queue(QUEUE_p_t queue, void *data);

/**
 * @brief return the item from the head of a queue without removing the item from the queue
 *
 * @param queue queue to peek
 * @return the item that was at the head of the queue, or NULL if the queue was empty
 *
 */
void *peek(QUEUE_p_t queue);

/**
 * @brief print each node in a queue. Nodes must be ints
 *
 * @param list reference to list to print
 * Example: Queue(1, 2, 3, 4, 5)
 *          Queue() for empty list
 *
 */
void print_int_queue(QUEUE_p_t list);

/**
 * @brief remove all items from the queue
 *
 * @param queue reference queue to clear
 * @return returns 0 on success or -1 on failure
 *
 */
int clear(QUEUE_p_t list);

/**
 * @brief remove all items from the atomic queue
 *
 * @param queue reference queue to clear
 * @return returns 0 on success or -1 on failure
 *
 */
int aclear(AQUEUE_p_t list);

/**
 * @brief destroy the queue and set its pointer to NULL
 *
 * @param queue reference to queue to destroy
 * @return returns 0 on success or -1 on failure
 *
 */
int destroy(QUEUE_p_t queue);

/**
 * @brief destroy the atomic queue and set its pointer to NULL
 *
 * @param queue reference to queue to destroy
 * @return returns 0 on success or -1 on failure
 *
 */
int adestroy(AQUEUE_p_t aqueue);

#endif
