/*
The communication protocol is 1 request -> 1 response
All the operations should be thread safe. (underlaying tcp communication is not though)
*/
#pragma once
#include <dicon_sockets.h>
#include <dicon_protocol.h>

/** remote void pointer
 * Points to memory address on a different (remote) device. */
typedef struct {
    universal_void_ptr ptr; /* not valid address on the local, will likely cause segfault if accessed */
    dic_connection_t *device;
} dic_rvoid_ptr_t;



dic_rvoid_ptr_t dic_rmalloc(dic_node_t *device, int size);

dic_rvoid_ptr_t dic_rrealloc(dic_rvoid_ptr_t rvoid, int size);

int dic_rfree(dic_rvoid_ptr_t rvoid);

enum DIC_MEMCPY_MODE{
    LOCAL_2_REMOTE,
    REMOTE_2_LOCAL
};

int dic_memcpy(void *local, dic_rvoid_ptr_t remote, int size, enum DIC_MEMCPY_MODE mode);

