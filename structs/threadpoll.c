#include "../include/threadpoll.h"
#include "../include/some_server.h"

/**
 * @brief The main thread loop. Until receiving the signal shutdown, polls for incoming connections. After receiving a
 * connection, passes the fd into an atomic queue for the polling threads to receive and act upon.
 *
 * @param main_args The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 * @param num_threads The supplied number of threads from the command line (getopt)
 * @param poll_args The instance of main_args passed to the polling thread functions
 * @return returns 0 on successful cleanup, or -1 on failure
 */
int main_loop(main_data_t *main_data_args, poll_data_t *p_poll_args, int num_threads, sig_atomic_t server_shutdown)
{
    int ret = -1;
    int thread_index = 0;
    int client_sockfd = 0;
    int poll_ret = 0;
    int poll_timeout = OS_TIMESLICE; // set to 100 m/s; a general OS scheduling timeslice
    nfds_t nfds = 1;                 // only require 1 poll fd in main for the server socket
    struct pollfd poll_fds[1] = {0};
    queue_data_t p_queue_args = {0};

    for (thread_index = 0; thread_index < num_threads;
         ++thread_index) // initialize each thread running poll and some_server 
    {
        thread_task(main_data_args->tpool, (thread_func)poll_func, (void *)p_poll_args);
    }
    poll_fds[0].fd = main_data_args->server_sockfd; // setup poll_fds
    poll_fds[0].events = POLLIN | POLLERR | POLLRDHUP;

    while (1 != server_shutdown) // Server functionality
    {
        poll_ret = poll(poll_fds, nfds, poll_timeout);
        if (poll_ret < 0)
        {
            perror("poll()");
            goto END;
        }
        if (poll_ret == 0)
        {
            continue;
        }
        else
        {
            client_sockfd = client_accept(main_data_args->server_sockfd);

            p_queue_args.client_sockfd = client_sockfd; // set p_queue_args arguments (currently just fd)
            // atomic_enqueue the fd and arguments
            debug_printf(("Sending polls a new conn.\n"));
            if (-1 == aenqueue(main_data_args->poll_fd_queue, &p_queue_args))
            {
                fprintf(stderr, "Failed aenqueue()\n");
                goto END;
            }
        }
    }

    ret = 0;
END:
    return ret;
}

/**
 * @brief The polling function within each thread. Each thread actively checks an atomic queue for new connections,
 * otherwise polling existing fd connections. Upon polling readable connections, performs the desired server operation.
 * This allows asynchronous IO across each thread's poll.
 *
 * @param args The client args struct passed as a void pointer
 */
void poll_func(void *args)
{
    poll_data_t *p_poll_args = NULL;
    AQUEUE_t *poll_fd_queue = NULL;
    client_data_t *p_client_args = NULL;
    queue_data_t *p_queue_args = NULL;
    Q_NODE_t *temp = NULL;
    struct pollfd poll_fds[MAX_FDS] = {0};
    size_t num_fds = 1;
    int poll_ret = 0;
    int poll_index = 0;
    int poll_timeout = 100;
    nfds_t nfds = 1; // begin poll array with 1 fd

    if (NULL == args)
    {
        fprintf(stderr, "Failed to pass client args.\n");
        goto END;
    }

    p_poll_args = (poll_data_t *)args;
    poll_fd_queue = p_poll_args->aqueue;
    p_client_args = p_poll_args->client_args;

    // setup poll_fds
    for (poll_index = 0; poll_index < MAX_FDS; poll_index++)
    {
        poll_fds[poll_index].fd = -1;
        poll_fds[poll_index].events = POLLIN | POLLERR | POLLRDHUP;
    }

    while (true == running) // poll functionality
    {

        if ((num_fds < MAX_FDS) && (poll_fd_queue->num_nodes > 0))
        {
            temp = adequeue(poll_fd_queue, 0); // peek here, then dequeue after some server
                                               // p_poll_args = (Q_NODE_t *)p_poll_args;
            if (NULL == temp)
            {
                continue;
            }

            p_queue_args = (queue_data_t *)temp->data;

            p_client_args->client_sockfd = p_queue_args->client_sockfd;

            // check if num_fds = -1, if so add
            for (poll_index = 0; poll_index < num_fds; poll_index++)
            {
                if (-1 == poll_fds[poll_index].fd)
                {
                    poll_fds[poll_index].fd = p_queue_args->client_sockfd;
                    break;
                }
            }

            num_fds++;
            nfds = num_fds;
            free(temp);
            temp = NULL;
        }

        poll_ret = poll(poll_fds, nfds, poll_timeout); // timeout set to 100 m/s so not blocking & can exit out
        if (0 > poll_ret)
        {
            perror("poll() error'd");
            goto END;
        }

        if (0 == poll_ret) // poll timeout; waiting for event
        {
            continue;
        }

        for (size_t iter = 0; iter < num_fds; iter++)
        {
            if (POLLERR == (poll_fds[iter].revents & POLLERR))
            {
                fprintf(stderr, "ERROR.\n");
                poll_fds[iter].fd = -1;
            }
            else if ((POLLHUP == (poll_fds[iter].revents & POLLHUP)) ||
                     (POLLRDHUP == (poll_fds[iter].revents & POLLRDHUP)))
            {
                debug_printf(("Client hung up.\n"));
                close(poll_fds[iter].fd);
                poll_fds[iter].fd = -1;
            }
            else if (POLLIN == (poll_fds[iter].revents & POLLIN))
            {
                p_client_args->client_sockfd = poll_fds[iter].fd;
                some_server(p_client_args); // perform server functionality
            }
            else
            {
                debug_printf(("poll() waiting."));
                continue;
            }
        }
    }

END:
    return;
}

