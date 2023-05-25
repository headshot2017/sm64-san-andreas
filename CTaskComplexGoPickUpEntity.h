/*
    From gta-reversed-modern
*/

#pragma once

#include "CTaskComplex.h"

class CEntity;

class PLUGIN_API CTaskComplexGoPickUpEntity : public CTaskComplex {
protected:
    CTaskComplexGoPickUpEntity(plugin::dummy_func_t a) : CTaskComplex(a) {}
public:
    //static constexpr auto Type = TASK_COMPLEX_GO_PICKUP_ENTITY;

    CEntity* m_pEntity;
    CVector  m_vecPosition;
    CVector  m_vecPickupPosition;
    uint32_t m_nTimePassedSinceLastSubTaskCreatedInMs;
    int32_t  m_nAnimGroupId;
    bool     m_bAnimBlockReferenced;
    char     _pad[3];
};

VALIDATE_SIZE(CTaskComplexGoPickUpEntity, 0x34);
