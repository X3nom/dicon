#pragma once

#include <inttypes.h>
#include <dicon_sockets.h>

#define DIC_MAIN_SERVER_PORT 4225

enum DIC_MAIN_OPERATION {
    DIC_ANNOUNCE_INFO,
    DIC_GET_INFO
};

#define ALIGNMENT __attribute__((__packed__))

// a base structure of header of all `client -> node` requests
typedef struct ALIGNMENT {
    uint16_t operation;
    uint16_t version;
    uint32_t body_size; // size of the following data

    uint8_t _body_placeholder[]; // placeholder for operation-specific data
} dic_main_server_req_head_generic;

// a base structure of header of all `node -> client` responses
typedef struct ALIGNMENT {
    uint32_t body_size;

    uint8_t _body_placeholder[];
} dic_main_server_resp_head_generic;


// OPERATIONS ==============


typedef struct ALIGNMENT {
    uint32_t core_count;
} dic_req_announce_info;


typedef struct ALIGNMENT {
    uint32_t error;
} dic_resp_announce_info;


typedef struct ALIGNMENT {
} dic_req_get_info;


typedef struct ALIGNMENT {
    ip_addr_t address;
    uint32_t core_count;
} dic_node_info_t;

typedef struct ALIGNMENT {
    uint32_t count;
    dic_node_info_t nodes[];
} dic_resp_get_info;