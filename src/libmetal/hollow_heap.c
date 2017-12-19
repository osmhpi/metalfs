#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "metal.h"

#include "hollow_heap.h"

#define HEAP_DB_NAME "heap"
#define INVALID_NODE 0

const char hollow_heap_key[] = "hollow-heap";

MDB_dbi heap_db = 0;

typedef uint64_t mtl_heap_node_id;

typedef struct mtl_heap_node {
    mtl_heap_node_id second_parent;
    mtl_heap_node_id next;
    mtl_heap_node_id child;
    uint64_t rank;
    bool hollow;

    uint64_t key;
    uint64_t value;
} mtl_heap_node;

typedef struct mtl_temp_heap_node {
    const mtl_heap_node *storage_node;

    struct mtl_temp_heap_node *second_parent;
    struct mtl_temp_heap_node *next;
    uint64_t rank;
} mtl_temp_heap_node;

typedef struct mtl_hollow_heap {
    uint64_t size;
    uint64_t max_rank;
    mtl_heap_node_id root;
} mtl_hollow_heap;

int mtl_ensure_heap_db_open(MDB_txn *txn) {
    if (heap_db == 0)
        mdb_dbi_open(txn, HEAP_DB_NAME, MDB_CREATE, &heap_db);
    return 0;
}

mtl_heap_node* mtl_link(mtl_heap_node *first, mtl_heap_node *second) {
    return NULL;
}

mtl_heap_node* mtl_ranked_link_nodes(mtl_heap_node *first, mtl_heap_node *second) {
    return NULL;
}

int mtl_load_node(MDB_txn *txn, mtl_heap_node_id node_id, mtl_heap_node **node) {
    if (node_id == INVALID_NODE) {
        *node = NULL;
        return MTL_SUCCESS;
    }

    mtl_ensure_heap_db_open(txn);
    MDB_val node_key = { .mv_size = sizeof(node_id), .mv_data = &node_id };
    MDB_val node_value;
    if (mdb_get(txn, heap_db, &node_key, &node_value) == MDB_NOTFOUND) {
        return MTL_ERROR_NOENTRY;
    }

    *node = (mtl_heap_node*) node_value.mv_data;
    return MTL_SUCCESS;
}

int mtl_del_node(uint64_t node_id) {
    if (node_id == INVALID_NODE)
        return MTL_ERROR_INVALID_ARGUMENT;

    return NULL;
}

mtl_heap_node* mtl_node_set_second_parent(const mtl_heap_node *node, uint64_t second_parent) {
    return NULL;
}

mtl_heap_node* mtl_node_set_next(const mtl_heap_node *node, uint64_t next) {
    return NULL;
}

mtl_heap_node* mtl_node_set_child(const mtl_heap_node *node, uint64_t child) {
    return NULL;
}

mtl_heap_node* mtl_node_set_rank(const mtl_heap_node *node, uint64_t rank) {
    return NULL;
}

mtl_heap_node* mtl_node_set_hollow(const mtl_heap_node *node, bool hollow) {
    return NULL;
}

int mtl_insert(mtl_heap_node *pq_root, mtl_heap_node *node) {
    link(pq_root, node);
    return 0;
}

mtl_temp_heap_node* mtl_create_temp_heap_node(const mtl_heap_node *node) {

    mtl_temp_heap_node *result = calloc(1, sizeof(mtl_temp_heap_node));
    return result;
}

mtl_temp_heap_node* mtl_load_temp_heap_node(MDB_txn *txn, mtl_heap_node_id node_id) {

    mtl_heap_node *storage_node;
    mtl_load_node(txn, node_id, &storage_node);

    if (storage_node == NULL)
        return NULL;

    return mtl_create_temp_heap_node(storage_node);
}

