#include "mario_custom_anims.h"

#include "raw/anim_test.raw.h"
#include "raw/anim_close_car_door_left.raw.h"
#include "raw/anim_close_car_door_right.raw.h"
#include "raw/anim_pickup_slow.raw.h"
#include "raw/anim_putdown_slow.raw.h"
#include "raw/anim_car_locked.raw.h"
#include "raw/anim_eat.raw.h"
#include "raw/anim_vomit.raw.h"
#include "raw/anim_crib_use_switch.raw.h"
#include "raw/anim_facepalm.raw.h"
#include "raw/anim_laugh01.raw.h"
#include "raw/anim_dance_loop.raw.h"
#include "raw/anim_dance_bad.raw.h"
#include "raw/anim_dance_good.raw.h"
#include "raw/anim_vending_machine.raw.h"
#include "raw/anim_gunpoint.raw.h"
#include "raw/anim_gunpoint_walk_start.raw.h"
#include "raw/anim_gunpoint_tiptoe.raw.h"
#include "raw/anim_gunpoint_walk.raw.h"
#include "raw/anim_gunpoint_run.raw.h"
#include "raw/anim_gunpoint_skid.raw.h"
#include "raw/anim_gunpoint_skid_stop.raw.h"
#include "raw/anim_gunpoint_skid_turn.raw.h"
#include "raw/anim_gunside_idle.raw.h"
#include "raw/anim_gunside_idle_alt.raw.h"
#include "raw/anim_gunside_walk_start.raw.h"
#include "raw/anim_gunside_tiptoe.raw.h"
#include "raw/anim_gunside_walk.raw.h"
#include "raw/anim_gunside_run.raw.h"
#include "raw/anim_gunside_skid_start.raw.h"
#include "raw/anim_gunside_skid_stop.raw.h"
#include "raw/anim_gunside_skid_turn.raw.h"
#include "raw/anim_rpg_idle.raw.h"
#include "raw/anim_rpg_idle_alt.raw.h"
#include "raw/anim_rpg_walk_start.raw.h"
#include "raw/anim_rpg_tiptoe.raw.h"
#include "raw/anim_rpg_walk.raw.h"
#include "raw/anim_rpg_run.raw.h"
#include "raw/anim_rpg_skid_start.raw.h"
#include "raw/anim_rpg_skid_stop.raw.h"
#include "raw/anim_rpg_skid_turn.raw.h"
#include "raw/anim_gunheavy_idle.raw.h"
#include "raw/anim_gunheavy_idle_alt.raw.h"
#include "raw/anim_gunheavy_walk_start.raw.h"
#include "raw/anim_gunheavy_tiptoe.raw.h"
#include "raw/anim_gunheavy_walk.raw.h"
#include "raw/anim_gunheavy_run.raw.h"
#include "raw/anim_gunheavy_skid_start.raw.h"
#include "raw/anim_gunheavy_skid_stop.raw.h"
#include "raw/anim_gunheavy_skid_turn.raw.h"
#include "raw/anim_rifle_aim.raw.h"
#include "raw/anim_rifle_aim_walk.raw.h"
#include "raw/anim_gunheavy_aim.raw.h"
#include "raw/anim_gunheavy_aim_walk.raw.h"
#include "raw/anim_gunlight_aim.raw.h"
#include "raw/anim_gunlight_aim_walk.raw.h"

extern "C" {
    #include <libsm64.h>
    #include <decomp/include/mario_animation_ids.h>
}

