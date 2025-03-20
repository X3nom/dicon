#pragma once
#include <dicon_protocol.h>
#include <dicon_sockets.h>


typedef struct msg_builder_ret {
    void *msg;
    int msg_len;
} msg_builder_ret;


int dic_node_recv_send(dic_conn_t *conn);