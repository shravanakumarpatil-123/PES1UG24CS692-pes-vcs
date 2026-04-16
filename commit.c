#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_entries(const void *a, const void *b) {
    const TreeEntry *e1 = (const TreeEntry *)a;
    const TreeEntry *e2 = (const TreeEntry *)b;
    return strcmp(e1->name, e2->name);
}

int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {

    Tree sorted = *tree;
    qsort(sorted.entries, sorted.count, sizeof(TreeEntry), compare_entries);

    size_t size = 0;

    for (int i = 0; i < sorted.count; i++) {
        size += strlen(sorted.entries[i].name) + HASH_SIZE + 16;
    }

    char *buffer = malloc(size);
    size_t offset = 0;

    for (int i = 0; i < sorted.count; i++) {

        TreeEntry *e = &sorted.entries[i];

        int written = sprintf(buffer + offset, "%o %s", e->mode, e->name);
        offset += written + 1;

        memcpy(buffer + offset, e->hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }

    *data_out = buffer;
    *len_out = offset;

    return 0;
}

int tree_parse(const void *data, size_t len, Tree *tree_out) {

    tree_out->count = 0;

    const char *ptr = data;
    const char *end = ptr + len;

    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {

        TreeEntry *e = &tree_out->entries[tree_out->count];

        sscanf(ptr, "%o", &e->mode);

        char *space = strchr(ptr, ' ');
        if (!space) return -1;
        ptr = space + 1;

        char *null = memchr(ptr, '\0', end - ptr);
        if (!null) return -1;

        size_t name_len = null - ptr;

        size_t copy_len = name_len < sizeof(e->name) - 1 ? name_len : sizeof(e->name) - 1;
        memcpy(e->name, ptr, copy_len);
        e->name[copy_len] = '\0';

        ptr = null + 1;

        if (ptr + HASH_SIZE > end) return -1;

        memcpy(e->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;

        tree_out->count++;
    }

    return 0;
}

int tree_from_index(ObjectID *id_out) {

    Index idx;
    index_load(&idx);

    if (idx.count == 0) return -1;

    Tree tree;
    tree.count = 0;

    for (int i = 0; i < idx.count; i++) {

        TreeEntry *e = &tree.entries[tree.count];

        e->mode = idx.entries[i].mode;

        const char *name = strrchr(idx.entries[i].path, '/');
        if (name) name++;
        else name = idx.entries[i].path;

        strncpy(e->name, name, sizeof(e->name) - 1);
        e->name[sizeof(e->name) - 1] = '\0';

        e->hash = idx.entries[i].hash;

        tree.count++;
    }

    void *data;
    size_t len;

    if (tree_serialize(&tree, &data, &len) != 0) return -1;

    if (object_write(OBJ_TREE, data, len, id_out) != 0) {
        free(data);
        return -1;
    }

    free(data);
    return 0;
}
