#include <node_client_communication.h>
#include <global_logging.h>

#include <unistd.h>
#include <getopt.h>

int GLOBAL_LOG_LEVEL = 0;


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
        LOG(1, "CONNECTION FROM CLIENT ACCEPTED\n");

        res = client_handling_loop(client_conn);

        LOG(1, "CLIENT DISCONNECTED\n");

        dic_conn_destroy(client_conn); // free the conn
    }
    
    return res;
}




int start_server(int port){
    LOG(2, "creating new listen_conn\n");
    dic_listen_conn_t *listen_conn = dic_conn_new_listening(port);

    LOG(2, "entering listening loop\n");
    listen_loop(listen_conn);

    dic_conn_destroy(listen_conn);
    return 0;
}



void show_help_and_exit(){
    printf("\
== DICON NODE ==\n\
options: \n\
    -h        | show this menu and exit\n\
    -p <port> | set port to listen at\n\
    -V <int>  | set logging level, default 0\n\
\n\
exiting...\n");
    exit(0);
}


int main(int argc, char *argv[]){

    int opt;

    int port = -1;

    while ((opt = getopt(argc, argv, "hp:V:")) != -1) {
        switch (opt) {
            case 'h':
                show_help_and_exit();
                break;

            case 'p':
                port = atoi(optarg);
                break;

            case 'V': // set logging level
                GLOBAL_LOG_LEVEL = atoi(optarg);
                break;
        }
    }

    if(port==-1) show_help_and_exit();

    LOG(0, "Starting server on port %d\n", port);
    start_server(port);

    return EXIT_SUCCESS;
}