#include "mario_custom_anims.h"

#include "raw/anim_test.raw.h"
#include "raw/anim_close_car_door_left.raw.h"
#include "raw/anim_close_car_door_right.raw.h"

extern "C" {
    #include <libsm64.h>
}

int MARIO_ANIM_CUSTOM_TEST;
int MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_LEFT;
int MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_RIGHT;

void marioInitCustomAnims()
{
    MARIO_ANIM_CUSTOM_TEST = sm64_custom_animation_init(marioAnimTestRaw, marioAnimTestRaw_length);
    MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_LEFT = sm64_custom_animation_init(marioAnimCloseCarDoorLeftRaw, marioAnimCloseCarDoorLeftRaw_length);
    MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_RIGHT = sm64_custom_animation_init(marioAnimCloseCarDoorRightRaw, marioAnimCloseCarDoorRightRaw_length);
}
