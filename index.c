int index_add(Index *index, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        perror("stat");
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("open file");
        return -1;
    }

    char *buf = malloc(st.st_size);
    fread(buf, 1, st.st_size, f);
    fclose(f);

    ObjectID oid;
    if (object_write(OBJ_BLOB, buf, st.st_size, &oid) != 0) {
        free(buf);
        return -1;
    }
    free(buf);

    IndexEntry *entry = index_find(index, path);
    if (!entry) {
        entry = &index->entries[index->count++];
    }

    entry->mode = st.st_mode;
    hash_to_hex(&oid, entry->hash);
    entry->mtime_sec = st.st_mtime;
    entry->size = st.st_size;
    strcpy(entry->path, path);

    return index_save(index);
}
