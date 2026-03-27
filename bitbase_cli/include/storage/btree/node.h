#pragma once

#include <cstdint>
#include "storage/pager/pager.h"

enum class NodeType : uint8_t
{
    INTERNAL = 0,
    LEAF = 1
};

// ---------- COMMON ----------
constexpr uint32_t NODE_TYPE_OFFSET = 0;
constexpr uint32_t IS_ROOT_OFFSET = 1;
constexpr uint32_t COMMON_NODE_HEADER_SIZE = 2;

// ---------- LEAF ----------
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + 4;

constexpr uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + 4 + 4;

constexpr uint32_t LEAF_NODE_CELL_SIZE = 8; // key + value

constexpr uint32_t LEAF_NODE_MAX_CELLS =
    (PAGE_SIZE - LEAF_NODE_HEADER_SIZE) / LEAF_NODE_CELL_SIZE;

// ---------- INTERNAL ----------
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + 4;
constexpr uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + 8;
constexpr uint32_t INTERNAL_NODE_CELL_SIZE = 8;

// ---------- ACCESS ----------
uint8_t *node_type(void *node);
uint8_t *node_is_root(void *node);

uint32_t *leaf_node_num_cells(void *node);
uint32_t *leaf_node_next_leaf(void *node);
uint32_t *leaf_node_key(void *node, uint32_t i);
uint32_t *leaf_node_value(void *node, uint32_t i);

uint32_t *internal_node_num_keys(void *node);
uint32_t *internal_node_child(void *node, uint32_t i);
uint32_t *internal_node_key(void *node, uint32_t i);
uint32_t *internal_node_right_child(void *node);

// ---------- OPS ----------
void initialize_leaf_node(void *node);
void initialize_internal_node(void *node);

uint32_t leaf_node_find(void *node, uint32_t key);

void leaf_node_insert(uint32_t page_num, void *node,
                      uint32_t key, uint32_t value, Pager *pager);

void leaf_node_split_and_insert(uint32_t old_page_num, void *old_node,
                                uint32_t key, uint32_t value, Pager *pager);

void create_new_root(Pager *pager, uint32_t left_page, uint32_t right_page);