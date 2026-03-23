#pragma once

#include <cstdint>
#include "storage/pager/pager.h"

// ---------------- NODE TYPES ----------------

enum class NodeType : uint8_t
{
    INTERNAL = 0,
    LEAF = 1
};

// ---------------- COMMON HEADER ----------------

constexpr uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
constexpr uint32_t NODE_TYPE_OFFSET = 0;

constexpr uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
constexpr uint32_t IS_ROOT_OFFSET = NODE_TYPE_OFFSET + NODE_TYPE_SIZE;

constexpr uint32_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE;

// ---------------- LEAF NODE HEADER ----------------

constexpr uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET =
    COMMON_NODE_HEADER_SIZE;

constexpr uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

// ---------------- LEAF NODE BODY ----------------

constexpr uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_VALUE_SIZE = sizeof(uint32_t);

constexpr uint32_t LEAF_NODE_CELL_SIZE =
    LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;

constexpr uint32_t LEAF_NODE_SPACE_FOR_CELLS =
    PAGE_SIZE - LEAF_NODE_HEADER_SIZE;

constexpr uint32_t LEAF_NODE_MAX_CELLS =
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

// ---------------- ACCESSORS ----------------

// Header
uint8_t *node_type(void *node);
uint8_t *node_is_root(void *node);

// Leaf
uint32_t *leaf_node_num_cells(void *node);
uint32_t *leaf_node_key(void *node, uint32_t cell_num);
uint32_t *leaf_node_value(void *node, uint32_t cell_num);

// ---------------- OPERATIONS ----------------

// Initialize a leaf node
void initialize_leaf_node(void *node);