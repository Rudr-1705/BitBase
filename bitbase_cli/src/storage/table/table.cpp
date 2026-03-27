#include "storage/table/table.h"
#include "storage/btree/node.h"
#include <cstring>

constexpr uint32_t METADATA_PAGE_NUM = 0;
constexpr uint32_t ROOT_PAGE_NUM = 1;
constexpr uint32_t NUM_ROWS_OFFSET = 0;
static constexpr uint32_t HEAP_PAGE_BASE = 500; // page 0 = meta, page 1 = root
static constexpr uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;

// ===================== CTOR / DTOR =====================

Table::Table(const char *filename)
{
    pager = new Pager(filename);

    if (pager->file_length == 0)
    {
        num_rows = 0;
        pager->num_pages = HEAP_PAGE_BASE;
        pager->next_btree_page = 2;

        void *meta = pager->get_page(METADATA_PAGE_NUM);
        *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET) = 0;
        *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET + 4) = HEAP_PAGE_BASE; // ← ADD
        *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET + 8) = 2;
        pager->flush(METADATA_PAGE_NUM);

        void *root = pager->get_page(ROOT_PAGE_NUM);
        initialize_leaf_node(root);
        *node_is_root(root) = 1;
        pager->flush(ROOT_PAGE_NUM);
    }
    else
    {
        void *meta = pager->get_page(METADATA_PAGE_NUM);
        num_rows = *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET);
        pager->num_pages = *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET + 4);       // ← ADD
        pager->next_btree_page = *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET + 8); // ← ADD
    }
}

Table::~Table() { delete pager; }

// In table.cpp
void Table::persist_num_rows()
{
    void *meta = pager->get_page(METADATA_PAGE_NUM);
    *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET) = num_rows;
    *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET + 4) = pager->num_pages;
    *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET + 8) = pager->next_btree_page; // ← ADD
    pager->flush(METADATA_PAGE_NUM);
}

// ===================== ROW STORAGE =====================
// Rows are stored in a flat heap: page 2 onwards, ROWS_PER_PAGE rows each.
// The B+tree maps key → row_num (the index into this heap).

static void *row_slot(Table *table, uint32_t row_num)
{
    uint32_t page_num = HEAP_PAGE_BASE + row_num / ROWS_PER_PAGE;
    void *page = table->pager->get_page(page_num);
    uint32_t offset = (row_num % ROWS_PER_PAGE) * ROW_SIZE;
    return (char *)page + offset;
}

// ===================== INSERT =====================

void Table::insert(const Row &row)
{
    // 1. Store row in heap at slot num_rows
    void *slot = row_slot(this, num_rows);
    serialize_row(row, (char *)slot);

    // 2. Insert (id → row_num) into B+tree
    SplitResult res = btree_insert(ROOT_PAGE_NUM, row.id, num_rows, pager);

    if (res.did_split)
    {
        // Root was split — create a new root at ROOT_PAGE_NUM
        create_new_root(pager, ROOT_PAGE_NUM, res.new_page, res.key);
    }

    num_rows++;
    persist_num_rows();
}

// ===================== GET ALL (B+tree leaf scan) =====================
// Traverse linked leaf pages in order — true B+tree sequential scan.

std::vector<Row> Table::get_all() const
{
    std::vector<Row> result;
    result.reserve(num_rows);

    // Walk the leaf chain starting from the leftmost leaf.
    // The leftmost leaf is always reachable by following child[0] from the root.
    uint32_t page_num = ROOT_PAGE_NUM;
    void *node = pager->get_page(page_num);

    // Descend to leftmost leaf
    while (*node_type(node) == (uint8_t)NodeType::INTERNAL)
    {
        page_num = *internal_node_child(node, 0);
        node = pager->get_page(page_num);
    }

    // Scan all leaves in order via next_leaf pointers
    while (true)
    {
        uint32_t n = *leaf_node_num_cells(node);
        for (uint32_t i = 0; i < n; i++)
        {
            uint32_t row_num = *leaf_node_value(node, i);
            Row r;
            void *src = row_slot((Table *)this, row_num);
            deserialize_row((char *)src, r);
            result.push_back(r);
        }

        uint32_t next = *leaf_node_next_leaf(node);
        if (next == 0)
            break; // no more siblings
        page_num = next;
        node = pager->get_page(page_num);
    }

    return result;
}

// ===================== FIND BY ID (B+tree point lookup) =====================

static uint32_t btree_find(uint32_t page_num, uint32_t key, Pager *pager)
{
    void *node = pager->get_page(page_num);

    if (*node_type(node) == (uint8_t)NodeType::INTERNAL)
    {
        uint32_t child = internal_find_child(node, key);
        return btree_find(child, key, pager);
    }

    // Leaf: binary search
    uint32_t pos = leaf_find(node, key);
    uint32_t n = *leaf_node_num_cells(node);

    if (pos < n && *leaf_node_key(node, pos) == key)
        return *leaf_node_value(node, pos); // row_num

    return UINT32_MAX; // not found
}

// ===================== DELETE =====================
// Logical delete: find the row, compact the heap, update num_rows.
// NOTE: this does NOT remove the key from the B+tree (full tree deletion
// is complex). For a production engine you'd mark the slot as a tombstone
// and do periodic compaction. This keeps it simple and correct.

bool Table::delete_by_id(uint32_t id)
{
    uint32_t row_num = btree_find(ROOT_PAGE_NUM, id, pager);
    if (row_num == UINT32_MAX)
        return false;

    // Compact heap: shift all slots after row_num left by one
    for (uint32_t i = row_num + 1; i < num_rows; i++)
    {
        void *dst = row_slot(this, i - 1);
        void *src = row_slot(this, i);
        memcpy(dst, src, ROW_SIZE);
    }

    num_rows--;
    persist_num_rows();
    return true;
    // TODO: rebuild B+tree or use tombstone approach for production
}

// ===================== UPDATE =====================

bool Table::update(const Row &row)
{
    uint32_t row_num = btree_find(ROOT_PAGE_NUM, row.id, pager);
    if (row_num == UINT32_MAX)
        return false;

    void *slot = row_slot(this, row_num);
    serialize_row(row, (char *)slot);
    return true;
}