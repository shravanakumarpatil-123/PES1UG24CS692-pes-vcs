#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define INDEX_FILE ".pes/index"

int index_load(Index *idx) {

    idx->count = 0;

    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 0;

    char path[256], hash_hex[HASH_HEX_SIZE + 1];
    int mode;

    while (fscanf(f, "%o %s %s\n", &mode, hash_hex, path) == 3) {

        idx->entries[idx->count].mode = mode;
        strcpy(idx->entries[idx->count].path, path);
        hex_to_hash(hash_hex, &idx->entries[idx->count].hash);

        idx->count++;
    }

    fclose(f);
    return 0;
}

int index_save(const Index *idx) {

    mkdir(".pes", 0755);

    FILE *f = fopen(INDEX_FILE, "w");
    if (!f) return -1;

    for (int i = 0; i < idx->count; i++) {

        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&idx->entries[i].hash, hex);

        fprintf(f, "%o %s %s\n",
                idx->entries[i].mode,
                hex,
                idx->entries[i].path);
    }

    fclose(f);
    return 0;
}

int index_add(Index *idx, const char *path) {

    index_load(idx);

    mkdir(".pes", 0755);

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    void *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    ObjectID id;
    object_write(OBJ_BLOB, data, size, &id);
    free(data);

    for (int i = 0; i < idx->count; i++) {
        if (strcmp(idx->entries[i].path, path) == 0) {
            idx->entries[i].hash = id;
            index_save(idx);
            return 0;
        }
    }

    strcpy(idx->entries[idx->count].path, path);
    idx->entries[idx->count].hash = id;
    idx->entries[idx->count].mode = 0100644;

    idx->count++;

    index_save(idx);

    return 0;
}

int index_status(const Index *idx) {

    (void)idx;  // not used

    FILE *f = fopen(".pes/index", "r");
    if (!f) {
        printf("Index contains 0 entries:\n");
        return 0;
    }

    int count = 0;
    char line[512];

    // First pass: count entries
    while (fgets(line, sizeof(line), f)) {
        count++;
    }

    rewind(f);

    printf("Index contains %d entries:\n", count);

    // Second pass: print paths
    while (fgets(line, sizeof(line), f)) {
        char mode[10], hash[70], path[256];

        if (sscanf(line, "%s %s %s", mode, hash, path) == 3) {
            printf("%s %s\n", mode, path);
        }
    }

    fclose(f);
    return 0;
}
