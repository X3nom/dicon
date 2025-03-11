#pragma once
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


typedef struct dic_connection{
    int sockfd;

    pthread_mutex_t recv_mut;
    pthread_mutex_t send_mut;

    int port; // for restarting connection
    char ip[48];
} dic_connection_t;


typedef dic_connection_t dic_server_t;
typedef dic_connection_t dic_node_t;





int dic_conn_disconnect(dic_connection_t *conn);

/** returns malloced dic_connection_t
 * - socket may be -1 (connection failed)
 * - recommended to check for `conn->sockfd != -1` or `dic_conn_tryconnect(conn) == 0` */
dic_connection_t *dic_conn_new(char *ip, int port);

int dic_conn_tryconnect(dic_connection_t *conn);

void dic_conn_destroy(dic_connection_t *conn);

int dic_conn_send(dic_connection_t *conn, const char *message);

int dic_conn_recv(dic_connection_t *conn, char *buffer, int buffer_size);



