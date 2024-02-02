/*
    Plugin-SDK (Grand Theft Auto San Andreas) header file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "PluginBase.h"
#include "CTaskSimple.h"
#include "CAnimBlendAssociation.h"
#include "CVector2D.h"

class PLUGIN_API CTaskSimpleDuck2 : public CTaskSimple {
protected:
    CTaskSimpleDuck2(plugin::dummy_func_t a) : CTaskSimple(a) {}
public:
    unsigned int m_nStartTime;
    unsigned short m_nLengthOfDuck;
    short m_nShotWhizzingCounter;
    CAnimBlendAssociation *m_pDuckAnim;
    CAnimBlendAssociation *m_pMoveAnim;

    bool m_bIsFinished;
    bool m_bIsAborting;
    bool m_bNeedToSetDuckFlag; // incase bIsDucking flag gets cleared elsewhere, so we know to stop duck task
    bool m_bIsInControl;	// if duck task is being controlled by another task then it requires continuous control

    CVector2D m_vecMoveCommand;
    unsigned char m_nDuckControlType;

    unsigned char m_nCountDownFrames;

    CTaskSimpleDuck2(eDuckControlTypes DuckControlType, unsigned short nLengthOfDuck, short nUseShotsWhizzingEvents = -1);

public:
    static bool CanPedDuck(CPed* ped)
    {
    	return plugin::CallAndReturn<bool, 0x692610, CPed*>(ped);
    }
};

VALIDATE_SIZE(CTaskSimpleDuck2, 0x28);
