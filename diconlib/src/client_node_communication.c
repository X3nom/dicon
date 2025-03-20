#include <client_node_communication.h>

#define REQ_HEAD_SIZE sizeof(dic_req_head_generic)
// Will not work with dynamic-size bodies
#define REQ_SIZE(_body) (REQ_HEAD_SIZE+sizeof(_body))


// (kinda) Evil macro
#define REQ_BUILDER_INIT_DYNAMIC(_req_body, _dynamic_size) \
    int msg_len = REQ_HEAD_SIZE + sizeof(_req_body) + _dynamic_size; \
    void *msg = malloc(msg_len); \
    dic_req_head_generic *head = msg; \
    _req_body *body = (void*)&((char*)msg)[REQ_HEAD_SIZE];

#define REQ_BUILDER_INIT(_req_body) REQ_BUILDER_INIT_DYNAMIC(_req_body, 0)

#define REQ_BUILDER_SET_HEAD(_operation) \
    head->operation = _operation; \
    head->version = 0; \
    head->body_size = msg_len-REQ_HEAD_SIZE;

#define REQ_BUILDER_RET (msg_builder_ret){msg, msg_len}





typedef struct msg_builder_ret {
    void *msg;
    int msg_len;
} msg_builder_ret;


typedef struct atomic_send_recv_ret {
    char *body;
    int body_size;
} atomic_send_recv_ret;



/*-----------------------------------------
|            MEM HANDLING                 |
-------------------------------------------*/


/*==========================================
|                GENERIC                   |
============================================*/


// generic function for atomic send+recieve operation.
atomic_send_recv_ret dic_atomic_send_recv(dic_node_t *device, msg_builder_ret msg){
    // wait until send is aviable
    pthread_mutex_lock(&device->send_mut);

    // send the request
    dic_conn_send(device, msg.msg, msg.msg_len);

    // keep send locked until recv is aviable (avoid different thread stealing the response)
    pthread_mutex_lock(&device->recv_mut);
    pthread_mutex_unlock(&device->send_mut);

    // recv generic head
    dic_resp_head_generic head;
    dic_conn_recv(device, (char*)&head, sizeof(head), 0);

    // recv rest of body (based on head info)
    char *body = (char*)malloc(head.body_size);
    dic_conn_recv(device, body, head.body_size, 0);

    // all recv done, unlock
    pthread_mutex_unlock(&device->recv_mut);

    return (atomic_send_recv_ret){body, head.body_size};
}




/*==========================================
|                RMALLOC                   |
============================================*/

msg_builder_ret build_rmalloc_req(int size){
    REQ_BUILDER_INIT(dic_req_malloc);
    REQ_BUILDER_SET_HEAD(DIC_MALLOC);

    body->size = size; // size to malloc on the remote

    return REQ_BUILDER_RET;
}


dic_rvoid_ptr_t dic_rmalloc(dic_node_t *device, int size){
    // build request
    msg_builder_ret msg = build_rmalloc_req(size);
    // send request and get responses
    atomic_send_recv_ret rcv = dic_atomic_send_recv(device, msg);
    // cast response to the right struct
    dic_resp_malloc *body = (dic_resp_malloc*)rcv.body;
    // extract the remote void pointer from response
    dic_rvoid_ptr_t rptr;
    rptr.ptr = body->mem_address;
    rptr.device = device;

    free(msg.msg);
    free(rcv.body);

    return rptr;
}


/*==========================================
|              RREALLOC                    |
============================================*/

msg_builder_ret build_rrealloc_req(dic_rvoid_ptr_t rvoid, int size){
    REQ_BUILDER_INIT(dic_req_realloc);
    REQ_BUILDER_SET_HEAD(DIC_REALLOC);
    
    body->void_ptr = rvoid.ptr;
    body->size = size;

    return REQ_BUILDER_RET;
}

dic_rvoid_ptr_t dic_rrealloc(dic_rvoid_ptr_t rvoid, int size){
    msg_builder_ret msg = build_rrealloc_req(rvoid, size);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(rvoid.device, msg);

    dic_resp_realloc *body = (dic_resp_realloc*)rcv.body;

    // reuse the rvoid variable as it is passed by value, avoids having to set device again
    rvoid.ptr = body->mem_address;
    
    free(msg.msg);
    free(rcv.body);

    return rvoid;
}


/*==========================================
|                RFREE                     |
============================================*/

msg_builder_ret build_rfree_req(dic_rvoid_ptr_t rvoid){
    REQ_BUILDER_INIT(dic_req_free);
    REQ_BUILDER_SET_HEAD(DIC_FREE);

    body->void_ptr = rvoid.ptr;

    return REQ_BUILDER_RET;
}

int dic_rfree(dic_rvoid_ptr_t rvoid){
    msg_builder_ret msg = build_rfree_req(rvoid);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(rvoid.device, msg);

    dic_resp_free *body = (dic_resp_free*)rcv.body;

    int err = body->err; // well.. fuck it why would you need an error :D

    free(msg.msg);
    free(body);

    return err;
}




/*==========================================
|           RMEMCPY [local-remote]         |
============================================*/
// this is one of the messier parts ...
// TODO: make it async

msg_builder_ret build_memcpy_c2n_req(void *local, dic_rvoid_ptr_t remote, int size){
    REQ_BUILDER_INIT_DYNAMIC(dic_req_memcpy_c2n, size);
    REQ_BUILDER_SET_HEAD(DIC_MEMCPY_C2N);

    // set body
    body->data_size = size;
    body->dest = remote.ptr;
    memcpy(body->_data_placeholder, local, size);
    
    return REQ_BUILDER_RET;
}


