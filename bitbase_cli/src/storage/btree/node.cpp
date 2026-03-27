#include "storage/btree/node.h"
#include <cstring>
#include <cassert>
#include <iostream>

// ===================== ACCESSORS =====================

uint8_t *node_type(void *node)
{
    return (uint8_t *)node + NODE_TYPE_OFFSET;
}
uint8_t *node_is_root(void *node)
{
    return (uint8_t *)node + IS_ROOT_OFFSET;
}

// ---- leaf ----
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
    // cell i starts at HEADER + i*8, key is first 4 bytes
    return (uint32_t *)((char *)node + LEAF_NODE_HEADER_SIZE + i * LEAF_NODE_CELL_SIZE);
}
uint32_t *leaf_node_value(void *node, uint32_t i)
{
    return (uint32_t *)((char *)node + LEAF_NODE_HEADER_SIZE + i * LEAF_NODE_CELL_SIZE + 4);
}

// ---- internal ----
// Cell layout: [child(4)][key(4)]
// child[i] is the subtree with all keys < key[i]
// right_child is the subtree with keys >= key[num_keys-1]
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
    // cell i: offset = HEADER + i*8, child is first word
    return (uint32_t *)((char *)node + INTERNAL_NODE_HEADER_SIZE + i * INTERNAL_NODE_CELL_SIZE);
}
uint32_t *internal_node_key(void *node, uint32_t i)
{
    return (uint32_t *)((char *)node + INTERNAL_NODE_HEADER_SIZE + i * INTERNAL_NODE_CELL_SIZE + 4);
}

// ===================== INIT =====================

void initialize_leaf_node(void *node)
{
    *node_type(node) = (uint8_t)NodeType::LEAF;
    *node_is_root(node) = 0;
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0; // no right sibling
}

void initialize_internal_node(void *node)
{
    *node_type(node) = (uint8_t)NodeType::INTERNAL;
    *node_is_root(node) = 0;
    *internal_node_num_keys(node) = 0;
    *internal_node_right_child(node) = 0;
    // zero out all child slots defensively
    for (uint32_t i = 0; i < INTERNAL_NODE_MAX_KEYS; i++)
    {
        *internal_node_child(node, i) = 0;
        *internal_node_key(node, i) = 0;
    }
}

// ===================== FIND =====================

