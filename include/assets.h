#pragma once

#include "include/defines.h"

#define MODEL_BUFFER_MAX_MODELS 32

struct Vertex 
{
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
};

struct SkinnedVertex 
{
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    i32 bones[3];
    float weights[3];
};

struct Model
{
    u32 index_count;
    u32 index_offset;
    u32 vertex_offset;
};
