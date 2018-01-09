#define _XOPEN_SOURCE 700

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>

#include <lmdb.h>
#include <metal_storage/storage.h>

#include "hollow_heap.h"
#include "extent.h"
#include "inode.h"
#include "meta.h"

#include "metal.h"

typedef struct mtl_dir {
    uint64_t length;
    mtl_directory_entry_head *first;
    mtl_directory_entry_head *next;
} mtl_dir;

MDB_env *env;
mtl_storage_metadata metadata;

int mtl_initialize(const char *metadata_store) {

    mdb_env_create(&env);
    mdb_env_set_maxdbs(env, 4); // inodes, extents, heap, meta
    int res = mdb_env_open(env, metadata_store, 0, 0644);

    if (res == MDB_INVALID) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    mtl_create_root_directory(txn);

    // Query storage metadata
    mtl_storage_get_metadata(&metadata);

    // Create a single extent spanning the entire storage (if necessary)
    mtl_initialize_extents(txn, metadata.num_blocks);

    mdb_txn_commit(txn);

    return MTL_SUCCESS;
}

int mtl_deinitialize() {
    mtl_reset_extents_db();
    mtl_reset_inodes_db();
    mtl_reset_meta_db();
    mtl_reset_heap_db();

    mdb_env_close(env);

    return MTL_SUCCESS;
}

int mtl_chown(const char *path, uid_t uid, gid_t gid) {

    int res;

    MDB_txn *txn;
    res = mdb_txn_begin(env, NULL, 0, &txn);
    
    uint64_t inode_id;
    res = mtl_resolve_inode(txn, path, &inode_id);
    if (res != MTL_SUCCESS) {
        mdb_txn_abort(txn);
        return -res;
    }
    
    mtl_inode *old_inode;
    mtl_inode new_inode;
    void *data;
    uint64_t data_length;
    res = mtl_load_inode(txn, inode_id, &old_inode, &data, &data_length);
    if (res != MTL_SUCCESS) {
        mdb_txn_abort(txn);
        return -res;
    }

    memcpy(&new_inode, old_inode, sizeof(mtl_inode));

    new_inode.user = uid;
    if ((int)gid >= 0) {
        new_inode.group = gid;
    }
    res = mtl_put_inode(txn, inode_id, &new_inode, data, data_length);
    
    mdb_txn_commit(txn);
    return res;
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

    if (strcmp(base, "/") == 0) {
        *inode_id = 0;
        res = MTL_SUCCESS;
    } else {
        res = mtl_resolve_inode_in_directory(txn, dir_inode_id, base, inode_id);
    }

    free(dirc), free(basec);
    return res;
}

int mtl_resolve_parent_dir_inode(MDB_txn *txn, const char *path, uint64_t *inode_id) {

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

int mtl_get_inode(const char *path, mtl_inode *inode) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    uint64_t inode_id;
    int res = mtl_resolve_inode(txn, path, &inode_id);
    if (res != MTL_SUCCESS) { // early exit because inode resolving failed
        mdb_txn_abort(txn);
        return res;
    }

    const mtl_inode *db_inode;
    res = mtl_load_inode(txn, inode_id, &db_inode, NULL, NULL); // NULLs because we are only interested in the inode, not the data behind it
    if (res == MTL_SUCCESS) {
        memcpy(inode, db_inode, sizeof(mtl_inode)); // need to copy because db_inode pointer points to invalid memory after aborting the DB transaction
    }

    mdb_txn_abort(txn);
    return res;
}

int mtl_open(const char* filename, uint64_t *inode_id) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    uint64_t id;
    int res = mtl_resolve_inode(txn, filename, &id);

    mdb_txn_abort(txn);

    if (inode_id)
        *inode_id = id;

    return res;
}

int mtl_opendir(const char *filename, mtl_dir **dir) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    uint64_t inode_id;
    int res = mtl_resolve_inode(txn, filename, &inode_id);

    mtl_inode *dir_inode;
    mtl_directory_entry_head *dir_entries;
    res = mtl_load_directory(txn, inode_id, &dir_inode, &dir_entries, NULL);

    char* dir_data = malloc(sizeof(mtl_dir) + dir_inode->length);

    *dir = (mtl_dir*) dir_data;
    (*dir)->length = dir_inode->length;
    (*dir)->next = (mtl_directory_entry_head*) (dir_data + sizeof(mtl_dir));
    (*dir)->first = (*dir)->next;

    memcpy(dir_data + sizeof(mtl_dir), dir_entries, dir_inode->length);

    // we can abort because we only read
    mdb_txn_abort(txn);

    return res;
}

