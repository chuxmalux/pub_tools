#include "../include/aqueues.h"

/**
 * @brief create a queue with items
 * @param items items to initialize queue with (for empty queue pass NULL)
 * @param numItems number of items in items (for empty queue pass 0)
 * @return pointer to beginning of queue, or NULL on failure
 *
 */
QUEUE_p_t create_queue(void **items, int numItems)
{
    QUEUE_p_t ret = NULL;
    QUEUE_p_t new_queue = NULL;
    int count = 0;

    // Checks
    if (NULL == items) // Creates empty queue.
    {
    }
    if ((0 > numItems) || (MAX_QUEUE_NODES < numItems))
    {
        fprintf(stderr, "numItems out of range.\n");
        goto FAIL;
    }

    new_queue = (QUEUE_t *)calloc(1, sizeof(QUEUE_t));
    if (NULL == new_queue)
    {
        fprintf(stderr, "Failed to alloc new_queue.\n");
        goto FAIL;
    }
    new_queue->head = NULL;
    new_queue->tail = NULL;

    for (count = 0; count < numItems; count++)
    {
        if (-1 == enqueue(ret, &items[count]))
        {
            fprintf(stderr, "Failed to enqueue items in create_queue()");
            goto FAIL;
        }
    }
    ret = new_queue;
    goto END;

FAIL:
    free(new_queue);
    new_queue = NULL;

END:
    return ret;
}

/**
 * @brief create an atomic queue with items
 *
 * @param items items to initialize queue with (for empty queue pass NULL)
 * @param numItems number of items in items
 * @return pointer to beginning of queue or NULL on failure
 *
 */
AQUEUE_p_t create_aqueue(void **items, int numItems)
{
    AQUEUE_p_t ret = NULL;
    AQUEUE_p_t new_queue = NULL;

    int count = 0;
    int check = 0;

    // Checks
    if (NULL == items) // Creates empty queue.
    {
    }
    if ((0 > numItems) || (MAX_QUEUE_NODES < numItems))
    {
        fprintf(stderr, "numItems out of range.\n");
        goto FAIL;
    }

    new_queue = (AQUEUE_t *)calloc(1, sizeof(AQUEUE_t));
    if (NULL == new_queue)
    {
        fprintf(stderr, "Failed to alloc new_queue.\n");
        goto FAIL;
    }
    new_queue->head = NULL;
    new_queue->tail = NULL;

    check = pthread_mutex_init(&new_queue->lock, NULL);
    if (0 != check)
    {
        fprintf(stderr, "Error initializing mutex.\n");
        goto END;
    }

    for (count = 0; count < numItems; count++)
    {
        if (-1 == aenqueue(ret, &items[count]))
        {
            fprintf(stderr, "Failed to enqueue items in create_queue()");
            goto FAIL;
        }
    }
    ret = new_queue;
    goto END;

FAIL:
    free(new_queue);
    new_queue = NULL;

END:
    return ret;
}

/**
 * @brief enqueue an item to the tail of a queue
 *
 * @param queue queue to add item to
 * @param item item to add to queue
 * @return returns 0 on success or -1 on failure
 */
int enqueue(QUEUE_p_t queue, void *item)
{
    int ret = -1;
    Q_NODE_p_t newnode = NULL;

    if (NULL == queue)
    {
        fprintf(stderr, "Invalid queue passed. Exiting... is null?!\n");
        goto FAIL;
    }

    newnode = (Q_NODE_t *)calloc(1, sizeof(Q_NODE_t));
    if (NULL == newnode)
    {
        fprintf(stderr, "Failed to alloc newnode.\n");
        goto FAIL;
    }
    newnode->data = item;

    if (queue->tail != NULL)
    {
        queue->tail->next = newnode;
    }

    queue->tail = newnode; // this was below/ not in this else...

    // Ensures head is set
    if (queue->head == NULL)
    {
        queue->head = newnode;
    }
    ret = 0;
    goto END;
FAIL:
    free(newnode);
    newnode = NULL;
END:
    return ret;
}

/**
 * @brief enqueue an item to the tail of an atomic queue
 *
 * @param queue queue to add item to
 * @param item item to add to queue
 * @return returns 0 on success or -1 on failure
 */
