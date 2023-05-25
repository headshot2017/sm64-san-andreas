#ifndef MARIO_H_INCLUDED
#define MARIO_H_INCLUDED

#define MARIO_SCALE 0.0085f
#define lerp(a, b, amnt) a + (b - a) * amnt
#define sign(a) (a>0 ? 1 : a<0 ? -1 : 0)

#include <stdint.h>

#include <RenderWare.h>
#include "CVector.h"
#include "CEntity.h"

#include <libsm64.h>

extern SM64MarioState marioState;
extern SM64MarioGeometryBuffers marioGeometry;
extern CVector marioInterpPos;

bool removeObject(CEntity* ent);

bool marioSpawned();

void marioSetPos(const CVector& pos, bool load=true);
void onWallAttack(uint32_t surfaceObjectID);

void marioSpawn();
void marioDestroy();
void marioTick(float dt);

void marioTestAnim();

#endif // MARIO_H_INCLUDED
