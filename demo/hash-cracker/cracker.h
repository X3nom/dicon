#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <openssl/md5.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_LEN 6
#define CHARSET "abcdefghijklmnopqrstuvwxyz0123456789"

typedef struct {
    uint64_t start_index;
    uint64_t end_index;
    unsigned char *target_hash;
} CrackArgs;


void *crack_thread(void *arg);