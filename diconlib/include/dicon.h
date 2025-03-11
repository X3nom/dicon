#pragma once

#include <pthread.h>

#include <libsocket/libunixsocket.h>
#include <netinet/in.h>

typedef struct dic_connection{
    int sockfd;

    pthread_mutex_t recv_mut;
    pthread_mutex_t send_mut;

    int port; // for restarting connection
    char ip[48];
} dic_connection_t;

typedef dic_connection_t dic_server_t;
typedef dic_connection_t dic_node_t;



/** remote void pointer
 * Points to memory address on a different (remote) device. **/
typedef struct {
    void *address;
    dic_connection_t device;
} dic_rvoid_ptr_t;

/** remote pthread */
typedef struct {
    pthread_t thread_id;
} dic_rthread_t;


// MEMORY MANAGEMENT ===========================

/** malloc memory on remote device
 * return: rvoid pointing to allocated memory */
dic_rvoid_ptr_t dic_malloc(dic_node_t node, int bytes);

void dic_free(dic_rvoid_ptr_t address);


typedef enum {
    LOCAL_2_REMOTE,
    REMOTE_2_LOCAL
} DIC_MEMCPY_MODE;

void dic_memcpy(void *local, dic_rvoid_ptr_t remote, DIC_MEMCPY_MODE mode);



// THREAD MANAGEMENT ==========================

void dic_create_rthread(const char* kernel, dic_node_t node, dic_rvoid_ptr_t args);

/** Join remote thread
 * return: rvoid pointing to returned value
 */
dic_rvoid_ptr_t dic_join_rthread(dic_rthread_t thread);


// CONNECTION MANAGEMENT

dic_server_t dic_server_connect(const char* ip, int socket);

