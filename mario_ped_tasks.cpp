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
#include "CTaskSimpleSwim.h"
#include "CTaskSimpleUseGun.h"
#include "CTaskSimpleClimb.h"
#include "CTaskSimpleStealthKill.h"
#include "CModelInfo.h"
extern "C" {
    #include <decomp/include/sm64shared.h>
    #include <decomp/include/mario_animation_ids.h>
}

#include "mario.h"
#include "mario_custom_anims.h"
#include "mario_cj_anims.h"

void moveEntityToMarioHands(CTaskSimpleHoldEntity2* task, float pickupLerp=1.f)
{
    if (pickupLerp < 0) pickupLerp = 0;
    task->m_vecPosition.z = -0.5f * pickupLerp;
    if (marioState.animInfo.animID == MARIO_ANIM_CUSTOM_VENDING_MACHINE)
        task->m_pEntityToHold->m_bIsVisible = 0;
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
    	sm64_set_mario_anim_override(marioId, 0);
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
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_NAMED_ANIM)))
    {
        CTaskSimpleRunNamedAnim* task = static_cast<CTaskSimpleRunNamedAnim*>(baseTask);

        //char buf[256];
        //sprintf(buf, "%d '%s' '%s'", task->m_nAnimId, task->m_animName, task->m_animGroupName);
        //CHud::SetMessage(buf);

        runAnimKey(task, marioId);
    }
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_USE_SEQUENCE)))
    {
        // primarily used for the vending machine animation
        CTaskComplexUseSequence* task = static_cast<CTaskComplexUseSequence*>(baseTask);
        if (task->m_pSubTask)
        {
            CTaskComplex* sub = static_cast<CTaskComplex*>(task->m_pSubTask);
            eTaskType sub_taskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(sub, 4))(sub);
            if (sub_taskID == TASK_COMPLEX_SEQUENCE)
            {
                CTaskComplexSequence2* sub2 = static_cast<CTaskComplexSequence2*>(sub);
                CTask* sub2_aTask = sub2->m_aTasks[sub2->m_nCurrentTaskIndex];
                eTaskType aTaskID = ((eTaskType (__thiscall *)(CTask *))plugin::GetVMT(sub2_aTask, 4))(sub2_aTask);
                if (aTaskID == TASK_SIMPLE_SLIDE_TO_COORD)
                {
                    // class CTaskSimpleSlideToCoord : public CTaskSimpleRunNamedAnim
                    CTaskSimpleRunNamedAnim* animTask = static_cast<CTaskSimpleRunNamedAnim*>(sub2_aTask);
                    runAnimKey(animTask, marioId);
                }
            }
        }
    }
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_CLIMB)))
    {
        // only happens in Catalyst when getting into the train
        CTaskSimpleClimb* task = static_cast<CTaskSimpleClimb*>(baseTask);
        sm64_set_mario_action_arg(marioId, ACT_CUSTOM_ANIM_TO_ACTION, 1);
        sm64_set_mario_action_arg2(marioId, ACT_IDLE);
        sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_CLIMB_CJ);
    }
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_SWIM)))
    {
        CTaskSimpleSwim* task = static_cast<CTaskSimpleSwim*>(baseTask);
        if ((marioState.action & ACT_GROUP_MASK) == ACT_GROUP_SUBMERGED && marioState.velocity[1] < 0 && task->m_nSwimState < SWIM_SPRINTING)
        {
            // submerge CJ
            task->m_nSwimState = SWIM_DIVE_UNDERWATER;
            ped->m_pPlayerData->m_fMoveBlendRatio = 0;
        }
    }
    else
    {
        _fallen = false;
        resetLastAnim();
    }

    /*
    char buf[256] = {0};
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
	CHud::SetMessage(buf);
	*/
}

