#include <dicon.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define IPV4(_ip) ipv4_from_str(_ip)



// REMOTE =========================================================================
bool is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i <= sqrt(n); i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

struct find_primes_args{
    uint64_t range_start;
    uint64_t range_end;
};

// the remote function has to be of signature `void*(*func)(void*)`
uint64_t *find_primes(struct find_primes_args* args){
    unsigned int start = args->range_start;
    unsigned int end = args->range_end;

    int allocated_len = 16;
    uint64_t *allocated = malloc(sizeof(uint64_t)*(allocated_len+1));
    uint64_t *count = allocated; // first int in the array is length of the array-1 (- self)
    uint64_t *found_primes = &allocated[1]; // start of the actual array
    *count = 0;

    for (unsigned int i = start; i <= end; i++) {
        if(is_prime(i)) {
            *count += 1;
            if(*count >= allocated_len){
                allocated_len += 16;
                allocated = realloc(allocated, (allocated_len+1)*sizeof(uint64_t));
                count = allocated;
                found_primes = &allocated[1];
            }

            found_primes[*count-1] = i;
        }
    }

    allocated_len = (*count);
    allocated = realloc(allocated, sizeof(uint64_t)*(allocated_len+1));

    return allocated;
}
// =================================================================================




#define RANGE_START 1
#define RANGE_END 10000000
// #define CHUNK_SIZE 10000

dic_conn_t **init_connections(dic_node_info_t *ips, int count){
    dic_conn_t **connections = malloc(sizeof(dic_conn_t*)*count); 
    for(int i=0; i<count; i++){
        connections[i] = dic_node_connect(ips[i].address);
    }
    return connections;
}


int main() {
    // SETUP ===========================================================
    dic_main_t *main_server = dic_main_connect(IPV4("172.0.0.1"));
    dic_nodes_info_t *nodes = dic_get_nodes_info(main_server);
    dic_conn_destroy(main_server);

    // initiate all connections
    dic_conn_t **conns = init_connections(nodes->nodes, nodes->count); 

    // load SO on all devices
    dic_rso_handle_t *so_handles = malloc(sizeof(dic_rso_handle_t)*nodes->count);
    for(int i=0; i<nodes->count; i++)
        so_handles[i] = dic_so_load(conns[i], "find-primes");

    // load function pointers on all devices
    dic_rfunc_ptr_t *func_ptrs = malloc(sizeof(dic_rfunc_ptr_t)*nodes->count);
    for(int i=0; i<nodes->count; i++)
        func_ptrs[i] = dic_func_load(so_handles[i], STR(find_primes));

    // ==================================================================

    int CHUNK_SIZE = 100000;
    int current_offset = 1;

    int range_size = (RANGE_END - RANGE_START) / nodes->count;
    int primes_total = 0;

    // array for holding remote thread IDs
    dic_rthread_t *rthreads = malloc(sizeof(dic_rthread_t)*nodes->count);

    // Assign work
    for (int i = 0; i < nodes->count; i++) {
        int start = RANGE_START + i * range_size;
        int end = (i == nodes->count - 1) ? RANGE_END : start + range_size - 1;

        struct find_primes_args args;
        args.range_start = start;
        args.range_end = end;

        dic_rvoid_ptr_t r_args = dic_rmalloc(conns[i], sizeof(args));
        dic_memcpy(&args, r_args, sizeof(args), LOCAL_2_REMOTE);

        rthreads[i] = dic_rthread_run(func_ptrs[i], r_args);
    }

    // Collect results
    for (int i = 0; i < nodes->count; i++) {
        dic_rvoid_ptr_t ret = dic_rthread_join(rthreads[i]);

        uint64_t arr_size;
        dic_memcpy(&arr_size, ret, sizeof(arr_size), REMOTE_2_LOCAL);

        uint64_t *arr = malloc(sizeof(uint64_t)*arr_size);
        dic_memcpy(arr, (dic_rvoid_ptr_t){ret.ptr+sizeof(uint64_t), ret.device}, arr_size*sizeof(uint64_t), REMOTE_2_LOCAL);

        // print results
        for(int j=0; j<arr_size; j++){
            // printf("found: %lu\n", arr[j]);
        }
        primes_total += arr_size;

        printf("Found %lu primes on node %d\n", arr_size, i);

        dic_rfree(ret);
        free(arr);
    }

    printf("Total primes found: %d\n", primes_total);


    for(int i=0; i<nodes->count; i++){
        dic_so_unload(so_handles[i]);
        dic_conn_destroy(conns[i]);
    }

    return 0;
}
