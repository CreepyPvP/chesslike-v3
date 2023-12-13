#include "include/arena.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void Arena::start_scope()
{
    scopes[scope] = current;
    scope++;
}

void Arena::end_scope()
{
    scope--;
    current = scopes[scope];
}

void* Arena::alloc(u32 bytes)
{
    assert(current + bytes <= capacity);
    void* ptr = base + current;
    current += bytes;
    return ptr;
}

void Arena::reset()
{
    scope = 0;
    current = 0;
}

void Arena::dispose()
{
    free(base);
}

void Arena::init(u32 bytes)
{
    base = (u8*) malloc(bytes);
    capacity = bytes;
    reset();
}

Arena asset_arena;
