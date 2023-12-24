#pragma once

#include "include/defines.h"
#include "include/assets.h"

#include <glm/mat4x4.hpp>

#define ACTOR_COUNT 64

struct Actor
{
    float x;
    float y;
    float z;
    float rot_x;
    float rot_y;
    float rot_z;
    float scale_x;
    float scale_y;
    float scale_z;
    u32 material;

    glm::mat4 prev_mvp;

    Model* model;
};

struct Scene 
{
    Actor actors[ACTOR_COUNT];
    u32 actor_count;

    void init();
    void add(Actor actor);
};
