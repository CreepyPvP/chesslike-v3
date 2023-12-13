#pragma once

#include "include/defines.h"

struct Arena
{
    u32 scopes[64];
    u8 scope;
    u32 current;
    u32 capacity;
    u8* base;

    void start_scope();
    void end_scope();
    void* alloc(u32 bytes);
    void reset();
    void dispose();
    void init();
};