int MARIO_ANIM_CUSTOM_TEST;
int MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_LEFT;
int MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_RIGHT;
int MARIO_ANIM_CUSTOM_PICKUP_SLOW;
int MARIO_ANIM_CUSTOM_PUTDOWN_SLOW;
int MARIO_ANIM_CUSTOM_CAR_LOCKED;
int MARIO_ANIM_CUSTOM_EAT;
int MARIO_ANIM_CUSTOM_VOMIT;
int MARIO_ANIM_CUSTOM_CRIB_SWITCH;
int MARIO_ANIM_CUSTOM_FACEPALM;
int MARIO_ANIM_CUSTOM_LAUGH01;
int MARIO_ANIM_CUSTOM_DANCE_LOOP;
int MARIO_ANIM_CUSTOM_DANCE_BAD;
int MARIO_ANIM_CUSTOM_DANCE_GOOD;
int MARIO_ANIM_CUSTOM_VENDING_MACHINE;
int MARIO_ANIM_CUSTOM_GUNPOINT;
int MARIO_ANIM_CUSTOM_GUNPOINT_WALK_START;
int MARIO_ANIM_CUSTOM_GUNPOINT_TIPTOE;
int MARIO_ANIM_CUSTOM_GUNPOINT_WALK;
int MARIO_ANIM_CUSTOM_GUNPOINT_RUN;
int MARIO_ANIM_CUSTOM_GUNPOINT_SKID;
int MARIO_ANIM_CUSTOM_GUNPOINT_SKID_STOP;
int MARIO_ANIM_CUSTOM_GUNPOINT_SKID_TURN;
int MARIO_ANIM_CUSTOM_GUNSIDE_IDLE;
int MARIO_ANIM_CUSTOM_GUNSIDE_IDLE_ALT;
int MARIO_ANIM_CUSTOM_GUNSIDE_WALK_START;
int MARIO_ANIM_CUSTOM_GUNSIDE_TIPTOE;
int MARIO_ANIM_CUSTOM_GUNSIDE_WALK;
int MARIO_ANIM_CUSTOM_GUNSIDE_RUN;
int MARIO_ANIM_CUSTOM_GUNSIDE_SKID;
int MARIO_ANIM_CUSTOM_GUNSIDE_SKID_STOP;
int MARIO_ANIM_CUSTOM_GUNSIDE_SKID_TURN;
int MARIO_ANIM_CUSTOM_RPG_IDLE;
int MARIO_ANIM_CUSTOM_RPG_IDLE_ALT;
int MARIO_ANIM_CUSTOM_RPG_WALK_START;
int MARIO_ANIM_CUSTOM_RPG_TIPTOE;
int MARIO_ANIM_CUSTOM_RPG_WALK;
int MARIO_ANIM_CUSTOM_RPG_RUN;
int MARIO_ANIM_CUSTOM_RPG_SKID;
int MARIO_ANIM_CUSTOM_RPG_SKID_STOP;
int MARIO_ANIM_CUSTOM_RPG_SKID_TURN;
int MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE;
int MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE_ALT;
int MARIO_ANIM_CUSTOM_GUNHEAVY_WALK_START;
int MARIO_ANIM_CUSTOM_GUNHEAVY_TIPTOE;
int MARIO_ANIM_CUSTOM_GUNHEAVY_WALK;
int MARIO_ANIM_CUSTOM_GUNHEAVY_RUN;
int MARIO_ANIM_CUSTOM_GUNHEAVY_SKID;
int MARIO_ANIM_CUSTOM_GUNHEAVY_SKID_STOP;
int MARIO_ANIM_CUSTOM_GUNHEAVY_SKID_TURN;
int MARIO_ANIM_CUSTOM_RIFLE_AIM;
int MARIO_ANIM_CUSTOM_RIFLE_AIM_WALK;
int MARIO_ANIM_CUSTOM_GUNHEAVY_AIM;
int MARIO_ANIM_CUSTOM_GUNHEAVY_AIM_WALK;
int MARIO_ANIM_CUSTOM_GUNLIGHT_AIM;
int MARIO_ANIM_CUSTOM_GUNLIGHT_AIM_WALK;

std::unordered_map<int, int> gunAnimOverrideTable;
std::unordered_map<int, int> gunSideAnimOverrideTable;
std::unordered_map<int, int> gunShoulderAnimOverrideTable;
std::unordered_map<int, int> gunHeavyAnimOverrideTable;

