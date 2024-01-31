#ifndef MARIO_RENDER_H_INCLUDED
#define MARIO_RENDER_H_INCLUDED

#include "mario.h"
#include "CPed.h"

class CRealTimeShadowManager;

extern RwObject* weaponObj;
extern int triangles;

void marioRenderToggleDebug();

void marioRenderInit();
void marioRenderDestroy();
void marioRenderUpdateGeometry(const SM64MarioGeometryBuffers& marioGeometry);
void marioRenderInterpolate(const SM64MarioGeometryBuffers& marioGeometry, float& ticks, bool useCJPos, const CVector& pos);

void marioRender();
void marioRenderPed(CPed* ped);
void marioRenderPedReset(CPed* ped);
void marioPreRender(CPed* ped);
void marioPreRenderReset(CPed* ped);

void marioRenderWeapon();

#endif // MARIORENDER_H_INCLUDED
