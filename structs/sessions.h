#ifndef SESSIONS_H
#define SESSIONS_H

#include <signal.h> /* for signal */
#include <stdatomic.h>
#include <stdint.h>
#include <time.h> /* for setitimer */

#include "aqueues.h"

#ifdef DEBUG
#define debug_printf(x) printf x
#else
#define debug_printf(x) ;
#endif

#define MAX_SESSIONS 100000

/**
 * @brief The temporal session object to be added to the sessions table. Each session exists for the set timeout length
 * before being removed from the sessions queue. On a successful login, a session is created, added to the sessions
 * queue, and sent back to the user. Whenever a request is received from the client, the received session id is also
 * looked up in the sessions queue. If found, the session's permissions value is referenced for operations permissions.
 *
 * Contains session ID, permissions, username, and username length
 */
typedef struct _session_t
{
    uint32_t session_id;
    uint8_t permissions;
    char *username;
    int username_len;
} session_t;

/**
 * @brief Initializes an empty sessions queue.
 *
 * @return returns pointer to the sessions queue on success. Otherwise returns NULL.
 */
QUEUE_t *create_sessions_queue(void);

/**
 * @brief When a user successfully authenticates, this function adds a session to the sessions queue. Sessions IDs are
 * pulled from an atomic integer. The session is only added if its integer value is not already in the queue. Otherwise
 * its number iterates until an available ID is found.
 *
 * @param permissions The authenticated user's permission level for the session
 * @param sessions The sessions queue holding session objects
 * @param username The username of the session user
 * @param username_len length of the username
 * @return returns a session ID number
 */
uint32_t add_session(uint8_t permissions, QUEUE_t *sessions, char *username, int username_len);

/**
 * @brief Dequeues a session from the sessions queue.
 *
 * @param sessions The sessions queue holding session objects
 * @return returns 0 on a successful session dequeue. Otherwise returns -1;
 */
int dequeue_session(QUEUE_t *p_sessions);

/**
 * @brief Checks if session exists on the queue.
 *
 * @param session_id the ID to verify
 * @param p_sessions The sessions queue holding session objects
 * @return If the session exists, returns the associated permission level, or 0 on failure
 */
uint8_t check_session(uint32_t session_id, QUEUE_t *p_sessions);

/**
 * @brief Checks if session exists on the queue.
 *
 * @param session_id the ID to look up/ verify
 * @param p_sessions The sessions queue holding session objects
 * @return If the session exists, returns the associated session struct, or NULL on failure
 */
session_t *find_session(uint32_t session_id, QUEUE_t *p_sessions);

/**
 * @brief Frees the allocated sessions queue from memory
 *
 * @param p_sessions The sessions queue holding session objects
 * @return returns 0 on successful free. Otherwise returns -1
 */
int destroy_sessions(QUEUE_t *p_sessions);

#endif

/*** end of file ***/
