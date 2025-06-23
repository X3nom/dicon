#include <so_operations.h>

dic_checksum_t dic_checksum(const char *path){
    FILE *file = fopen(path, "rb");
    if (!file) return 0;

    dic_checksum_t sum = 0;
    uint8_t buf[4096];
    size_t len;

    while ((len = fread(buf, 1, sizeof(buf), file)) > 0) {
        for(size_t i = 0; i < len; ++i){
            sum = (sum << 1) | (sum >> 63);
            sum ^= buf[i];
        }
    }

    fclose(file);
    return sum;
}