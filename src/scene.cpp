#include <assert.h>

#include "include/scene.h"
#include "include/arena.h"

void init_scene(Scene* scene)
{
    scene->actor_count = 0;
}

void push_actor(Scene* scene, Actor actor)
{
    assert(scene->actor_count < ACTOR_COUNT);
    scene->actors[scene->actor_count] = actor;
    scene->actor_count++;
}
