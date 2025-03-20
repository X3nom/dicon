#pragma once
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>


// enum DIC_ASYNC_RESULT{ RUNNING, SUCCESS, ERROR };

//! SUPPORT FOR IPV6 NOT YET ADDED
typedef struct ip_addr{
    int family;  // AF_INET or AF_INET6
    union {
        uint32_t ipv4;
        uint16_t ipv6[8]; // can take both ipv4 and ipv6
    } addr;
} ip_addr_t;

#define NONE_IP_ADDR ((ip_addr_t){-1, {0}})

typedef struct dic_connection{
    int sockfd;

    pthread_mutex_t recv_mut;
    pthread_mutex_t send_mut;

    int port; // for restarting connection
    ip_addr_t ip;
} dic_conn_t;


typedef dic_conn_t dic_listen_conn_t;
typedef dic_conn_t dic_node_t;



/*IP ADDRESSES*/

ip_addr_t ip_from_str(int family, const char *ip_str);
ip_addr_t ipv4_from_str(const char *ip_str);
ip_addr_t ipv6_from_str(const char *ip_str);



/*CONNECTIONS*/


/** returns malloced dic_connection_t
 * - socket may be -1 (connection failed)
 * - recommended to check for `conn->sockfd != -1` or `dic_conn_tryconnect(conn) == 0` */
dic_conn_t *dic_conn_new(ip_addr_t ip, int port);

/* disconnects tcp connection
- DOES NOT DESTROY / FREE THE CONNECTION, USE `dic_conn_destroy` FOR THAT USECASEs */
int dic_conn_disconnect(dic_conn_t *conn);



/* tries to establish tcp connection
- returns `0` if connection is successful or already established
- returns `1` if connection fails */
int dic_conn_tryconnect(dic_conn_t *conn);

// destroy connection (disconnect + free)
void dic_conn_destroy(dic_conn_t *conn);

// `SEND_STR` replaces `int len` in `dic_conn_send` for automatic string length detection.
#define SEND_STR -1
/* Sends `message` of `len` characters. 
- if sending `\0` terminated string, use `SEND_STR` in place of `len`) */
int dic_conn_send(dic_conn_t *conn, const char *message, int len);

/* will recieve EXACTLY `n` bytes from `conn` into `buffer`
*/
int dic_conn_recv(dic_conn_t *conn, char *buffer, int n, bool null_terminate);



// LISTENING (server)

dic_listen_conn_t *dic_conn_new_listening(int port);

dic_conn_t *dic_conn_accept(dic_listen_conn_t *listen_conn);
