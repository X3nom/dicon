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
            printf("found prime: %d\n", i);
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


int main() {
    // SETUP ===========================================================
    dic_main_t *main_server = dic_main_connect(IPV4("127.0.0.1"));

    dic_nodes_info_t *nodes_info = dic_get_nodes_info(main_server);
    printf("found %d nodes!\n", nodes_info->count);

    dic_conn_destroy(main_server);

    for(int i=0; i<nodes_info->count; i++){
        printf("  %d -> cores: %d\n", i, nodes_info->nodes[i].core_count);
    }

    // initiate all connections
    dic_conn_t **nodes = dic_nodes_connect_all(nodes_info); 



    // load SO on all devices
    dic_rso_handle_t *so_handles = malloc(sizeof(dic_rso_handle_t)*nodes_info->count);
    for(int i=0; i<nodes_info->count; i++){
        #define REMOTE_NAME "find-primes_"
        // so_handles[i] = dic_so_load(nodes[i], REMOTE_NAME);

        // if(so_handles[i].ptr == 0){ // handle is NULL, upload the .so
        // upload the so
        dic_so_upload(nodes[i], "./find-primes.so" , REMOTE_NAME);

        // try to load the so again (this time shouldn't fail)
        so_handles[i] = dic_so_load(nodes[i], REMOTE_NAME);
        // }
    }



    // load function pointers on all devices
    dic_rfunc_ptr_t *func_ptrs = malloc(sizeof(dic_rfunc_ptr_t)*nodes_info->count);
    for(int i=0; i<nodes_info->count; i++)
        func_ptrs[i] = dic_func_load(so_handles[i], STR(find_primes));



    // ==================================================================

    int total_cores_count = 0;
    for(int i=0; i<nodes_info->count; i++){
        total_cores_count += nodes_info->nodes[i].core_count;
    }

    int range_size = (RANGE_END - RANGE_START) / total_cores_count;
    int primes_total = 0;

    // array for holding remote thread IDs
    dic_rthread_t *rthreads = malloc(sizeof(dic_rthread_t)*total_cores_count);


    // Assign work
    int thread_i = 0;
    for (int node_i=0; node_i < nodes_info->count; node_i++) {
        for(int core_i=0; core_i < nodes_info->nodes[node_i].core_count; core_i++){

            int start = RANGE_START + thread_i * range_size;
            int end = (thread_i == total_cores_count - 1) ? RANGE_END : start + range_size - 1;

            struct find_primes_args args;
            args.range_start = start;
            args.range_end = end;

            dic_rvoid_ptr_t r_args = dic_rmalloc(nodes[node_i], sizeof(args));
            dic_memcpy(&args, r_args, sizeof(args), LOCAL_2_REMOTE);

            rthreads[thread_i] = dic_rthread_run(func_ptrs[node_i], r_args);
            
            thread_i++;
        }
    }


    // Collect results
    for (int i = 0; i < total_cores_count; i++) {
        dic_rvoid_ptr_t ret = dic_rthread_join(rthreads[i]);

        uint64_t arr_size;
        dic_memcpy(&arr_size, ret, sizeof(arr_size), REMOTE_2_LOCAL);

        uint64_t *arr = malloc(sizeof(uint64_t)*arr_size);
        dic_memcpy(arr, (dic_rvoid_ptr_t){ret.ptr+sizeof(uint64_t), ret.device}, arr_size*sizeof(uint64_t), REMOTE_2_LOCAL);


        printf("Found %lu primes on node %d\n", arr_size, i);

        // print results
        for(int j=0; j<arr_size; j++){
            printf("   - found: %lu\n", arr[j]);
        }
        primes_total += arr_size;

        // free the remote memory
        dic_rfree(ret);
        // and also local memory
        free(arr);
    }

    printf("Total primes found: %d\n", primes_total);


    for(int i=0; i<nodes_info->count; i++){
        dic_so_unload(so_handles[i]);
        dic_conn_destroy(nodes[i]);
    }

    return 0;
}