// Binary search: returns index of first cell whose key >= target.
// This is the insertion point AND the search position.
uint32_t leaf_find(void *node, uint32_t key)
{
    uint32_t lo = 0, hi = *leaf_node_num_cells(node);
    while (lo < hi)
    {
        uint32_t mid = (lo + hi) / 2;
        if (*leaf_node_key(node, mid) >= key)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}

// Returns the PAGE NUMBER of the child that should contain 'key'.
// Invariant: child[i] holds all keys < key[i].
// right_child holds all keys >= key[num_keys-1].
uint32_t internal_find_child(void *node, uint32_t key)
{
    uint32_t n = *internal_node_num_keys(node);
    // Binary search for the first key[i] > key
    uint32_t lo = 0, hi = n;
    while (lo < hi)
    {
        uint32_t mid = (lo + hi) / 2;
        if (*internal_node_key(node, mid) > key)
            hi = mid;
        else
            lo = mid + 1;
    }
    // lo == n means key >= all keys → go right_child
    if (lo == n)
        return *internal_node_right_child(node);
    return *internal_node_child(node, lo);
}

// ===================== INSERT =====================

// Insert separator_key + right_page into an internal node.
// Called from the recursive insert when a child splits.
// Returns true if THIS node also had to split (not implemented here —
// extend with internal node splitting if needed).
static SplitResult internal_insert_after_child_split(
    void *node, uint32_t page_num,
    uint32_t separator_key, uint32_t right_page,
    Pager *pager)
{
    uint32_t n = *internal_node_num_keys(node);

    if (n >= INTERNAL_NODE_MAX_KEYS)
    {
        // --- Internal node split (required for large trees) ---
        // Split into two internal nodes and push median up.
        uint32_t new_page_num = pager->allocate_btree_page();
        void *new_node = pager->get_page(new_page_num);
        initialize_internal_node(new_node);

        // Collect all (child, key) pairs + the new pair into a temp buffer
        uint32_t total = n + 1; // n existing keys + 1 new
        uint32_t tmp_children[INTERNAL_NODE_MAX_KEYS + 2];
        uint32_t tmp_keys[INTERNAL_NODE_MAX_KEYS + 1];

        // Copy existing pairs into tmp
        for (uint32_t i = 0; i < n; i++)
        {
            tmp_children[i] = *internal_node_child(node, i);
            tmp_keys[i] = *internal_node_key(node, i);
        }
        tmp_children[n] = *internal_node_right_child(node);

        // Find insertion position for new separator
        uint32_t pos = 0;
        while (pos < n && tmp_keys[pos] < separator_key)
            pos++;

        // Shift right to make room
        for (uint32_t i = total; i > pos; i--)
        {
            tmp_children[i + 1] = tmp_children[i];
        }
        for (uint32_t i = n; i > pos; i--)
        {
            tmp_keys[i] = tmp_keys[i - 1];
        }
        tmp_children[pos + 1] = right_page;
        tmp_keys[pos] = separator_key;
        // total children is now n + 2, total keys is n + 1

        uint32_t mid = total / 2; // index of key that gets pushed up
        uint32_t pushed_key = tmp_keys[mid];

        // Left node: keys [0..mid-1], children [0..mid]
        *internal_node_num_keys(node) = mid;
        for (uint32_t i = 0; i < mid; i++)
        {
            *internal_node_child(node, i) = tmp_children[i];
            *internal_node_key(node, i) = tmp_keys[i];
        }
        *internal_node_right_child(node) = tmp_children[mid];

        // Right node: keys [mid+1..total], children [mid+1..total]
        uint32_t right_keys = total - mid - 1;
        *internal_node_num_keys(new_node) = right_keys;
        for (uint32_t i = 0; i < right_keys; i++)
        {
            *internal_node_child(new_node, i) = tmp_children[mid + 1 + i];
            *internal_node_key(new_node, i) = tmp_keys[mid + 1 + i];
        }
        *internal_node_right_child(new_node) = tmp_children[total];

        return {true, pushed_key, new_page_num};
    }

    // --- No split needed: find position and insert ---
    // Find first key[i] >= separator_key
    uint32_t pos = 0;
    while (pos < n && *internal_node_key(node, pos) < separator_key)
        pos++;

    // Shift keys and children right from pos
    for (uint32_t i = n; i > pos; i--)
    {
        *internal_node_key(node, i) = *internal_node_key(node, i - 1);
        *internal_node_child(node, i) = *internal_node_child(node, i - 1);
    }

    // The right_child of the position we're inserting at becomes child[pos+1]
    // and the new right_page becomes the new entry at pos+1... but wait:
    // We need to be careful. After inserting key[pos] = separator_key:
    //   child[pos]   = old child at pos (already shifted to i+1 above? No — we shifted from pos)
    //
    // Actually the clean invariant is:
    //   child[pos] holds keys < key[pos]   → old child[pos] stays
    //   key[pos]   = separator_key
    //   child[pos+1] = right_page          → the new right child of the separator
    //
    // But we also must preserve right_child for keys beyond the last key.
    // When pos == n (appending at end), right_child becomes child[n] and
    // right_page becomes the new right_child.

    if (pos == n)
    {
        // Appending: old right_child becomes child[n], new right_child = right_page
        *internal_node_child(node, n) = *internal_node_right_child(node);
        *internal_node_right_child(node) = right_page;
    }
    else
    {
        // Inserting in the middle: child[pos] was already shifted to child[pos+1].
        // child[pos] keeps its old value (keys < separator_key already go there).
        // child[pos+1] = right_page.
        *internal_node_child(node, pos + 1) = right_page;
    }

    *internal_node_key(node, pos) = separator_key;
    (*internal_node_num_keys(node))++;

    return {false, 0, 0};
}

SplitResult btree_insert(uint32_t page_num, uint32_t key, uint32_t value, Pager *pager)
{
    void *node = pager->get_page(page_num);

    // ---- Leaf node ----
    if (*node_type(node) == (uint8_t)NodeType::LEAF)
    {
        uint32_t n = *leaf_node_num_cells(node);
        uint32_t pos = leaf_find(node, key);

        if (n < LEAF_NODE_MAX_CELLS)
        {
            // Shift cells right to make room at pos
            if (pos < n)
            {
                memmove(
                    leaf_node_key(node, pos + 1),
                    leaf_node_key(node, pos),
                    (n - pos) * LEAF_NODE_CELL_SIZE);
            }
            *leaf_node_key(node, pos) = key;
            *leaf_node_value(node, pos) = value;
            (*leaf_node_num_cells(node))++;
            return {false, 0, 0};
        }

        // ---- Leaf split ----
        uint32_t new_page_num = pager->allocate_btree_page();
        void *new_node = pager->get_page(new_page_num);
        initialize_leaf_node(new_node);

        // Copy all existing cells + new one into a temp buffer
        uint32_t tmp_keys[LEAF_NODE_MAX_CELLS + 1];
        uint32_t tmp_vals[LEAF_NODE_MAX_CELLS + 1];

        // Copy cells before pos
        for (uint32_t i = 0; i < pos; i++)
        {
            tmp_keys[i] = *leaf_node_key(node, i);
            tmp_vals[i] = *leaf_node_value(node, i);
        }
        // Insert new cell
        tmp_keys[pos] = key;
        tmp_vals[pos] = value;
        // Copy cells after pos
        for (uint32_t i = pos; i < n; i++)
        {
            tmp_keys[i + 1] = *leaf_node_key(node, i);
            tmp_vals[i + 1] = *leaf_node_value(node, i);
        }

        // Split: left gets ceil((n+1)/2), right gets the rest
        uint32_t left_count = (n + 1 + 1) / 2; // ceil
        uint32_t right_count = (n + 1) - left_count;

        // Rebuild left node
        *leaf_node_num_cells(node) = left_count;
        for (uint32_t i = 0; i < left_count; i++)
        {
            *leaf_node_key(node, i) = tmp_keys[i];
            *leaf_node_value(node, i) = tmp_vals[i];
        }

        // Populate right node
        *leaf_node_num_cells(new_node) = right_count;
        for (uint32_t i = 0; i < right_count; i++)
        {
            *leaf_node_key(new_node, i) = tmp_keys[left_count + i];
            *leaf_node_value(new_node, i) = tmp_vals[left_count + i];
        }

        // Wire sibling pointer (for B+ tree leaf traversal)
        *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(node);
        *leaf_node_next_leaf(node) = new_page_num;

        // Separator key = smallest key in right node (B+ tree: copy-up)
        uint32_t separator = *leaf_node_key(new_node, 0);

        return {true, separator, new_page_num};
    }

    // ---- Internal node ----
    uint32_t child_page = internal_find_child(node, key);

    if (child_page == 0 || child_page >= pager->num_pages)
    {
        std::cerr << "btree_insert: invalid child page " << child_page
                  << " from internal node " << page_num << "\n";
        exit(1);
    }

    SplitResult res = btree_insert(child_page, key, value, pager);

    if (!res.did_split)
        return {false, 0, 0};

    // Child split: insert separator into this internal node
    // Re-fetch node pointer — recursive call may have triggered pager eviction
    node = pager->get_page(page_num);
    return internal_insert_after_child_split(node, page_num, res.key, res.new_page, pager);
}

// ===================== ROOT =====================

// Called when btree_insert on the root returns did_split=true.
// The root's content has been split: the old root is now the left child,
// right_page is the new right child, separator_key divides them.
//
// Strategy: copy old root to a new page, reinitialise page 1 as internal root.
void create_new_root(Pager *pager,
                     uint32_t old_root_page,
                     uint32_t right_page,
                     uint32_t separator_key)
{
    // Allocate a page for the left child (copy of old root)
    uint32_t left_page = pager->allocate_btree_page();
    void *root = pager->get_page(old_root_page);
    void *left = pager->get_page(left_page);

    // Copy old root content to left page
    memcpy(left, root, PAGE_SIZE);
    *node_is_root(left) = 0;

    // If left is a leaf, fix its next_leaf to point to right_page
    if (*node_type(left) == (uint8_t)NodeType::LEAF)
    {
        // right_page is the right sibling; whatever left used to point to
        // is now beyond right_page — right_page already inherited it during split
        *leaf_node_next_leaf(left) = right_page;
    }

    // Reinitialise old_root_page as a new internal node
    initialize_internal_node(root);
    *node_is_root(root) = 1;

    *internal_node_num_keys(root) = 1;
    *internal_node_child(root, 0) = left_page;
    *internal_node_key(root, 0) = separator_key;
    *internal_node_right_child(root) = right_page;

    // Propagate is_root=0 to right child if needed
    void *right = pager->get_page(right_page);
    *node_is_root(right) = 0;

    // std::cout << ">>> new root at page " << old_root_page
    //           << " key=" << separator_key
    //           << " left=" << left_page
    //           << " right=" << right_page << "\n";
}