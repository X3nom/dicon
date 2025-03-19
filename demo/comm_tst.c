#include <node_communication.h>




int main(){
    ip_addr_t ip = ipv4_from_str("127.0.0.0");
    dic_conn_t *conn = dic_conn_new(ip, 12345);

    dic_rso_handle_t handle = dic_so_load(conn, "meow.so");


    printf("%ld\n", handle.ptr);

    return 0;
}