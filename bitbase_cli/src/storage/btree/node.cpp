#include "storage/btree/node.h"
#include <cstring>

// ---------------- COMMON ----------------

uint8_t *node_type(void *node)
{
    return (uint8_t *)((char *)node + NODE_TYPE_OFFSET);
}

uint8_t *node_is_root(void *node)
{
    return (uint8_t *)((char *)node + IS_ROOT_OFFSET);
}

// ---------------- LEAF ----------------

uint32_t *leaf_node_num_cells(void *node)
{
    return (uint32_t *)((char *)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

uint32_t *leaf_node_key(void *node, uint32_t cell_num)
{
    return (uint32_t *)((char *)node + LEAF_NODE_HEADER_SIZE +
                        cell_num * LEAF_NODE_CELL_SIZE);
}

uint32_t *leaf_node_value(void *node, uint32_t cell_num)
{
    return (uint32_t *)((char *)node + LEAF_NODE_HEADER_SIZE +
                        cell_num * LEAF_NODE_CELL_SIZE +
                        LEAF_NODE_KEY_SIZE);
}

// ---------------- INIT ----------------

void initialize_leaf_node(void *node)
{
    *node_type(node) = (uint8_t)NodeType::LEAF;
    *node_is_root(node) = 0;

    *leaf_node_num_cells(node) = 0;
}