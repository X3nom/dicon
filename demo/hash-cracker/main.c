#include <dicon.h>

#include "./cracker.h"


#define RANGE_START 1
#define RANGE_END 100000000
// #define CHUNK_SIZE 10000

#define LOCAL_SO_PATH "./cracker.so"
#define REMOTE_NAME "cracker"

#define TARGET_PLAIN "hello"

int main() {
    const char *target_plain = TARGET_PLAIN;
    unsigned char target_hash[MD5_DIGEST_LENGTH];
    MD5((unsigned char *)target_plain, strlen(target_plain), target_hash);

    // SETUP ===========================================================
    dic_main_t *main_server = dic_main_connect(ipv4_from_str("127.0.0.1"));

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
        dic_so_upload(nodes[i], LOCAL_SO_PATH , REMOTE_NAME);

        so_handles[i] = dic_so_load(nodes[i], REMOTE_NAME);
    }



    // load function pointers on all devices
    dic_rfunc_ptr_t *func_ptrs = malloc(sizeof(dic_rfunc_ptr_t)*nodes_info->count);
    for(int i=0; i<nodes_info->count; i++)
        func_ptrs[i] = dic_func_load(so_handles[i], STR(crack_thread));



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

            dic_rvoid_ptr_t remote_target_hash = dic_rmalloc(nodes[node_i], sizeof(target_hash));
            dic_memcpy(target_hash, remote_target_hash, sizeof(target_hash), DIC_MEMCPY_C2N); // copy from client to node

            CrackArgs args;
            args.start_index = start;
            args.end_index = end;
            args.target_hash = (void*)remote_target_hash.ptr;
            

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
