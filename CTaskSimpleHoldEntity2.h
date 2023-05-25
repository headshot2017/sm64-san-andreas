/*
    From gta-reversed-modern
*/

#pragma once

#include "PluginBase.h"
#include "CTaskSimple.h"

enum eHoldEntityBoneFlags {
    HOLD_ENTITY_FLAG_1 = 0x1,
    HOLD_ENTITY_UPDATE_TRANSLATION_ONLY = 0x10
};

class CEntity;
class CAnimBlock;
class CAnimBlendHierarchy;
class CAnimBlendAssociation;

class PLUGIN_API CTaskSimpleHoldEntity2 : public CTaskSimple {
protected:
    CTaskSimpleHoldEntity2(plugin::dummy_func_t a) : CTaskSimple(a) {}
public:
    CEntity*               m_pEntityToHold;
    CVector                m_vecPosition;
    uint8_t                m_nBoneFrameId; // see ePedNode
    uint8_t                m_bBoneFlags;   // See eHoldEntityBoneFlags
    bool                   field_1A[2];
    float                  m_fRotation;
    int32_t                m_nAnimId;
    int32_t                m_nAnimGroupId;
    int32_t                m_animFlags; // see eAnimationFlags
    CAnimBlock*            m_pAnimBlock;
    CAnimBlendHierarchy*   m_pAnimBlendHierarchy; // If set, m_animID and m_groupID are ignored in StartAnim method
    bool                   m_bEntityDropped;
    bool                   m_bEntityRequiresProcessing;
    bool                   m_bDisallowDroppingOnAnimEnd;
    bool                   field_37;
    CAnimBlendAssociation* m_pAnimBlendAssociation;
};
