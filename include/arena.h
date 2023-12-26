#pragma once

#include "include/defines.h"

#define MEMORY_PAGE_SIZE 2000000
#define MEMORY_PAGE_COUNT 64


struct MemoryPage
{
    u8* memory;
    u32 next;
    u32 size;
};

struct MemoryPool 
{
    MemoryPage pages[MEMORY_PAGE_COUNT];
    u32 free_pages[MEMORY_PAGE_COUNT];
    u32 free_count;
};

struct Arena
{
    MemoryPool* pool;
    i32 first;
    i32 page;
    u32 current;
};

void init_pool(MemoryPool* pool);
i32 get_page(MemoryPool* pool, u32 min_size);
void free_page(MemoryPool* pool, i32 page_id);

void init_arena(Arena* arena, MemoryPool* pool);
void* push_size(Arena* arena, u32 size);
void dispose(Arena* arena);

// 0 => static meshes, 1 => skinned meshses
extern Arena vertex_arena[2];
extern Arena index_arena[2];

extern Arena asset_arena;

extern Arena tmp_arena;
