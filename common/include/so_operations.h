#pragma once

#include <inttypes.h>
#include <stdio.h>

// output of file checksums (unsigned 64-bit int)
typedef uint64_t dic_checksum_t;

dic_checksum_t dic_checksum(const char *path);
