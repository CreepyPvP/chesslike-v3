#include "include/assets.h"
#include "include/defines.h"
#include "include/utils.h"
#include "include/arena.h"

#include <stdio.h>
#include <stdlib.h>


enum CtxType 
{
    NONE,
    MODEL,
    ACTOR,
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

bool is_alphabetic(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

char* read_ident(const char** ptr)
{
    const char* start = *ptr;
    i32 len = 0;
    while (is_alphabetic(**ptr)) {
        (*ptr)++;
        len++;
    }
    char* str = (char*) asset_arena.alloc(len + 1);
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

void flush_ctx(CtxType type, Actor actor_buffer, Scene* scene)
{
    if (type == ACTOR) {
        scene->add(actor_buffer);
    }
}

void load_scene(const char* file, Scene* scene) 
{
    i32 len;
    asset_arena.start_scope();
    const char* content = read_file(file, &len, &asset_arena);
    if (!content) {
        printf("Failed to load scene: %s\n", file);
        exit(1);
    }
    printf("Parsing scene: %s\n", file);
    const char** ptr = &content;
    bool reached_end = false;
    CtxType ctx_type = NONE;
    Actor actor_buffer{};

    i32 version = read_version(&content);
    while (!reached_end) {
        if (prefix("MODEL", ptr)) {
            flush_ctx(ctx_type, actor_buffer, scene);

            skip_whitespaces(ptr);
            char* name = read_ident(ptr);
            next_line(ptr);
            ctx_type = MODEL;
            // ctx.ptr = ...
        } else if (prefix("ACTOR", ptr)) {
            flush_ctx(ctx_type, actor_buffer, scene);

            skip_whitespaces(ptr);
            char* model = read_ident(ptr);
            next_line(ptr);
            ctx_type = ACTOR;
            actor_buffer = {};
            actor_buffer.scale_x = 1;
            actor_buffer.scale_y = 1;
            actor_buffer.scale_z = 1;
        } else if (prefix("PATH", ptr)) {
            skip_whitespaces(ptr);
            char* path = read_ident(ptr);
            next_line(ptr);
            if (ctx_type == MODEL) {
                // set model path here
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
                actor_buffer.x = x;
                actor_buffer.y = y;
                actor_buffer.z = z;
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
                actor_buffer.rot_x = x;
                actor_buffer.rot_y = y;
                actor_buffer.rot_z = z;
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
                actor_buffer.scale_x = x;
                actor_buffer.scale_y = y;
                actor_buffer.scale_z = z;
            }
            next_line(ptr);
        } else if (**ptr == 0) {
            reached_end = true;
        }
    }
    flush_ctx(ctx_type, actor_buffer, scene);
    asset_arena.end_scope();
}
