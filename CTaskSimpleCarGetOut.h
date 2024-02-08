/*
	From gta-reversed-modern
*/

#pragma once

#include "CTaskSimple.h"

class CVehicle;
class CAnimBlendAssociation;
class CTaskUtilityLineUpPedWithCar;

// VMT: 0x86ee4c | Size: 9
class CTaskSimpleCarGetOut : public CTaskSimple {
protected:
    CTaskSimpleCarGetOut(plugin::dummy_func_t a) : CTaskSimple(a) {}

public:
    bool m_finished{};                            // 8
    bool m_vehHasDoorToOpen{};                    // 9
    CAnimBlendAssociation* m_anim{};              // 0xC
    bool m_isUpsideDown{};                        // 0x10
    CVehicle* m_veh{};                            // 0x14
    uint32_t m_door{};                            // 0x18
    CTaskUtilityLineUpPedWithCar* m_taskLineUp{}; // 0x1C

public:
    CTaskSimpleCarGetOut(CVehicle* veh, uint32_t door, CTaskUtilityLineUpPedWithCar* taskLineUp);
    CTaskSimpleCarGetOut(const CTaskSimpleCarGetOut&); // NOTSA
    ~CTaskSimpleCarGetOut();
};
