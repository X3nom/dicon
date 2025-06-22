#pragma once
#include <dicon_sockets.h>
#include <dicon_main_server_protocol.h>


typedef dic_resp_get_info dic_nodes_info_t;

typedef dic_conn_t dic_main_t;


dic_main_t *dic_main_connect(ip_addr_t ip_address);

dic_nodes_info_t *dic_get_nodes_info(dic_conn_t *main_server_connection);

dic_node_t **dic_nodes_connect_all(dic_nodes_info_t *nodes_info);
