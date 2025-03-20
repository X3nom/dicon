#include <node_client_communication.h>

#define DO_LOGS
#ifdef DO_LOGS
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif



int client_handling_loop(dic_conn_t *conn){
    int res = 0;
    while (res == 0){
        res = dic_node_recv_send(conn);
    }
    return res;
}



int listen_loop(dic_listen_conn_t *listen_conn){
    int res=0;
    while(1){
        dic_conn_t *client_conn = dic_conn_accept(listen_conn);
        LOG("CONNECTION FROM CLIENT ACCEPTED\n");

        res = client_handling_loop(client_conn);

        dic_conn_destroy(client_conn); // free the conn
    }
    
    return res;
}



int main(){
    int port = 12345;

    LOG("creating new listen_conn\n");
    dic_listen_conn_t *listen_conn = dic_conn_new_listening(port);

    LOG("entering listening loop\n");
    listen_loop(listen_conn);

    dic_conn_destroy(listen_conn);
}