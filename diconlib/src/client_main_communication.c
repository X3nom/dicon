#include <client_main_communication.h>
#include <dicon_main_server_protocol.h>
#include <client_node_communication.h>



dic_main_t *dic_main_connect(ip_addr_t ip_address){
    return dic_conn_new(ip_address, DIC_MAIN_SERVER_PORT);
}


dic_nodes_info_t *dic_get_nodes_info(dic_main_t *main_server_connection){
    dic_main_server_req_head_generic req_head;

    // this req has no body
    req_head.body_size = 0;
    req_head.operation = DIC_GET_INFO;
    req_head.version = 0;

    dic_conn_send(main_server_connection, (void*)&req_head, sizeof(req_head));

    dic_main_server_resp_head_generic res_head;
    dic_conn_recv(main_server_connection, (char*)&res_head, sizeof(res_head), 0);

    dic_resp_get_info *res_body = malloc(res_head.body_size);
    dic_conn_recv(main_server_connection, (char*)res_body, res_head.body_size, 0);

    return res_body;
}

dic_node_t **dic_nodes_connect_all(dic_nodes_info_t *nodes_info){
    dic_node_t **nodes = malloc(sizeof(dic_node_t*) * nodes_info->count);

    for(int i=0; i<nodes_info->count; i++){
        dic_node_t *node = dic_node_connect(nodes_info->nodes[i].address);
        nodes[i] = node;
    }

    return nodes;
}
