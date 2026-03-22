#include "storage/pager/pager.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

// Constructor
Pager::Pager(const char *filename)
{
    file = fopen(filename, "r + b");

    if (file == nullptr)
    {
        file = fopen(filename, "w + b");
    }

    if (file == nullptr)
    {
        std::cerr << "Unable to open file" << std::endl;
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    file_length = ftell(file);

    num_pages = file_length / PAGE_SIZE;

    if (file_length % PAGE_SIZE != 0)
    {
        std::cerr << "DB file is not a whole number of pages" << std::endl;
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        pages[i] = nullptr;
    }
}

// Destructor
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

// Get page (lazy load)
void *Pager::get_page(uint32_t page_num)
{
    if (page_num >= TABLE_MAX_PAGES)
    {
        std::cerr << "Page number out of bounds" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pages[page_num] == nullptr)
    {
        // Allocate memory
        void *page = malloc(PAGE_SIZE);

        uint32_t num_pages_in_file = file_length / PAGE_SIZE;

        if (file_length % PAGE_SIZE)
        {
            num_pages_in_file += 1;
        }

        // Load from disk if exists
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

// Write page to disk
void Pager::flush(uint32_t page_num)
{
    if (pages[page_num] == nullptr)
    {
        std::cerr << "Tried to flush null page" << std::endl;
        exit(EXIT_FAILURE);
    }

    fseek(file, page_num * PAGE_SIZE, SEEK_SET);
    fwrite(pages[page_num], PAGE_SIZE, 1, file);
}