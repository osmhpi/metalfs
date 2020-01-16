#pragma once

#include <stdint.h>

#include <lmdb.h>

#include <metal-filesystem/storage.h>

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

int mtl_load_file(MDB_txn *txn, uint64_t inode_id, const mtl_inode **inode,
                  const mtl_file_extent **extents, uint64_t *extents_length);
int mtl_load_directory(MDB_txn *txn, uint64_t inode_id, const mtl_inode **inode,
                       mtl_directory_entry_head **dir_entries,
                       uint64_t *entries_length);
int mtl_add_extent_to_file(MDB_txn *txn, uint64_t inode_id,
                           mtl_file_extent *new_extent, uint64_t new_length);
int mtl_extend_last_extent_in_file(MDB_txn *txn, uint64_t inode_id,
                                   mtl_file_extent *new_extent,
                                   uint64_t new_length);
int mtl_truncate_file_extents(MDB_txn *txn, uint64_t inode_id,
                              uint64_t new_file_length, uint64_t extents_length,
                              uint64_t *update_last_extent_length);
int mtl_resolve_inode_in_directory(MDB_txn *txn, uint64_t dir_inode_id,
                                   char *filename, uint64_t *file_inode_id);
int mtl_append_inode_id_to_directory(MDB_txn *txn, uint64_t dir_inode_id,
                                     char *filename, uint64_t inode_id);
int mtl_remove_entry_from_directory(MDB_txn *txn, uint64_t dir_inode_id,
                                    char *filename, uint64_t *inode_id);
int mtl_remove_directory(MDB_txn *txn, uint64_t dir_inode_id);

int mtl_create_root_directory(MDB_txn *txn);
int mtl_create_directory_in_directory(MDB_txn *txn, uint64_t dir_inode_id,
                                      char *filename,
                                      uint64_t *directory_file_inode_id);
int mtl_create_file_in_directory(MDB_txn *txn, uint64_t dir_inode_id,
                                 char *filename, uint64_t *file_inode_id);

int mtl_load_inode(MDB_txn *txn, uint64_t inode_id, const mtl_inode **inode,
                   const void **data, uint64_t *data_length);
int mtl_put_inode(MDB_txn *txn, uint64_t inode_id, mtl_inode *inode,
                  const void *data, uint64_t data_length);
int mtl_delete_inode(MDB_txn *txn, uint64_t inode_id);

int mtl_reset_inodes_db();
