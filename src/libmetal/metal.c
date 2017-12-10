#define _XOPEN_SOURCE 700

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

#include <lmdb.h>

#include "inode.h"

#include "metal.h"

MDB_env *env;

int mtl_initialize(const char *metadata_store) {

    mdb_env_create(&env);
    mdb_env_set_maxdbs(env, 2); // inodes, extents
    mdb_env_open(env, metadata_store, 0, 0644);

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    mtl_create_root_directory(txn);

    // Create a single extent spanning the entire storage
    // TODO

    mdb_txn_commit(txn);

    return 0;
}

int mtl_deinitialize() {
    mtl_reset_extents_db();
    mtl_reset_inodes_db();
    mtl_reset_meta_db();

    mdb_env_close(env);

    return MTL_SUCCESS;
}

int mtl_resolve_inode(MDB_txn *txn, const char *path, uint64_t *inode_id) {

    int res;
    char *dirc, *basec;
    char *dir, *base;

    dirc = strdup(path);
    basec = strdup(path);

    dir = dirname(dirc);
    base = basename(basec);

    uint64_t dir_inode_id;
    if (strcmp(dir, "/") == 0) {
        // Perform a lookup in the root directory file
        dir_inode_id = 0;
    } else {
        res = mtl_resolve_inode(txn, dir, &dir_inode_id);
        if (res != MTL_SUCCESS) {
            free(dirc), free(basec);
            return res;
        }
    }

    res = mtl_resolve_inode_in_directory(txn, dir_inode_id, base, inode_id);
    free(dirc), free(basec);
    return res;
}

int mtl_resolve_parent_dir_inode(MDB_txn *txn, const char *path, uint64_t *inode_id)
{
    int res;
    char *dirc, *dir;

    dirc = strdup(path);
    dir = dirname(dirc);

    if (strcmp(dir, "/") == 0) {
        *inode_id = 0;
        res = MTL_SUCCESS;
    } else {
        res = mtl_resolve_inode(txn, dir, inode_id);
    }
    
    free(dirc);
    return res;
}

int mtl_open(const char* filename) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    // // Open the files database if necessary
    // if (inodes_db == 0)
    //     mdb_dbi_open(txn, INODES_DB_NAME, MDB_CREATE, &inodes_db);

    // // Try to find a file object with the provided name
    // MDB_val key = { .mv_size = strlen(filename), .mv_data = filename };
    // MDB_val inode_val;
    // int res = mdb_get(txn, inodes_db, &key, &inode_val);
    // if (res == MDB_NOTFOUND) {
    //     // Create if not exists
    //     mtl_inode inode = { .type = 0 };
    //     inode_val.mv_size = sizeof(inode), inode_val.mv_data = &inode;
    //     mdb_put(txn, inodes_db, &key, &inode_val, 0);
    // }

    
    uint64_t inode_id;
    int res = mtl_resolve_inode(txn, filename, &inode_id);
    
    mdb_txn_abort(txn);

    return res;
}

int mtl_mkdir(const char *filename) {

    int res;

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    uint64_t parent_dir_inode_id;
    res = mtl_resolve_parent_dir_inode(txn, filename, &parent_dir_inode_id);
    if (res != MTL_SUCCESS) {
        mdb_txn_abort(txn);
        return res;
    }

    char *basec, *base;
    basec = strdup(filename);
    base = basename(basec);

    uint64_t new_dir_inode_id;
    res = mtl_create_directory_in_directory(txn, parent_dir_inode_id, base, &new_dir_inode_id);
    if (res != MTL_SUCCESS) {
        mdb_txn_abort(txn);
        free(basec);
        return res;
    }

    mdb_txn_commit(txn);
    free(basec);
    return MTL_SUCCESS;
}

int mtl_create(const char *filename) {

    int res;

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    uint64_t parent_dir_inode_id;
    res = mtl_resolve_parent_dir_inode(txn, filename, &parent_dir_inode_id);
    if (res != MTL_SUCCESS) {
        mdb_txn_abort(txn);
        return res;
    }

    char *basec, *base;
    basec = strdup(filename);
    base = basename(basec);

    uint64_t file_inode_id;
    res = mtl_create_file_in_directory(txn, parent_dir_inode_id, base, &file_inode_id);
    if (res != MTL_SUCCESS) {
        mdb_txn_abort(txn);
        free(basec);
        return res;
    }

    mdb_txn_commit(txn);
    free(basec);
    return MTL_SUCCESS;
}
