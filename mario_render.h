#ifndef MARIO_RENDER_H_INCLUDED
#define MARIO_RENDER_H_INCLUDED

#include "mario.h"
#include "CPed.h"

void marioRenderToggleDebug();

void marioRenderInit();
void marioRenderDestroy();
void marioRenderUpdateGeometry(const SM64MarioGeometryBuffers& marioGeometry);
void marioRenderInterpolate(const SM64MarioGeometryBuffers& marioGeometry, float& ticks, bool useCJPos, const CVector& pos);

void marioRender();
void marioRenderPed(CPed* ped);
void marioRenderPedReset(CPed* ped);

#endif // MARIORENDER_H_INCLUDED
