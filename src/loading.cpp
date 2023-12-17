#include "include/loading.h"
#include "include/defines.h"
#include "include/utils.h"
#include "include/arena.h"
#include "include/assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_MODELS 64


enum CtxType 
{
    NONE,
    MODEL,
    ACTOR,
};

struct ModelParams
{
    Vertex* vertices;
    u32* indices;
    u32 vertex_count;
    u32 index_count;
    char* name;
};

// returns true if ptr starts with prefix. 
// Also removes the prefix string from ptr
bool prefix(const char* prefix, const char** ptr)
{
    i32 i = 0;
    while (prefix[i]) {
        if (prefix[i] != (*ptr)[i])
            return false;
        ++i;
    }
    (*ptr) += i;
    return true;
}

void skip_whitespaces(const char** ptr)
{
    while (**ptr == ' ') {
        (*ptr)++;
    }
}

void next_line(const char** ptr) 
{
    while (**ptr != '\n' && **ptr != 0) {
        (*ptr)++;
    }
    // praise windows
    while (**ptr == '\n' || **ptr == '\r') {
        (*ptr)++;
    }
}

i32 read_version(const char** ptr)
{
    i32 version = atoi(*ptr);
    next_line(ptr);
    return version;
}

bool is_char(char c)
{
    return (c >= 'a' && c <= 'z') || 
        (c >= 'A' && c <= 'Z') || 
        c == '/' ||
        c == '.';
}

char* read_ident(const char** ptr)
{
    const char* start = *ptr;
    i32 len = 0;
    while (is_char(**ptr)) {
        (*ptr)++;
        len++;
    }
    char* str = (char*) tmp_arena.alloc(len + 1);
    for (i32 i = 0; i < len; ++i) {
        str[i] = start[i];
    }
    str[len] = 0;
    return str;
}

float read_float(const char** ptr)
{
    float result = atof(*ptr);
    while (**ptr != 0 && **ptr != ' ' && **ptr != '\n') {
        (*ptr)++;
    }
    return result;
}

i32 read_int(const char** ptr) 
{
    i32 result = atoi(*ptr);
    while (**ptr != 0 && **ptr != ' ' && **ptr != '\n') {
        (*ptr)++;
    }
    return result;
}

void flush_ctx(CtxType type, 
               Actor actor_ctx, 
               ModelParams model_ctx, 
               Scene* scene,
               char** model_names,
               Model** models,
               u32* model_count)
{
    if (type == ACTOR) {
        scene->add(actor_ctx);
    } else if (type == MODEL) {
        if (model_ctx.vertices == NULL || model_ctx.indices == NULL) {
            printf("No path specified for model: %s\n", model_ctx.name);
            return;
        }
        Model* ptr = model_buffer.stage(model_ctx.vertices, 
                                        model_ctx.vertex_count, 
                                        model_ctx.indices, 
                                        model_ctx.index_count);
        assert(*model_count < MAX_MODELS);
        model_names[*model_count] = model_ctx.name;
        models[*model_count] = ptr;
        (*model_count) += 1;
    }
}

void load_model(const char* file, ModelParams* model) 
{
    i32 file_len;
    char* buffer = read_file(file, &file_len, &asset_arena);
    if (!buffer)
        exit(1);

    u32* int_ptr = (u32*) buffer;
    u32 vertex_count = *int_ptr;
    ++int_ptr;
    u32 index_count = *int_ptr;
    ++int_ptr;
    
    Vertex* vertex_ptr = (Vertex*) int_ptr;
    Vertex* vertices =  vertex_ptr;
    vertex_ptr += vertex_count;
    int_ptr = (u32*) vertex_ptr;
    u32* indices = int_ptr;

    model->vertex_count = vertex_count;
    model->index_count = index_count;
    model->vertices = vertices;
    model->indices = indices;
}