int dic_memcpy_c2n(void *local, dic_rvoid_ptr_t remote, int size){
    msg_builder_ret msg = build_memcpy_c2n_req(local, remote, size);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(remote.device, msg);

    dic_resp_memcpy_c2n *body = (dic_resp_memcpy_c2n*)rcv.body;

    int err = body->err;

    free(rcv.body);
    free(msg.msg);

    return err;
}


/*==========================================
|           RMEMCPY [remote-local]         |
============================================*/

msg_builder_ret build_memcpy_n2c_req(void *local, dic_rvoid_ptr_t remote, int size){
    REQ_BUILDER_INIT(dic_req_memcpy_n2c);
    REQ_BUILDER_SET_HEAD(DIC_MEMCPY_N2C);

    body->data_size = size;
    body->src = remote.ptr;

    return REQ_BUILDER_RET;
}

int dic_memcpy_n2c(void *local, dic_rvoid_ptr_t remote, int size){
    msg_builder_ret msg = build_memcpy_n2c_req(local, remote, size);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(remote.device, msg);

    dic_resp_memcpy_n2c *body = (dic_resp_memcpy_n2c*)rcv.body;

    memcpy(local, body->_data_placeholder, body->data_size);

    free(msg.msg);
    free(rcv.body);

    return 0;
}

/*==========================================
|               RMEMCPY                    |
============================================*/

int dic_memcpy(void *local, dic_rvoid_ptr_t remote, int size, enum DIC_MEMCPY_MODE mode){
    switch(mode){
        case LOCAL_2_REMOTE:{
            return dic_memcpy_c2n(local, remote, size);
        }
        case REMOTE_2_LOCAL:{
            return dic_memcpy_n2c(local, remote, size);
        }
        default:{
            return 1;
        }
    }
}


/*-----------------------------------------
|            SO HANDLING                  |
-------------------------------------------*/

/*==========================================
|               VERIFY                     |
============================================*/
// TODO

/*==========================================
|               SO_LOAD                    |
============================================*/

msg_builder_ret build_so_load(dic_node_t *device, const char *name){
    int name_len = strlen(name) + 1;
    REQ_BUILDER_INIT_DYNAMIC(dic_req_so_load, name_len);
    REQ_BUILDER_SET_HEAD(DIC_SO_LOAD);

    body->so_len = name_len;
    strcpy(body->so_name, name);

    return REQ_BUILDER_RET;
}

dic_rso_handle_t dic_so_load(dic_node_t *device, const char *name){
    msg_builder_ret msg = build_so_load(device, name);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(device, msg);

    dic_resp_so_load *body = (dic_resp_so_load*)rcv.body;

    dic_rso_handle_t handle;
    handle.ptr = body->handle;
    handle.device = device;

    free(rcv.body);
    free(msg.msg);

    return handle;
}


/*==========================================
|               SO_UNLOAD                  |
============================================*/

msg_builder_ret build_so_unload(dic_rso_handle_t handle){
    REQ_BUILDER_INIT(dic_req_so_unload);
    REQ_BUILDER_SET_HEAD(DIC_SO_UNLOAD);

    body->handle = handle.ptr;

    return REQ_BUILDER_RET;
}

int dic_so_unload(dic_rso_handle_t handle){
    msg_builder_ret msg = build_so_unload(handle);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(handle.device, msg);

    dic_resp_so_unload *body = (dic_resp_so_unload*)rcv.body;

    int succ = body->success;

    free(rcv.body);
    free(msg.msg);

    return succ;
}


/*------------------------------------------
|            FUNCTION HANDLING             |
--------------------------------------------*/

/*==========================================
|                  RUN                     |
============================================*/

msg_builder_ret build_rthread_run(dic_rso_handle_t handle, const char *symbol, dic_rvoid_ptr_t args_ptr){
    int sym_len = strlen(symbol); 
    REQ_BUILDER_INIT_DYNAMIC(dic_req_run, sym_len);
    REQ_BUILDER_SET_HEAD(DIC_RUN)

    body->args_ptr = args_ptr.ptr;
    body->so_handle = handle.ptr;
    strcpy(body->symbol, symbol);
    body->symlen = sym_len;

    return REQ_BUILDER_RET;
}

dic_rthread_t dic_rthread_run(dic_rso_handle_t handle, const char *symbol, dic_rvoid_ptr_t args_ptr){
    msg_builder_ret msg = build_rthread_run(handle, symbol, args_ptr);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(handle.device, msg);

    dic_resp_run *body = (dic_resp_run*)rcv.body;

    dic_rthread_t rtid;
    rtid.device = handle.device;
    rtid.tid = body->tid;

    free(rcv.body);
    free(msg.msg);

    return rtid;
}


/*==========================================
|                  JOIN                    |
============================================*/


msg_builder_ret build_rthread_join(dic_rthread_t thread_id){
    REQ_BUILDER_INIT(dic_req_join);
    REQ_BUILDER_SET_HEAD(DIC_JOIN);

    body->tid = thread_id.tid;

    return REQ_BUILDER_RET;
}

dic_rvoid_ptr_t dic_rthread_join(dic_rthread_t thread_id){
    msg_builder_ret msg = build_rthread_join(thread_id);

    atomic_send_recv_ret rcv = dic_atomic_send_recv(thread_id.device, msg);

    dic_resp_join *body = (dic_resp_join*)rcv.body;

    dic_rvoid_ptr_t ret_ptr;
    ret_ptr.ptr = body->ret_ptr;
    ret_ptr.device = thread_id.device;

    free(rcv.body);
    free(msg.msg);

    return ret_ptr;
}