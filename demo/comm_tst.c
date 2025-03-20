#include <dicon.h>




int main(){
    ip_addr_t ip = ipv4_from_str("127.0.0.1");
    printf("got ip\n");
    dic_conn_t *conn = dic_conn_new(ip, 12345);
    printf("conn established\n");

    char test[] = "hello world";
    char test2[32];

    // allocate 32 bytes on remote
    dic_rvoid_ptr_t rtest = dic_rmalloc(conn, 32);
    printf("remote allocation done\n");
    // copy test to remote buffer
    dic_memcpy(test, rtest, sizeof(test), LOCAL_2_REMOTE);
    printf("memcpy local->remote done\n");
    // copy from remote buffer to test2
    dic_memcpy(test2, rtest, 32, REMOTE_2_LOCAL);
    printf("memcpy remote->local done\n");

    // if everything works, test2 should contain content of test
    printf("%s\n", test2);

    // cleanup
    dic_rfree(rtest);
    dic_conn_destroy(conn);

    return 0;
}