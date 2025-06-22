#include <dicon.h>

#include <stdio.h>




int main(){
    // connect to main server, get info about all aviable nodes, then close main server connection
    dic_main_t *main_server = dic_main_connect(ipv4_from_str("127.0.0.1"));

    dic_nodes_info_t *nodes_info = dic_get_nodes_info(main_server);
    dic_conn_destroy(main_server);

    // create list of connections to nodes
    dic_node_t **nodes = malloc(sizeof(dic_node_t*) * nodes_info->count);

    printf("count: %d\n", nodes_info->count);

    for(int i=0; i<nodes_info->count; i++){
        dic_node_t *node = dic_node_connect(nodes_info->nodes[i].address);
        nodes[i] = node;

        printf("ip family: %d\n", node->ip.family);
        

        printf("established connection\n");
    }

    

}