#include "../include/node_client_communication.h"
#include <global_logging.h>
#include <dlfcn.h>

#define RESP_HEAD_SIZE sizeof(dic_resp_head_generic)

#define RESP_BUILDER_INIT_DYNAMIC(_resp_body, _dynamic_size) \
    int msg_len = RESP_HEAD_SIZE + sizeof(_resp_body) + _dynamic_size; \
    void *msg = malloc(msg_len); \
    dic_resp_head_generic *resp_head = msg; \
    _resp_body *resp_body = (void*)&((char*)msg)[RESP_HEAD_SIZE];

#define RESP_BUILDER_INIT(_resp_body) RESP_BUILDER_INIT_DYNAMIC(_resp_body, 0)

#define RESP_SET_HEAD() \
    resp_head->body_size = msg_len - RESP_HEAD_SIZE;

#define RESP_BUILD_RET (msg_builder_ret){msg, msg_len}




msg_builder_ret handle_malloc(dic_req_malloc *req_body){
    RESP_BUILDER_INIT(dic_resp_malloc);
    RESP_SET_HEAD();

    if(req_body->size <= 0){
        // why the fuck did you decide to malloc 0 bytes
        resp_body->mem_address = UNULL;
        return RESP_BUILD_RET;
    }

    //! SPACE ALLOCATED BY THE CLIENT -> POTENTIAL LEAK OF MEMORY
    void *ptr = malloc(req_body->size);

    resp_body->mem_address = (universal_void_ptr) ptr;

    return RESP_BUILD_RET;
}

msg_builder_ret handle_realloc(dic_req_realloc *req_body){
    RESP_BUILDER_INIT(dic_resp_realloc);
    RESP_SET_HEAD();

    void *new_ptr = realloc((void*)req_body->void_ptr, req_body->size);

    resp_body->mem_address = (universal_void_ptr)new_ptr;

    return RESP_BUILD_RET;
}

msg_builder_ret handle_free(dic_req_free *req_body){
    RESP_BUILDER_INIT(dic_resp_free);
    RESP_SET_HEAD();

    void *ptr = (void*)(req_body->void_ptr);

    free(ptr);
    resp_body->err = 0;

    return RESP_BUILD_RET;
}



/*-------------------------------------------
|       MEMCPY                              |
-------------------------------------------*/

// recieve data on address
msg_builder_ret handle_memcpy_c2n(dic_req_memcpy_c2n *req_body){
    RESP_BUILDER_INIT(dic_resp_memcpy_c2n);
    RESP_SET_HEAD();

    void *dest_ptr = (void*)req_body->dest;
    //! POSSIBLE SEGFAULT - USER DIRECTLY MANIPULATES MEMORY
    memcpy(dest_ptr, req_body->_data_placeholder, req_body->data_size);

    resp_body->err = 0;

    return RESP_BUILD_RET;
}

msg_builder_ret handle_memcpy_n2c(dic_req_memcpy_n2c *req_body){
    RESP_BUILDER_INIT_DYNAMIC(dic_resp_memcpy_n2c, req_body->data_size);
    RESP_SET_HEAD();

    resp_body->data_size = req_body->data_size;
    //! POSSIBLE SEGFAULT - USER DIRECTLY MANIPULATES MEMORY
    memcpy(resp_body->_data_placeholder, (void*)req_body->src, req_body->data_size);

    return RESP_BUILD_RET;
}


/*==========================================
|             SO                           |
==========================================*/

msg_builder_ret handle_so_load(dic_req_so_load *req_body){
    RESP_BUILDER_INIT(dic_resp_so_load);
    RESP_SET_HEAD();

    // path formatting
    char so_path_prefix[] = "user_objects/";
    char *so_path = malloc(sizeof(so_path_prefix) + req_body->so_len);
    sprintf(so_path, "%s%s.so", so_path_prefix, req_body->so_name);
    // ------


    void *handle = dlopen(so_path, RTLD_LAZY);

    free(so_path);

    resp_body->handle = (universal_void_ptr)handle;

    return RESP_BUILD_RET;
}

msg_builder_ret handle_so_unload(dic_req_so_unload *req_body){
    RESP_BUILDER_INIT(dic_resp_so_unload);
    RESP_SET_HEAD();

    dlclose((void*)req_body->handle);

    resp_body->success = 0;
    
    return RESP_BUILD_RET;
}


msg_builder_ret handle_func_load(dic_req_func_load *req_body){
    RESP_BUILDER_INIT(dic_resp_func_load);
    RESP_SET_HEAD();

    void *func = dlsym((void*)req_body->so_handle, req_body->symbol);

    resp_body->func_ptr = (universal_void_ptr)func;

    return RESP_BUILD_RET;
}

/*========================================
|         RTHREAD                        |
========================================*/

msg_builder_ret handle_run(dic_req_run *req_body){
    RESP_BUILDER_INIT(dic_resp_run);
    RESP_SET_HEAD();

    void *(*func)(void*) = (void*)req_body->func_ptr;

    // spawn the requested thread
    pthread_t t_id;
    pthread_create(&t_id, NULL, func, (void*)req_body->args_ptr);

    resp_body->tid = t_id;

    return RESP_BUILD_RET;
}


msg_builder_ret handle_join(dic_req_join *req_body){
    RESP_BUILDER_INIT(dic_resp_join);
    RESP_SET_HEAD();

    pthread_t t_id = req_body->tid;
    void *ret_ptr;
    pthread_join(t_id, &ret_ptr);

    resp_body->ret_ptr = (universal_void_ptr)ret_ptr;

    return RESP_BUILD_RET;
}








int dic_node_recv_send(dic_conn_t *conn){
    dic_req_head_generic head;

    int res = dic_conn_recv(conn, (char*)&head, sizeof(head), 0);
    if(res!=0) return res;

    void *body = malloc(head.body_size);

    res = dic_conn_recv(conn, (char*)body, head.body_size, 0);
    if(res!=0) return res;

    LOG(2, "recieved request from client [operation: %d]\n", head.operation);

    msg_builder_ret msg;
    // case shorthand
    #define ccase(_cond, _func) case _cond: msg = _func(body); break;
    switch(head.operation){
        ccase(DIC_MALLOC, handle_malloc);
        ccase(DIC_REALLOC, handle_realloc);
        ccase(DIC_FREE, handle_free);

        ccase(DIC_MEMCPY_C2N, handle_memcpy_c2n);
        ccase(DIC_MEMCPY_N2C, handle_memcpy_n2c);

        ccase(DIC_SO_LOAD, handle_so_load);
        ccase(DIC_SO_UNLOAD, handle_so_unload);

        ccase(DIC_FUNC_LOAD, handle_func_load);

        ccase(DIC_RUN, handle_run);
        ccase(DIC_JOIN, handle_join);

        default: // shouldn't happen
            msg.msg = NULL;
            msg.msg_len = 0;
            break;
    }
    #undef ccase // undef the temporary shorthand


    if(msg.msg != NULL)
        dic_conn_send(conn, msg.msg, msg.msg_len);

    free(body);
    free(msg.msg);

    return 0;
}