void marioPedTasksMaxFPS(CPlayerPed* ped, const int& marioId)
{
    CTask* baseTask;
    int16_t animFrame = marioState.animInfo.animFrame;
    int16_t loopEnd = (marioState.animInfo.curAnim) ? marioState.animInfo.curAnim->loopEnd : 0;
    const CWeapon& activeWeapon = ped->m_aWeapons[ped->m_nActiveWeaponSlot];

    if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_GO_PICKUP_ENTITY)))
    {
        CTaskComplexGoPickUpEntity* task = static_cast<CTaskComplexGoPickUpEntity*>(baseTask);
        sm64_set_mario_anim_override(marioId, 0);

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
        sm64_set_mario_anim_override(marioId, 0);
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
    	sm64_set_mario_anim_override(marioId, 0);
    }
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_STEALTH_KILL)))
    {
        CTaskSimpleStealthKill* task = static_cast<CTaskSimpleStealthKill*>(baseTask);

        //char buf[256];
        //sprintf(buf, "stealth %d %d %d %d", task->b_bIsAborting, task->b_bIsFinished, task->m_bKeepTargetAlive, task->m_nTime);
        //CHud::SetMessage(buf);

        sm64_set_mario_anim_override(marioId, 0);
        sm64_set_mario_rightarm_angle(marioId, 0, 0, 0);
        sm64_set_mario_leftarm_angle(marioId, 0, 0, 0);
    }
    else if ((baseTask = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_USE_GUN)))
    {
        CTaskSimpleUseGun* task = static_cast<CTaskSimpleUseGun*>(baseTask);
        bool moving = (ped->m_vecMoveSpeed.x || ped->m_vecMoveSpeed.y);
        static float armsShoot = 0;
		static float torsoShoot = 0;

        if (task->m_pWeaponInfo->m_nFlags.bAimWithArm)
        {
            // used with normal pistol, sawed-off shotgun and uzi/tec9
            bool reloading = !task->m_ArmIKInUse && !task->m_bSkipAim && !task->m_LookIKInUse;
            if (!reloading && gunAnimOverrideTable.count(marioState.animInfo.animOverride.current))
            {
                sm64_set_mario_anim_override(marioId, gunAnimOverrideTable[marioState.animInfo.animOverride.current]);

                // get rotations for arms...
                static float rightArmShoot = 0;
                static float leftArmShoot = 0;
                if (task->bRightHand) rightArmShoot = (task->m_nFireGunThisFrame * task->bRightHand) / 1.0f;
                if (task->bLefttHand) leftArmShoot = (task->m_nFireGunThisFrame * task->bLefttHand) / 2.f;
                rightArmShoot /= 1.5f;
                leftArmShoot /= 2.f;

                float updown = (marioState.headAngle[0] >= 0) ? 16 : -8; // if looking up or down
                float rightarm[] = {
                    (task->m_ArmIKInUse) ? -(float)M_PI_2-marioState.headAngle[0]*4.f : -(float)M_PI,
                    (task->m_ArmIKInUse) ? (float)M_PI_2+marioState.headAngle[0]*updown : 0,
                    ( (task->m_ArmIKInUse) ? marioState.headAngle[1]-0.45f : 0 ) - rightArmShoot
                };
                float leftarm[] = {
                    (task->m_ArmIKInUse) ? -(float)M_PI_2-marioState.headAngle[0]*4.f : 0,
                    (task->m_ArmIKInUse) ? -(float)M_PI_2-marioState.headAngle[0]*updown : 0,
                    ( (task->m_ArmIKInUse) ? marioState.headAngle[1]+0.45f : 0 ) + leftArmShoot
                };
                sm64_set_mario_rightarm_angle(marioId, rightarm[0], rightarm[1], rightarm[2]);
                if (task->m_pWeaponInfo->m_nFlags.bTwinPistol)
                    sm64_set_mario_leftarm_angle(marioId, leftarm[0], leftarm[1], leftarm[2]);
            }
            else
            {
                // no animation or is reloading gun
                sm64_set_mario_anim_override(marioId, 0);
                sm64_set_mario_rightarm_angle(marioId, 0, 0, 0);
                sm64_set_mario_leftarm_angle(marioId, 0, 0, 0);
            }
        }
        else if (sideAnimWeaponIDs.count(activeWeapon.m_eWeaponType))
        {
            // heavier weapon (deagle, shotgun, mp5, rifles...)
            sm64_set_mario_anim_override(marioId, (moving) ? MARIO_ANIM_CUSTOM_RIFLE_AIM_WALK : MARIO_ANIM_CUSTOM_RIFLE_AIM);

            if (task->m_nFireGunThisFrame && weaponKnockbacks.count(activeWeapon.m_eWeaponType))
			{
				armsShoot = weaponKnockbacks[activeWeapon.m_eWeaponType].first;
				torsoShoot = weaponKnockbacks[activeWeapon.m_eWeaponType].second;
			}
            armsShoot /= 1.1f;
            torsoShoot /= 1.1f;

            sm64_set_mario_rightarm_angle(marioId, 0, -armsShoot, 0);
            sm64_set_mario_leftarm_angle(marioId, 0, -armsShoot, 0);
            sm64_set_mario_torsoangle(marioId, -torsoShoot/2.75f, 0, torsoShoot);
        }
        else if (shoulderWeaponIDs.count(activeWeapon.m_eWeaponType))
		{
			// rocket launcher
			sm64_set_mario_anim_override(marioId, (moving) ? MARIO_ANIM_CUSTOM_RIFLE_AIM_WALK : MARIO_ANIM_CUSTOM_RIFLE_AIM);
			sm64_set_mario_rightarm_angle(marioId, 0, 0, 0);
			sm64_set_mario_leftarm_angle(marioId, 0, 0, 0);
		}
		else if (heavyWeaponIDs.count(activeWeapon.m_eWeaponType))
		{
			// minigun, flamethrower, extinguisher
			sm64_set_mario_anim_override(marioId, (moving) ? MARIO_ANIM_CUSTOM_GUNHEAVY_AIM_WALK : MARIO_ANIM_CUSTOM_GUNHEAVY_AIM);
			sm64_set_mario_rightarm_angle(marioId, 0, 0, 0);
			sm64_set_mario_leftarm_angle(marioId, 0, 0, 0);
		}
		else if (lightWeaponIDs.count(activeWeapon.m_eWeaponType))
		{
			sm64_set_mario_anim_override(marioId, (moving) ? MARIO_ANIM_CUSTOM_GUNLIGHT_AIM_WALK : MARIO_ANIM_CUSTOM_GUNLIGHT_AIM);

			if (weaponKnockbacks.count(activeWeapon.m_eWeaponType))
			{
				if (task->m_nFireGunThisFrame)
				{
					armsShoot = weaponKnockbacks[activeWeapon.m_eWeaponType].first;
					torsoShoot = weaponKnockbacks[activeWeapon.m_eWeaponType].second;
				}

				armsShoot /= 1.1f;
				torsoShoot /= 1.1f;

				sm64_set_mario_rightarm_angle(marioId, -armsShoot, 0, 0);
				sm64_set_mario_torsoangle(marioId, -torsoShoot, 0, 0);
			}
			else if (task->m_nFireGunThisFrame && activeWeapon.m_eWeaponType == WEAPON_SPRAYCAN)
				sm64_set_mario_rightarm_angle(marioId, sinf(CTimer::m_snTimeInMilliseconds/300.f) / 1.5f, 0, (sinf(CTimer::m_snTimeInMilliseconds/150.f)) / 3.5f);
			else
				sm64_set_mario_rightarm_angle(marioId, 0, 0, 0);
		}
        else
		{
			sm64_set_mario_anim_override(marioId, 0);
			sm64_set_mario_rightarm_angle(marioId, 0, 0, 0);
			sm64_set_mario_leftarm_angle(marioId, 0, 0, 0);
		}
    }
    else
    {
        sm64_set_mario_rightarm_angle(marioId, 0, 0, 0);
        sm64_set_mario_leftarm_angle(marioId, 0, 0, 0);

		CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(activeWeapon.m_eWeaponType, ped->GetWeaponSkill());

        if (sideAnimWeaponIDs.count(activeWeapon.m_eWeaponType) && gunSideAnimOverrideTable.count(marioState.animInfo.animOverride.current))
			sm64_set_mario_anim_override(marioId, gunSideAnimOverrideTable[marioState.animInfo.animOverride.current]);
		else if (shoulderWeaponIDs.count(activeWeapon.m_eWeaponType) && gunShoulderAnimOverrideTable.count(marioState.animInfo.animOverride.current))
			sm64_set_mario_anim_override(marioId, gunShoulderAnimOverrideTable[marioState.animInfo.animOverride.current]);
		else if (heavyWeaponIDs.count(activeWeapon.m_eWeaponType) && gunHeavyAnimOverrideTable.count(marioState.animInfo.animOverride.current) && activeWeapon.m_eWeaponType != WEAPON_EXTINGUISHER)
			sm64_set_mario_anim_override(marioId, gunHeavyAnimOverrideTable[marioState.animInfo.animOverride.current]);
		else
			sm64_set_mario_anim_override(marioId, 0);
    }

	//char buf[32];
	//sprintf(buf, "%d %d %d", marioState.animInfo.animOverride.current, marioState.actionState, marioState.animInfo.animFrame);
	//CHud::SetMessage(buf);
}
