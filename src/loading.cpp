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
#define MAX_SKELETONS 64
#define MAX_BONES 64

#define MODEL_FLAG_SKINNED 1 << 0


enum ContextType 
{
    NONE = 0,
    MODEL,
    ACTOR,
    SKELETON,
};

struct ModelContext 
{
    char* name;
    char* file;
    Model model;
    u8 flags;
};

struct SkeletonContext
{
    glm::mat4 bones[MAX_BONES];
    u32 bone_count;
};

struct Context 
{
    union {
        ModelContext model;
        Actor actor;
        SkeletonContext skeleton;
    };

    ContextType type;

    char* model_names[MAX_MODELS];
    Model* models[MAX_MODELS];
    u32 model_count;

    char* skeleton_names[MAX_SKELETONS];
    u32 skeleton_count;
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
        c == '_' ||
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

void load_model(const char* file, ModelContext* model) 
{
    tmp_arena.start_scope();
    i32 file_len;
    char* buffer = read_file(file, &file_len, &tmp_arena);
    if (!buffer)
        exit(1);

    u32* int_ptr = (u32*) buffer;
    u32 vertex_count = *int_ptr;
    ++int_ptr;
    u32 index_count = *int_ptr;
    ++int_ptr;

    u32 vertex_stride;
    u8* vertex_memory;
    Arena* vertex_acc;
    Arena* index_acc;
    if (model->flags & MODEL_FLAG_SKINNED) {
        vertex_stride = sizeof(SkinnedVertex);
        vertex_acc = vertex_arena + 1;
        index_acc = index_arena + 1;
    } else {
        vertex_stride = sizeof(Vertex);
        vertex_acc = vertex_arena;
        index_acc = index_arena;
    }

    model->model.index_count = index_count;
    model->model.index_offset = index_acc->current / sizeof(u32);
    model->model.vertex_offset = vertex_acc->current / vertex_stride;

    u32 byte_size = vertex_count * vertex_stride;
    vertex_memory = (u8*) vertex_acc->alloc(vertex_stride * vertex_count);
    
    // TODO: does little / big endian matter here? Investigate!
    u8* ptr = (u8*) int_ptr;
    memcpy(vertex_memory, ptr, byte_size);
    ptr += byte_size;

    u32 index_bytes = sizeof(u32) * index_count;
    u8* index_memory = (u8*) index_acc->alloc(index_bytes);
    memcpy(index_memory, ptr, index_bytes);

    tmp_arena.end_scope();
}

void flush_ctx(Context* context, Scene* scene)
{
    if (context->type == ACTOR) {
        scene->add(context->actor);
    } else if (context->type == MODEL) {
        if (!context->model.file) {
            printf("No path specified for model: %s\n", context->model.name);
            return;
        }
        load_model(context->model.file, &context->model);
        assert(context->model_count < MAX_MODELS);
        Model* model = (Model*) asset_arena.alloc(sizeof(Model));
        *model = context->model.model;
        context->model_names[context->model_count] = context->model.name;
        context->models[context->model_count] = model;
        context->model_count++;
    } else if (context->type == SKELETON) {

    }
}

void source_file(const char* file, Scene* scene) 
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
    Context context{};

    i32 version = read_version(&content);
    while (!reached_end) {
        if (prefix("MODEL", ptr)) {
            flush_ctx(&context, scene);

            skip_whitespaces(ptr);
            char* name = read_ident(ptr);
            next_line(ptr);
            context.type = MODEL;
            context.model = ModelContext{};
            context.model.name = name;
        } else if (prefix("ACTOR", ptr)) {
            flush_ctx(&context, scene);

            skip_whitespaces(ptr);
            char* model = read_ident(ptr);
            next_line(ptr);
            context.type = ACTOR;
            context.actor = {};
            context.actor.scale_x = 1;
            context.actor.scale_y = 1;
            context.actor.scale_z = 1;
            for (u32 i = 0; i < context.model_count; ++i) {
                if (strcmp(context.model_names[i], model) == 0) {
                    context.actor.model = context.models[i];
                    break;
                }
            }
            if (context.actor.model == NULL) {
                printf("Unknown model: %s\n", model);
            }
        } else if (prefix("PATH", ptr)) {
            skip_whitespaces(ptr);
            char* path = read_ident(ptr);
            next_line(ptr);
            if (context.type == MODEL) {
                context.model.file = path;
            }
        } else if (prefix("POSITION", ptr)) {
            skip_whitespaces(ptr);
            if (context.type == ACTOR) {
                float x = read_float(ptr);
                skip_whitespaces(ptr);
                float y = read_float(ptr);
                skip_whitespaces(ptr);
                float z = read_float(ptr);
                skip_whitespaces(ptr);
                context.actor.x = x;
                context.actor.y = y;
                context.actor.z = z;
            }
            next_line(ptr);
        } else if (prefix("ROTATION", ptr)) {
            skip_whitespaces(ptr);
            if (context.type == ACTOR) {
                float x = read_float(ptr);
                skip_whitespaces(ptr);
                float y = read_float(ptr);
                skip_whitespaces(ptr);
                float z = read_float(ptr);
                skip_whitespaces(ptr);
                context.actor.rot_x = x;
                context.actor.rot_y = y;
                context.actor.rot_z = z;
            }
            next_line(ptr);
        } else if (prefix("SCALE", ptr)) {
            skip_whitespaces(ptr);
            if (context.type == ACTOR) {
                float x = read_float(ptr);
                skip_whitespaces(ptr);
                float y = read_float(ptr);
                skip_whitespaces(ptr);
                float z = read_float(ptr);
                skip_whitespaces(ptr);
                context.actor.scale_x = x;
                context.actor.scale_y = y;
                context.actor.scale_z = z;
            }
            next_line(ptr);
        } else if (prefix("MATERIAL", ptr)) {
            skip_whitespaces(ptr);
            if (context.type == ACTOR) {
                i32 material = read_int(ptr);
                if (material >= 0) {
                    context.actor.material = material;
                }
            }
            next_line(ptr);
        } else if (prefix("SKELETON", ptr)) {
            flush_ctx(&context, scene);
            skip_whitespaces(ptr);
            char* name = read_ident(ptr);
            context.type = SKELETON;
            context.skeleton = {};
            assert(context.skeleton_count < MAX_SKELETONS);
            context.skeleton_names[context.skeleton_count] = name;
            next_line(ptr);
        } else if (prefix("BONE", ptr)) {
            skip_whitespaces(ptr);
            if (context.type != SKELETON) {
                printf("BONE has to be in skeleton context\n");
                exit(1);
            }
            assert(context.skeleton.bone_count < MAX_BONES);
            char* name = read_ident(ptr);
            float* matrix_value = (float*) (context.skeleton.bones + context.skeleton.bone_count);
            for (u32 i = 0; i < 16; ++i) {
                skip_whitespaces(ptr);
                *matrix_value = read_float(ptr);
                matrix_value++;
            }
            context.skeleton.bone_count++;
            next_line(ptr);
        } else if (prefix("USE_SKELETON", ptr)) {
            if (context.type != MODEL) {
                printf("USE_SKELETON has to be used in Model context\n");
                exit(1);
            }
            context.model.flags |= MODEL_FLAG_SKINNED;
            skip_whitespaces(ptr);
            char* name = read_ident(ptr);
            next_line(ptr);
        } else if (prefix("#", ptr)) {
            next_line(ptr);
        } else if (**ptr == 0) {
            reached_end = true;
        } else {
            printf("Failed to parse file at symbol: %c", **ptr);
            exit(1);
        }
    }
    flush_ctx(&context, scene);
    tmp_arena.end_scope();
}
