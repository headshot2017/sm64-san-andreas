#include "mario_ped_tasks.h"

#include <stdio.h>

#include "plugin.h"
#include "CHud.h"
#include "CTaskComplexEnterCar.h"
#include "CTaskComplexLeaveCar.h"
#include "CTaskComplexGoPickUpEntity.h"
#include "CTaskSimpleHoldEntity2.h"
#include "CTaskComplexUseSequence.h"
#include "CTaskComplexSequence2.h"
#include "CTaskSimpleRunNamedAnim.h"
#include "CModelInfo.h"
extern "C" {
    #include <decomp/include/sm64shared.h>
}

#include "mario.h"
#include "mario_custom_anims.h"

void moveEntityToMarioHands(CTaskSimpleHoldEntity2* task, float pickupLerp=1.f)
{
    if (pickupLerp < 0) pickupLerp = 0;
    task->m_vecPosition.z = -0.5f * pickupLerp;
}

void marioPedTasks(CPlayerPed* ped, const int& marioId)
{
    CTask* baseTask;

    // enter vehicle animation handling
    if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER)))
    {
        CTaskComplexEnterCar* task = static_cast<CTaskComplexEnterCar*>(baseTask);

        float targetAngle = 0;
        if (task->m_nTargetDoor == 10) // front left door
            targetAngle = task->m_pTargetVehicle->GetHeading() + M_PI_2;
        else if (task->m_nTargetDoor == TARGET_DOOR_FRONT_RIGHT)
            targetAngle = task->m_pTargetVehicle->GetHeading() - M_PI_2;

        if (targetAngle < -M_PI) targetAngle += M_PI*2;
        if (targetAngle > M_PI) targetAngle -= M_PI*2;

        // create actionArg with bitflags to tell libsm64 how to play the anims
        uint32_t arg = (task->m_nTargetDoor == 10 || task->m_nTargetDoor == TARGET_DOOR_REAR_LEFT) ? SM64_VEHICLE_DOOR_LEFT : SM64_VEHICLE_DOOR_RIGHT;
        if (task->m_pTargetVehicle->m_nVehicleClass == VEHICLE_BIKE || task->m_pTargetVehicle->m_nVehicleClass == VEHICLE_BMX)
            arg |= SM64_VEHICLE_BIKE;


        // calculate car seat target position
        CVehicleModelInfo* modelInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(task->m_pTargetVehicle->m_nModelIndex);
        CVector seatPos = modelInfo->m_pVehicleStruct->m_avDummyPos[4]; // DUMMY_SEAT_FRONT
        seatPos.z -= 0.375f;
        seatPos.y += 0.375f;

        // by default, seatPos.x is passenger seat. if entering from left side, invert X pos to get driver seat
        if (arg & SM64_VEHICLE_DOOR_LEFT && !(arg & SM64_VEHICLE_BIKE))
            seatPos.x *= -1;

        CVector targetPos = *(task->m_pTargetVehicle->m_matrix) * seatPos;

        seatPos.x += 1.1f * sign(seatPos.x);
        seatPos.y -= 0.35f;

        CVector startPos = task->m_vTargetDoorPos - CVector(0,0,1);
        if (!(arg & SM64_VEHICLE_BIKE))
        {
            startPos = *(task->m_pTargetVehicle->m_matrix) * seatPos;
            startPos.z = task->m_vTargetDoorPos.z-1;
        }


        // set the action!
        if (task->m_pSubTask)
        {
            CTask* sub = task->m_pSubTask;

            // calling sub->GetID() returns some garbage number, so i copied this from plugin_sa CTask.cpp
            eTaskType taskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(sub, 4))(sub);

            if (taskID == TASK_SIMPLE_BIKE_PICK_UP)
            {
                if (marioState.action != ACT_BIKE_PICK_UP)
                    sm64_set_mario_action_arg(marioId, ACT_BIKE_PICK_UP, arg);
                marioSetPos(task->m_vTargetDoorPos - CVector(0,0,1), false);
                sm64_set_mario_faceangle(marioId, targetAngle);
            }
            else if (taskID == TASK_SIMPLE_CAR_OPEN_LOCKED_DOOR_FROM_OUTSIDE)
            {
                marioSetPos(startPos, false);
                sm64_set_mario_faceangle(marioId, targetAngle);

                if (marioState.action != ACT_CUSTOM_ANIM_TO_ACTION)
                {
                    sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
                    sm64_set_mario_action_arg2(marioId, ACT_IDLE);
                    sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_CAR_LOCKED);
                }
            }
            else if (taskID == TASK_SIMPLE_CAR_OPEN_DOOR_FROM_OUTSIDE)
            {
                if (marioState.action != ACT_ENTER_VEHICLE_OPENDOOR)
                    sm64_set_mario_action_arg(marioId, ACT_ENTER_VEHICLE_OPENDOOR, arg);
                sm64_set_mario_faceangle(marioId, targetAngle);

                CVector doorPos = task->m_vTargetDoorPos - CVector(0,0,1);
                if (marioState.actionTimer)
                {
                    uint32_t ticks = marioState.actionTimer;
                    if (ticks > 10) ticks = 10;
                    doorPos.x = lerp(doorPos.x, startPos.x, ticks/10.f);
                    doorPos.y = lerp(doorPos.y, startPos.y, ticks/10.f);
                    doorPos.z = lerp(doorPos.z, startPos.z, ticks/10.f);
                }
                marioSetPos(doorPos, false);
            }
            else if (marioState.action != ACT_ENTER_VEHICLE_DRAGPED && (taskID == TASK_SIMPLE_CAR_SLOW_DRAG_PED_OUT))
            {
                marioSetPos(startPos, false);
                sm64_set_mario_action_arg(marioId, ACT_ENTER_VEHICLE_DRAGPED, arg);
            }
            else if (taskID == TASK_SIMPLE_CAR_GET_IN)
            {
                if (marioState.action != ACT_ENTER_VEHICLE_JUMPINSIDE)
                {
                    sm64_set_mario_action_arg(marioId, ACT_ENTER_VEHICLE_JUMPINSIDE, arg);
                    marioState.actionTimer = 0;
                }

                if (marioState.actionTimer < 7)
                {
                    // jumping inside
                    CVector newPos(
                        lerp(startPos.x, targetPos.x, marioState.actionTimer/7.f),
                        lerp(startPos.y, targetPos.y, marioState.actionTimer/7.f),
                        lerp(startPos.z, targetPos.z, marioState.actionTimer/7.f)
                    );

                    marioSetPos(newPos, false);
                    sm64_set_mario_faceangle(marioId, targetAngle);
                }
                else
                {
                    // now inside
                    marioSetPos(targetPos, false);

                    float watchTheDamnRoadAngle = targetAngle + (arg&SM64_VEHICLE_DOOR_LEFT ? M_PI_2 : -M_PI_2);
                    if (watchTheDamnRoadAngle < -M_PI) watchTheDamnRoadAngle += M_PI*2;
                    if (watchTheDamnRoadAngle > M_PI) watchTheDamnRoadAngle -= M_PI*2;

                    sm64_set_mario_faceangle(marioId, watchTheDamnRoadAngle);
                }
            }
            else if (taskID == TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_INSIDE)
            {
                if (marioState.action != ACT_CUSTOM_ANIM)
                {
                    sm64_set_mario_action(marioId, ACT_CUSTOM_ANIM);
                    sm64_set_mario_animation(marioId, (arg&SM64_VEHICLE_DOOR_LEFT) ? MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_LEFT : MARIO_ANIM_CUSTOM_CLOSE_CAR_DOOR_RIGHT);
                }

                marioSetPos(targetPos, false);

                float watchTheDamnRoadAngle = targetAngle + (arg&SM64_VEHICLE_DOOR_LEFT ? M_PI_2 : -M_PI_2);
                if (watchTheDamnRoadAngle < -M_PI) watchTheDamnRoadAngle += M_PI*2;
                if (watchTheDamnRoadAngle > M_PI) watchTheDamnRoadAngle -= M_PI*2;

                sm64_set_mario_faceangle(marioId, watchTheDamnRoadAngle);
            }
            else if (taskID == TASK_SIMPLE_CAR_SHUFFLE)
            {
                // switch from passenger to driver seat
                CVector driverSeatPos = modelInfo->m_pVehicleStruct->m_avDummyPos[4]; // DUMMY_SEAT_FRONT
                driverSeatPos.z -= 0.375f;
                driverSeatPos.y += 0.375f;
                driverSeatPos.x *= -1;

                CVector driverTargetPos = *(task->m_pTargetVehicle->m_matrix) * driverSeatPos;

                if (marioState.action != ACT_ENTER_VEHICLE_JUMPINSIDE)
                {
                    sm64_set_mario_action_arg(marioId, ACT_ENTER_VEHICLE_JUMPINSIDE, arg);
                    marioState.actionTimer = 0;
                }

                if (marioState.actionTimer < 7)
                {
                    // jumping to different seat
                    CVector newPos(
                        lerp(targetPos.x, driverTargetPos.x, marioState.actionTimer/7.f),
                        lerp(targetPos.y, driverTargetPos.y, marioState.actionTimer/7.f),
                        lerp(targetPos.z, driverTargetPos.z, marioState.actionTimer/7.f)
                    );

                    marioSetPos(newPos, false);
                    sm64_set_mario_faceangle(marioId, targetAngle);
                }
                else
                {
                    // now in driver seat
                    marioSetPos(driverTargetPos, false);

                    float watchTheDamnRoadAngle = targetAngle + (arg&SM64_VEHICLE_DOOR_LEFT ? M_PI_2 : -M_PI_2);
                    if (watchTheDamnRoadAngle < -M_PI) watchTheDamnRoadAngle += M_PI*2;
                    if (watchTheDamnRoadAngle > M_PI) watchTheDamnRoadAngle -= M_PI*2;

                    sm64_set_mario_faceangle(marioId, watchTheDamnRoadAngle);
                }
            }
        }
    }
    else if (!ped->m_nPedFlags.bInVehicle && marioState.action >= ACT_ENTER_VEHICLE_OPENDOOR && marioState.action <= ACT_BIKE_PICK_UP)
        sm64_set_mario_action(marioId, ACT_FREEFALL);

    // exit vehicle animation handling
    static int jumpedOut = 0;
    if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR)))
    {
        CTaskComplexLeaveCar* task = static_cast<CTaskComplexLeaveCar*>(baseTask);

        float targetAngle = 0;
        if (task->m_nTargetDoor == 10) // front left door
            targetAngle = task->m_pTargetVehicle->GetHeading() - M_PI_2;
        else if (task->m_nTargetDoor == TARGET_DOOR_FRONT_RIGHT)
            targetAngle = task->m_pTargetVehicle->GetHeading() + M_PI_2;

        if (targetAngle < -M_PI) targetAngle += M_PI*2;
        if (targetAngle > M_PI) targetAngle -= M_PI*2;

        // create actionArg with bitflags to tell libsm64 how to play the anims
        uint32_t arg = (task->m_nTargetDoor == 10 || task->m_nTargetDoor == TARGET_DOOR_REAR_LEFT) ? SM64_VEHICLE_DOOR_LEFT : SM64_VEHICLE_DOOR_RIGHT;
        if (task->m_pTargetVehicle->m_nVehicleClass == VEHICLE_BIKE || task->m_pTargetVehicle->m_nVehicleClass == VEHICLE_BMX)
            arg |= SM64_VEHICLE_BIKE;

        // calculate car seat target position
        CVehicleModelInfo* modelInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(task->m_pTargetVehicle->m_nModelIndex);
        CVector seatPos = modelInfo->m_pVehicleStruct->m_avDummyPos[4]; // DUMMY_SEAT_FRONT
        seatPos.z -= 0.375f;
        seatPos.y += 0.375f;

        // by default, seatPos.x is passenger seat. if entering from left side, invert X pos to get driver seat
        if (arg & SM64_VEHICLE_BIKE)
            seatPos.x = 0.005f;
        if (arg & SM64_VEHICLE_DOOR_LEFT)
            seatPos.x *= -1;

        CVector startPos = *(task->m_pTargetVehicle->m_matrix) * seatPos;

        seatPos.x += 1.1f * sign(seatPos.x);
        seatPos.y -= 0.35f;
        CVector targetPos = *(task->m_pTargetVehicle->m_matrix) * seatPos;
        targetPos.z = sm64_surface_find_floor_height(targetPos.x/MARIO_SCALE, targetPos.z/MARIO_SCALE, -targetPos.y/MARIO_SCALE) * MARIO_SCALE;


        // set the action!
        if (task->m_pSubTask)
        {
            CTask* sub = task->m_pSubTask;

            // calling sub->GetID() returns some garbage number, so i copied this from plugin_sa CTask.cpp
            eTaskType taskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(sub, 4))(sub);

            switch(taskID)
            {
                case TASK_SIMPLE_CAR_GET_OUT:
                    if (marioState.action != ACT_LEAVE_VEHICLE_JUMPOUT)
                        sm64_set_mario_action_arg(marioId, ACT_LEAVE_VEHICLE_JUMPOUT, arg);

                    if (marioState.actionTimer < 7)
                    {
                        // jumping outside
                        CVector newPos(
                            lerp(startPos.x, targetPos.x, marioState.actionTimer/7.f),
                            lerp(startPos.y, targetPos.y, marioState.actionTimer/7.f),
                            lerp(startPos.z, targetPos.z, marioState.actionTimer/7.f)
                        );

                        marioSetPos(newPos, false);
                    }
                    else
                    {
                        // now outside
                        marioSetPos(targetPos, false);
                    }
                    sm64_set_mario_faceangle(marioId, targetAngle);
                    break;

                case TASK_SIMPLE_CAR_CLOSE_DOOR_FROM_OUTSIDE:
                    {
                        if (marioState.action != ACT_LEAVE_VEHICLE_CLOSEDOOR)
                            sm64_set_mario_action_arg(marioId, ACT_LEAVE_VEHICLE_CLOSEDOOR, arg);
                        marioSetPos(targetPos, false);

                        float faceDoorAngle = targetAngle + M_PI;
                        if (faceDoorAngle < -M_PI) faceDoorAngle += M_PI*2;
                        if (faceDoorAngle > M_PI) faceDoorAngle -= M_PI*2;

                        sm64_set_mario_faceangle(marioId, faceDoorAngle);
                    }
                    break;

                case TASK_SIMPLE_CAR_JUMP_OUT:
                    if (jumpedOut < 5)
                    {
                        sm64_set_mario_forward_velocity(marioId, 70);
                        sm64_set_mario_faceangle(marioId, targetAngle);
                        CVector targetPos = *(task->m_pTargetVehicle->m_matrix) * seatPos;
                        marioSetPos(CVector(targetPos.x, targetPos.y, startPos.z), false);
                    }

                    if (marioState.action != ACT_DIVE && !jumpedOut)
                    {
                        sm64_set_mario_action(marioId, ACT_DIVE);
                    }

                    jumpedOut++;
                    break;
            }
        }
    }
    else
    {
        jumpedOut = 0;
        if (marioState.action >= ACT_LEAVE_VEHICLE_JUMPOUT && marioState.action <= ACT_LEAVE_VEHICLE_CLOSEDOOR)
            sm64_set_mario_action(marioId, ACT_IDLE);
    }

    static bool _fallen = false;
    if (ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT))
    {
        // get dragged out of vehicle - animation handling
        uint32_t arg = (ped->m_pVehicle->IsDriver(ped)) ? SM64_VEHICLE_DOOR_LEFT : SM64_VEHICLE_DOOR_RIGHT;
        if (ped->m_pVehicle->m_nVehicleClass == VEHICLE_BIKE || ped->m_pVehicle->m_nVehicleClass == VEHICLE_BMX)
            arg |= SM64_VEHICLE_BIKE;

        // calculate car seat target position
        CVehicleModelInfo* modelInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(ped->m_pVehicle->m_nModelIndex);
        CVector seatPos = modelInfo->m_pVehicleStruct->m_avDummyPos[4]; // DUMMY_SEAT_FRONT
        seatPos.z -= 0.375f;
        seatPos.y += 0.375f;

        // by default, seatPos.x is passenger seat. if entering from left side, invert X pos to get driver seat
        if (arg & SM64_VEHICLE_BIKE)
            seatPos.x = 0.005f;
        if (arg & SM64_VEHICLE_DOOR_LEFT)
            seatPos.x *= -1;

        CVector startPos = *(ped->m_pVehicle->m_matrix) * seatPos;

        seatPos.x += 1.1f * sign(seatPos.x);
        seatPos.y -= 0.35f;
        CVector targetPos = *(ped->m_pVehicle->m_matrix) * seatPos;

        if (!_fallen && marioState.action != ACT_VEHICLE_JACKED)
        {
            _fallen = true;
            marioSetPos(startPos, false);
            sm64_set_mario_action_arg(marioId, ACT_VEHICLE_JACKED, arg);
        }

        if (marioState.actionState == 2)
        {
            CVector newPos(
                lerp(startPos.x, targetPos.x, marioState.actionTimer/8.f),
                lerp(startPos.y, targetPos.y, marioState.actionTimer/8.f),
                lerp(startPos.z, targetPos.z, marioState.actionTimer/8.f)
            );

            marioSetPos(newPos, false);
        }
    }
    else if (ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_FALL_AND_GET_UP))
    {
        if (!_fallen)
        {
            float _angle1 = (ped->m_pVehicle && ped->m_pVehicle->m_vecMoveSpeed.Magnitude2D()) ? atan2(ped->m_pVehicle->m_vecMoveSpeed.y, ped->m_pVehicle->m_vecMoveSpeed.x) : -1;
            float _angle2 = (ped->m_pVehicle) ? ped->m_pVehicle->GetHeading()+M_PI_2 : 0;
            if (_angle2 < -M_PI) _angle2 += M_PI*2;
            if (_angle2 > M_PI) _angle2 -= M_PI*2;
            int angle1 = (int)_angle1;
            int angle2 = (int)_angle2;

            _fallen = true;

            if (angle1 != angle2)
                sm64_set_mario_action_arg(marioId, ACT_HARD_BACKWARD_AIR_KB, 1);
            else
                sm64_set_mario_action_arg(marioId, ACT_HARD_FORWARD_AIR_KB, 1);
        }
    }
    /*
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_USE_SEQUENCE)))
    {
        CTaskComplexUseSequence* task = static_cast<CTaskComplexUseSequence*>(baseTask);
        char buf[256] = {0};
        char a[64];
        sprintf(a, "%d - %d %d %d %d %x(%d)", task->m_nCurrentTaskIndex, task->m_nEndTaskIndex, task->m_nSequenceIndex, task->m_nSequenceRepeatedCount, task->m_pSubTask, (task->m_pSubTask) ? task->m_pSubTask->IsSimple() : 0);
        strcat(buf, a);
        if (task->m_pSubTask)
        {
            CTaskComplex* sub = static_cast<CTaskComplex*>(task->m_pSubTask);
            bool simple = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(sub, 3))(sub);
            eTaskType taskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(sub, 4))(sub);
            sprintf(a, " - %x(%d) %d", sub, taskID, simple);
            strcat(buf, a);
            if (taskID == TASK_COMPLEX_SEQUENCE)
            {
                CTaskComplexSequence2* sub2 = static_cast<CTaskComplexSequence2*>(sub->m_pSubTask);
                sprintf(a, " - %x", (sub2) ? sub2->m_aTasks[0] : 0);
                strcat(buf, a);
            }
        }
        CHud::SetMessage(buf);
    }
    */
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_NAMED_ANIM)))
    {
        CTaskSimpleRunNamedAnim* task = static_cast<CTaskSimpleRunNamedAnim*>(baseTask);
        //char buf[256];
        //sprintf(buf, "%d '%s' '%s'", task->m_nAnimId, task->m_animName, task->m_animGroupName);
        //CHud::SetMessage(buf);

        if (!strcmp(task->m_animName, "EAT_VOMIT_P") && marioState.action != ACT_CUSTOM_ANIM_TO_ACTION)
        {
            sm64_set_mario_action(marioId, ACT_VOMIT);
            sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_VOMIT);
        }
        else if (!strcmp(task->m_animGroupName, "FOOD") && marioState.action != ACT_CUSTOM_ANIM_TO_ACTION)
        {
            sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
            sm64_set_mario_action_arg2(marioId, ACT_IDLE);
            sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_EAT);
        }
        else if (!strcmp(task->m_animName, "CLO_IN"))
        {
            // entering dresser
        }
        else if (!strcmp(task->m_animName, "CLO_OUT"))
        {
            // leaving dresser
        }
        else if (!strcmp(task->m_animName, "CLO_POSE_IN"))
        {
            // leaving dresser to look in the mirror
        }
        else if (!strcmp(task->m_animName, "CLO_POSE_OUT"))
        {
            // re-entering dresser after looking in the mirror
        }
        else if (!strncmp(task->m_animName, "CLO_POSE", 8))
        {
            // posing around
        }
        else if (!strcmp(task->m_animName, "CLO_BUY"))
        {
            // confirmed clothes change
        }
        else if (!strcmp(task->m_animName, "BRB_SIT_IN"))
        {
            // sit in the barber chair
        }
        else if (!strcmp(task->m_animName, "BRB_SIT_LOOP"))
        {
            // sitting in barber chair
        }
        else if (!strcmp(task->m_animName, "BRB_SIT_OUT"))
        {
            // get off the barber chair
        }
        else if (!strcmp(task->m_animName, "TAT_SIT_IN_P"))
        {
            // sit in the tattoo chair
        }
        else if (!strcmp(task->m_animName, "TAT_SIT_LOOP_P"))
        {
            // sitting in the tattoo chair
        }
        else if (!strcmp(task->m_animName, "TAT_SIT_OUT_P"))
        {
            // get off the tattoo chair
        }
    }
    else
    {
        _fallen = false;

        /*char buf[256] = {0};
        char a[32];
        for (int i=0; i<TASK_PRIMARY_MAX; i++)
        {
            sprintf(a, (ped->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[i]) ? "%x" : "%x ", ped->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[i]);
            strcat(buf, a);
            if (ped->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[i])
            {
                CTask* task = ped->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[i];
                eTaskType taskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(task, 4))(task);
                sprintf(a, "(%d) ", taskID);
                strcat(buf, a);
            }
        }
        strcat(buf, "- ");
        for (int i=0; i<TASK_SECONDARY_MAX; i++)
        {
            sprintf(a, (i == TASK_SECONDARY_MAX-1 || ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[i]) ? "%x" : "%x ", ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[i]);
            strcat(buf, a);
            if (ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[i])
            {
                CTask* task = ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[i];
                eTaskType taskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(task, 4))(task);
                sprintf(a, (i == TASK_SECONDARY_MAX-1) ? "(%d)" : "(%d) ", taskID);
                strcat(buf, a);
            }
        }
        CHud::SetMessage(buf);*/
    }
}

