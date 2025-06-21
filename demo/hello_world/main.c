#include <dicon.h>
#include <stdio.h>

#define LOG(...) fprintf(stderr, __VA_ARGS__)

#define MSG_SIZE 32



// This will be ran remotely
char *hello_dicon(void*x){
    char message[MSG_SIZE] = "hello dicon!";
    char *allocated_msg = malloc(MSG_SIZE); 
    sprintf(allocated_msg, "%s", message);

    return allocated_msg;
}



int main(){
    // get ip address of nodes
    ip_addr_t ip = ipv4_from_str("127.0.0.0");
    // connect to node
    dic_conn_t *conn = dic_node_connect(ip);
    if(conn==NULL || conn->sockfd == -1){
        perror("Connection failed!");
        exit(EXIT_FAILURE);
    }

    // assuming the compiled shared object is uploaded to node and in `user_objects/hello_world.so`
    dic_rso_handle_t so_handle = dic_so_load(conn, "hello_world");
    LOG("got so handle\n");
    // retrieve remote function ptr
    dic_rfunc_ptr_t remote_hello_dicon = dic_func_load(so_handle, STR(hello_dicon));
    LOG("loaded func ptr\n");
    // creates remote thread
    dic_rthread_t rt_id = dic_rthread_run(remote_hello_dicon, RNULL2);
    LOG("running func\n");
    // join the remote thread
    dic_rvoid_ptr_t ret = dic_rthread_join(rt_id);
    LOG("func joined\n");



    // copy ret back
    char *ret_local = malloc(MSG_SIZE);
    LOG("starting memcpy\n");
    dic_memcpy(ret_local, ret, MSG_SIZE, REMOTE_2_LOCAL);
    LOG("memcpy done\n");

    printf("%s\n", ret_local);


    dic_rfree(ret); // free remotely allocated ret
    free(ret_local);
    dic_conn_destroy(conn);
}