int mtl_delete_node(MDB_txn *txn, const mtl_hollow_heap *heap, const mtl_heap_node *node, mtl_heap_node_id node_id) {

    // Set node to be hollow
    node = mtl_node_set_hollow(node, true);

    // If the node wasn't the heap's root, there's nothing more to do
    if (heap->root != node->key)
        return MTL_SUCCESS;

    // Remove hollow roots, starting with 'node'

    mtl_temp_heap_node *next_deleted_root_node = mtl_create_temp_heap_node(node);
    mtl_heap_node_id next_deleted_root_node_id = node_id;

    mtl_temp_heap_node *hollow_roots_list_head = next_deleted_root_node;

    uint64_t upper_bound_of_max_rank = ceil(log2(heap->size));
    mtl_temp_heap_node *filled_roots_by_rank_list_heads[upper_bound_of_max_rank]; // Hopefully fits onto the stack

    // Iterate over the graph, finding both hollow roots (to be deleted) or
    // filled roots (to be ranked_link'ed later on)

    while (next_deleted_root_node) {

        // Look at all children because they will become roots after deleting 'next_deleted_root_node'
        mtl_temp_heap_node *current_child_node;
        mtl_temp_heap_node *next_child_node = mtl_load_temp_heap_node(txn, next_deleted_root_node->storage_node->child);

        while (next_child_node) {
            current_child_node = next_child_node;
            next_child_node = mtl_load_temp_heap_node(txn, current_child_node->storage_node->next);

            if (current_child_node->storage_node->hollow) {
                // If it doesn't have a second parent either, append it to the list of hollow roots
                if (current_child_node->storage_node->second_parent == INVALID_NODE) {
                    current_child_node->next = hollow_roots_list_head;
                    hollow_roots_list_head = current_child_node;
                } else {
                    if (current_child_node->storage_node->second_parent == next_deleted_root_node_id) {
                        // We've reached the (only) 'secondary child' of the 'next_deleted_root_node'.
                        // This is the end of the child list, so stop iterating here.

                        // TODO: Fix second_parent shadowing
                        current_child_node->second_parent = NULL;
                        break;
                    } else {
                        // TODO:
                        current_child_node->second_parent = NULL;
                        current_child_node->next = NULL;
                    }
                }
            } else {

            }

        }

    }


    // uint64_t max_rank = 0;
    // mtl_heap_node *root_node = node;
    // root_node = mtl_node_set_next(root_node, INVALID_NODE);

    // while (root_node) {
    //     mtl_heap_node *current = mtl_get_node(root_node->child);
    //     mtl_heap_node *parent = root_node;
    //     mtl_heap_node *previous;

    //     root_node = mtl_get_node(root_node->next);

    //     while (current) {
    //         previous = current;
    //         current = mtl_get_node(current->next);

    //         if (previous->hollow) {
    //             if (!previous->second_parent) {
    //                 previous = mtl_node_set_next(previous, root_node->key);
    //                 root_node = previous;
    //             } else {
    //                 if (previous->second_parent == parent->key) {
    //                     current = NULL;
    //                 } else {
    //                     previous = mtl_node_set_next(previous, INVALID_NODE);
    //                 }
    //                 previous = mtl_node_set_second_parent(previous, INVALID_NODE);
    //             }
    //         } else {
    //             while (nodes_by_rank[previous->rank]) {
    //                 previous = mtl_link_nodes(previous, nodes_by_rank[previous->rank]);
    //                 previous = mtl_node_set_rank(previous, previous->rank + 1);
    //                 nodes_by_rank[previous->rank] = NULL;
    //             }
    //             nodes_by_rank[previous->rank] = previous;
    //             if (previous->rank > max_rank) {
    //                 max_rank = previous->rank;
    //             }
    //         }
    //     }

    //     mtl_del_node(parent->key);
    // }

    // for (uint64_t i = 0; i < max_rank; ++i) {
    //     if (nodes_by_rank[i]) {
    //         if (!root_node) {
    //             root_node = nodes_by_rank[i];
    //         } else {
    //             root_node = link(root_node, nodes_by_rank[i]);
    //         }
    //         nodes_by_rank[i] = NULL;
    //     }
    // }

    return 0;
}

int mtl_load_or_create_heap(MDB_txn *txn, const mtl_hollow_heap **heap) {

    mtl_ensure_heap_db_open(txn);

    MDB_val heap_key = { .mv_size = sizeof(hollow_heap_key), .mv_data = hollow_heap_key };
    MDB_val heap_value;
    if (mdb_get(txn, heap_db, &heap_key, &heap_value) == MDB_NOTFOUND) {

        mtl_hollow_heap new_heap = { .size = 0, .root = INVALID_NODE };
        heap_value.mv_data = &new_heap;
        heap_value.mv_size = sizeof(new_heap);
        mdb_put(txn, heap_db, &heap_key, &heap_value, 0);

        return mtl_load_or_create_heap(txn, heap);
    }

    *heap = (mtl_hollow_heap*) heap_value.mv_data;
    return MTL_SUCCESS;
}

int mtl_heap_extract_max(MDB_txn *txn, uint64_t *max_value) {

    const mtl_hollow_heap *heap;
    mtl_load_or_create_heap(txn, &heap);

    if (heap->root == INVALID_NODE)
        return MTL_ERROR_NOENTRY;

    mtl_heap_node *node;
    mtl_load_node(txn, heap->root, &node);

    *max_value = node->value;

    mtl_delete_node(txn, heap, heap->root, node);

    return MTL_SUCCESS;
}