/**
 * @brief Allocates an instance of the main_args structs necessary to be passed into the polling thread functions
 *
 * @param main_args The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 * @return Returns the poll_data struct
 */
poll_data_t *init_p_poll_args(main_data_t *main_args)
{
    poll_data_t *ret = NULL;
    poll_data_t *temp_args = NULL;

    if (NULL == main_args)
    {
        fprintf(stderr, "Null main_args.\n");
        goto FAIL;
    }

    temp_args = calloc(1, sizeof(poll_data_t));
    if (NULL == temp_args)
    {
        fprintf(stderr, "Failed to alloc p_poll_args\n");
        goto END;
    }

    temp_args->client_args = calloc(1, sizeof(client_data_t));
    if (NULL == temp_args->client_args)
    {
        fprintf(stderr, "Failed to alloc p_poll_args->client_args");
        goto FAIL;
    }

    temp_args->aqueue = main_args->poll_fd_queue;
    temp_args->client_args->p_auth_table = main_args->p_auth_table;
    temp_args->client_args->p_storage_table = main_args->p_storage_table;
    temp_args->client_args->p_sessions = main_args->p_sessions;
    temp_args->client_args->root_dir_fd = main_args->root_dir_fd;

    ret = temp_args;
    goto END;
FAIL:
    free(main_args);
    main_args = NULL;

    if (NULL != temp_args)
    {
        free(temp_args->client_args);
        temp_args->client_args = NULL;
        free(temp_args);
        temp_args = NULL;
    }

END:

    return ret;
}

/**
 * @brief Initialize the the data structs and variables used throughout the program; the authentication hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 *
 * @param p_port The supplied server port from the command line (getopt)
 * @param p_base_dir The supplied server directory from the command line (getopt)
 * @param num_threads The supplied number of threads from the command line (getopt)
 * @return The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 */
main_data_t *init_main_data(char *p_port, char *p_base_dir, int num_threads)
{
    main_data_t *ret = NULL;
    main_data_t *new_main_data = NULL;

    new_main_data = calloc(1, sizeof(main_data_t));
    if (NULL == new_main_data)
    {
        fprintf(stderr, "Failed to alloc new_main_data\n");
        goto FAIL;
    }

    // checks
    new_main_data->p_auth_table = create_hashtable((hash_func)djb2); // Authentication table setup
    if (NULL == new_main_data->p_auth_table)
    {
        fprintf(stderr, "Failed to create authentication table. Exiting.");
        goto FAIL;
    }

    new_main_data->p_storage_table = create_hashtable((hash_func)djb2); // Authentication table setup
    if (NULL == new_main_data->p_storage_table)
    {
        fprintf(stderr, "Failed to create storage table. Exiting.");
        goto FAIL;
    }

    new_main_data->p_sessions = create_sessions_queue(); // Sessions setup
    if (NULL == new_main_data->p_sessions)
    {
        fprintf(stderr, "Failed to create sessions queue. Exiting.\n");
        goto FAIL;
    }

    new_main_data->tpool = create_threadpool(num_threads); // Threadpool setup
    if (NULL == new_main_data->tpool)
    {
        fprintf(stderr, "Failed to allocate threadpool.\n");
        goto FAIL;
    }
    threads_init(new_main_data->tpool, &thread_spin, num_threads);

    if (NULL == p_base_dir)
    {
        fprintf(stderr, "Invalid root dir set. Exiting.\n");
        goto FAIL;
    }

    new_main_data->poll_fd_queue = calloc(1, sizeof(AQUEUE_t)); // poll queue setup
    if (NULL == new_main_data->poll_fd_queue)
    {
        fprintf(stderr, "Failed to init poll_fd_queue");
        goto FAIL;
    }

    new_main_data->root_dir_fd = open(p_base_dir, O_PATH); // root directory setup
    if (-1 == new_main_data->root_dir_fd)
    {
        perror("Failed to open the directory.\n");
        goto FAIL;
    }

    if (-1 == authentication_setup(new_main_data->p_auth_table, new_main_data->root_dir_fd))
    {
        fprintf(stderr, "Failed to set up authentication table. Exiting.\n");
        goto FAIL;
    }

    new_main_data->server_sockfd = init_server_tcp(p_port, 1); // server setup
    if (-1 == new_main_data->server_sockfd)
    {
        fprintf(stderr, "Failed init_server_tcp()");
        goto FAIL;
    }

    ret = new_main_data;
    goto END;
FAIL:
    if (-1 == main_cleanup(new_main_data, num_threads, NULL))
    {
        fprintf(stderr, "Failed new main args cleanup.\n");
    }

END:
    return ret;
}

