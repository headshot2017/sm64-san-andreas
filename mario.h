#ifndef MARIO_H_INCLUDED
#define MARIO_H_INCLUDED

#define MARIO_SCALE 0.0085f

#include <stdint.h>

#include <RenderWare.h>
#include "CVector.h"
#include "CEntity.h"

#include <libsm64.h>

extern SM64MarioState marioState;
extern SM64MarioInputs marioInput;

void marioToggleDebug();

bool marioSpawned();

void marioSetPos(const CVector& pos);
void onWallAttack(uint32_t surfaceObjectID);

void marioSpawn();
void marioDestroy();
void marioTick(float dt);
void marioRender();

#endif // MARIO_H_INCLUDED
