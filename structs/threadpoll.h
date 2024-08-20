#ifndef MAIN_FUNCS_H
#define MAIN_FUNCS_H

// #include "some_server.h"

#define DEFAULT_PORT "8989"
#define DEFAULT_THREADS 4
#define MAIN_OS_TIMESLICE 100000000L // 100 m/s

/**
 * @brief a struct to store all initialized server structures and variables
 */
typedef struct main_data
{
    hash_table_t *p_auth_table;
    hash_table_t *p_storage_table;
    QUEUE_t *p_sessions;
    thpool *tpool;
    AQUEUE_t *poll_fd_queue;
    int root_dir_fd;
    int server_sockfd;
} main_data_t;

/**
 * @brief a struct to hold client_data (operational) arguments, and the atomic qeueue checked by the polling thread
 * functions
 */
typedef struct _poll_data // passed to the threaded poll func
{
    AQUEUE_t *aqueue;
    client_data_t *client_args;
} poll_data_t;

/**
 * @brief a struct to hold client_data (operational) arguments, and the client socket descriptor
 */
typedef struct _queue_data // passed to the threaded poll func
{
    int client_sockfd;
    // could also include additional socket information for logging
} queue_data_t;

/**
 * @brief The main thread loop. Until receiving the signal shutdown, polls for incoming connections. After receiving a
 * connection, passes the fd into an atomic queue for the polling threads to receive and act upon.
 *
 * @param main_args The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 * @param num_threads The supplied number of threads from the command line (getopt)
 * @param poll_args The instance of main_args passed to the polling thread functions
 * @param server_shutdown the global signal server_shutdown variable, checked for server termination
 * @return returns 0 on successful cleanup, or -1 on failure
 */
int main_loop(main_data_t *main_data_args, poll_data_t *p_poll_args, int num_threads, sig_atomic_t server_shutdown);

/**
 * @brief Cleans up the structs initialized in main; main_data_args and p_poll_args
 *
 * @param main_args The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 * @param num_threads The supplied number of threads from the command line (getopt)
 * @param poll_args The instance of main_args passed to the polling thread functions
 * @return returns 0 on successful cleanup, or -1 on failure
 */
int main_cleanup(main_data_t *main_args, int num_threads, poll_data_t *poll_args);

/**
 * @brief Allocates an instance of the main_args structs necessary to be passed into the polling thread functions
 *
 * @param main_args The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 * @return Returns the poll_data struct
 */
poll_data_t *init_p_poll_args(main_data_t *main_args);

/**
 * @brief Initialize the data structs and variables used throughout the program; the authentication hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 *
 * @param p_port The supplied server port from the command line (getopt)
 * @param p_base_dir The supplied server directory from the command line (getopt)
 * @param num_threads The supplied number of threads from the command line (getopt)
 * @return The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 */
main_data_t *init_main_data(char *p_port, char *p_base_dir, int num_threads);

/**
 * @brief The polling function within each thread. Each thread actively checks an atomic queue for new connections,
 * otherwise polling existing fd connections. Upon polling readable connections, performs the desired server operation.
 *
 * @param args The client args struct passed as a void pointer
 */
void poll_func(void *args);

/**
 * @brief Reads arguments passed in from the commandline
 *
 * @param opt the opt code for the arguments passed **** EDIT!!!
 * @param num_threads the number of threads to run for the server
 * @param port The supplied server port
 * @param base_dir The supplied file server base directory
 * @return If arguments are successfully read returns 0. Otherwise returns -1.
 */
int read_args(int opt, uint32_t *num_threads, char **port, char **base_dir);

/**
 * @brief Checks that mandatory arguments were supplied
 *
 * @param port The supplied server port
 * @param base_dir The supplied file server base directory
 * @param num_threads the supplied number of server threads
 * @return If arguments are successfully supplied returns 0. Otherwise returns 1.
 */
int args_check(char *port, char *base_dir, int num_threads);

#endif

/*** end of file ***/