/**
 * @brief Cleans up the structs initialized in main; main_data_args and p_poll_args
 *
 * @param main_args The main data struct containing the auth hash
 * table, sessions queue, threadpool, poll queue, server, and directory fd.
 * @param num_threads The supplied number of threads from the command line (getopt)
 * @param poll_args The instance of main_args passed to the polling thread functions
 * @return returns 0 on successful cleanup, or -1 on failure
 */
int main_cleanup(main_data_t *main_args, int num_threads, poll_data_t *poll_args)
{
    int ret = -1;
    if (NULL == main_args)
    {
        debug_printf(("Null main_args\n"));
        goto END;
    }

    if (-1 == shutdown_threadpool(main_args->tpool, num_threads))
    {
        fprintf(stderr, "Failed to shutdown threadpool\n");
        goto END;
    }
    if (-1 == destroy_threadpool(main_args->tpool))
    {
        fprintf(stderr, "Failed to destroy threadpool\n");
        goto END;
    }
    if (-1 == destroy_sessions(main_args->p_sessions))
    {
        fprintf(stderr, "Failed to destroy sessions queue.\n");
        goto END;
    }

    if (-1 == dump_table(main_args->p_auth_table, main_args->root_dir_fd, "auth_users", 1))
    {
        fprintf(stderr, "Failed to dump auth_table.\n");
        goto END;
    }
    if (-1 == empty_authtable(main_args->p_auth_table))
    {
        fprintf(stderr, "Failed to destroy sessions authentication table.\n");
        goto END;
    }
    if (-1 == empty_storagetable(main_args->p_storage_table))
    {
        fprintf(stderr, "Failed to destroy sessions authentication table.\n");
        goto END;
    }
    if (-1 == adestroy(main_args->poll_fd_queue))
    {
        fprintf(stderr, "Failed to destroy sessions authentication table.\n");
        goto END;
    }
    close(main_args->root_dir_fd);
    close(main_args->server_sockfd);

    if (NULL != poll_args)
    {
        free(poll_args->client_args);
        poll_args->client_args = NULL;

        free(poll_args);
        poll_args = NULL;
    }
    ret = 0;
END:
    free(main_args);
    main_args = NULL;

    return ret;
}

/**
 * @brief Reads arguments passed in from the commandline
 *
 * @param opt the opt code for the arguments passed
 * @param num_threads the number of threads to run for the server
 * @param port The supplied server port
 * @param base_dir The supplied file server base directory
 * @return If arguments are successfully read returns 0. Otherwise returns 1.
 */
int read_args(int opt, uint32_t *num_threads, char **port, char **base_dir)
{
    int b_ret = -1;
    uint32_t port_check = 0;
    char *p_opt_arg = NULL;
    switch (opt)
    {
    case 'd':
        *base_dir = optarg;
        if (0 == strncmp(*base_dir, "/", 2))
        {
            fprintf(stderr, "Cannot set the chat log directory to local root directory.\n");
            goto END;
        }
        break;
    case 'p':
        *port = optarg;
        port_check = strtol(*port, &p_opt_arg, 10);
        if (0 != *p_opt_arg)
        {
            fprintf(stderr, "Failed to convert string. Exiting\n");
            goto END;
        }
        else if ((0 > port_check) || (65535 < port_check))
        {
            fprintf(stderr, "Invalid port range. Please try another port.\n");
            goto END;
        }

        break;
    case 'n':
        *num_threads = strtol(optarg, &p_opt_arg, 10);
        if (0 != *p_opt_arg)
        {
            fprintf(stderr, "Failed to convert string. Exiting\n");
            goto END;
        }
        else if (0 == num_threads)
        {
            fprintf(stderr, "Cannot set 0 threads. Exiting.\n\n");
            goto END;
        }
        break;
    case 'h':
        fprintf(stdout, "file transfer capstone - secure file transfer service\n\nUsage: capstone "
                        "[options...]\n\n\t-d\tset the server's root directory\n\t-p\tset the server's "
                        "port\n\t-n\tset the number of server threads\n\n");
    default:
        debug_printf(("Invalid option passed.\n"));
        goto END;
    }

    b_ret = 0;
END:
    return b_ret;
}

/**
 * @brief Checks that mandatory arguments were supplied
 *
 * @param port The supplied server port
 * @param base_dir The supplied file server base directory
 * @param num_threads the supplied number of server threads
 * @return If arguments are successfully supplied returns 0. Otherwise returns 1.
 */
int args_check(char *port, char *base_dir, int num_threads)
{
    int ret = 1;

    if (NULL == base_dir)
    {
        fprintf(stderr, "Chat log directory required. Exiting.\n");
        goto END;
    }
    if (0 >= num_threads)
    {
        fprintf(stderr, "Invalid number of threads.\n");
        goto END;
    }

    ret = 0;
END:
    return ret;
}

/*** end of file ***/threadpoll/
