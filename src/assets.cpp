#include "include/assets.h"

#include <assert.h>

#include <include/arena.h>

Model* ModelBuffer::stage(Vertex* vtx, 
                          u32 vtx_count, 
                          u32* indices, 
                          u32 index_count)
{
    assert(staged_models < MODEL_BUFFER_MAX_MODELS);
    vtx_queue[staged_models] = vtx;
    vtx_queue_len[staged_models] = vtx_count;
    index_queue[staged_models] = indices;
    index_queue_len[staged_models] = index_count;
    staged_models++;

    Model model;
    model.start_index = index_counter;
    model.index_count = index_count;
    model.vertex_offset = vertex_counter;
    Model* ptr = (Model*) asset_arena.alloc(sizeof(Model));
    *ptr = model;

    index_counter += index_count;
    vertex_counter += vtx_count;

    return ptr;
}

void ModelBuffer::clear()
{
    staged_models = 0;
    index_counter = 0;
    vertex_counter = 0;
}

void ModelBuffer::init()
{
    vtx_queue = (Vertex**) asset_arena.alloc(sizeof(Vertex*) * MODEL_BUFFER_MAX_MODELS);
    vtx_queue_len = (u32*) asset_arena.alloc(sizeof(u32) * MODEL_BUFFER_MAX_MODELS);
    index_queue = (u32**) asset_arena.alloc(sizeof(u32*) * MODEL_BUFFER_MAX_MODELS);
    index_queue_len = (u32*) asset_arena.alloc(sizeof(u32) * MODEL_BUFFER_MAX_MODELS);
    clear();
}

ModelBuffer model_buffer;
