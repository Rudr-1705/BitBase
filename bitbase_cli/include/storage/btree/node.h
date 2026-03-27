#pragma once
#include <cstdint>
#include "storage/pager/pager.h"

enum class NodeType : uint8_t
{
    INTERNAL = 0,
    LEAF = 1
};

// -------- common header: [node_type(1)][is_root(1)] --------
constexpr uint32_t NODE_TYPE_OFFSET = 0;
constexpr uint32_t IS_ROOT_OFFSET = 1;
constexpr uint32_t COMMON_NODE_HEADER_SIZE = 2;

// -------- leaf header: [num_cells(4)][next_leaf(4)] --------
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;        // 2
constexpr uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + 4; // 6
constexpr uint32_t LEAF_NODE_HEADER_SIZE = LEAF_NODE_NEXT_LEAF_OFFSET + 4;      // 10

constexpr uint32_t LEAF_NODE_CELL_SIZE = 8; // [key(4)][value(4)]
constexpr uint32_t LEAF_NODE_MAX_CELLS =
    (PAGE_SIZE - LEAF_NODE_HEADER_SIZE) / LEAF_NODE_CELL_SIZE;

// -------- internal header: [num_keys(4)][right_child(4)] --------
// cell layout: [child(4)][key(4)]  — child[i] holds keys < key[i]
// right_child holds all keys >= last key
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;              // 2
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + 4; // 6
constexpr uint32_t INTERNAL_NODE_HEADER_SIZE = INTERNAL_NODE_RIGHT_CHILD_OFFSET + 4;     // 10

constexpr uint32_t INTERNAL_NODE_CELL_SIZE = 8; // [child(4)][key(4)]
constexpr uint32_t INTERNAL_NODE_MAX_KEYS =
    (PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE) / INTERNAL_NODE_CELL_SIZE;
// right_child is stored separately, so max children = max_keys + 1
constexpr uint32_t INTERNAL_NODE_MAX_CHILDREN = INTERNAL_NODE_MAX_KEYS + 1;

// -------- accessors --------
uint8_t *node_type(void *node);
uint8_t *node_is_root(void *node);

uint32_t *leaf_node_num_cells(void *node);
uint32_t *leaf_node_next_leaf(void *node); // page num of right sibling, 0 = none
uint32_t *leaf_node_key(void *node, uint32_t i);
uint32_t *leaf_node_value(void *node, uint32_t i);

uint32_t *internal_node_num_keys(void *node);
uint32_t *internal_node_right_child(void *node);
uint32_t *internal_node_child(void *node, uint32_t i); // child before key[i]
uint32_t *internal_node_key(void *node, uint32_t i);

void initialize_leaf_node(void *node);
void initialize_internal_node(void *node);

// -------- find --------
// Returns cell index (leaf) or child page number (internal)
uint32_t leaf_find(void *node, uint32_t key);
uint32_t internal_find_child(void *node, uint32_t key); // returns PAGE number

// -------- insert --------
struct SplitResult
{
    bool did_split;
    uint32_t key;      // separator key pushed up
    uint32_t new_page; // right child after split
};

// Recursive insert. Returns SplitResult to parent.
SplitResult btree_insert(uint32_t page_num, uint32_t key, uint32_t value, Pager *pager);

// Call after btree_insert returns did_split=true on the root
void create_new_root(Pager *pager, uint32_t old_root_page,
                     uint32_t right_page, uint32_t separator_key);