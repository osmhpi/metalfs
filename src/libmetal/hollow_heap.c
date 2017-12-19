#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "metal.h"
#include "meta.h"

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

typedef struct mtl_heap_node_id_list {
    mtl_heap_node_id id;
    struct mtl_heap_node_id_list* next;
} mtl_heap_node_id_list;

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

int mtl_put_heap(MDB_txn *txn, mtl_hollow_heap *heap) {

    MDB_val heap_key = { .mv_size = sizeof(hollow_heap_key), .mv_data = &hollow_heap_key };
    MDB_val heap_value = { .mv_size = sizeof(mtl_hollow_heap), .mv_data = heap };
    mdb_put(txn, heap_db, &heap_key, &heap_value, 0);

    return MTL_SUCCESS;
}

int mtl_load_node(MDB_txn *txn, mtl_heap_node_id node_id, mtl_heap_node **node) {
    if (node_id == INVALID_NODE) {
        *node = NULL;
        return MTL_SUCCESS;
    }

    MDB_val node_key = { .mv_size = sizeof(node_id), .mv_data = &node_id };
    MDB_val node_value;
    if (mdb_get(txn, heap_db, &node_key, &node_value) == MDB_NOTFOUND) {
        return MTL_ERROR_NOENTRY;
    }

    *node = (mtl_heap_node*) node_value.mv_data;
    return MTL_SUCCESS;
}

int mtl_put_node(MDB_txn *txn, mtl_heap_node_id node_id, mtl_heap_node *node) {
    MDB_val node_key = { .mv_size = sizeof(node_id), .mv_data = &node_id };
    MDB_val node_data = { .mv_size = sizeof(*node), .mv_data = node };

    mdb_put(txn, heap_db, &node_key, &node_data, 0);

    return MTL_SUCCESS;
}

mtl_heap_node_id mtl_link(MDB_txn *txn, mtl_heap_node_id first_id, mtl_heap_node first, mtl_heap_node_id second_id, mtl_heap_node second, bool ranked) {
    mtl_heap_node_id root;

    if (first.key >= second.key) {
        second.next = first.child;
        first.child = second_id;
        root = first_id;
        if (ranked)
            ++first.rank;
    } else {
        first.next = second.child;
        second.child = first_id;
        root = second_id;
        if (ranked)
            ++second.rank;
    }

    mtl_put_node(txn, first_id, &first);
    mtl_put_node(txn, second_id, &second);

    return root;
}

mtl_heap_node_id mtl_link_ids(MDB_txn *txn, mtl_heap_node_id first, mtl_heap_node_id second, bool ranked) {
    mtl_heap_node *first_node;
    mtl_load_node(txn, first, &first_node);

    mtl_heap_node *second_node;
    mtl_load_node(txn, second, &second_node);

    return mtl_link(txn, first, *first_node, second, *second_node, ranked);
}

int mtl_destroy_node(MDB_txn *txn, mtl_heap_node_id node_id) {

    if (node_id == INVALID_NODE)
        return MTL_ERROR_INVALID_ARGUMENT;

    MDB_val key = { .mv_size = sizeof(node_id), .mv_data = &node_id };
    if (mdb_del(txn, heap_db, &key, NULL) == MDB_NOTFOUND)
        return MTL_ERROR_INVALID_ARGUMENT;

    return MTL_SUCCESS;
}

int mtl_node_set_second_parent(MDB_txn *txn, mtl_heap_node_id node_id, const mtl_heap_node *node, mtl_heap_node_id second_parent) {
    mtl_heap_node updated_node = *node;
    updated_node.second_parent = second_parent;
    return mtl_put_node(txn, node_id, &updated_node);
}

int mtl_node_set_second_parent_and_next(MDB_txn *txn, mtl_heap_node_id node_id, const mtl_heap_node *node, mtl_heap_node_id second_parent, mtl_heap_node_id next) {
    mtl_heap_node updated_node = *node;
    updated_node.second_parent = second_parent;
    updated_node.next = next;
    return mtl_put_node(txn, node_id, &updated_node);
}

// void mtl_node_set_child(const mtl_heap_node *node, uint64_t child) {
//     return;
// }

