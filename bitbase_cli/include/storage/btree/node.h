#pragma once
#include <cstdint>
#include "storage/pager/pager.h"

// ===================== ROW POINTER =====================

struct RowPointer
{
    uint32_t page_id;
    uint32_t offset;
};

// ===================== NODE TYPES =====================

enum class NodeType : uint8_t
{
    INTERNAL = 0,
    LEAF = 1
};

// ===================== COMMON HEADER =====================
// [node_type(1)][is_root(1)]

constexpr uint32_t NODE_TYPE_OFFSET = 0;
constexpr uint32_t IS_ROOT_OFFSET = 1;
constexpr uint32_t COMMON_NODE_HEADER_SIZE = 2;

// ===================== LEAF NODE =====================
// header: [num_cells(4)][next_leaf(4)]

constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;        // 2
constexpr uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + 4; // 6
constexpr uint32_t LEAF_NODE_HEADER_SIZE = LEAF_NODE_NEXT_LEAF_OFFSET + 4;      // 10

// cell: [key(4)][page_id(4)][offset(4)]
constexpr uint32_t LEAF_NODE_CELL_SIZE = 12;

constexpr uint32_t LEAF_NODE_MAX_CELLS =
    (PAGE_SIZE - LEAF_NODE_HEADER_SIZE) / LEAF_NODE_CELL_SIZE;

// ===================== INTERNAL NODE =====================
// header: [num_keys(4)][right_child(4)]
// cell: [child(4)][key(4)]

constexpr uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;              // 2
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + 4; // 6
constexpr uint32_t INTERNAL_NODE_HEADER_SIZE = INTERNAL_NODE_RIGHT_CHILD_OFFSET + 4;     // 10

constexpr uint32_t INTERNAL_NODE_CELL_SIZE = 8;

constexpr uint32_t INTERNAL_NODE_MAX_KEYS =
    (PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE) / INTERNAL_NODE_CELL_SIZE;

constexpr uint32_t INTERNAL_NODE_MAX_CHILDREN =
    INTERNAL_NODE_MAX_KEYS + 1;

// ===================== ACCESSORS =====================

uint8_t *node_type(void *node);
uint8_t *node_is_root(void *node);

// leaf
uint32_t *leaf_node_num_cells(void *node);
uint32_t *leaf_node_next_leaf(void *node);
uint32_t *leaf_node_key(void *node, uint32_t i);
RowPointer *leaf_node_value(void *node, uint32_t i);

// internal
uint32_t *internal_node_num_keys(void *node);
uint32_t *internal_node_right_child(void *node);
uint32_t *internal_node_child(void *node, uint32_t i);
uint32_t *internal_node_key(void *node, uint32_t i);

// ===================== INIT =====================

void initialize_leaf_node(void *node);
void initialize_internal_node(void *node);

// ===================== FIND =====================

uint32_t leaf_find(void *node, uint32_t key);
uint32_t internal_find_child(void *node, uint32_t key);

// ===================== INSERT =====================

struct SplitResult
{
    bool did_split;
    uint32_t key;      // separator
    uint32_t new_page; // right child
};

SplitResult btree_insert(uint32_t page_num,
                         uint32_t key,
                         RowPointer value,
                         Pager *pager);

// ===================== ROOT =====================

void create_new_root(Pager *pager,
                     uint32_t old_root_page,
                     uint32_t right_page,
                     uint32_t separator_key);

void print_leaf(void *node);

void print_all_leaves(Pager *pager, uint32_t start_page);

uint32_t find_leftmost_leaf(Pager *pager, uint32_t page_num);

bool leaf_search(void *node, uint32_t key, RowPointer &out);
RowPointer btree_find(uint32_t root_page, uint32_t key, Pager *pager, bool &found);