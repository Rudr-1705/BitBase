#include "storage/pager/pager.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

// Constructor
Pager::Pager(const char *filename)
{
    file = fopen(filename, "r+b");

    if (file == nullptr)
    {
        file = fopen(filename, "w+b");
    }

    if (file == nullptr)
    {
        std::cerr << "Unable to open file" << std::endl;
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    file_length = ftell(file);

    if (file_length % PAGE_SIZE != 0)
    {
        std::cerr << "DB file is not a whole number of pages" << std::endl;
        exit(EXIT_FAILURE);
    }

    num_pages = file_length / PAGE_SIZE;
    next_btree_page = 2; // default — overwritten by Table ctor when reopening

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        pages[i] = nullptr;
    }
}

// Destructor — flush every in-memory page to disk
Pager::~Pager()
{
    for (uint32_t i = 0; i < num_pages; i++)
    {
        if (pages[i] == nullptr)
            continue;

        flush(i);
        free(pages[i]);
        pages[i] = nullptr;
    }

    fclose(file);
}

// Get page — lazy load from disk if it exists, otherwise fresh zeroed page
void *Pager::get_page(uint32_t page_num)
{
    if (page_num >= TABLE_MAX_PAGES)
    {
        std::cerr << "Page number out of bounds: " << page_num << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pages[page_num] == nullptr)
    {
        void *page = malloc(PAGE_SIZE);
        memset(page, 0, PAGE_SIZE); // zero-initialise so unwritten pages are clean

        uint32_t num_pages_in_file = file_length / PAGE_SIZE;

        // Load from disk if this page already exists in the file
        if (page_num < num_pages_in_file)
        {
            fseek(file, page_num * PAGE_SIZE, SEEK_SET);
            fread(page, PAGE_SIZE, 1, file);
        }

        pages[page_num] = page;

        if (page_num >= num_pages)
        {
            num_pages = page_num + 1;
        }
    }

    return pages[page_num];
}

// Flush a single page to disk
void Pager::flush(uint32_t page_num)
{
    if (pages[page_num] == nullptr)
    {
        std::cerr << "Tried to flush null page: " << page_num << std::endl;
        exit(EXIT_FAILURE);
    }

    fseek(file, page_num * PAGE_SIZE, SEEK_SET);
    fwrite(pages[page_num], PAGE_SIZE, 1, file);
    fflush(file); // make sure OS buffer is written to disk
}

// Allocate a heap page (row storage) — grows num_pages upward from HEAP_PAGE_BASE
uint32_t Pager::allocate_page()
{
    uint32_t page_num = num_pages;
    get_page(page_num); // allocates memory + updates num_pages
    return page_num;
}

// Allocate a btree node page — grows next_btree_page upward from 2
// Completely separate from heap pages so they never collide
uint32_t Pager::allocate_btree_page()
{
    uint32_t page_num = next_btree_page++;

    get_page(page_num); // allocates memory + updates num_pages if needed

    return page_num;
}