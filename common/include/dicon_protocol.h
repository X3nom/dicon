/*-----------------------------------------------------------
| This is declaration of the Client-Node protocol.          |
| Protocol is simple, 1 Request => 1 Response               |
| Node can ONLY respond, NEVER send request by itself       |
|                                                           |
|    1) [Client] ==Request==> [Node]                        |
|    2) [Client] <=Response== [Node]                        |
|    3) <END, Node is waining for new request>              |
-------------------------------------------------------------*/

#pragma once
#include <inttypes.h>


// use for casting and declarations, ensures fixed 16bit size in header
typedef uint16_t DIC_OPERATION;


// Should be casted from/to `void*`. (uin64_t for compatibility of different architectures)
typedef uint64_t universal_void_ptr;
// NULL value of `universal_void_ptr`
#define UNULL ((universal_void_ptr)NULL)

// Should be casted from/to `phtread_t` on the node (not on client for safety purposes)
typedef uint64_t universal_pthread_t;


/* ## type of the requested operation 
- ### Do not use for casting
- `enum DIC_OPERATION` should not be used for casting or declarations, use type `DIC_OPERATION` instead for fixed size! */
enum DIC_OPERATION {
    DIC_MALLOC,
    DIC_REALLOC,
    DIC_FREE,
    // memcpy CLIENT -> NODE
    DIC_MEMCPY_C2N,
    // memcpy NODE -> CLIENT
    DIC_MEMCPY_N2C,
    
    DIC_VERIFY,
    DIC_SO_LOAD,
    DIC_SO_UNLOAD,

    DIC_RUN,
    DIC_JOIN
};


#define ALIGNMENT __attribute__((__packed__))


/*==========================================
|           GENERIC HEADERS                |
============================================*/

// a base structure of header of all `client -> node` requests
typedef struct ALIGNMENT dic_req_head_generic {
    DIC_OPERATION operation;
    uint16_t version;
    uint32_t body_size; // size of the following data

    uint8_t _body_placeholder[]; // placeholder for operation-specific data
} dic_req_head_generic;

// a base structure of header of all `node -> client` responses
typedef struct ALIGNMENT dic_resp_head_generic {
    uint32_t body_size;

    uint8_t _body_placeholder[];
} dic_resp_head_generic;


/*-----------------------------------------
|            REQUEST BODIES                |
-------------------------------------------*/
/*==========================================
|           MEMORY HANDLING                |
============================================*/

// MALLOC ==================================

typedef struct ALIGNMENT dic_req_malloc {
    uint32_t size;
} dic_req_malloc;

typedef struct ALIGNMENT dic_resp_malloc {
    universal_void_ptr mem_address;
} dic_resp_malloc;

// REALLOC ==================================

typedef struct ALIGNMENT dic_req_realloc {
    universal_void_ptr void_ptr;
    uint32_t size;
} dic_req_realloc;

typedef struct ALIGNMENT dic_resp_realloc {
    universal_void_ptr mem_address;
} dic_resp_realloc;

// FREE ==================================

typedef struct ALIGNMENT dic_req_free {
    universal_void_ptr void_ptr;
} dic_req_free;

typedef struct ALIGNMENT dic_resp_free {
    int8_t err;
} dic_resp_free;

// MEMCPY [client => node] ==================================

typedef struct ALIGNMENT dic_req_memcpy_c2n {
    universal_void_ptr dest;
    uint32_t data_size;
    uint8_t _data_placeholder[];
} dic_req_memcpy_c2n;

typedef struct ALIGNMENT dic_resp_memcpy_c2n {
    uint32_t err;
} dic_resp_memcpy_c2n;

// MEMCPY [node => client] ==================================

typedef struct ALIGNMENT dic_req_memcpy_n2c {
    universal_void_ptr src;
    uint32_t data_size;
} dic_req_memcpy_n2c;

typedef struct ALIGNMENT dic_resp_memcpy_n2c {
    uint32_t data_size;
    uint8_t _data_placeholder[];
} dic_resp_memcpy_n2c;




/*==========================================
|       SHARED OBJECT HANDLING             |
============================================*/

// VERIFY ==================================

typedef struct ALIGNMENT dic_req_verify {
    char hash[32];
    uint32_t so_len;
    char so_name[];
} dic_req_verify;

typedef struct ALIGNMENT dic_resp_verify {
    int32_t success;
} dic_resp_verify;

// SO_LOAD ==================================

typedef struct ALIGNMENT dic_req_so_load {
    uint32_t so_len;
    char so_name[];
} dic_req_so_load;

typedef struct ALIGNMENT dic_resp_so_load {
    universal_void_ptr handle;
} dic_resp_so_load;

// SO_UNLOAD ==================================

typedef struct ALIGNMENT dic_req_so_unload {
    universal_void_ptr handle;
} dic_req_so_unload;

typedef struct ALIGNMENT dic_resp_so_unload {
    int32_t success;
} dic_resp_so_unload;



/*==========================================
|            FUNCTION HANDLING             |
============================================*/

// RUN ==================================

typedef struct ALIGNMENT dic_req_run {
    universal_void_ptr so_handle;
    universal_void_ptr args_ptr;
    uint32_t symlen;
    // symbol (name) of the function to be called of length `symlen`
    char symbol[];
} dic_req_run;

typedef struct ALIGNMENT dic_resp_run {
    universal_pthread_t tid;
} dic_resp_run;

// JOIN ==================================

typedef struct ALIGNMENT dic_req_join {
    universal_pthread_t tid;
} dic_req_join;

typedef struct ALIGNMENT dic_resp_join {
    universal_void_ptr ret_ptr;
} dic_resp_join;



#undef ALIGNMENT