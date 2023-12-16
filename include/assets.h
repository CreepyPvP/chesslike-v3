#pragma once

#include "include/defines.h"

#define MODEL_BUFFER_MAX_MODELS 32

struct Vertex 
{
    float x;
    float y;
    float z;
    float n_x;
    float n_y;
    float n_z;
};

struct Model
{
    u32 start_index;
    u32 index_count;
    u32 vertex_offset;
};

// This is so utterly fucking dogshit why did i do this why why why
struct ModelBuffer
{
    Vertex** vtx_queue;
    u32* vtx_queue_len;
    u32** index_queue;
    u32* index_queue_len;
    u32 staged_models;

    u32 index_counter;
    u32 vertex_counter;

    Model* stage(Vertex* vtx, u32 vtx_count, u32* indices, u32 index_count);
    void clear();
    void init();
};

extern ModelBuffer model_buffer;
