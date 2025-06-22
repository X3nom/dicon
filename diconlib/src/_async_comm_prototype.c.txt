#include <client_node_communication.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>


#define MUTEX_BLOCK(_mutex, ...) pthread_mutex_lock(_mutex); __VA_ARGS__ pthread_mutex_unlock(_mutex);

#define VA(...) __VA_ARGS__

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


#define GEN_MSG_BUILDER_DYN(_req_body, _args, _d_size, _code) \
msg_builder_ret build_##_req_body(_args){ \
    REQ_BUILDER_INIT_DYNAMIC(_req_body, _d_size); \
    REQ_BUILDER_SET_HEAD(DIC_MALLOC); \
    _code \
    return REQ_BUILDER_RET; \
}
#define GEN_MSG_BUILDER(_req_body, _args, _code) GEN_MSG_BUILDER_DYN(_req_body, VA(_args), 0, VA(_code));


// Very evil macro, if you fuck something up, enjoy debugging ;)
#define GEN_REQUEST_FUNCS_SETUP(_return_T, _func_name, _func_params, _req_body, _resp_body, _device, _builder_args, _main_setup_code, _thread_setup_code, _reponse_handling_code) \
_return_T _func_name(_func_params){ \
    dic_node_t *dev = _device; \
    msg_builder_ret msg = build_##_req_body(_builder_args); \
    MUTEX_BLOCK(&dev->queue_mut, \
        int queue_pos = dev->queue_len; \
        dev->queue_len += 1; \
    ) \
    atomic_send_recv_ret rcv = dic_atomic_send_recv(dev, msg, queue_pos); \
    free(msg.msg); \
    _resp_body *body = (_resp_body*) rcv.body; \
    _reponse_handling_code \
} \
_return_T _##_func_name##_thread_handle_rcv(atomic_send_recv_ret rcv, dic_node_t *dev, void *additional){ \
    _resp_body *body = (_resp_body*) rcv.body; \
    _thread_setup_code \
    _reponse_handling_code \
} \
void *_##_func_name##_thread(async_req_func_param *args){ \
    MUTEX_BLOCK(&args->device->queue_mut, \
        int queue_pos = args->device->queue_len; \
        args->device->queue_len += 1; \
    ) \
    atomic_send_recv_ret rcv = dic_atomic_send_recv(args->device, args->msg, queue_pos); \
    free(args->msg.msg); \
    if(args->dest == NULL) _##_func_name##_thread_handle_rcv(rcv, args->device, args->additional); \
    else *(_return_T*)(args->dest) = _##_func_name##_thread_handle_rcv(rcv, args->device, args->additional); \
    return NULL; \
} \
pthread_t _func_name##_async(_return_T *dest, _func_params){ \
    async_req_func_param *args = malloc(sizeof(async_req_func_param)); \
    args->msg = build_##_req_body(_builder_args); \
    args->device = _device; \
    args->dest = dest; \
    args->additional = NULL; \
    pthread_t t_id; \
    _main_setup_code \
    pthread_create(&t_id, NULL, (void*)_##_func_name##_thread, args); \
    return t_id; \
}

#define GEN_REQUEST_FUNCS(_return_T, _func_name, _func_params, _req_body, _resp_body, _device, _builder_args, _reponse_handling_code) \
    GEN_REQUEST_FUNCS_SETUP(_return_T, _func_name, VA(_func_params), _req_body, _resp_body, _device, VA(_builder_args), , , VA(_reponse_handling_code))


typedef struct msg_builder_ret {
    void *msg;
    int msg_len;
} msg_builder_ret;


typedef struct atomic_send_recv_ret {
    char *body;
    int body_size;
} atomic_send_recv_ret;


typedef struct{
    dic_node_t *device;
    msg_builder_ret msg;
    void *dest;
    void *additional;
} async_req_func_param;




