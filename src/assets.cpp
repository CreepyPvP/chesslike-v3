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

    // TODO
    return NULL;
}

void ModelBuffer::write_to_gpu()
{

}

void ModelBuffer::clear()
{
    staged_models = 0;
    index_counter = 0;
}

void ModelBuffer::init()
{
    vtx_queue = (Vertex**) 
        asset_arena.alloc(sizeof(Vertex*) * MODEL_BUFFER_MAX_MODELS);
    index_queue = (u32**) 
        asset_arena.alloc(sizeof(u32*) * MODEL_BUFFER_MAX_MODELS);
    index_queue_len = (u32*) 
        asset_arena.alloc(sizeof(u32) * MODEL_BUFFER_MAX_MODELS);
    clear();
}

ModelBuffer model_buffer;
