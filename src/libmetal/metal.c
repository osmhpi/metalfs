#define _XOPEN_SOURCE 700

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

#include <lmdb.h>

#include "extent.h"
#include "inode.h"
#include "meta.h"

#include "metal.h"

MDB_env *env;

int mtl_initialize(const char *metadata_store) {

    mdb_env_create(&env);
    mdb_env_set_maxdbs(env, 3); // inodes, extents, meta
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

int mtl_write(uint64_t inode_id, const char *buffer, uint64_t size, uint64_t offset) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    // Check how long we intend to write
    uint64_t write_end = offset + size;

    mtl_inode *inode;
    mtl_file_extent *extents;
    uint64_t extents_length;
    mtl_load_file(txn, inode_id, &inode, &extents, &extents_length);

    while (inode->length < write_end) {

        // Allocate a new occupied extent with the requested length
        mtl_file_extent new_extent;
        new_extent.length = mtl_reserve_extent(inode->length - write_end, &new_extent.offset);

        // Assign it to the file
        mtl_add_extent_to_file(txn, inode_id, inode, extents, extents_length, &new_extent);
    }


    mdb_txn_commit(txn);

    // Copy the actual data to the FPGA
    
    return MTL_SUCCESS;
}
