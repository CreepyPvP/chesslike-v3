#include <assert.h>

#include "include/scene.h"
#include "include/arena.h"

void Scene::init()
{
    actor_count = 0;
}

void Scene::add(Actor actor)
{
    assert(actor_count < ACTOR_COUNT);
    actors[actor_count] = actor;
    actor_count++;
}
