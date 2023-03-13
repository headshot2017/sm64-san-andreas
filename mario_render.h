#ifndef MARIO_RENDER_H_INCLUDED
#define MARIO_RENDER_H_INCLUDED

#include "mario.h"

void marioRenderToggleDebug();

void marioRenderInit();
void marioRenderDestroy();
void marioRenderUpdateGeometry(const SM64MarioGeometryBuffers& marioGeometry);
void marioRenderInterpolate(const SM64MarioGeometryBuffers& marioGeometry, float& ticks, bool useCJPos, const CVector& pos);

void marioRender();

#endif // MARIORENDER_H_INCLUDED