// void mtl_node_set_rank(const mtl_heap_node *node, uint64_t rank) {
//     return;
// }

int mtl_node_set_hollow(MDB_txn *txn, mtl_heap_node_id node_id, const mtl_heap_node *node) {
    mtl_heap_node updated_node = *node;
    updated_node.hollow = true;
    return mtl_put_node(txn, node_id, &updated_node);
}

void mtl_heap_node_id_list_insert(mtl_heap_node_id_list** list, mtl_heap_node_id id) {
    mtl_heap_node_id_list *result = calloc(1, sizeof(mtl_heap_node_id_list));
    result->id = id;
    result->next = *list;
    *list = result;
}

mtl_heap_node_id mtl_heap_node_id_list_pop(mtl_heap_node_id_list** list) {
    mtl_heap_node_id id = INVALID_NODE;
    if (*list != NULL) {
        id = (*list)->id;
        mtl_heap_node_id_list* front = *list;
        *list = (*list)->next;
        free(front);
    }

    return id;
}

int mtl_delete_node(MDB_txn *txn, const mtl_hollow_heap *heap, mtl_heap_node_id node_id, const mtl_heap_node *node) {

    // Set node to be hollow
    mtl_node_set_hollow(txn, node_id, node);

    // If the node wasn't the heap's root, there's nothing more to do
    if (heap->root != node_id)
        return MTL_SUCCESS;

    // Make writable copy of the heap
    mtl_hollow_heap updated_heap = *heap;

    // Remove hollow roots, starting with 'node'

    mtl_heap_node_id_list *hollow_roots_list_head;
    mtl_heap_node_id_list_insert(&hollow_roots_list_head, node_id);

    uint64_t upper_bound_of_max_rank = ceil(log2(heap->size)) + 1;
    uint64_t max_rank = 0;
    mtl_heap_node_id filled_roots_by_rank_list_heads[upper_bound_of_max_rank]; // Hopefully fits onto the stack
    for (uint64_t i = 0; i < upper_bound_of_max_rank; ++i) filled_roots_by_rank_list_heads[i] = INVALID_NODE;

    // Iterate over the graph, finding both hollow roots (to be deleted) or
    // filled roots (to be (rank) link'ed later on)

    mtl_heap_node_id next_deleted_root_node_id;
    while ((next_deleted_root_node_id = mtl_heap_node_id_list_pop(&hollow_roots_list_head)) != INVALID_NODE) {

        // Load it a last time
        mtl_heap_node *next_deleted_root_node;
        mtl_load_node(txn, next_deleted_root_node_id, &next_deleted_root_node);

        // Look at all children because they will become roots after deleting 'next_deleted_root_node'
        mtl_heap_node *current_child_node;
        mtl_heap_node_id current_child_node_id;
        mtl_heap_node *next_child_node;
        mtl_heap_node_id next_child_node_id = next_deleted_root_node->child;
        mtl_load_node(txn, next_child_node_id, &next_child_node);

        while (next_child_node) {
            current_child_node_id = next_child_node_id;
            current_child_node = next_child_node;

            next_child_node_id = current_child_node->next;
            mtl_load_node(txn, next_child_node_id, &next_child_node);

            if (current_child_node->hollow) {
                // If it doesn't have a second parent either, append it to the list of hollow roots
                if (current_child_node->second_parent == INVALID_NODE) {
                    mtl_heap_node_id_list_insert(&hollow_roots_list_head, current_child_node_id);
                } else {
                    if (current_child_node->second_parent == next_deleted_root_node_id) {
                        // We've reached the (only) 'secondary child' of the 'next_deleted_root_node'.
                        // This is the end of the child list, so stop iterating here.

                        mtl_node_set_second_parent(txn, current_child_node_id, current_child_node, INVALID_NODE);
                        break;
                    } else {
                        mtl_node_set_second_parent_and_next(txn, current_child_node_id, current_child_node, INVALID_NODE, INVALID_NODE);
                    }
                }
            } else {
                // If filled_roots_by_rank_list_heads is empty at rank: Insert node id into filled_roots_by_rank_list_heads
                // Else perform ranked_link, repeat with winner

                mtl_heap_node_id winner = current_child_node_id;
                uint64_t rank = current_child_node->rank;
                for (;;) {
                    if (filled_roots_by_rank_list_heads[rank] == INVALID_NODE) {
                        filled_roots_by_rank_list_heads[rank] = winner;
                        if (rank > max_rank)
                            max_rank = rank;
                        break;
                    } else {
                        winner = mtl_link_ids(txn, current_child_node_id, filled_roots_by_rank_list_heads[rank], true);
                        filled_roots_by_rank_list_heads[rank] = INVALID_NODE;
                        ++rank;
                    }
                }
            }
        }

        // Delete the hollow root from the db (invalidates previous pointers)
        mtl_destroy_node(txn, next_deleted_root_node_id);
        --updated_heap.size;
    }

    // Perform link with remaining items in A
    mtl_heap_node_id filled_root = filled_roots_by_rank_list_heads[max_rank];
    for (uint64_t i = 0; i < max_rank; ++i) {
        if (filled_roots_by_rank_list_heads[max_rank - i] != INVALID_NODE)
            filled_root = mtl_link_ids(txn, filled_root, filled_roots_by_rank_list_heads[max_rank - i], false);
    }

    // Update hollow heap
    updated_heap.root = filled_root;
    mtl_put_heap(txn, &updated_heap);

    return 0;
}