int aenqueue(AQUEUE_p_t aqueue, void *item)
{
    int ret = -1;
    Q_NODE_p_t newnode = NULL;

    if (NULL == aqueue)
    {
        fprintf(stderr, "Invalid queue passed.\n");
        goto END;
    }

    newnode = (Q_NODE_t *)calloc(1, sizeof(Q_NODE_t));
    if (NULL == newnode)
    {
        fprintf(stderr, "Failed to alloc newnode.\n");
        goto FAIL;
    }

    pthread_mutex_lock(&aqueue->lock);
    if ((NULL == aqueue) || (NULL == item) || (aqueue->num_nodes > MAX_QUEUE_NODES))
    {
        fprintf(stderr, "Failed aenqueue()\n");
        free(newnode);
        newnode = NULL;
    }
    else
    {
        newnode->data = item;

        if (aqueue->tail != NULL)
        {
            aqueue->tail->next = newnode;
        }

        aqueue->tail = newnode;

        // Ensures head is set
        if (aqueue->head == NULL)
        {
            aqueue->head = newnode;
        }
        aqueue->num_nodes++;
    }
    pthread_mutex_unlock(&aqueue->lock);

    ret = 0;
    goto END;

FAIL:
    free(newnode);
    newnode = NULL;

END:
    return ret;
}

/**
 * @brief dequeue an item from the head of a queue
 *
 * @param queue queue to add item to
 * @param ret_address if set to 1, returns the address of dequeued item. Otherwise returns the item.
 * @return the item that was at the head of the queue, or NULL if the queue was empty
 *
 */
void *dequeue(QUEUE_p_t queue, int ret_address)
{
    void *ret = NULL;

    if (NULL == queue)
    {
        goto END;
    }
    if (NULL == queue->head) // Check if queue is empty
    {
        goto END;
    }

    Q_NODE_p_t temp = queue->head; // Save the head of queue

    if (ret_address == 1)
    {
        ret = &(temp->data); // Save the address of data to return **** remember this for later? Think inverse of
                             // sending the address, need to return it!
    }
    else
    {
        ret = (temp->data);
    }

    queue->head = queue->head->next; // Remove the head
    if (NULL == queue->head)
    {
        queue->tail = NULL;
    }

END:
    return ret;
}

/**
 * @brief dequeue an item from the head of an atomic queue
 *
 * @param queue queue to add item to
 * @param ret_address if set to 1, returns the address of dequeued item. Otherwise returns the item.
 * @return the item that was at the head of the queue, or NULL if the queue was empty
 *
 */
void *adequeue(AQUEUE_p_t aqueue, int ret_address)
{
    void *ret = NULL;

    if (NULL == aqueue)
    {
        goto END;
    }
    if (0 == aqueue->num_nodes) // two checks exist to prevent time of use vs time of check issue
    {
        fprintf(stderr, "cannot dequeue with 0 num_nodes.\n");
        goto END;
    }

    pthread_mutex_lock(&aqueue->lock);
    if ((NULL == aqueue) || (0 == aqueue->num_nodes) || (NULL == aqueue->head))
    {
        fprintf(stderr, "failed adequeue(). invalid parameters.\n");
    }
    else
    {
        Q_NODE_p_t temp = aqueue->head; // Save the head of queue

        if (ret_address == 1)
        {
            ret = &temp; // Save the address of data to return
        }
        else
        {
            ret = temp;
        }

        aqueue->head = aqueue->head->next; // Remove the head
        if (NULL == aqueue->head)
        {
            aqueue->tail = NULL;
        }
        aqueue->num_nodes--;
    }
    pthread_mutex_unlock(&aqueue->lock);

END:
    return ret;
}

/**
 * @brief checks if an item is already in the queue
 *
 * @param queue queue to search through
 * @return returns 1 if item is in queue, otherwise returns -1
 *
 */
int check_queue(QUEUE_p_t queue, void *data)
{
    int ret = -1;
    Q_NODE_p_t temp = NULL;

    if (NULL == queue)
    {
        fprintf(stderr, "Queue is empty. Exiting check_queue.\n");
        goto END;
    }
    else if (NULL == queue->head) // if queue has no nodes, exit
    {
        goto END;
    }

    temp = queue->head;
    while (temp != queue->tail->next)
    {
        if (temp->data == data)
        {
            ret = 1;
            goto END;
        }
        temp = temp->next;
    }

END:
    return ret;
}

