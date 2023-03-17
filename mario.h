#ifndef MARIO_H_INCLUDED
#define MARIO_H_INCLUDED

#define MARIO_SCALE 0.0085f
#define lerp(a, b, amnt) a + (b - a) * amnt

#include <stdint.h>

#include <RenderWare.h>
#include "CVector.h"
#include "CEntity.h"

#include <libsm64.h>

extern SM64MarioState marioState;
extern CVector marioInterpPos;

bool marioSpawned();

void marioSetPos(const CVector& pos);
void onWallAttack(uint32_t surfaceObjectID);

void marioSpawn();
void marioDestroy();
void marioTick(float dt);

#endif // MARIO_H_INCLUDED
