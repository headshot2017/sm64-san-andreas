#ifndef MARIO_CUSTOM_ANIMS_H_INCLUDED
#define MARIO_CUSTOM_ANIMS_H_INCLUDED

#include <unordered_map>

extern int MARIO_ANIM_CUSTOM_TEST;
extern int MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_LEFT;
extern int MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_RIGHT;
extern int MARIO_ANIM_CUSTOM_PICKUP_SLOW;
extern int MARIO_ANIM_CUSTOM_PUTDOWN_SLOW;
extern int MARIO_ANIM_CUSTOM_CAR_LOCKED;
extern int MARIO_ANIM_CUSTOM_EAT;
extern int MARIO_ANIM_CUSTOM_VOMIT;
extern int MARIO_ANIM_CUSTOM_CRIB_SWITCH;
extern int MARIO_ANIM_CUSTOM_FACEPALM;
extern int MARIO_ANIM_CUSTOM_LAUGH01;
extern int MARIO_ANIM_CUSTOM_DANCE_LOOP;
extern int MARIO_ANIM_CUSTOM_DANCE_BAD;
extern int MARIO_ANIM_CUSTOM_DANCE_GOOD;
extern int MARIO_ANIM_CUSTOM_VENDING_MACHINE;
extern int MARIO_ANIM_CUSTOM_GUNPOINT;
extern int MARIO_ANIM_CUSTOM_GUNPOINT_WALK_START;
extern int MARIO_ANIM_CUSTOM_GUNPOINT_TIPTOE;
extern int MARIO_ANIM_CUSTOM_GUNPOINT_WALK;
extern int MARIO_ANIM_CUSTOM_GUNPOINT_RUN;
extern int MARIO_ANIM_CUSTOM_GUNPOINT_SKID;
extern int MARIO_ANIM_CUSTOM_GUNPOINT_SKID_STOP;
extern int MARIO_ANIM_CUSTOM_GUNPOINT_SKID_TURN;

extern std::unordered_map<int, int> gunAnimOverrideTable;

void marioInitCustomAnims();

#endif // MARIO_CUSTOM_ANIMS_H_INCLUDED
