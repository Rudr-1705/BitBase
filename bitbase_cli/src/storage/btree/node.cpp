#include "storage/btree/node.h"
#include <cstring>
#include <vector>
#include <iostream>

// ===================== COMMON =====================

uint8_t *node_type(void *node)
{
    return (uint8_t *)((char *)node + NODE_TYPE_OFFSET);
}

uint8_t *node_is_root(void *node)
{
    return (uint8_t *)((char *)node + IS_ROOT_OFFSET);
}

// ===================== LEAF =====================

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
    return (uint32_t *)((char *)node + LEAF_NODE_HEADER_SIZE +
                        i * LEAF_NODE_CELL_SIZE);
}

RowPointer *leaf_node_value(void *node, uint32_t i)
{
    return (RowPointer *)((char *)node + LEAF_NODE_HEADER_SIZE +
                          i * LEAF_NODE_CELL_SIZE + sizeof(uint32_t));
}

// ===================== INTERNAL =====================

uint32_t *internal_node_num_keys(void *node)
{
    return (uint32_t *)((char *)node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t *internal_node_right_child(void *node)
{
    return (uint32_t *)((char *)node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t *internal_node_child(void *node, uint32_t i)
{
    return (uint32_t *)((char *)node + INTERNAL_NODE_HEADER_SIZE +
                        i * INTERNAL_NODE_CELL_SIZE);
}

uint32_t *internal_node_key(void *node, uint32_t i)
{
    return (uint32_t *)((char *)node + INTERNAL_NODE_HEADER_SIZE +
                        i * INTERNAL_NODE_CELL_SIZE + sizeof(uint32_t));
}

// ===================== INIT =====================

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
    *internal_node_num_keys(node) = 0;
}

// ===================== FIND =====================

uint32_t leaf_find(void *node, uint32_t key)
{
    uint32_t num_cells = *leaf_node_num_cells(node);

    uint32_t left = 0, right = num_cells;
    while (left < right)
    {
        uint32_t mid = (left + right) / 2;
        if (*leaf_node_key(node, mid) >= key)
            right = mid;
        else
            left = mid + 1;
    }
    return left;
}

uint32_t internal_find_child(void *node, uint32_t key)
{
    uint32_t n = *internal_node_num_keys(node);

    uint32_t i = 0;
    while (i < n && key >= *internal_node_key(node, i))
    {
        i++;
    }

    if (i == n)
    {
        return *internal_node_right_child(node);
    }

    return *internal_node_child(node, i);
}

// ===================== LEAF INSERT =====================

void leaf_insert(void *node, uint32_t key, RowPointer value)
{
    uint32_t num = *leaf_node_num_cells(node);
    uint32_t idx = leaf_find(node, key);

    for (uint32_t i = num; i > idx; i--)
    {
        std::memcpy(
            (char *)node + LEAF_NODE_HEADER_SIZE + i * LEAF_NODE_CELL_SIZE,
            (char *)node + LEAF_NODE_HEADER_SIZE + (i - 1) * LEAF_NODE_CELL_SIZE,
            LEAF_NODE_CELL_SIZE);
    }

    *leaf_node_key(node, idx) = key;
    *leaf_node_value(node, idx) = value;
    (*leaf_node_num_cells(node))++;
}

// ===================== LEAF SPLIT =====================

SplitResult leaf_split_and_insert(uint32_t page_num,
                                  uint32_t key,
                                  RowPointer value,
                                  Pager *pager)
{
    void *old = pager->get_page(page_num);

    uint32_t new_page = pager->num_pages++;
    void *new_node = pager->get_page(new_page);
    initialize_leaf_node(new_node);

    std::vector<uint32_t> keys;
    std::vector<RowPointer> vals;

    uint32_t n = *leaf_node_num_cells(old);
    for (uint32_t i = 0; i < n; i++)
    {
        keys.push_back(*leaf_node_key(old, i));
        vals.push_back(*leaf_node_value(old, i));
    }

    uint32_t pos = leaf_find(old, key);
    keys.insert(keys.begin() + pos, key);
    vals.insert(vals.begin() + pos, value);

    uint32_t total = keys.size();
    uint32_t split = (total + 1) / 2;

    *leaf_node_num_cells(old) = 0;

    for (uint32_t i = 0; i < split; i++)
    {
        *leaf_node_key(old, i) = keys[i];
        *leaf_node_value(old, i) = vals[i];
    }
    *leaf_node_num_cells(old) = split;

    for (uint32_t i = split; i < total; i++)
    {
        *leaf_node_key(new_node, i - split) = keys[i];
        *leaf_node_value(new_node, i - split) = vals[i];
    }
    *leaf_node_num_cells(new_node) = total - split;

    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old);
    *leaf_node_next_leaf(old) = new_page;

    return {true, *leaf_node_key(new_node, 0), new_page};
}

// ===================== INTERNAL INSERT =====================

void internal_insert(void *node, uint32_t key, uint32_t right_child_page)
{
    uint32_t n = *internal_node_num_keys(node);

    uint32_t i = n;

    // SHIFT keys + children
    while (i > 0 && *internal_node_key(node, i - 1) > key)
    {
        *internal_node_key(node, i) = *internal_node_key(node, i - 1);

        *internal_node_child(node, i + 1) =
            *internal_node_child(node, i);

        i--;
    }

    // insert key
    *internal_node_key(node, i) = key;

    if (i == n)
    {
        // inserting at end → update right_child
        *internal_node_right_child(node) = right_child_page;
    }
    else
    {
        *internal_node_child(node, i + 1) = right_child_page;
    }

    (*internal_node_num_keys(node))++;
}

// ===================== INTERNAL SPLIT =====================

SplitResult internal_split(uint32_t page_num, Pager *pager)
{
    void *old = pager->get_page(page_num);

    uint32_t new_page = pager->num_pages++;
    void *new_node = pager->get_page(new_page);
    initialize_internal_node(new_node);

    uint32_t n = *internal_node_num_keys(old);
    uint32_t mid = n / 2;

    uint32_t sep = *internal_node_key(old, mid);

    uint32_t j = 0;
    for (uint32_t i = mid + 1; i < n; i++, j++)
    {
        *internal_node_key(new_node, j) = *internal_node_key(old, i);
        *internal_node_child(new_node, j) = *internal_node_child(old, i);
    }

    *internal_node_right_child(new_node) = *internal_node_right_child(old);
    *internal_node_num_keys(new_node) = j;

    *internal_node_num_keys(old) = mid;
    *internal_node_right_child(old) = *internal_node_child(old, mid);

    return {true, sep, new_page};
}

// ===================== BTREE INSERT =====================

SplitResult btree_insert(uint32_t page_num,
                         uint32_t key,
                         RowPointer value,
                         Pager *pager)
{
    void *node = pager->get_page(page_num);

    if (*node_type(node) == (uint8_t)NodeType::LEAF)
    {
        if (*leaf_node_num_cells(node) < LEAF_NODE_MAX_CELLS)
        {
            leaf_insert(node, key, value);
            return {false, 0, 0};
        }
        return leaf_split_and_insert(page_num, key, value, pager);
    }

    uint32_t child = internal_find_child(node, key);
    SplitResult res = btree_insert(child, key, value, pager);

    if (!res.did_split)
        return {false, 0, 0};

    internal_insert(node, res.key, res.new_page);

    if (*internal_node_num_keys(node) > INTERNAL_NODE_MAX_KEYS)
        return internal_split(page_num, pager);

    return {false, 0, 0};
}

// ===================== NEW ROOT =====================

void create_new_root(Pager *pager,
                     uint32_t old_root_page,
                     uint32_t right_page,
                     uint32_t separator_key)
{
    uint32_t new_root_page = pager->num_pages++;
    void *root = pager->get_page(new_root_page);

    initialize_internal_node(root);
    *node_is_root(root) = 1;

    *internal_node_num_keys(root) = 1;

    *internal_node_child(root, 0) = old_root_page;
    *internal_node_key(root, 0) = separator_key;
    *internal_node_right_child(root) = right_page;
}

// ===================== DEBUG PRINT =====================

void print_all_leaves(Pager *pager, uint32_t start_page)
{
    uint32_t page = start_page;

    std::cout << "[LEAF CHAIN]\n";

    while (page != 0)
    {
        void *node = pager->get_page(page);

        std::cout << "Page " << page << ": ";

        uint32_t num = *leaf_node_num_cells(node);

        for (uint32_t i = 0; i < num; i++)
        {
            std::cout << *leaf_node_key(node, i) << " ";
        }

        std::cout << "\n";

        page = *leaf_node_next_leaf(node);
    }

    std::cout << "----\n";
}

uint32_t find_leftmost_leaf(Pager *pager, uint32_t page_num)
{
    void *node = pager->get_page(page_num);

    while (*node_type(node) == (uint8_t)NodeType::INTERNAL)
    {
        page_num = *internal_node_child(node, 0);
        node = pager->get_page(page_num);
    }

    return page_num;
}

bool leaf_search(void *node, uint32_t key, RowPointer &out)
{
    uint32_t n = *leaf_node_num_cells(node);

    uint32_t left = 0, right = n;

    while (left < right)
    {
        uint32_t mid = (left + right) / 2;

        uint32_t k = *leaf_node_key(node, mid);

        if (k == key)
        {
            out = *leaf_node_value(node, mid);
            return true;
        }
        else if (k < key)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    return false;
}

RowPointer btree_find(uint32_t page_num, uint32_t key, Pager *pager, bool &found)
{
    void *node = pager->get_page(page_num);

    if (*node_type(node) == (uint8_t)NodeType::LEAF)
    {
        RowPointer rp;

        found = leaf_search(node, key, rp);
        return rp;
    }
    else
    {
        uint32_t child = internal_find_child(node, key);
        return btree_find(child, key, pager, found);
    }
}

uint32_t btree_find_leaf(uint32_t page_num, uint32_t key, Pager *pager)
{
    void *node = pager->get_page(page_num);

    if (*node_type(node) == (uint8_t)NodeType::LEAF)
    {
        return page_num;
    }

    uint32_t child = internal_find_child(node, key);
    return btree_find_leaf(child, key, pager);
}

bool leaf_delete(void *node, uint32_t key)
{
    uint32_t n = *leaf_node_num_cells(node);

    uint32_t i = 0;
    while (i < n && *leaf_node_key(node, i) != key)
        i++;

    if (i == n)
        return false;

    // shift left
    for (uint32_t j = i; j < n - 1; j++)
    {
        *leaf_node_key(node, j) = *leaf_node_key(node, j + 1);
        *leaf_node_value(node, j) = *leaf_node_value(node, j + 1);
    }

    (*leaf_node_num_cells(node))--;

    return true;
}

bool btree_delete(uint32_t page_num, uint32_t key, Pager *pager)
{
    void *node = pager->get_page(page_num);

    if (*node_type(node) == (uint8_t)NodeType::LEAF)
    {
        return leaf_delete(node, key);
    }

    uint32_t child = internal_find_child(node, key);
    return btree_delete(child, key, pager);
}