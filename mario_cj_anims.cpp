#include "mario_cj_anims.h"

extern "C" {
    #include <decomp/include/sm64shared.h>
}

#include "mario.h"
#include "mario_custom_anims.h"

extern std::unordered_map<std::string, std::function<void(const int&)> > cjAnimKeys;

bool animKeyExists(CTaskSimpleRunNamedAnim* task)
{
    return cjAnimKeys.count(task->m_animGroupName) || cjAnimKeys.count(task->m_animName);
}

bool animTaskExists(CPlayerPed* ped)
{
    CTaskSimpleRunNamedAnim* task = static_cast<CTaskSimpleRunNamedAnim*>(ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_NAMED_ANIM));
    return task && animKeyExists(task);
}

void runAnimKey(CTaskSimpleRunNamedAnim* task, const int& marioId)
{
    if (cjAnimKeys.count(task->m_animName))
        cjAnimKeys[task->m_animName](marioId);
    else if (cjAnimKeys.count(task->m_animGroupName))
        cjAnimKeys[task->m_animGroupName](marioId);
}


/// all implemented animation keys starting below

void animEatVomitP(const int& marioId)
{
    // vomit
    if (marioState.action != ACT_CUSTOM_ANIM_TO_ACTION)
    {
        sm64_set_mario_action(marioId, ACT_VOMIT);
        sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_VOMIT);
    }
}

void animGroupFood(const int& marioId)
{
    // eating food
    if (marioState.action != ACT_CUSTOM_ANIM_TO_ACTION)
    {
        sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
        sm64_set_mario_action_arg2(marioId, ACT_IDLE);
        sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_EAT);
    }
}

void animCloIn(const int& marioId)
{
    // entering dresser
}

void animCloOut(const int& marioId)
{
    // leaving dresser
}

void animCloPoseIn(const int& marioId)
{
    // leaving dresser to look and pose in the mirror
}

void animCloPoseOut(const int& marioId)
{
    // go back in the dresser after posing
}

void animCloBuy(const int& marioId)
{
    // confirm clothes purchase
}

void animBrbSitIn(const int& marioId)
{
    // sit in barber chair
}

void animBrbSitLoop(const int& marioId)
{
    // idle in barber chair
}

void animBrbSitOut(const int& marioId)
{
    // get off barber chair
}

void animTatSitInP(const int& marioId)
{
    // sit in tattoo chair
}

void animTatSitLoopP(const int& marioId)
{
    // idle in tattoo chair
}

void animTatSitOutP(const int& marioId)
{
    // get off tattoo chair
}

void animGymTreadGetOn(const int& marioId)
{
    // get on gym treadmill
}

void animGymTreadGetOff(const int& marioId)
{
    // get off gym treadmill
}

void animGymTreadWalk(const int& marioId)
{
    // walk on gym treadmill
}

void animGymTreadJog(const int& marioId)
{
    // jog on gym treadmill
}

void animGymTreadSprint(const int& marioId)
{
    // sprint on gym treadmill
}

void animGymTreadFallOff(const int& marioId)
{
    // fall off the treadmill
}

void animGymBpGetOn(const int& marioId)
{
    // get on benchpress
}

void animGymBpGetOff(const int& marioId)
{
    // get off benchpress
}

void animGymBpUp(const int& marioId)
{
    // raising benchpress (alternating keys)
}

void animGymBpDown(const int& marioId)
{
    // lowering benchpress after alternating keys
}

void animGymFreePickup(const int& marioId)
{
    // pick up freeweights
}

void animGymFreePutdown(const int& marioId)
{
    // put down freeweights
}

void animGymFreeUp(const int& marioId)
{
    // raising freeweights (alternating keys)
}

void animGymFreeDown(const int& marioId)
{
    // lowering freeweights after alternating keys
}

void animGymBikeStill(const int& marioId)
{
    // idle on gym bike
}

void animGymBikeSlow(const int& marioId)
{
    // gym bike slow speed
}

void animGymBikeFast(const int& marioId)
{
    // gym bike fast speed
}

void animJustBusiness(const int& marioId)
{
    // Big Smoke talking to CJ before entering the building in Just Business
}

std::unordered_map<std::string, std::function<void(const int&)> > cjAnimKeys =
{
    {"EAT_VOMIT_P",         animEatVomitP},
    {"FOOD",                animGroupFood},

    {"CLO_IN",              animCloIn},
    {"CLO_OUT",             animCloOut},
    {"CLO_POSE_IN",         animCloPoseIn},
    {"CLO_POSE_OUT",        animCloPoseOut},
    {"CLO_BUY",             animCloBuy},

    {"BRB_SIT_IN",          animBrbSitIn},
    {"BRB_SIT_LOOP",        animBrbSitLoop},
    {"BRB_SIT_OUT",         animBrbSitOut},

    {"TAT_SIT_IN_P",        animTatSitInP},
    {"TAT_SIT_LOOP_P",      animTatSitLoopP},
    {"TAT_SIT_OUT_P",       animTatSitOutP},

    {"GYM_TREAD_GETON",     animGymTreadGetOn},
    {"GYM_TREAD_GETOFF",    animGymTreadGetOff},
    {"GYM_WALK_FALLOFF",    animGymTreadGetOff},
    {"GYM_TREAD_WALK",      animGymTreadWalk},
    {"GYM_TREAD_JOG",       animGymTreadJog},
    {"GYM_TREAD_SPRINT",    animGymTreadSprint},
    {"GYM_JOG_FALLOFF",     animGymTreadFallOff},
    {"GYM_TREAD_FALLOFF",   animGymTreadFallOff},

    {"GYM_BP_GETON",        animGymBpGetOn},
    {"GYM_BP_GETOFF",       animGymBpGetOff},
    {"GYM_BP_UP_SMOOTH",    animGymBpUp},
    {"GYM_BP_UP_A",         animGymBpUp},
    {"GYM_BP_DOWN",         animGymBpDown},

    {"GYM_FREE_PICKUP",     animGymFreePickup},
    {"GYM_FREE_PUTDOWN",    animGymFreePutdown},
    {"GYM_FREE_UP_SMOOTH",  animGymFreeUp},
    {"GYM_FREE_A",          animGymFreeUp},
    {"GYM_FREE_DOWN",       animGymFreeDown},

    {"GYM_BIKE_GETON",      animGymBikeStill},
    {"GYM_BIKE_STILL",      animGymBikeStill},
    {"GYM_BIKE_SLOW",       animGymBikeSlow},
    {"GYM_BIKE_FAST",       animGymBikeFast},

    // missions
    {"JST_BUISNESS",        animJustBusiness},
};
