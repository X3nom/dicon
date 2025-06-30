#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <openssl/md5.h>
#include <stdint.h>
#include <stdbool.h>
#include "./cracker.h"

const char *charset = CHARSET;
const size_t charset_len = sizeof(CHARSET) - 1;

void index_to_string(uint64_t index, char *out, int length) {
    for (int i = length - 1; i >= 0; --i) {
        out[i] = charset[index % charset_len];
        index /= charset_len;
    }
    out[length] = '\0';
}

void *crack_thread(void *arg) {
    CrackArgs *args = (CrackArgs *)arg;

    char candidate[MAX_LEN + 1];
    unsigned char hash[MD5_DIGEST_LENGTH];

    for (uint64_t idx = args->start_index; idx < args->end_index; ++idx) {
        // Choose your fixed length (e.g. 5)
        index_to_string(idx, candidate, MAX_LEN);

        MD5((unsigned char *)candidate, MAX_LEN, hash);

        if (memcmp(hash, args->target_hash, MD5_DIGEST_LENGTH) == 0) {
            printf("Found collision: %s (index: %lu)\n", candidate, idx);
            break;
        }
    }

    char *candidate_allocated = malloc(sizeof(candidate));
    strcpy(candidate_allocated, candidate);
    return candidate_allocated;
}