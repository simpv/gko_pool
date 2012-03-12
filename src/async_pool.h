/**
 *  async_pool.h
 *  gingko
 *
 *  Created on: Mar 9, 2012
 *      Author: auxten
 *
 **/
#ifndef ASYNC_POOL_H_
#define ASYNC_POOL_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <pthread.h>
#include <errno.h>

#include "event.h"

/// Thread worker
struct thread_worker
{
    int id;
    pthread_t tid;
    struct event_base *ev_base;
    struct event ev_notify;
    int notify_recv_fd;
    int notify_send_fd;
};

enum conn_states {
    conn_listening,  /**< the socket which listens for connections */
    conn_new_cmd,    /**< Prepare connection for next command */
    conn_waiting,    /**< waiting for a readable socket */
    conn_read,       /**< reading in a command line */
    conn_parse_cmd,  /**< try to parse a command from the input buffer */
    conn_write,      /**< writing out a simple response */
    conn_nread,      /**< reading in a fixed number of bytes */
    conn_swallow,    /**< swallowing unnecessary bytes w/o storing */
    conn_closing,    /**< closing this connection */
    conn_mwrite,     /**< writing out many items sequentially */
    conn_max_state   /**< Max state value (used for assertion) */
};

/// Connection client
struct conn_client
{
    int id;
    int client_fd;
    unsigned long client_addr;
    int client_port;
    unsigned int conn_time;
    func_t handle_client;
    struct event event;
    int state;

    char *read_buffer;
    unsigned int rbuf_size;
    unsigned int need_read;
    unsigned int have_read;

    char *write_buffer;
    unsigned int wbuf_size;
    unsigned int need_write;
    unsigned int have_write;

};

/// Connection server struct
struct conn_server
{
    char is_server;
    int listen_fd;
    struct sockaddr_in listen_addr;
    in_addr_t srv_addr;
    int srv_port;
    unsigned int start_time;
    int nonblock;
    int listen_queue_length;
    int tcp_send_buffer_size;
    int tcp_recv_buffer_size;
    int send_timeout;
    int tcp_reuse;
    int tcp_nodelay;
    struct event ev_accept;
    void (* on_data_callback)(int, void *, unsigned int);
};

class gko_pool
{
private:
    static gko_pool * _instance;
    /// FUNC DICT
    static char (* cmd_list_p)[CMD_LEN];
    ///server func list
    static func_t * func_list_p;
    /// cmd type conut
    static int cmd_count;
    /// global lock
    static pthread_mutex_t instance_lock;

    int g_curr_thread;
    struct thread_worker ** g_worker_list;
    struct event_base *g_ev_base;
    int g_total_clients;
    struct conn_client **g_client_list;
    struct conn_server *g_server;
    int port;
    s_option_t * option;

    static void conn_send_data(int fd, void *str, unsigned int len);
    /// Accept new connection
    static void conn_tcp_server_accept(int fd, short ev, void *arg);
    /// Close conn, shutdown && close
    static void * thread_worker_init(void *arg);
    /// Close conn, shutdown && close
    static void thread_worker_process(int fd, short ev, void *arg);
    /// Worker's event handler
    static void worker_event_handler(const int fd, const short which, void *arg);
    /// ReAdd the event
    static bool update_event(conn_client *c, const int new_flags);
    /// State updater
    static void conn_set_state(conn_client *c, enum conn_states state);
    /// Event handler for worker thread
    void event_handler(const int fd, const short which, void *arg);
    /// The drive machine of memcached
    static void state_machine(conn_client *c);
    /// Event on data from client
    static void conn_tcp_server_on_data(int fd, short ev, void *arg);
    /// Parse the request return the proper func handle num
    static int parse_req(char *req);

    int thread_worker_new(int id);
    int thread_list_find_next(void);
    int conn_client_list_init(void);
    int gingko_serv_async_server_base_init(void);
    int gingko_clnt_async_server_base_init(s_host_t * the_host);
    int gko_async_server_base_init(void);
    /// Accept new connection, start listen etc.
    int conn_tcp_server(struct conn_server *c);
    /// Accept new connection
    struct conn_client * add_new_conn_client(int client_fd);
    /// Event on data from client
    int conn_client_list_find_free();
    /// clear client struct
    int conn_client_clear(struct conn_client *client);
    /// clear the "session"
    int conn_client_free(struct conn_client *client);
    /// Get client object from pool by given client_id
    struct conn_client * conn_client_list_get(int id);
    /// Dispatch to worker
    void thread_worker_dispatch(int sig_id);
    /// init the whole thread pool
    int thread_init();
    /// construct func
    gko_pool(const int pt);
    /// another construct func
    gko_pool();

public:
    static s_host_t gko_serv;

    static gko_pool *getInstance();
    static void setFuncTable(char(*cmd_list)[CMD_LEN], func_t * func_list,
            int cmdcount);

    int getPort() const;
    void setPort(int port);
    s_option_t *getOption() const;
    void setOption(s_option_t *option);

    /// close conn, shutdown && close
    int conn_close();
    /// global run func
    int gko_run();
    int gko_loopexit(int timeout);

};

#endif /** ASYNC_POOL_H_ **/