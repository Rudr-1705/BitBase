#pragma once

#include <cstdint>
#include <cstdio>

constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t TABLE_MAX_PAGES = 10000;

class Pager
{
public:
    FILE *file;
    uint32_t file_length;
    uint32_t num_pages;
    uint32_t next_btree_page; // btree allocates from here upward (starts at 2)

    void *pages[TABLE_MAX_PAGES];

    Pager(const char *filename);
    ~Pager();

    void *get_page(uint32_t page_num);
    void flush(uint32_t page_num);

    uint32_t allocate_page();       // used by heap (row storage) only
    uint32_t allocate_btree_page(); // used by btree nodes only
};