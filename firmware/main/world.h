#pragma once
#include <stdbool.h>
#include <stdint.h>
enum { WORLD_ROLES=3, WORLD_TASKS_PER_ROLE=10, WORLD_TASKS=30 };
typedef struct { uint32_t role, active[3], done[3], rng, wait_ms; } world_t;
extern const char *const world_roles[3];
extern const char *const mission_names[WORLD_TASKS];
extern const char *const checklist_labels[7];
void world_init(world_t *w);
bool world_valid(const world_t *w);
bool world_step(world_t *w,uint32_t elapsed,bool eligible);
bool world_complete(world_t *w,unsigned task);
int world_task(const world_t *w,unsigned row);
unsigned world_count(const world_t *w,bool done);