void marioPedTasksMaxFPS(CPlayerPed* ped, const int& marioId)
{
    CTask* baseTask;
    int16_t animFrame = marioState.animInfo.animFrame;
    int16_t loopEnd = (marioState.animInfo.curAnim) ? marioState.animInfo.curAnim->loopEnd : 0;

    if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_GO_PICKUP_ENTITY)))
    {
        CTaskComplexGoPickUpEntity* task = static_cast<CTaskComplexGoPickUpEntity*>(baseTask);

        if (task->m_pSubTask)
        {
            CTask* sub = task->m_pSubTask;
            removeObject(task->m_pEntity);

            // calling sub->GetID() returns some garbage number, so i copied this from plugin_sa CTask.cpp
            eTaskType taskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(sub, 4))(sub);

            // for this complex task, we're only checking for one case
            static int wait = 0;
            if (taskID == TASK_SIMPLE_PICKUP_ENTITY)
            {
                if (marioState.action != ACT_CUSTOM_ANIM_TO_ACTION && !wait)
                {
                    wait = CTimer::m_snTimeInMilliseconds;
                    sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
                    sm64_set_mario_action_arg2(marioId, ACT_HOLD_IDLE);
                    sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_PICKUP_SLOW);
                }
                else if (CTimer::m_snTimeInMilliseconds - wait < 500)
                    sm64_set_mario_anim_frame(marioId, 0);

                moveEntityToMarioHands((CTaskSimpleHoldEntity2*)sub, (marioState.animInfo.animID == MARIO_ANIM_CUSTOM_PICKUP_SLOW) ? (float)animFrame/(float)loopEnd : 1.f);
            }
            else
                wait = 0;
        }
    }
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_PUTDOWN_ENTITY)))
    {
        CTaskSimpleHoldEntity2* task = static_cast<CTaskSimpleHoldEntity2*>(baseTask);
        moveEntityToMarioHands(task, (float)(loopEnd - animFrame - 10)/(float)(loopEnd-10));
        if (marioState.action != ACT_CUSTOM_ANIM_TO_ACTION && task->m_pEntityToHold)
        {
            sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
            sm64_set_mario_action_arg2(marioId, ACT_IDLE);
            sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_PUTDOWN_SLOW);
        }
    }
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_HOLD_ENTITY)))
    {
        CTaskSimpleHoldEntity2* task = static_cast<CTaskSimpleHoldEntity2*>(baseTask);
        moveEntityToMarioHands(task);
    }
}
