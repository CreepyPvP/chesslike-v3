#pragma once

#include "include/defines.h"

// TODO: implement arena growing
struct Arena
{
    u32 scopes[16];
    u32 current;
    u32 capacity;
    u8* base;
    u8 scope;

    void start_scope();
    void end_scope();
    void* alloc(u32 bytes);
    void reset();
    void dispose();
    void init(u32 bytes);
};

// 0 => static meshes, 1 => skinned meshses
extern Arena vertex_arena[2];
extern Arena index_arena[2];

extern Arena asset_arena;

extern Arena tmp_arena;
