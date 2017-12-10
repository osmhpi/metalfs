#pragma once

#include <stdint.h>

#include <lmdb.h>

typedef enum MTL_inode_type {
    MTL_FILE = 'f',
    MTL_DIRECTORY = 'd'
} MTL_inode_type;

typedef struct mtl_inode {
    MTL_inode_type type;
    uint64_t length;
    int user;
    int group;
    int accessed;
    int modified;
    int created;
} mtl_inode;

typedef struct mtl_directory_entry_head {
    uint64_t inode_id;
    uint8_t name_len;
} mtl_directory_entry_head;

int mtl_resolve_inode_in_directory(MDB_txn *txn, uint64_t dir_inode_id, char *filename, uint64_t *file_inode_id);

int mtl_create_root_directory(MDB_txn *txn);
int mtl_create_directory_in_directory(MDB_txn *txn, uint64_t dir_inode_id, char *filename, uint64_t *directory_file_inode_id);
int mtl_create_file_in_directory(MDB_txn *txn, uint64_t dir_inode_id, char *filename, uint64_t *file_inode_id);

int mtl_reset_inodes_db();
