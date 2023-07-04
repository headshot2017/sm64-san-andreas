#include "mario_cj_anims.h"

#include <functional>
#include <utility>

extern "C" {
    #include <decomp/include/sm64shared.h>
}

#include "mario.h"
#include "mario_custom_anims.h"

struct ConvertedAnim
{
    std::function<void(const int&)> callback;
    bool repeat;
};
extern std::unordered_map<std::string, ConvertedAnim> cjAnimKeys;
std::string lastAnim;

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
    std::string key = (cjAnimKeys.count(task->m_animName)) ? task->m_animName :
                      (cjAnimKeys.count(task->m_animGroupName)) ? task->m_animGroupName :
                      "";

    if (key.empty() || (!cjAnimKeys[key].repeat && lastAnim == key)) return;
    lastAnim = key;

    cjAnimKeys[key].callback(marioId);
}


/// all implemented animation keys starting below

void animEatVomitP(const int& marioId)
{
    // vomit
    sm64_set_mario_action(marioId, ACT_VOMIT);
    sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_VOMIT);
}

void animGroupFood(const int& marioId)
{
    // eating food
    sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
    sm64_set_mario_action_arg2(marioId, ACT_IDLE);
    sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_EAT);
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

void animCribUseSwitch(const int& marioId)
{
    // seen during the OG Loc mission, when ringing the bell at Freddy's house
    sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
    sm64_set_mario_action_arg2(marioId, ACT_IDLE);
    sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_CRIB_SWITCH);
}

void animPlayerShakeHead(const int& marioId)
{
    // seen during the OG Loc mission, when he's talking to Freddy
    sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
    sm64_set_mario_action_arg2(marioId, ACT_IDLE);
    sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_FACEPALM);
}

void animLaugh01(const int& marioId)
{
    // CJ's honest reaction to that information
    sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
    sm64_set_mario_action_arg2(marioId, ACT_IDLE);
    sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_LAUGH01);
}

std::unordered_map<std::string, ConvertedAnim> cjAnimKeys =
{
    {"EAT_VOMIT_P",         {animEatVomitP, false}},
    {"FOOD",                {animGroupFood, false}},

    {"CLO_IN",              {animCloIn, false}},
    {"CLO_OUT",             {animCloOut, false}},
    {"CLO_POSE_IN",         {animCloPoseIn, false}},
    {"CLO_POSE_OUT",        {animCloPoseOut, false}},
    {"CLO_BUY",             {animCloBuy, false}},

    {"BRB_SIT_IN",          {animBrbSitIn, false}},
    {"BRB_SIT_LOOP",        {animBrbSitLoop, false}},
    {"BRB_SIT_OUT",         {animBrbSitOut, false}},

    {"TAT_SIT_IN_P",        {animTatSitInP, false}},
    {"TAT_SIT_LOOP_P",      {animTatSitLoopP, false}},
    {"TAT_SIT_OUT_P",       {animTatSitOutP, false}},

    {"GYM_TREAD_GETON",     {animGymTreadGetOn, false}},
    {"GYM_TREAD_GETOFF",    {animGymTreadGetOff, false}},
    {"GYM_WALK_FALLOFF",    {animGymTreadGetOff, false}},
    {"GYM_TREAD_WALK",      {animGymTreadWalk, true}},
    {"GYM_TREAD_JOG",       {animGymTreadJog, true}},
    {"GYM_TREAD_SPRINT",    {animGymTreadSprint, true}},
    {"GYM_JOG_FALLOFF",     {animGymTreadFallOff, false}},
    {"GYM_TREAD_FALLOFF",   {animGymTreadFallOff, false}},

    {"GYM_BP_GETON",        {animGymBpGetOn, false}},
    {"GYM_BP_GETOFF",       {animGymBpGetOff, false}},
    {"GYM_BP_UP_SMOOTH",    {animGymBpUp, true}},
    {"GYM_BP_UP_A",         {animGymBpUp, true}},
    {"GYM_BP_DOWN",         {animGymBpDown, false}},

    {"GYM_FREE_PICKUP",     {animGymFreePickup, false}},
    {"GYM_FREE_PUTDOWN",    {animGymFreePutdown, false}},
    {"GYM_FREE_UP_SMOOTH",  {animGymFreeUp, true}},
    {"GYM_FREE_A",          {animGymFreeUp, true}},
    {"GYM_FREE_DOWN",       {animGymFreeDown, false}},

    {"GYM_BIKE_GETON",      {animGymBikeStill, true}},
    {"GYM_BIKE_STILL",      {animGymBikeStill, true}},
    {"GYM_BIKE_SLOW",       {animGymBikeSlow, true}},
    {"GYM_BIKE_FAST",       {animGymBikeFast, true}},

    // missions
    {"JST_BUISNESS",        {animJustBusiness, false}},

    // misc
    {"CRIB_USE_SWITCH",     {animCribUseSwitch, false}},
    {"PLYR_SHKHEAD",        {animPlayerShakeHead, false}},
    {"LAUGH_01",            {animLaugh01, false}},
};