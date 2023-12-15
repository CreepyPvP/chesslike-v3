#include <assert.h>

#include "include/scene.h"
#include "include/arena.h"

void Scene::init()
{
    actor_capacity = 10;
    actor_count = 2;
    actors = (Actor*) scene_arena.alloc(sizeof(Actor) * actor_capacity);
}

void Scene::add(Actor actor)
{
    assert(actor_count < actor_capacity);
    actors[actor_count] = actor;
    actor_count++;
}
