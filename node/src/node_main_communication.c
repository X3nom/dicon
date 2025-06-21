#include <dicon_sockets.h>
#include <node_main_communication.h>
#include <dicon_main_server_protocol.h>
#include <unistd.h>


int notify_main_server(dic_conn_t *main_conn){
    int cores = sysconf(_SC_NPROCESSORS_ONLN); // Online (usable) cores

    dic_req_announce_info body;
    body.core_count = cores;


    dic_main_server_req_head_generic head;
    head.body_size = sizeof(body);
    head.operation = DIC_ANNOUNCE_INFO;
    head.version = 1;

    int res;
    // send the announcement
    res = dic_conn_send(main_conn, (void*)&head, sizeof(head));
    if(res != 0) return res;
    res = dic_conn_send(main_conn, (void*)&body, sizeof(body));
    if(res != 0) return res;

    dic_main_server_resp_head_generic res_head;
    dic_resp_announce_info res_body;

    dic_conn_recv(main_conn, (char*)&res_head, sizeof(res_head), 0);
    dic_conn_recv(main_conn, (char*)&res_body, res_head.body_size, 0);

    return res_body.error;
}