/**
 * @brief return the item from the head of a queue without removing the item from the queue
 *
 * @param queue queue to peek
 * @return the item that was at the head of the queue, or NULL if the queue was empty
 *
 */
void *peek(QUEUE_p_t queue)
{
    void *ret = NULL;
    if (NULL == queue->head)
    {
        perror("Queue is empty. Exiting peek.\n");
    }
    else
    {
        ret = queue->head->data;
    }

    return ret;
}

/**
 * @brief print each node in a queue. Nodes must be ints.
 *
 * @param list reference to list to print
 * Example: Queue(1, 2, 3, 4, 5)
 *          Queue() for empty list
 *
 */
void print_int_queue(QUEUE_p_t list)
{
    Q_NODE_p_t temp = NULL;

    if (NULL == list->head)
    {
        fprintf(stdout, "Queue()\n");
    }
    else
    {
        fprintf(stdout, "Queue(");
        temp = list->head;

        while (temp != NULL)
        {
            if (NULL == temp->next)
            {
                fprintf(stdout, "%d)\n", *(int *)temp->data);
                temp = temp->next;
            }
            else
            {
                fprintf(stdout, "%d, ", *(int *)temp->data);
                temp = temp->next;
            }
        }
    }

    return;
}

/**
 * @brief remove all items from the queue
 *
 * @param queue reference queue to clear
 * @return returns 0 on success or -1 on failure
 *
 */
int clear(QUEUE_p_t list)
{
    int ret = -1;
    Q_NODE_p_t temp = NULL;

    if (NULL == list)
    {
        fprintf(stderr, "Queue is empty. Exiting clear.\n");
        goto END;
    }

    while (list->head != NULL)
    {
        temp = dequeue(list, 1);
        free(temp);
        temp = NULL;
    }

    if (NULL == list->head)
    {
        list->tail = NULL;
    }

    ret = 0;
END:
    return ret;
}

/**
 * @brief remove all items from the atomic queue
 *
 * @param queue reference queue to clear
 * @return returns 0 on success or -1 on failure
 *
 */
int aclear(AQUEUE_p_t list)
{
    {
        int ret = -1;
        Q_NODE_p_t temp = NULL;

        if (NULL == list)
        {
            fprintf(stderr, "Queue is empty. Exiting clear.\n");
            goto END;
        }

        while (list->head != NULL)
        {
            temp = (Q_NODE_p_t)adequeue(list, 0);

            free(temp);
            temp = NULL;
        }

        if (NULL == list->head)
        {
            list->tail = NULL;
        }

        ret = 0;
    END:
        return ret;
    }
}

/**
 * @brief destroy the queue and set its pointer to NULL
 *
 * @param queue reference to queue to destroy
 * @return returns 0 on success or -1 on failure
 */
int destroy(QUEUE_p_t queue)
{
    int ret = -1;

    if (NULL == queue)
    {
        fprintf(stderr, "Queue is empty. Exiting destroy.\n");
        goto END;
    }

    if (-1 == clear(queue))
    {
        fprintf(stderr, "Failed to clear queue.\n");
        goto END;
    }

    ret = 0;

END:
    free(queue);
    queue = NULL;

    return ret;
}

/**
 * @brief destroy the atomic queue and set its pointer to NULL
 *
 * @param queue reference to queue to destroy
 * @return returns 0 on success or -1 on failure
 *
 */
int adestroy(AQUEUE_p_t aqueue)
{
    int ret = -1;

    if (NULL == aqueue)
    {
        fprintf(stderr, "Queue is empty. Exiting destroy.\n");
        goto END;
    }

    if (-1 == aclear(aqueue))
    {
        fprintf(stderr, "Failed to clear queue.\n");
        goto END;
    }
    pthread_mutex_destroy(&(aqueue->lock));

    ret = 0;
END:
    free(aqueue);
    aqueue = NULL;
    return ret;
}

/*** end of file ***/
