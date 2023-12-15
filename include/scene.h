#pragma once

#include "include/defines.h"

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
};

struct Scene 
{
    Actor* actors;
    u32 actor_count;
    u32 actor_capacity;

    void init();
    void add(Actor actor);
};
