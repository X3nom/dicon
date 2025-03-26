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
#define RANGE_END 10000
#define CHUNK_SIZE 10000

dic_conn_t **init_connections(ip_addr_t *ips, int count, int port){
    dic_conn_t **connections = malloc(sizeof(dic_conn_t*)*count); 
    for(int i=0; i<count; i++){
        connections[i] = dic_conn_new(ips[i], port);
    }
    return connections;
}


int main() {
    // SETUP ===========================================================
    int port = 12345;
    ip_addr_t ips[] = {IPV4("127.0.0.0"), IPV4("172.17.17.102")};

    int NODE_COUNT = sizeof(ips) / sizeof(ip_addr_t);

    // initiate all connections
    dic_conn_t **conns = init_connections(ips, NODE_COUNT, port); 

    // load SO on all devices
    dic_rso_handle_t *so_handles = malloc(sizeof(dic_rso_handle_t)*NODE_COUNT);
    for(int i=0; i<NODE_COUNT; i++)
        so_handles[i] = dic_so_load(conns[i], "find-primes");

    // load function pointers on all devices
    dic_rfunc_ptr_t *func_ptrs = malloc(sizeof(dic_rfunc_ptr_t)*NODE_COUNT);
    for(int i=0; i<NODE_COUNT; i++)
        func_ptrs[i] = dic_func_load(so_handles[i], STR(find_primes));

    // ==================================================================



    int range_size = (RANGE_END - RANGE_START) / NODE_COUNT;
    int primes_total = 0;

    // array for holding remote thread IDs
    dic_rthread_t *rthreads = malloc(sizeof(dic_rthread_t)*NODE_COUNT);

    // Assign work
    for (int i = 0; i < NODE_COUNT; i++) {
        int start = RANGE_START + i * range_size;
        int end = (i == NODE_COUNT - 1) ? RANGE_END : start + range_size - 1;

        struct find_primes_args args;
        args.range_start = start;
        args.range_end = end;

        dic_rvoid_ptr_t r_args = dic_rmalloc(conns[i], sizeof(args));
        dic_memcpy(&args, r_args, sizeof(args), LOCAL_2_REMOTE);

        rthreads[i] = dic_rthread_run(func_ptrs[i], r_args);
    }

    // Collect results
    for (int i = 0; i < NODE_COUNT; i++) {
        dic_rvoid_ptr_t ret = dic_rthread_join(rthreads[i]);

        uint64_t arr_size;
        dic_memcpy(&arr_size, ret, sizeof(arr_size), REMOTE_2_LOCAL);

        uint64_t *arr = malloc(sizeof(uint64_t)*arr_size);
        dic_memcpy(arr, (dic_rvoid_ptr_t){ret.ptr+sizeof(uint64_t), ret.device}, arr_size*sizeof(uint64_t), REMOTE_2_LOCAL);

        // print results
        for(int j=0; j<arr_size; j++) printf("found: %lu\n", arr[j]);
        primes_total += arr_size;

        dic_rfree(ret);
        free(arr);
    }

    printf("\nTotal primes found: %d\n", primes_total);


    for(int i=0; i<NODE_COUNT; i++){
        dic_so_unload(so_handles[i]);
        dic_conn_destroy(conns[i]);
    }

    return 0;
}
