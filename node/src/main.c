#include <node_client_communication.h>
#include <node_main_communication.h>
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
    -h              | show this menu and exit\n\
    -s <password>   | run server password protected\n\
    -m <ip_address> | set main-server ip address (optional)\n\
    -V <int>        | set logging level, default 0\n\
\n\
exiting...\n");
    exit(0);
}


int main(int argc, char *argv[]){

    int opt;

    int port = 4224;
    int main_server_port = 4225;
    
    bool main_server_specified = false;
    ip_addr_t main_server_addr;
    dic_conn_t *main_server_conn;

    while ((opt = getopt(argc, argv, "hm:V:s:")) != -1) {
        switch (opt) {
            case 'h':
                show_help_and_exit();
                break;
            
            case 'm':
                main_server_specified = true;
                main_server_addr = ipv4_from_str(optarg);
                break;

            case 'V': // set logging level
                GLOBAL_LOG_LEVEL = atoi(optarg);
                break;
        }
    }

    if(port==-1) show_help_and_exit();

    if(main_server_specified){
        LOG(0, "Attempting connection to the main server\n");
        // this connection should be held open for as long as this node is online
        main_server_conn = dic_conn_new(main_server_addr, main_server_port);

        while(main_server_conn->sockfd == -1){
            LOG(0, "connection to the main server failed, attempting again in 1 second\n");
            sleep(1);
            dic_conn_tryconnect(main_server_conn);
        }

        int res = notify_main_server(main_server_conn);
        if(res != 0){ LOG(0, "notify main server failed (code: %d)\n", res); }
        else { LOG(1, "server notified successfully\n"); }
    }

    LOG(0, "Starting server on port %d\n", port);
    start_server(port);


    if(main_server_specified){
        dic_conn_destroy(main_server_conn);
    }

    return EXIT_SUCCESS;
}