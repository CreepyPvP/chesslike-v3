#include "include/assets.h"
#include "include/defines.h"
#include "include/utils.h"
#include "include/arena.h"

#include <stdio.h>
#include <stdlib.h>


enum CtxType 
{
    None,
    Model,
    Object,
};

struct ParserCtx 
{
    void* ptr;
    CtxType type;
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

void load_scene(const char* file) 
{
    i32 len;
    asset_arena.start_scope();
    const char* content = read_file(file, &len, &asset_arena);
    if (!content) {
        printf("Failed to load scene: %s\n", file);
        exit(1);
    }
    printf("Parsing scene: %s\n", file);
    ParserCtx ctx;
    ctx.type = None;
    ctx.ptr = NULL;
    i32 version = read_version(&content);
    const char** ptr = &content;
    bool reached_end = false;
    while (!reached_end) {
        if (prefix("MODEL", ptr)) {
            skip_whitespaces(ptr);
            char* name = read_ident(ptr);
            next_line(ptr);
            ctx.type = Model;
            // ctx.ptr = ...
        } else if (prefix("OBJECT", ptr)) {
            skip_whitespaces(ptr);
            char* model = read_ident(ptr);
            next_line(ptr);
            ctx.type = Object;
            // ctx.ptr = ...
        } else if (prefix("PATH", ptr)) {
            skip_whitespaces(ptr);
            char* path = read_ident(ptr);
            next_line(ptr);
            if (ctx.type == Model) {
                // set model path here
            }
        } else if (prefix("POSITION", ptr)) {
            skip_whitespaces(ptr);
            next_line(ptr);
        } else if (prefix("ROTATION", ptr)) {
            skip_whitespaces(ptr);
            next_line(ptr);
        } else if (prefix("SCALE", ptr)) {
            skip_whitespaces(ptr);
            next_line(ptr);
        } else if (**ptr == 0) {
            reached_end = true;
        }
    }
    asset_arena.end_scope();
}
