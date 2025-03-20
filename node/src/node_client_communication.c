#include "../include/node_client_communication.h"

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

    void *ptr = (void*)req_body->void_ptr;
    
    free(ptr);

    return RESP_BUILD_RET;
}









void dic_recv_send(dic_conn_t *conn){
    dic_req_head_generic head;

    dic_conn_recv(conn, (char*)&head, sizeof(head), 0);

    void *body = malloc(head.body_size);

    dic_conn_recv(conn, (char*)body, head.body_size, 0);

    msg_builder_ret msg;
    switch(head.operation){
        case DIC_MALLOC:
            msg = handle_malloc(body);
            break;

        case DIC_REALLOC:
            msg = handle_realloc(body);
            break;

        case DIC_FREE:
            msg = handle_free(body);
            break;

        default: // shouldn't happen
            msg.msg = NULL;
            msg.msg_len = 0;
    }

    dic_conn_send(conn, msg.msg, msg.msg_len);

    free(body);
    free(msg.msg);
}