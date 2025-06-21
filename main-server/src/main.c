#define _GNU_SOURCE

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <dicon_sockets.h>
#include <global_logging.h>
#include <dicon_main_server_protocol.h>


#define MUTEX_BLOCK(mut_ptr, ...) \
    pthread_mutex_lock(mut_ptr); \
    __VA_ARGS__ \
    pthread_mutex_unlock(mut_ptr);

int GLOBAL_LOG_LEVEL = 0;


typedef struct {
    dic_conn_t *conn;
} connection_handler_args;

typedef struct linked_list_node_info {
    dic_node_info_t info;
    struct linked_list_node_info *prev;
    struct linked_list_node_info *next;
} linked_list_node_info;

linked_list_node_info *NODES_INFO = NULL;
pthread_mutex_t NODES_INFO_MUT;

linked_list_node_info *append_info(linked_list_node_info **first, dic_node_info_t info){
    linked_list_node_info *new_node = malloc(sizeof(linked_list_node_info));
    new_node->info = info;
    new_node->next = NULL;

    linked_list_node_info *curr_node = *first;
    if(curr_node != NULL){
        while(curr_node->next != NULL){
            curr_node = curr_node->next;
        }

        curr_node->next = new_node;
        new_node->prev = curr_node;
    }
    else{
        new_node->prev = NULL;
        *first = new_node;
    }
    return new_node;
}

void remove_info(linked_list_node_info **first, linked_list_node_info *to_remove){
    // last
    if(to_remove->next != NULL)
        to_remove->next->prev = to_remove->prev;
    if(to_remove->prev != NULL)
        to_remove->prev->next = to_remove->next;
    else{ // rm first
        *first = NULL;
    }
    
    free(to_remove);
}


linked_list_node_info *handle_push_info(dic_conn_t *conn, dic_main_server_req_head_generic head){
    dic_req_announce_info body;
    int res = dic_conn_recv(conn, (void*)&body, head.body_size, 0);

    dic_node_info_t info;
    info.address = conn->ip;
    info.core_count = body.core_count;


    dic_main_server_resp_head_generic resp_head;
    resp_head.body_size = 0;
    dic_conn_send(conn, (void*)&resp_head, sizeof(resp_head));

    return append_info(&NODES_INFO, info);
}

void handle_get_info(dic_conn_t *conn, dic_main_server_req_head_generic head){
    int len = 0;
    linked_list_node_info *curr_node = NODES_INFO;

    if(curr_node != NULL){
        len++;
        for(len=0; curr_node->next != NULL; len++) curr_node = curr_node->next;
    }

    dic_resp_get_info *body = malloc(sizeof(dic_resp_get_info) + sizeof(dic_node_info_t) * len);


    curr_node = NODES_INFO;

    body->count = len;
    for(int i=0; i<len; i++){
        body->nodes[i] = curr_node->info;
        curr_node = curr_node->next;
    }
    

    dic_main_server_resp_head_generic resp_head;
    resp_head.body_size = 0;



    dic_conn_send(conn, (void*)&resp_head, sizeof(resp_head));

}


void *connection_handler_thread(connection_handler_args *args){
    LOG(2, "in connection handler thread");
    dic_conn_t *client_conn = args->conn;

    linked_list_node_info *conn_info = NULL;

    bool is_compute_node = false;

    while(true){

        dic_main_server_req_head_generic head;
        
        int res = dic_conn_recv(client_conn, (char*)&head, sizeof(head), 0);

        if(res == 1) break; // conn closed

        switch(head.operation){
            case DIC_ANNOUNCE_INFO:{
                LOG(2, "recieved ANNOUNCE_INFO");
                MUTEX_BLOCK(&NODES_INFO_MUT,
                    conn_info = handle_push_info(client_conn, head);
                    is_compute_node = true;
                )
                break;
            }
            case DIC_GET_INFO:{
                LOG(2, "recieved GET_INFO");
                MUTEX_BLOCK(&NODES_INFO_MUT,
                    handle_get_info(client_conn, head);
                )
                break;
            }
        }
    }
    if(is_compute_node){
        MUTEX_BLOCK(&NODES_INFO_MUT,
            remove_info(&NODES_INFO, conn_info);
        )
    }

    dic_conn_destroy(client_conn); // free the conn
}


void listen_loop(dic_listen_conn_t *listen_conn){
    int res=0;
    while(1){
        dic_conn_t *client_conn = dic_conn_accept(listen_conn);
        LOG(1, "CONNECTION ACCEPTED\n");

        connection_handler_args *args = malloc(sizeof(connection_handler_args));
        args->conn = client_conn;

        pthread_t thread = pthread_create(NULL, NULL, (void*)connection_handler_thread, (void*)args);
        pthread_detach(thread);
    }   
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
exiting...\n");
    exit(0);
}


int main(int argc, char *argv[]){
    pthread_mutex_init(&NODES_INFO_MUT, NULL);

    int opt;

    int port = DIC_MAIN_SERVER_PORT;
    ip_addr_t ip = (ip_addr_t){0};

    while ((opt = getopt(argc, argv, "hp:V:a:")) != -1) {
        switch (opt) {
            case 'h':
                show_help_and_exit();
                break;

            case 'V': // set logging level
                GLOBAL_LOG_LEVEL = atoi(optarg);
                break;
        }
    }

    // runs infinitely
    start_server(port);
    
    return 0;
}