// generic function for atomic send+recieve operation.
atomic_send_recv_ret dic_atomic_send_recv(dic_node_t *device, msg_builder_ret msg, int queue_pos){
    // wait untill this threads position in queue reaches 0
    pthread_mutex_lock(&device->queue_mut);
    while(queue_pos > 0 || device->queue_len <= 0) {
        pthread_cond_wait(&device->queue_cond, &device->queue_mut);
        queue_pos--;
    }
    
    // wait until send is aviable
    pthread_mutex_lock(&device->send_mut);

    // send the request
    dic_conn_send(device, msg.msg, msg.msg_len);

    // keep send locked until recv is aviable (avoid different thread stealing the response)
    pthread_mutex_lock(&device->recv_mut);

    MUTEX_BLOCK(&device->queue_mut, // decrement queue by one
        device->queue_len -= 1;
    )
    pthread_cond_broadcast(&device->queue_cond); // wake other waiting threads

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

GEN_MSG_BUILDER(dic_req_malloc, VA(int size),
    VA(
        body->size = size; // size to malloc on the remote
    )
)
GEN_REQUEST_FUNCS(dic_rvoid_ptr_t, dic_rmalloc, VA(dic_node_t *device, int size),
    dic_req_malloc, dic_resp_malloc, device,
    VA(size),
    VA(
        dic_rvoid_ptr_t rptr;
        rptr.ptr = body->mem_address;
        rptr.device = dev;
        free(rcv.body);
        return rptr;
    )
)

// Realloc ===============
GEN_MSG_BUILDER(dic_req_realloc, VA(dic_rvoid_ptr_t rptr, int size),
    VA(
        body->void_ptr = (universal_void_ptr) rptr.ptr;
        body->size = size;
    )
)
GEN_REQUEST_FUNCS(dic_rvoid_ptr_t, dic_rrealloc, VA(dic_rvoid_ptr_t rptr, int size),
    dic_req_realloc, dic_resp_realloc, rptr.device,
    VA(rptr, size),
    VA(
        dic_rvoid_ptr_t ptr;
        ptr.ptr = body->mem_address;
        ptr.device = dev;
        free(rcv.body);
        return ptr;
    )
)

// Free ===============
GEN_MSG_BUILDER(dic_req_free, VA(dic_rvoid_ptr_t rvoid),
    VA(
        body->void_ptr = rvoid.ptr;
    )
)
GEN_REQUEST_FUNCS(int, dic_rfree, VA(dic_rvoid_ptr_t rvoid),
    dic_req_free, dic_resp_free, rvoid.device,
    VA(rvoid),
    VA(
        int err = body->err;
        free(rcv.body);
        return err;
    )
)

// Memcpy c2n =========
GEN_MSG_BUILDER_DYN(dic_req_memcpy_c2n, VA(void *local, dic_rvoid_ptr_t remote, int size), size,
    VA(
        body->data_size = size;
        body->dest = remote.ptr;
        memcpy(body->_data_placeholder, local, size);   
    )
)
GEN_REQUEST_FUNCS(int, dic_memcpy_c2n, VA(void *local, dic_rvoid_ptr_t remote, int size),
    dic_req_memcpy_c2n, dic_resp_memcpy_c2n, remote.device, 
    VA(local, remote, size),
    VA(
        int err = body->err;
        free(rcv.body);
        return err;
    )
)

// Memcpy n2c ========
GEN_MSG_BUILDER(dic_req_memcpy_n2c, VA(void *local, dic_rvoid_ptr_t remote, int size),
    VA(
        body->data_size = size;
        body->src = remote.ptr;
    )
)
GEN_REQUEST_FUNCS_SETUP(int, dic_memcpy_n2c, VA(void *local, dic_rvoid_ptr_t remote, int size),
    dic_req_memcpy_n2c, dic_resp_memcpy_n2c, remote.device, 
    VA(local, remote, size),
    VA(
        args->additional = local;
    ),
    VA(
        void *local = additional;
    ),
    VA(
        memcpy(local, body->_data_placeholder, body->data_size);
        free(rcv.body);
        return 0;
    )
)

// Memcpy universal
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


// so load
GEN_MSG_BUILDER_DYN(dic_req_so_load, VA(dic_node_t *device, const char *name), strlen(name) + 1,
    VA(
        body->so_len = strlen(name)+1;
        strcpy(body->so_name, name);
    )
)
GEN_REQUEST_FUNCS(dic_rso_handle_t, dic_so_load, VA(dic_node_t *device, const char *name),
dic_req_so_load, dic_resp_so_load, device,
    VA(device, name),
    VA(
        dic_rso_handle_t handle;
        handle.ptr = body->handle;
        handle.device = dev;

        free(rcv.body);
        return handle;
    )
)

// so unload
GEN_MSG_BUILDER(dic_req_so_unload, VA(dic_rso_handle_t handle),
    VA(
        body->handle = handle.ptr;
    )
)
GEN_REQUEST_FUNCS(int, dic_so_unload, VA(dic_rso_handle_t handle),
dic_req_so_unload, dic_resp_so_unload, handle.device,
    VA(handle),
    VA(
        int succ = body->success;

        free(rcv.body);
        return succ;
    )
)

