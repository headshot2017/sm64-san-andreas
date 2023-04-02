#pragma once

#include <stdint.h>
#include <stddef.h>

#include "decomp/include/types.h"

extern void load_mario_animation(struct MarioAnimation *a, u32 index);

extern void load_mario_anims_from_rom( const uint8_t *rom);
extern uint32_t load_mario_custom_anim_from_data(const uint8_t *data, const uint32_t size);


extern void unload_mario_anims( void );
