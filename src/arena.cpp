#include "include/arena.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <include/game_math.h>

void init_pool(MemoryPool* pool)
{
    pool->free_count = MEMORY_PAGE_COUNT;
    for (u32 i = 0; i < MEMORY_PAGE_COUNT; ++i) {
        pool->pages[i] = {};
        pool->free_pages[i] = i;
    }
};

void init_arena(Arena* arena, MemoryPool* pool)
{
    arena->page = -1;
    arena->first = -1;
    arena->current = 0;
    arena->pool = pool;
}

void* push_size(Arena* arena, u32 size)
{
    if (arena->first < 0 || arena->page < 0) {
        assert(arena->first == arena->page);
        arena->first = get_page(arena->pool, size);
        arena->page = arena->first;
    }
    
    MemoryPage* p = arena->pool->pages + arena->page;
    if (p->size - arena->current >= size) {
        void* result = p->memory + arena->current;
        arena->current += size;
        return result;
    } else {
        i32 n_id = get_page(arena->pool, size);
        MemoryPage* n = arena->pool->pages + n_id;
        p->next = n_id;
        arena->page = n_id;
        arena->current = size;
        return n->memory;
    }
};

i32 get_page(MemoryPool* pool, u32 min_size)
{
    // TODO: Handle this somehow
    assert(pool->free_count > 0);

    // Look if a suitable page that is already allocated
    for (u32 i = 0; i < pool->free_count; ++i) {
        u32 page_id = pool->free_pages[i];
        MemoryPage* page = pool->pages + page_id;
        if (page->memory && page->size > min_size) {
            pool->free_count--;
            pool->free_pages[i] = pool->free_pages[pool->free_count];
            return page_id;
        }
    }

    // Try to allocate a new page
    u32 size = max(min_size, MEMORY_PAGE_SIZE);
    for (u32 i = 0; i < pool->free_count; ++i) {
        u32 page_id = pool->free_pages[i];
        MemoryPage* page = pool->pages + page_id;
        if (page->memory == NULL) {
            pool->free_count--;
            pool->free_pages[i] = pool->free_pages[pool->free_count];
            page->memory = (u8*) malloc(size);
            page->size = size;
            return page_id;
        }
    }

    // TODO: Try realloc unused but already allocated page
    assert(0);
    return -1;
}

void free_page(MemoryPool* pool, i32 page_id)
{
    pool->free_pages[pool->free_count] = page_id;
    pool->free_count++;
}

void dispose(Arena* arena)
{
    i32 page = arena->first;
    while (page > 0) {
        free_page(arena->pool, page);
    }
    arena->first = -1;
    arena->page = -1;
}