void marioInitCustomAnims()
{
    MARIO_ANIM_CUSTOM_TEST = sm64_custom_animation_init(marioAnimTestRaw, marioAnimTestRaw_length);
    MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_LEFT = sm64_custom_animation_init(marioAnimCloseCarDoorLeftRaw, marioAnimCloseCarDoorLeftRaw_length);
    MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_RIGHT = sm64_custom_animation_init(marioAnimCloseCarDoorRightRaw, marioAnimCloseCarDoorRightRaw_length);
    MARIO_ANIM_CUSTOM_PICKUP_SLOW = sm64_custom_animation_init(marioAnimPickupSlowRaw, marioAnimPickupSlowRaw_length);
    MARIO_ANIM_CUSTOM_PUTDOWN_SLOW = sm64_custom_animation_init(marioAnimPutdownSlowRaw, marioAnimPutdownSlowRaw_length);
    MARIO_ANIM_CUSTOM_CAR_LOCKED = sm64_custom_animation_init(marioAnimCarLockedRaw, marioAnimCarLockedRaw_length);
    MARIO_ANIM_CUSTOM_EAT = sm64_custom_animation_init(marioAnimEatRaw, marioAnimEatRaw_length);
    MARIO_ANIM_CUSTOM_VOMIT = sm64_custom_animation_init(marioAnimVomitRaw, marioAnimVomitRaw_length);
    MARIO_ANIM_CUSTOM_CRIB_SWITCH = sm64_custom_animation_init(marioAnimCribUseSwitchRaw, marioAnimCribUseSwitchRaw_length);
    MARIO_ANIM_CUSTOM_FACEPALM = sm64_custom_animation_init(marioAnimFacepalmRaw, marioAnimFacepalmRaw_length);
    MARIO_ANIM_CUSTOM_LAUGH01 = sm64_custom_animation_init(marioAnimLaugh01Raw, marioAnimLaugh01Raw_length);
    MARIO_ANIM_CUSTOM_DANCE_LOOP = sm64_custom_animation_init(marioAnimDanceLoopRaw, marioAnimDanceLoopRaw_length);
    MARIO_ANIM_CUSTOM_DANCE_BAD = sm64_custom_animation_init(marioAnimDanceBadRaw, marioAnimDanceBadRaw_length);
    MARIO_ANIM_CUSTOM_DANCE_GOOD = sm64_custom_animation_init(marioAnimDanceGoodRaw, marioAnimDanceGoodRaw_length);
    MARIO_ANIM_CUSTOM_VENDING_MACHINE = sm64_custom_animation_init(marioAnimVendingMachineRaw, marioAnimVendingMachineRaw_length);

    MARIO_ANIM_CUSTOM_GUNPOINT = sm64_custom_animation_init(marioAnimGunpointRaw, marioAnimGunpointRaw_length);
    MARIO_ANIM_CUSTOM_GUNPOINT_WALK_START = sm64_custom_animation_init(marioAnimGunpointWalkStartRaw, marioAnimGunpointWalkStartRaw_length);
    MARIO_ANIM_CUSTOM_GUNPOINT_TIPTOE = sm64_custom_animation_init(marioAnimGunpointTiptoeRaw, marioAnimGunpointTiptoeRaw_length);
    MARIO_ANIM_CUSTOM_GUNPOINT_WALK = sm64_custom_animation_init(marioAnimGunpointWalkRaw, marioAnimGunpointWalkRaw_length);
    MARIO_ANIM_CUSTOM_GUNPOINT_RUN = sm64_custom_animation_init(marioAnimGunpointRunRaw, marioAnimGunpointRunRaw_length);
    MARIO_ANIM_CUSTOM_GUNPOINT_SKID = sm64_custom_animation_init(marioAnimGunpointSkidRaw, marioAnimGunpointSkidRaw_length);
    MARIO_ANIM_CUSTOM_GUNPOINT_SKID_STOP = sm64_custom_animation_init(marioAnimGunpointSkidStopRaw, marioAnimGunpointSkidStopRaw_length);
    MARIO_ANIM_CUSTOM_GUNPOINT_SKID_TURN = sm64_custom_animation_init(marioAnimGunpointSkidTurnRaw, marioAnimGunpointSkidTurnRaw_length);

    MARIO_ANIM_CUSTOM_GUNSIDE_IDLE = sm64_custom_animation_init(marioAnimGunsideIdleRaw, marioAnimGunsideIdleRaw_length);
    MARIO_ANIM_CUSTOM_GUNSIDE_IDLE_ALT = sm64_custom_animation_init(marioAnimGunsideIdleAltRaw, marioAnimGunsideIdleAltRaw_length);
	MARIO_ANIM_CUSTOM_GUNSIDE_WALK_START = sm64_custom_animation_init(marioAnimGunsideWalkStartRaw, marioAnimGunsideWalkStartRaw_length);
	MARIO_ANIM_CUSTOM_GUNSIDE_TIPTOE = sm64_custom_animation_init(marioAnimGunsideTiptoeRaw, marioAnimGunsideTiptoeRaw_length);
	MARIO_ANIM_CUSTOM_GUNSIDE_WALK = sm64_custom_animation_init(marioAnimGunsideWalkRaw, marioAnimGunsideWalkRaw_length);
	MARIO_ANIM_CUSTOM_GUNSIDE_RUN = sm64_custom_animation_init(marioAnimGunsideRunRaw, marioAnimGunsideRunRaw_length);
	MARIO_ANIM_CUSTOM_GUNSIDE_SKID = sm64_custom_animation_init(marioAnimGunsideSkidStartRaw, marioAnimGunsideSkidStartRaw_length);
	MARIO_ANIM_CUSTOM_GUNSIDE_SKID_STOP = sm64_custom_animation_init(marioAnimGunsideSkidStopRaw, marioAnimGunsideSkidStopRaw_length);
	MARIO_ANIM_CUSTOM_GUNSIDE_SKID_TURN = sm64_custom_animation_init(marioAnimGunsideSkidTurnRaw, marioAnimGunsideSkidTurnRaw_length);

    MARIO_ANIM_CUSTOM_RPG_IDLE = sm64_custom_animation_init(marioAnimRpgIdleRaw, marioAnimRpgIdleRaw_length);
    MARIO_ANIM_CUSTOM_RPG_IDLE_ALT = sm64_custom_animation_init(marioAnimRpgIdleAltRaw, marioAnimRpgIdleAltRaw_length);
    MARIO_ANIM_CUSTOM_RPG_WALK_START = sm64_custom_animation_init(marioAnimRpgWalkStartRaw, marioAnimRpgWalkStartRaw_length);
	MARIO_ANIM_CUSTOM_RPG_TIPTOE = sm64_custom_animation_init(marioAnimRpgTiptoeRaw, marioAnimRpgTiptoeRaw_length);
	MARIO_ANIM_CUSTOM_RPG_WALK = sm64_custom_animation_init(marioAnimRpgWalkRaw, marioAnimRpgWalkRaw_length);
	MARIO_ANIM_CUSTOM_RPG_RUN = sm64_custom_animation_init(marioAnimRpgRunRaw, marioAnimRpgRunRaw_length);
	MARIO_ANIM_CUSTOM_RPG_SKID = sm64_custom_animation_init(marioAnimRpgSkidStartRaw, marioAnimRpgSkidStartRaw_length);
	MARIO_ANIM_CUSTOM_RPG_SKID_STOP = sm64_custom_animation_init(marioAnimRpgSkidStopRaw, marioAnimRpgSkidStopRaw_length);
	MARIO_ANIM_CUSTOM_RPG_SKID_TURN = sm64_custom_animation_init(marioAnimRpgSkidTurnRaw, marioAnimRpgSkidTurnRaw_length);

    MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE = sm64_custom_animation_init(marioAnimGunheavyIdleRaw, marioAnimGunheavyIdleRaw_length);
    MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE_ALT = sm64_custom_animation_init(marioAnimGunheavyIdleAltRaw, marioAnimGunheavyIdleAltRaw_length);
    MARIO_ANIM_CUSTOM_GUNHEAVY_WALK_START = sm64_custom_animation_init(marioAnimGunheavyWalkStartRaw, marioAnimGunheavyWalkStartRaw_length);
	MARIO_ANIM_CUSTOM_GUNHEAVY_TIPTOE = sm64_custom_animation_init(marioAnimGunheavyTiptoeRaw, marioAnimGunheavyTiptoeRaw_length);
	MARIO_ANIM_CUSTOM_GUNHEAVY_WALK = sm64_custom_animation_init(marioAnimGunheavyWalkRaw, marioAnimGunheavyWalkRaw_length);
	MARIO_ANIM_CUSTOM_GUNHEAVY_RUN = sm64_custom_animation_init(marioAnimGunheavyRunRaw, marioAnimGunheavyRunRaw_length);
	MARIO_ANIM_CUSTOM_GUNHEAVY_SKID = sm64_custom_animation_init(marioAnimGunheavySkidStartRaw, marioAnimGunheavySkidStartRaw_length);
	MARIO_ANIM_CUSTOM_GUNHEAVY_SKID_STOP = sm64_custom_animation_init(marioAnimGunheavySkidStopRaw, marioAnimGunheavySkidStopRaw_length);
	MARIO_ANIM_CUSTOM_GUNHEAVY_SKID_TURN = sm64_custom_animation_init(marioAnimGunheavySkidTurnRaw, marioAnimGunheavySkidTurnRaw_length);

    MARIO_ANIM_CUSTOM_RIFLE_AIM = sm64_custom_animation_init(marioAnimRifleAimRaw, marioAnimRifleAimRaw_length);
    MARIO_ANIM_CUSTOM_RIFLE_AIM_WALK = sm64_custom_animation_init(marioAnimRifleAimWalkRaw, marioAnimRifleAimWalkRaw_length);

    MARIO_ANIM_CUSTOM_GUNHEAVY_AIM = sm64_custom_animation_init(marioAnimGunheavyAimRaw, marioAnimGunheavyAimRaw_length);
    MARIO_ANIM_CUSTOM_GUNHEAVY_AIM_WALK = sm64_custom_animation_init(marioAnimGunheavyAimWalkRaw, marioAnimGunheavyAimWalkRaw_length);

    MARIO_ANIM_CUSTOM_GUNLIGHT_AIM = sm64_custom_animation_init(marioAnimGunlightAimRaw, marioAnimGunlightAimRaw_length);
    MARIO_ANIM_CUSTOM_GUNLIGHT_AIM_WALK = sm64_custom_animation_init(marioAnimGunlightAimWalkRaw, marioAnimGunlightAimWalkRaw_length);


    gunAnimOverrideTable = {
        {MARIO_ANIM_IDLE_HEAD_CENTER,   MARIO_ANIM_CUSTOM_GUNPOINT},
        {MARIO_ANIM_IDLE_HEAD_LEFT,     MARIO_ANIM_CUSTOM_GUNPOINT},
        {MARIO_ANIM_IDLE_HEAD_RIGHT,    MARIO_ANIM_CUSTOM_GUNPOINT},
        {MARIO_ANIM_FIRST_PERSON,       MARIO_ANIM_CUSTOM_GUNPOINT},
        {MARIO_ANIM_CROUCHING,          MARIO_ANIM_CUSTOM_GUNPOINT},
        {MARIO_ANIM_STOP_CROUCHING,     MARIO_ANIM_CUSTOM_GUNPOINT},
        {MARIO_ANIM_WALK_PANTING,       MARIO_ANIM_CUSTOM_GUNPOINT},
        {MARIO_ANIM_START_TIPTOE,       MARIO_ANIM_CUSTOM_GUNPOINT_WALK_START},
        {MARIO_ANIM_TIPTOE,             MARIO_ANIM_CUSTOM_GUNPOINT_TIPTOE},
        {MARIO_ANIM_WALKING,            MARIO_ANIM_CUSTOM_GUNPOINT_WALK},
        {MARIO_ANIM_RUNNING,            MARIO_ANIM_CUSTOM_GUNPOINT_RUN},
        {MARIO_ANIM_SKID_ON_GROUND,     MARIO_ANIM_CUSTOM_GUNPOINT_SKID},
        {MARIO_ANIM_STOP_SKID,          MARIO_ANIM_CUSTOM_GUNPOINT_SKID_STOP},
        {MARIO_ANIM_TURNING_PART1,      MARIO_ANIM_CUSTOM_GUNPOINT_SKID},
        {MARIO_ANIM_TURNING_PART2,      MARIO_ANIM_CUSTOM_GUNPOINT_SKID_TURN},
    };

    gunSideAnimOverrideTable = {
		{MARIO_ANIM_IDLE_HEAD_LEFT,     MARIO_ANIM_CUSTOM_GUNSIDE_IDLE},
		{MARIO_ANIM_IDLE_HEAD_RIGHT,    MARIO_ANIM_CUSTOM_GUNSIDE_IDLE},
		{MARIO_ANIM_IDLE_HEAD_CENTER,   MARIO_ANIM_CUSTOM_GUNSIDE_IDLE},
		{MARIO_ANIM_FIRST_PERSON,       MARIO_ANIM_CUSTOM_GUNSIDE_IDLE_ALT},
		{MARIO_ANIM_CROUCHING,          MARIO_ANIM_CUSTOM_GUNSIDE_IDLE},
        {MARIO_ANIM_STOP_CROUCHING,     MARIO_ANIM_CUSTOM_GUNSIDE_IDLE},
        {MARIO_ANIM_WALK_PANTING,       MARIO_ANIM_CUSTOM_GUNSIDE_IDLE},
        {MARIO_ANIM_START_TIPTOE,       MARIO_ANIM_CUSTOM_GUNSIDE_WALK_START},
        {MARIO_ANIM_TIPTOE,             MARIO_ANIM_CUSTOM_GUNSIDE_TIPTOE},
        {MARIO_ANIM_WALKING,            MARIO_ANIM_CUSTOM_GUNSIDE_WALK},
        {MARIO_ANIM_RUNNING,            MARIO_ANIM_CUSTOM_GUNSIDE_RUN},
        {MARIO_ANIM_SKID_ON_GROUND,     MARIO_ANIM_CUSTOM_GUNSIDE_SKID},
        {MARIO_ANIM_STOP_SKID,          MARIO_ANIM_CUSTOM_GUNSIDE_SKID_STOP},
        {MARIO_ANIM_TURNING_PART1,      MARIO_ANIM_CUSTOM_GUNSIDE_SKID},
        {MARIO_ANIM_TURNING_PART2,      MARIO_ANIM_CUSTOM_GUNSIDE_SKID_TURN},
    };

    gunShoulderAnimOverrideTable = {
		{MARIO_ANIM_IDLE_HEAD_LEFT,     MARIO_ANIM_CUSTOM_RPG_IDLE},
		{MARIO_ANIM_IDLE_HEAD_RIGHT,    MARIO_ANIM_CUSTOM_RPG_IDLE},
		{MARIO_ANIM_IDLE_HEAD_CENTER,   MARIO_ANIM_CUSTOM_RPG_IDLE},
		{MARIO_ANIM_FIRST_PERSON,       MARIO_ANIM_CUSTOM_RPG_IDLE_ALT},
		{MARIO_ANIM_CROUCHING,          MARIO_ANIM_CUSTOM_RPG_IDLE},
        {MARIO_ANIM_STOP_CROUCHING,     MARIO_ANIM_CUSTOM_RPG_IDLE},
        {MARIO_ANIM_WALK_PANTING,       MARIO_ANIM_CUSTOM_RPG_IDLE},
        {MARIO_ANIM_START_TIPTOE,       MARIO_ANIM_CUSTOM_RPG_WALK_START},
        {MARIO_ANIM_TIPTOE,             MARIO_ANIM_CUSTOM_RPG_TIPTOE},
        {MARIO_ANIM_WALKING,            MARIO_ANIM_CUSTOM_RPG_WALK},
        {MARIO_ANIM_RUNNING,            MARIO_ANIM_CUSTOM_RPG_RUN},
        {MARIO_ANIM_SKID_ON_GROUND,     MARIO_ANIM_CUSTOM_RPG_SKID},
        {MARIO_ANIM_STOP_SKID,          MARIO_ANIM_CUSTOM_RPG_SKID_STOP},
        {MARIO_ANIM_TURNING_PART1,      MARIO_ANIM_CUSTOM_RPG_SKID},
        {MARIO_ANIM_TURNING_PART2,      MARIO_ANIM_CUSTOM_RPG_SKID_TURN},
    };

    gunHeavyAnimOverrideTable = {
		{MARIO_ANIM_IDLE_HEAD_LEFT,     MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE},
		{MARIO_ANIM_IDLE_HEAD_RIGHT,    MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE},
		{MARIO_ANIM_IDLE_HEAD_CENTER,   MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE},
		{MARIO_ANIM_FIRST_PERSON,       MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE_ALT},
		{MARIO_ANIM_CROUCHING,          MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE},
        {MARIO_ANIM_STOP_CROUCHING,     MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE},
        {MARIO_ANIM_WALK_PANTING,       MARIO_ANIM_CUSTOM_GUNHEAVY_IDLE},
        {MARIO_ANIM_START_TIPTOE,       MARIO_ANIM_CUSTOM_GUNHEAVY_WALK_START},
        {MARIO_ANIM_TIPTOE,             MARIO_ANIM_CUSTOM_GUNHEAVY_TIPTOE},
        {MARIO_ANIM_WALKING,            MARIO_ANIM_CUSTOM_GUNHEAVY_WALK},
        {MARIO_ANIM_RUNNING,            MARIO_ANIM_CUSTOM_GUNHEAVY_RUN},
        {MARIO_ANIM_SKID_ON_GROUND,     MARIO_ANIM_CUSTOM_GUNHEAVY_SKID},
        {MARIO_ANIM_STOP_SKID,          MARIO_ANIM_CUSTOM_GUNHEAVY_SKID_STOP},
        {MARIO_ANIM_TURNING_PART1,      MARIO_ANIM_CUSTOM_GUNHEAVY_SKID},
        {MARIO_ANIM_TURNING_PART2,      MARIO_ANIM_CUSTOM_GUNHEAVY_SKID_TURN},
    };
}
