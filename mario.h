#ifndef MARIO_H_INCLUDED
#define MARIO_H_INCLUDED

#define MARIO_SCALE 0.0085f
#define lerp(a, b, amnt) a + (b - a) * amnt
#define sign(a) (a>0 ? 1 : a<0 ? -1 : 0)

#include <stdint.h>
#include <unordered_set>
#include <unordered_map>

#include <RenderWare.h>
#include "CVector.h"
#include "CEntity.h"
#include "eWeaponType.h"

#include <libsm64.h>

class CPlayerPed;


extern std::unordered_set<eWeaponType> sideAnimWeaponIDs;
extern std::unordered_set<eWeaponType> shoulderWeaponIDs;
extern std::unordered_set<eWeaponType> heavyWeaponIDs;
extern std::unordered_set<eWeaponType> lightWeaponIDs;
extern std::unordered_map<eWeaponType, std::pair<float, float> > weaponKnockbacks;

extern SM64MarioState marioState;
extern SM64MarioGeometryBuffers marioGeometry;
extern CVector marioInterpPos;

extern float headAngleTarget[2];
extern bool overrideHeadAngle;

bool removeObject(CEntity* ent);

bool marioSpawned();

void marioSetPos(const CVector& pos, bool load=true);
void onWallAttack(uint32_t surfaceObjectID);

void marioSpawn();
void marioDestroy();
void marioTick(float dt);

void marioProcessWeapon(CPlayerPed* player);
void marioTestAnim();

#endif // MARIO_H_INCLUDED
