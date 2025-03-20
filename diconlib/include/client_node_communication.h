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
    dic_conn_t *device;
} dic_rvoid_ptr_t;

/* NOT COMPARABLE
Use `rvoid.ptr == NULL` or `IS_RNULL(rvoid)` instead of `rvoid == RNULL`*/
#define RNULL(_device) (dic_rvoid_ptr_t){0, _device}
#define RNULL2 RNULL(NULL)

#define IS_RNULL(_rvoid) (_rvoid.ptr == NULL)


typedef struct {
    universal_pthread_t tid;
    dic_conn_t *device;
} dic_rthread_t;


/*==========================================
|           MEMORY HANDLING                |
============================================*/

dic_rvoid_ptr_t dic_rmalloc(dic_node_t *device, int size);

dic_rvoid_ptr_t dic_rrealloc(dic_rvoid_ptr_t rvoid, int size);

int dic_rfree(dic_rvoid_ptr_t rvoid);

enum DIC_MEMCPY_MODE{
    LOCAL_2_REMOTE,
    REMOTE_2_LOCAL
};

int dic_memcpy(void *local, dic_rvoid_ptr_t remote, int size, enum DIC_MEMCPY_MODE mode);


/*==========================================
|       SHARED OBJECT HANDLING             |
============================================*/

typedef dic_rvoid_ptr_t dic_rso_handle_t;

int dic_verify(dic_node_t *device, const char *name);

dic_rso_handle_t dic_so_load(dic_node_t *device, const char *name);

int dic_so_unload(dic_rso_handle_t handle);

/*==========================================
|            FUNCTION HANDLING             |
============================================*/

/* Load function from `handle` and run it with `args_ptr`

*/
dic_rthread_t dic_rthread_run(dic_rso_handle_t handle, const char *symbol, dic_rvoid_ptr_t args_ptr);


dic_rvoid_ptr_t dic_rthread_join(dic_rthread_t thread_id);