void load_scene(const char* file, Scene* scene) 
{
    i32 len;
    tmp_arena.start_scope();
    const char* content = read_file(file, &len, &tmp_arena);
    if (!content) {
        printf("Failed to load scene: %s\n", file);
        exit(1);
    }
    printf("Parsing scene: %s\n", file);
    const char** ptr = &content;
    bool reached_end = false;
    CtxType ctx_type = NONE;
    Actor actor_ctx{};
    ModelParams model_ctx{};
    char* model_names[MAX_MODELS];
    Model* models[MAX_MODELS];
    u32 model_count = 0;

    i32 version = read_version(&content);
    while (!reached_end) {
        if (prefix("MODEL", ptr)) {
            flush_ctx(ctx_type, 
                      actor_ctx, 
                      model_ctx,
                      scene, 
                      model_names, 
                      models, 
                      &model_count);

            skip_whitespaces(ptr);
            char* name = read_ident(ptr);
            next_line(ptr);
            ctx_type = MODEL;
            model_ctx = ModelParams{};
            model_ctx.name = name;
        } else if (prefix("ACTOR", ptr)) {
            flush_ctx(ctx_type, 
                      actor_ctx, 
                      model_ctx,
                      scene, 
                      model_names, 
                      models, 
                      &model_count);

            skip_whitespaces(ptr);
            char* model = read_ident(ptr);
            next_line(ptr);
            ctx_type = ACTOR;
            actor_ctx = {};
            actor_ctx.scale_x = 1;
            actor_ctx.scale_y = 1;
            actor_ctx.scale_z = 1;
            for (u32 i = 0; i < model_count; ++i) {
                if (strcmp(model_names[i], model) == 0) {
                    actor_ctx.model = models[i];
                    break;
                }
            }
            if (actor_ctx.model == NULL) {
                printf("Unknown model: %s\n", model);
            }
        } else if (prefix("PATH", ptr)) {
            skip_whitespaces(ptr);
            char* path = read_ident(ptr);
            next_line(ptr);
            if (ctx_type == MODEL) {
                load_model(path, &model_ctx);
            }
        } else if (prefix("POSITION", ptr)) {
            skip_whitespaces(ptr);
            if (ctx_type == ACTOR) {
                float x = read_float(ptr);
                skip_whitespaces(ptr);
                float y = read_float(ptr);
                skip_whitespaces(ptr);
                float z = read_float(ptr);
                skip_whitespaces(ptr);
                actor_ctx.x = x;
                actor_ctx.y = y;
                actor_ctx.z = z;
            }
            next_line(ptr);
        } else if (prefix("ROTATION", ptr)) {
            skip_whitespaces(ptr);
            if (ctx_type == ACTOR) {
                float x = read_float(ptr);
                skip_whitespaces(ptr);
                float y = read_float(ptr);
                skip_whitespaces(ptr);
                float z = read_float(ptr);
                skip_whitespaces(ptr);
                actor_ctx.rot_x = x;
                actor_ctx.rot_y = y;
                actor_ctx.rot_z = z;
            }
            next_line(ptr);
        } else if (prefix("SCALE", ptr)) {
            skip_whitespaces(ptr);
            if (ctx_type == ACTOR) {
                float x = read_float(ptr);
                skip_whitespaces(ptr);
                float y = read_float(ptr);
                skip_whitespaces(ptr);
                float z = read_float(ptr);
                skip_whitespaces(ptr);
                actor_ctx.scale_x = x;
                actor_ctx.scale_y = y;
                actor_ctx.scale_z = z;
            }
            next_line(ptr);
        } else if (prefix("MATERIAL", ptr)) {
            skip_whitespaces(ptr);
            if (ctx_type == ACTOR) {
                i32 material = read_int(ptr);
                if (material >= 0) {
                    actor_ctx.material = material;
                }
            }
            next_line(ptr);
        }else if (**ptr == 0) {
            reached_end = true;
        }
    }
    flush_ctx(ctx_type, 
              actor_ctx, 
              model_ctx,
              scene, 
              model_names, 
              models, 
              &model_count);
    tmp_arena.end_scope();
}
