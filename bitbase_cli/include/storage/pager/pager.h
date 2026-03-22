#pragma once

#include <cstdint>
#include <cstdio>

constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t TABLE_MAX_PAGES = 100;

class Pager
{
public:
    FILE *file;
    uint32_t file_length;
    uint32_t num_pages;

    void *pages[TABLE_MAX_PAGES];

    Pager(const char *filename);
    ~Pager();

    void *get_page(uint32_t page_num);
    void flush(uint32_t page_num);
};