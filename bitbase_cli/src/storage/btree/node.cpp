#include "storage/btree/node.h"
#include <cstring>
#include <iostream>

// ---------- ACCESS ----------
uint8_t *node_type(void *node) { return (uint8_t *)node; }
uint8_t *node_is_root(void *node) { return (uint8_t *)node + 1; }

uint32_t *leaf_node_num_cells(void *node)
{
    return (uint32_t *)((char *)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

uint32_t *leaf_node_next_leaf(void *node)
{
    return (uint32_t *)((char *)node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

uint32_t *leaf_node_key(void *node, uint32_t i)
{
    return (uint32_t *)((char *)node + LEAF_NODE_HEADER_SIZE + i * 8);
}

uint32_t *leaf_node_value(void *node, uint32_t i)
{
    return (uint32_t *)((char *)node + LEAF_NODE_HEADER_SIZE + i * 8 + 4);
}

// ---------- INIT ----------
void initialize_leaf_node(void *node)
{
    *node_type(node) = (uint8_t)NodeType::LEAF;
    *node_is_root(node) = 0;
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0;
}

void initialize_internal_node(void *node)
{
    *node_type(node) = (uint8_t)NodeType::INTERNAL;
    *node_is_root(node) = 0;
    *(uint32_t *)((char *)node + INTERNAL_NODE_NUM_KEYS_OFFSET) = 0;
}

// ---------- SEARCH ----------
uint32_t leaf_node_find(void *node, uint32_t key)
{
    uint32_t l = 0, r = *leaf_node_num_cells(node);

    while (l < r)
    {
        uint32_t m = (l + r) / 2;
        if (*leaf_node_key(node, m) >= key)
            r = m;
        else
            l = m + 1;
    }
    return l;
}

// ---------- INSERT ----------
void leaf_node_insert(uint32_t page_num, void *node,
                      uint32_t key, uint32_t value, Pager *pager)
{
    uint32_t n = *leaf_node_num_cells(node);

    if (n >= LEAF_NODE_MAX_CELLS)
    {
        leaf_node_split_and_insert(page_num, node, key, value, pager);
        return;
    }

    uint32_t i = leaf_node_find(node, key);

    if (i < n && *leaf_node_key(node, i) == key)
    {
        std::cerr << "Duplicate key\n";
        exit(1);
    }

    memmove(leaf_node_key(node, i + 1),
            leaf_node_key(node, i),
            (n - i) * 8);

    *leaf_node_key(node, i) = key;
    *leaf_node_value(node, i) = value;

    (*leaf_node_num_cells(node))++;
}

// ---------- SPLIT ----------
void leaf_node_split_and_insert(uint32_t old_page_num, void *old_node,
                                uint32_t key, uint32_t value, Pager *pager)
{
    std::cout << ">>> SPLIT HAPPENED on page " << old_page_num << "\n";
    uint32_t new_page = pager->num_pages;
    void *new_node = pager->get_page(new_page);
    initialize_leaf_node(new_node);

    uint32_t temp_keys[LEAF_NODE_MAX_CELLS + 1];
    uint32_t temp_vals[LEAF_NODE_MAX_CELLS + 1];

    uint32_t n = *leaf_node_num_cells(old_node);

    for (uint32_t i = 0; i < n; i++)
    {
        temp_keys[i] = *leaf_node_key(old_node, i);
        temp_vals[i] = *leaf_node_value(old_node, i);
    }

    uint32_t pos = leaf_node_find(old_node, key);

    for (uint32_t i = n; i > pos; i--)
    {
        temp_keys[i] = temp_keys[i - 1];
        temp_vals[i] = temp_vals[i - 1];
    }

    temp_keys[pos] = key;
    temp_vals[pos] = value;

    uint32_t split = (n + 1) / 2;

    *leaf_node_num_cells(old_node) = 0;

    for (uint32_t i = 0; i < split; i++)
    {
        *leaf_node_key(old_node, i) = temp_keys[i];
        *leaf_node_value(old_node, i) = temp_vals[i];
    }
    *leaf_node_num_cells(old_node) = split;

    for (uint32_t i = split; i < n + 1; i++)
    {
        uint32_t j = i - split;
        *leaf_node_key(new_node, j) = temp_keys[i];
        *leaf_node_value(new_node, j) = temp_vals[i];
    }
    *leaf_node_num_cells(new_node) = n + 1 - split;

    if (*node_is_root(old_node))
    {
        create_new_root(pager, old_page_num, new_page);
    }
}

// ---------- ROOT ----------
void create_new_root(Pager *pager, uint32_t left_page, uint32_t right_page)
{
    std::cout << ">>> ROOT CREATED | left: " << left_page
              << " right: " << right_page << "\n";
    void *root = pager->get_page(1);

    uint32_t new_left_page = pager->num_pages;
    void *left = pager->get_page(new_left_page);

    memcpy(left, pager->get_page(left_page), PAGE_SIZE);
    *node_is_root(left) = 0;

    initialize_internal_node(root);
    *node_is_root(root) = 1;

    *(uint32_t *)((char *)root + INTERNAL_NODE_NUM_KEYS_OFFSET) = 1;

    *(uint32_t *)((char *)root + INTERNAL_NODE_HEADER_SIZE) = new_left_page;
    *(uint32_t *)((char *)root + INTERNAL_NODE_HEADER_SIZE + 4) =
        *leaf_node_key(left, *leaf_node_num_cells(left) - 1);

    *(uint32_t *)((char *)root + INTERNAL_NODE_RIGHT_CHILD_OFFSET) = right_page;
}