int mtl_readdir(mtl_dir *dir, char *buffer, uint64_t size) {

    if (dir->next != NULL) {
        strncpy(
            buffer,
            (char*) dir->next + sizeof(mtl_directory_entry_head),
            size < dir->next->name_len ? size : dir->next->name_len
        );

        // Null-terminate
        if (size > dir->next->name_len)
            buffer[dir->next->name_len] = '\0';

        mtl_directory_entry_head *next = (mtl_directory_entry_head*) (
            (char*) dir->next +
            sizeof(mtl_directory_entry_head) +
            dir->next->name_len
        );

        if ((char*) next + sizeof(mtl_directory_entry_head) > (char*) dir->first + dir->length) {
            next = NULL;
        }
        dir->next = next;

        return MTL_SUCCESS;
    }

    return MTL_COMPLETE;
}

int mtl_closedir(mtl_dir *dir) {
    free(dir);
    return MTL_SUCCESS;
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

int mtl_create(const char *filename, uint64_t *inode_id) {

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

    if (inode_id)
        *inode_id = file_inode_id;

    return MTL_SUCCESS;
}

int mtl_write(uint64_t inode_id, const char *buffer, uint64_t size, uint64_t offset) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    // Check how long we intend to write
    uint64_t write_end_bytes = offset + size;
    uint64_t write_end_blocks = write_end_bytes / metadata.block_size;
    if (write_end_bytes % metadata.block_size)
        ++write_end_blocks;

    uint64_t current_inode_length_blocks;
    {
        mtl_inode *inode;
        mtl_load_file(txn, inode_id, &inode, NULL, NULL);
        current_inode_length_blocks = inode->length;
    }

    while (current_inode_length_blocks < write_end_blocks) {

        // Allocate a new occupied extent with the requested length
        mtl_file_extent new_extent;
        new_extent.length = mtl_reserve_extent(txn, write_end_blocks - current_inode_length_blocks, &new_extent.offset, true);
        current_inode_length_blocks += new_extent.length;

        // Assign it to the file
        uint64_t new_length = write_end_bytes > current_inode_length_blocks * metadata.block_size
            ? current_inode_length_blocks * metadata.block_size
            : write_end_bytes;
        mtl_add_extent_to_file(txn, inode_id, &new_extent, new_length);
    }

    mdb_txn_commit(txn);

    // Prepare the storage
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    const mtl_file_extent *extents;
    uint64_t extents_length;
    mtl_load_file(txn, inode_id, NULL, &extents, &extents_length);
    mtl_storage_set_active_extent_list(extents, extents_length);
    mdb_txn_abort(txn);

    // Copy the actual data to storage
    mtl_storage_write(offset, buffer, size);

    return MTL_SUCCESS;
}

uint64_t mtl_read(uint64_t inode_id, char *buffer, uint64_t size, uint64_t offset) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    uint64_t read_len = size;

    // Prepare the storage and check how much we can read
    mtl_inode *inode;
    const mtl_file_extent *extents;
    uint64_t extents_length;
    mtl_load_file(txn, inode_id, &inode, &extents, &extents_length);

    if (inode->length < offset) {
        read_len = 0;
    } else if (inode->length < offset + size) {
        read_len -= (offset + size) - inode->length;
    }

    mtl_storage_set_active_extent_list(extents, extents_length);

    mdb_txn_abort(txn);

    if (read_len > 0)
        // Copy the actual data from storage
        mtl_storage_read(offset, buffer, read_len);

    return read_len;
}

int mtl_truncate(uint64_t inode_id, uint64_t offset) {

    MDB_txn *txn;
    mdb_txn_begin(env, NULL, 0, &txn);

    mtl_inode *inode;
    const mtl_file_extent *extents;
    uint64_t extents_length;
    mtl_load_file(txn, inode_id, &inode, &extents, &extents_length);

    if (offset >= inode->length) {
        // That was easy
        mdb_txn_abort(txn);
        return MTL_SUCCESS;
    }

    // Figure out how many extents we can keep
    uint64_t previous_extent_end = 0;
    for (uint64_t i = 0; i < extents_length; ++i) {
        uint64_t extent_end = previous_extent_end + (extents[i].length * metadata.block_size);

        if (previous_extent_end > offset) {
            // The current extent has to be freed

            mtl_free_extent(txn, extents[i].offset);
        } else if (extent_end > offset) {
            // This is the first extent that has to be modified or dropped
            // Update the inode in the process

            uint64_t new_extent_length_bytes = offset - previous_extent_end;
            uint64_t new_extent_length_blocks = new_extent_length_bytes / metadata.block_size;
            if (new_extent_length_bytes % metadata.block_size)
                ++new_extent_length_blocks;

            if (new_extent_length_blocks) {
                // We have to modify the extent
                mtl_truncate_extent(txn, extents[i].offset, new_extent_length_blocks);
                mtl_truncate_file_extents(txn, inode_id, offset, i + 1, &new_extent_length_blocks);
            } else {
                // We can drop the extent
                mtl_free_extent(txn, extents[i].offset);
                mtl_truncate_file_extents(txn, inode_id, offset, i, NULL);
            }
        } else {
            // This extent is still valid -- keep unmodified
        }

        previous_extent_end = extent_end;
    }

    mdb_txn_commit(txn);

    return MTL_SUCCESS;
}