int mtl_load_or_create_heap(MDB_txn *txn, const mtl_hollow_heap **heap) {

    MDB_val heap_key = { .mv_size = sizeof(hollow_heap_key), .mv_data = &hollow_heap_key };
    MDB_val heap_value;

    int res = mdb_get(txn, heap_db, &heap_key, &heap_value);
    if (res == MDB_NOTFOUND) {

        mtl_hollow_heap new_heap = { .size = 0, .root = INVALID_NODE };
        mtl_put_heap(txn, &new_heap);

        return mtl_load_or_create_heap(txn, heap);
    } else if (res == MDB_SUCCESS) {
        *heap = (mtl_hollow_heap*) heap_value.mv_data;
        return MTL_SUCCESS;
    }

    return MTL_ERROR_INVALID_ARGUMENT;
}

int mtl_heap_insert(MDB_txn *txn, uint64_t key, uint64_t value) {

    mtl_ensure_heap_db_open(txn);

    const mtl_hollow_heap *heap;
    mtl_load_or_create_heap(txn, &heap);

    mtl_heap_node_id new_root_id;
    if (heap->root == INVALID_NODE) {
        mtl_heap_node_id new_node_id = mtl_next_heap_node_id(txn);
        mtl_heap_node new_node = { .key = key, .value = value };

        MDB_val new_node_key = { .mv_size = sizeof(new_node_id), .mv_data = &new_node_id };
        MDB_val new_node_data = { .mv_size = sizeof(new_node), .mv_data = &new_node };
        mdb_put(txn, heap_db, &new_node_key, &new_node_data, 0);

        new_root_id = new_node_id;
    } else {
        mtl_heap_node *root;
        mtl_load_node(txn, heap->root, &root);

        mtl_heap_node_id new_node_id = mtl_next_heap_node_id(txn);
        mtl_heap_node new_node = { .key = key, .value = value };
        new_root_id = mtl_link(txn, heap->root, *root, new_node_id, new_node, false);
    }

    mtl_hollow_heap updated_heap = *heap;
    updated_heap.root = new_root_id;
    ++updated_heap.size;
    mtl_put_heap(txn, &updated_heap);

    return MTL_SUCCESS;
}

int mtl_heap_extract_max(MDB_txn *txn, uint64_t *max_value) {

    mtl_ensure_heap_db_open(txn);

    const mtl_hollow_heap *heap;
    mtl_load_or_create_heap(txn, &heap);

    if (heap->root == INVALID_NODE)
        return MTL_ERROR_NOENTRY;

    mtl_heap_node *node;
    mtl_load_node(txn, heap->root, &node);

    *max_value = node->value;

    return mtl_delete_node(txn, heap, heap->root, node);
}

int mtl_reset_heap_db() {
    heap_db = 0;
    return MTL_SUCCESS;
}
