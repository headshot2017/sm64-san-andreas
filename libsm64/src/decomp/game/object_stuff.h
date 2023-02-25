#pragma once

#include "../include/types.h"
#include "../../libsm64.h"

struct Object *hack_allocate_mario(void);
void bhv_mario_update(void);
void create_transformation_from_matrices(Mat4 a0, Mat4 a1, Mat4 a2);
void obj_update_pos_from_parent_transformation(Mat4 a0, struct Object *a1);
void obj_set_gfx_pos_from_pos(struct Object *obj);

void resolve_object_collisions();
uint32_t add_object_collider(struct SM64ObjectCollider* collider);
void move_object_collider(uint32_t objId, float x, float y, float z);
void delete_object_collider(uint32_t objId);
void free_obj_pool();
