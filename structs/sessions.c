#include "../include/sessions.h"

#define _GNU_SOURCE

atomic_int session_number = 1;

/**
 * @brief Initializes an empty sessions queue.
 *
 * @return returns pointer to the sessions queue on success. Otherwise returns NULL.
 */
QUEUE_t *create_sessions_queue(void)
{
    QUEUE_t *p_ret = NULL;
    QUEUE_t *p_sessions = NULL;

    p_sessions = create_queue(NULL, 0);
    if (NULL == p_sessions)
    {
        fprintf(stderr, "Failed to create sessions queue. Exiting.\n");
        goto END;
    }

    p_ret = p_sessions;
END:
    return p_ret;
}

/**
 * @brief When a user successfully authenticates, this function adds a session to the sessions queue. Sessions IDs are
 * pulled from an atomic integer. The session is only added if its integer value is not already in the queue. Otherwise
 * its number iterates until an available ID is found.
 *
 * @param permissions The authenticated user's permission level for the session
 * @param sessions The sessions queue holding session objects
 * @param username The username of the session user
 * @param username_len length of the username
 * @return returns a session ID number or 0 on failure
 */
uint32_t add_session(uint8_t permissions, QUEUE_t *p_sessions, char *username, int username_len)
{
    debug_printf(("Creating new session.\n"));

    uint32_t ret = 0;
    int random_num = 0;
    session_t *new_session = NULL;

    random_num = random();
    session_number = random_num % MAX_SESSIONS;

    while (1 == check_queue(p_sessions, &session_number)) // This prevents reusing same session_id
    {
        session_number = ((session_number + 1) % MAX_SESSIONS); // move to next possible ID
        debug_printf(("session already exists. Incremented to %d\n", session_number));
    }

    new_session = calloc(1, sizeof(session_t));
    if (NULL == new_session)
    {
        fprintf(stderr, "Failed to alloc new_session.\n");
        goto END;
    }

    new_session->session_id = session_number;
    new_session->permissions = permissions;
    new_session->username = username;
    new_session->username_len = username_len;
    if (-1 == enqueue(p_sessions, new_session))
    {
        fprintf(stderr, "Failed enqueue()\n");
        goto END;
    }
    debug_printf(("\nSession(%d) created.\n", session_number));
    ret = session_number;
    session_number = ((session_number + 1) % MAX_SESSIONS); // increment next ID

    debug_printf(("RETURNING - Session(%d).\n", ret));

END:
    return ret;
}

/**
 * @brief Dequeues a session from the sessions queue.
 *
 * @param sessions The sessions queue holding session objects
 * @return returns 0 on a successful session dequeue. Otherwise returns -1;
 */
int dequeue_session(QUEUE_t *p_sessions)
{
    int ret = -1;

    session_t *expired_session = NULL;

    expired_session = (session_t *)dequeue(p_sessions, 1);

    if (NULL == expired_session)
    {
        debug_printf(("Failed to dequeue the session.\n"));
    }
    else
    {
        free(expired_session);
        expired_session = NULL;
        ret = 0;
    }
    return ret;
}

/**
 * @brief Checks if session exists on the queue.
 *
 * @param p_sessions The sessions queue holding session objects
 * @return If the session exists, returns the associated permission level or 0 on failure
 */
uint8_t check_session(uint32_t session_id, QUEUE_t *p_sessions)
{
    uint8_t ret = 0;
    Q_NODE_t *temp = NULL;
    session_t *match = NULL;

    if (NULL == p_sessions)
    {
        fprintf(stderr, "Queue is empty. Exiting check_queue.\n");
    }
    else if (NULL == p_sessions->head)
    {
        debug_printf(("Queue is empty check_queue. Exiting.\n"));
    }
    else
    {
        temp = p_sessions->head;
        while (temp != p_sessions->tail->next)
        {
            match = (session_t *)temp->data;
            debug_printf(("Comparing (%d) to sessions_table %d value.\n", session_id, match->session_id));
            if (match->session_id == session_id)
            {
                ret = match->permissions;
                debug_printf(("Session found. Returning perms: %d\n", ret));
                goto END;
            }
            temp = temp->next;
        }
    }
    debug_printf(("Session not found.\n"));

END:
    return ret;
}

/**
 * @brief Checks if session exists on the queue.
 *
 * @param session_id the ID to look up/ verify
 * @param p_sessions The sessions queue holding session objects
 * @return If the session exists, returns the associated session struct, or NULL on failure
 */
session_t *find_session(uint32_t session_id, QUEUE_t *p_sessions)
{
    session_t *ret = NULL;
    Q_NODE_t *temp = NULL;
    session_t *match = NULL;

    if (NULL == p_sessions)
    {
        fprintf(stderr, "Queue is empty. Exiting check_queue.\n");
    }
    else if (NULL == p_sessions->head)
    {
        debug_printf(("Queue is empty check_queue. Exiting.\n"));
    }
    else
    {
        temp = p_sessions->head;
        while (temp != p_sessions->tail->next)
        {
            match = (session_t *)temp->data;
            debug_printf(("Comparing (%d) to sessions_table %d value.\n", session_id, match->session_id));
            if (match->session_id == session_id)
            {
                ret = match;
                debug_printf(("Session found. Returning perms: %d\n", match->permissions));
                goto END;
            }
            temp = temp->next;
        }
    }
    debug_printf(("Session not found.\n"));

END:
    return ret;
}

/**
 * @brief Frees the allocated sessions queue from memory
 *
 * @param p_sessions The sessions queue holding session objects
 * @return returns 0 on successful free. Otherwise returns -1
 */
int destroy_sessions(QUEUE_t *p_sessions)
{
    int ret = -1;
    Q_NODE_t *temp_node = NULL;
    session_t *temp_session = NULL;
    Q_NODE_t *start = NULL;

    if (NULL == p_sessions)
    {
        fprintf(stderr, "Sessions queue is already NULL. Exiting.\n");
        goto END;
    }

    start = p_sessions->head;
    while (start != NULL)
    {
        temp_node = start;
        start = start->next;

        temp_session = (session_t *)temp_node->data;
        if (temp_session)
        {
            free(temp_session->username);
            temp_session->username = NULL;
            free(temp_session);
            temp_session = NULL;
        }
        free(temp_node);
        temp_node = NULL;
    }

    free(p_sessions);
    p_sessions = NULL;

    ret = 0;
END:
    return ret;
}

/*** end of file ***/