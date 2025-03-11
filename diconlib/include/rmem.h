/**
 * REMOTE MEMORY [rmem]
 */
#pragma once
#include <dicon_sockets.h>
#include <protocol_enums.h>

/** remote void pointer
 * Points to memory address on a different (remote) device. */
typedef struct {
    void *address; // not valid address on the local, will likely cause segfault if accessed
    dic_connection_t *device;
} dic_rvoid_ptr_t;



dic_rvoid_ptr_t dic_rmalloc(dic_node_t *device, int size);

dic_rvoid_ptr_t dic_rrealloc(dic_rvoid_ptr_t rvoid, int size);

void dic_rfree(dic_rvoid_ptr_t rvoid);

enum DIC_MEMCPY_MODE{
    LOCAL_2_REMOTE,
    REMOTE_2_LOCAL
};

int dic_memcpy(void *local, dic_rvoid_ptr_t remote, int size, DIC_MEMCPY_MODE mode);
