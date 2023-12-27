#pragma once

#include "include/defines.h"

#define MAX_MESSAGES 128

struct StaticObject
{
    u32 uniform_slot;
    u32 material;
    u32 vertex_offset;
    u32 index_offset;
    u32 index_count;
};

struct Message 
{
    StaticObject object;
    u32 pipeline;
};

struct RenderQueue
{
    Message messages[MAX_MESSAGES];
    u32 message_count;
};
