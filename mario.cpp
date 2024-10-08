#include "mario.h"

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "plugin.h"
#include "CHud.h"
#include "CCamera.h"
#include "CPlayerPed.h"
#include "CWorld.h"
#include "CGame.h"
#include "CMessages.h"
#include "CTaskSimpleIKLookAt.h"
#include "CTaskSimpleIKManager.h"
#include "CTaskSimpleUseGun.h"
#include "CTaskSimpleDuck2.h"
#include "CTaskComplexLeaveCar.h"
#include "CEntryExitManager.h"
#include "CCutsceneMgr.h"
#include "ePedBones.h"
#include "eSurfaceType.h"
extern "C" {
    #include <decomp/include/PR/ultratypes.h>
    #include <decomp/include/audio_defines.h>
    #include <decomp/include/surface_terrains.h>
    #include <decomp/include/sm64shared.h>
    #include <decomp/include/mario_animation_ids.h>
}

#include "d3d9_funcs.h"
#include "main.h"
#include "mario_render.h"
#include "mario_custom_anims.h"
#include "mario_ped_tasks.h"

#define MAX_OBJS 512
#define PED_HEIGHT 10.f

struct LoadedSurface
{
    LoadedSurface() : ID(UINT_MAX), transform(), ent(nullptr), cachedPos(), cachedRotations{0,0,0} {}

    uint32_t ID;
    SM64ObjectTransform transform;
    CEntity* ent;
    CVector cachedPos;
    float cachedRotations[3];
};

struct LoadedPed
{
    LoadedPed() : ID(UINT_MAX), ent(nullptr), cachedPos() {}

    uint32_t ID;
    CEntity* ent;
    CVector cachedPos;
};

LoadedSurface loadedBuildings[MAX_OBJS];
LoadedSurface loadedObjects[MAX_OBJS];
LoadedPed loadedPeds[MAX_OBJS];

std::unordered_set<eWeaponType> sideAnimWeaponIDs = {
	WEAPON_SHOTGUN,
	WEAPON_SPAS12,
	WEAPON_AK47,
	WEAPON_M4,
	WEAPON_COUNTRYRIFLE,
	WEAPON_SNIPERRIFLE
};
std::unordered_set<eWeaponType> shoulderWeaponIDs = {
	WEAPON_RLAUNCHER,
	WEAPON_RLAUNCHER_HS
};
std::unordered_set<eWeaponType> heavyWeaponIDs = {
	WEAPON_MINIGUN,
	WEAPON_FTHROWER,
	WEAPON_EXTINGUISHER,
	WEAPON_CHAINSAW
};
std::unordered_set<eWeaponType> lightWeaponIDs = {
	WEAPON_PISTOL_SILENCED,
	WEAPON_DESERT_EAGLE,
	WEAPON_MP5,
	WEAPON_SPRAYCAN,
	WEAPON_CAMERA
};
std::unordered_map<eWeaponType, std::pair<float, float> > weaponKnockbacks = {
	// first value = arms knockback, second value = torso knockback
	{WEAPON_SHOTGUN, {2.5f, 0.5f}},
	{WEAPON_SPAS12, {2.f, 0.35f}},
	{WEAPON_AK47, {0.4f, 0.2f}},
	{WEAPON_M4, {0.4f, 0.2f}},
	{WEAPON_MP5, {0.2f, 0.25f}},
	{WEAPON_DESERT_EAGLE, {0.75f, 0.5f}},
	{WEAPON_PISTOL_SILENCED, {0.5f, 0.2f}},
};

SM64MarioState marioState;
SM64MarioInputs marioInput;
SM64MarioGeometryBuffers marioGeometry;

CVector marioLastPos, marioCurrPos, marioInterpPos, marioBlocksPos;
uint32_t elapsedTicks = 0;
uint32_t safeTicks = 0;
int marioId = -1;
float ticks = 0;
float headAngle[2] = {0};
float headAngleTarget[2] = {0};
bool overrideHeadAngle = false;


uint8_t* EulerIndices1 = (uint8_t*)0x866D9C;
uint8_t* EulerIndices2 = (uint8_t*)0x866D94;
enum eMatrixEulerFlags : uint32_t {
    TAIT_BRYAN_ANGLES = 0x0,
    SWAP_XZ = 0x01,
    EULER_ANGLES = 0x2,
    _ORDER_NEEDS_SWAP = 0x4,
};
void ConvertToEulerAngles(RwMatrix* m, float* pX, float* pY, float* pZ, uint32_t uiFlags)
{
    /*
        CVector m_right;        // 0x0  // RW: Right
        CVector m_forward;      // 0x10 // RW: Up
        CVector m_up;           // 0x20 // RW: At
    */

    float fArr[3][3];

    fArr[0][0] = m->right.x;
    fArr[0][1] = m->right.y;
    fArr[0][2] = m->right.z;

    fArr[1][0] = m->up.x;
    fArr[1][1] = m->up.y;
    fArr[1][2] = m->up.z;

    fArr[2][0] = m->at.x;
    fArr[2][1] = m->at.y;
    fArr[2][2] = m->at.z;

    int8_t iInd1 = EulerIndices1[(uiFlags >> 3) & 0x3];
    int8_t iInd2 = EulerIndices2[iInd1 + ((uiFlags & 0x4) != 0)];
    int8_t iInd3 = EulerIndices2[iInd1 - ((uiFlags & 0x4) != 0) + 1];

    if (uiFlags & EULER_ANGLES) {
        auto r13 = fArr[iInd1][iInd3];
        auto r12 = fArr[iInd1][iInd2];
        auto cy = sqrt(r12 * r12 + r13 * r13);
        if (cy > 0.0000019073486) { // Some epsilon?
            *pX = atan2(r12, r13);
            *pY = atan2(cy, fArr[iInd1][iInd1]);
            *pZ = atan2(fArr[iInd2][iInd3], -fArr[iInd3][iInd1]);
        }
        else {
            *pX = atan2(-fArr[iInd2][iInd3], fArr[iInd2][iInd2]);
            *pY = atan2(cy, fArr[iInd1][iInd1]);
            *pZ = 0.0F;
        }
    }
    else {
        auto r21 = fArr[iInd2][iInd1];
        auto r11 = fArr[iInd1][iInd1];
        auto cy = sqrt(r11 * r11 + r21 * r21);
        if (cy > 0.0000019073486) { // Some epsilon?
            *pX = atan2(fArr[iInd3][iInd2], fArr[iInd3][iInd3]);
            *pY = atan2(-fArr[iInd3][iInd1], cy);
            *pZ = atan2(r21, r11);
        }
        else {
            *pX = atan2(-fArr[iInd2][iInd3], fArr[iInd2][iInd2]);
            *pY = atan2(-fArr[iInd3][iInd1], cy);
            *pZ = 0.0F;
        }
    }

    if (uiFlags & SWAP_XZ)
        std::swap(*pX, *pZ);

    if (uiFlags & _ORDER_NEEDS_SWAP) {
        *pX = -*pX;
        *pY = -*pY;
        *pZ = -*pZ;
    }
}

bool removeObject(CEntity* ent)
{
    for (int i=0; i<MAX_OBJS; i++)
    {
        LoadedSurface& obj = loadedObjects[i];

        if (obj.ent && obj.ent == ent)
        {
            sm64_surface_object_delete(obj.ID);
            obj.ent = nullptr;
            obj.ID = UINT_MAX;
            return true;
        }
    }

    return false;
}

bool marioSpawned()
{
    return marioId != -1;
}

uint8_t determineMarioTerrain(uint8_t GTAmaterial)
{
    switch(GTAmaterial)
    {
        case SURFACE_SAND_DEEP:
        case SURFACE_SAND_MEDIUM:
        case SURFACE_SAND_COMPACT:
        case SURFACE_SAND_ARID:
        case SURFACE_SAND_MORE:
        case SURFACE_SAND_BEACH:
        case SURFACE_P_SAND:
        case SURFACE_P_SANDBEACH:
        case SURFACE_P_SAND_DENSE:
        case SURFACE_P_SAND_ARID:
        case SURFACE_P_SAND_COMPACT:
            return TERRAIN_SAND;
            break;

        case SURFACE_GRASS_LONG_DRY:
        case SURFACE_GRASS_LONG_LUSH:
        case SURFACE_GRASS_MEDIUM_DRY:
        case SURFACE_GRASS_MEDIUM_LUSH:
        case SURFACE_GRASS_SHORT_DRY:
        case SURFACE_GRASS_SHORT_LUSH:
        case SURFACE_DIRT:
        case SURFACE_DIRTTRACK:
        case SURFACE_GRAVEL:
        case SURFACE_VEGETATION:
        case SURFACE_RUBBER:
        case SURFACE_PLASTIC:
        case SURFACE_CARPET:
        case SURFACE_PAVEMENT:
        case SURFACE_PAVEMENT_FUCKED:
        case SURFACE_STAIRSCARPET:
        case SURFACE_RAILTRACK:
        case SURFACE_FLOORBOARD:
            return TERRAIN_GRASS;
            break;

        case SURFACE_WOOD_SOLID:
        case SURFACE_WOOD_THIN:
        case SURFACE_P_WOODDENSE:
            return TERRAIN_SPOOKY;
            break;
    }

    return TERRAIN_STONE;
}

void deleteBuildings()
{
    for (int i=0; i<MAX_OBJS; i++)
    {
        if (loadedBuildings[i].ent)
        {
            sm64_surface_object_delete(loadedBuildings[i].ID);
            loadedBuildings[i].ent = nullptr;
            loadedBuildings[i].ID = UINT_MAX;
        }
    }
}

void loadBuildings(const CVector& pos)
{
    deleteBuildings();

    char buf[256];
    marioBlocksPos = pos;

    // look for static GTA surfaces (buildings) nearby
    short foundObjs = 0;
    CEntity* outEntities[MAX_OBJS] = {0};
    CWorld::FindObjectsIntersectingCube(pos-CVector(64,64,64), pos+CVector(64,64,64), &foundObjs, MAX_OBJS, outEntities, true, false, false, false, true);
    //FindObjectsIntersectingCube(CVector const& cornerA, CVector const& cornerB, short* outCount, short maxCount, CEntity** outEntities, bool buildings, bool vehicles, bool peds, bool objects, bool dummies);
    printf("%d foundObjs\n", foundObjs);
    fflush(stdout);

    for (short i=0; i<foundObjs; i++)
    {
        CCollisionData* colData = outEntities[i]->GetColModel()->m_pColData;
        if (!colData) continue;

        uint8_t terrainType = TERRAIN_STONE;
        CVector ePos = outEntities[i]->GetPosition();
        float heading = outEntities[i]->GetHeading();
        float orX = outEntities[i]->GetMatrix()->up.z;
        float orY = outEntities[i]->GetMatrix()->right.z;

        SM64SurfaceObject obj;
        memset(&obj, 0, sizeof(SM64SurfaceObject));
        obj.transform.position[0] = ePos.x/MARIO_SCALE;
        obj.transform.position[1] = ePos.z/MARIO_SCALE;
        obj.transform.position[2] = -ePos.y/MARIO_SCALE;
        obj.transform.eulerRotation[0] = -orX * 180.f / M_PI;
        obj.transform.eulerRotation[1] = -heading * 180.f / M_PI;
        obj.transform.eulerRotation[2] = -orY * 180.f / M_PI;

        for (uint16_t j=0; j<colData->m_nNumTriangles; j++)
        {
            if (terrainType == TERRAIN_STONE)
                terrainType = determineMarioTerrain(colData->m_pTriangles[j].m_nMaterial);

            CVector vertA, vertB, vertC;
            colData->GetTrianglePoint(vertA, colData->m_pTriangles[j].m_nVertA);
            colData->GetTrianglePoint(vertB, colData->m_pTriangles[j].m_nVertB);
            colData->GetTrianglePoint(vertC, colData->m_pTriangles[j].m_nVertC);
            vertA /= MARIO_SCALE; vertB /= MARIO_SCALE; vertC /= MARIO_SCALE;

            obj.surfaceCount++;
            obj.surfaces = (SM64Surface*)realloc(obj.surfaces, sizeof(SM64Surface) * obj.surfaceCount);

            obj.surfaces[obj.surfaceCount-1].vertices[0][0] = vertC.x;  obj.surfaces[obj.surfaceCount-1].vertices[0][1] = vertC.z;  obj.surfaces[obj.surfaceCount-1].vertices[0][2] = -vertC.y;
            obj.surfaces[obj.surfaceCount-1].vertices[1][0] = vertB.x;  obj.surfaces[obj.surfaceCount-1].vertices[1][1] = vertB.z;	obj.surfaces[obj.surfaceCount-1].vertices[1][2] = -vertB.y;
            obj.surfaces[obj.surfaceCount-1].vertices[2][0] = vertA.x;  obj.surfaces[obj.surfaceCount-1].vertices[2][1] = vertA.z;	obj.surfaces[obj.surfaceCount-1].vertices[2][2] = -vertA.y;
        }

        for (uint16_t j=0; j<colData->m_nNumBoxes; j++)
        {
            if (terrainType == TERRAIN_STONE)
                terrainType = determineMarioTerrain(colData->m_pBoxes[j].m_nMaterial);

            obj.surfaceCount += 12;
            obj.surfaces = (SM64Surface*)realloc(obj.surfaces, sizeof(SM64Surface) * obj.surfaceCount);

            CVector Min = colData->m_pBoxes[j].m_vecMin;
            CVector Max = colData->m_pBoxes[j].m_vecMax;
            Min /= MARIO_SCALE; Max /= MARIO_SCALE;

            // block ground face
            obj.surfaces[obj.surfaceCount-12].vertices[0][0] = Max.x;	obj.surfaces[obj.surfaceCount-12].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-12].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-12].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-12].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-12].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-12].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-12].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-12].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-11].vertices[0][0] = Min.x; 	obj.surfaces[obj.surfaceCount-11].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-11].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-11].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-11].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-11].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-11].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-11].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-11].vertices[2][2] = -Max.y;

            // left (Y+)
            obj.surfaces[obj.surfaceCount-10].vertices[0][0] = Min.x;	obj.surfaces[obj.surfaceCount-10].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-10].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-10].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-10].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-10].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-10].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-10].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-10].vertices[2][2] = -Max.y;

            obj.surfaces[obj.surfaceCount-9].vertices[0][0] = Max.x; 	obj.surfaces[obj.surfaceCount-9].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-9].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-9].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-9].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-9].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-9].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-9].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-9].vertices[2][2] = -Max.y;

            // right (Y-)
            obj.surfaces[obj.surfaceCount-8].vertices[0][0] = Max.x;	obj.surfaces[obj.surfaceCount-8].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-8].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-8].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-8].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-8].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-8].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-8].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-8].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-7].vertices[0][0] = Min.x; 	obj.surfaces[obj.surfaceCount-7].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-7].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-7].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-7].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-7].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-7].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-7].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-7].vertices[2][2] = -Min.y;

            // back (X+)
            obj.surfaces[obj.surfaceCount-6].vertices[0][0] = Max.x;	obj.surfaces[obj.surfaceCount-6].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-6].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-6].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-6].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-6].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-6].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-6].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-6].vertices[2][2] = -Max.y;

            obj.surfaces[obj.surfaceCount-5].vertices[0][0] = Max.x; 	obj.surfaces[obj.surfaceCount-5].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-5].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-5].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-5].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-5].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-5].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-5].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-5].vertices[2][2] = -Min.y;

            // front (X-)
            obj.surfaces[obj.surfaceCount-4].vertices[0][0] = Min.x;	obj.surfaces[obj.surfaceCount-4].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-4].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-4].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-4].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-4].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-4].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-4].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-4].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-3].vertices[0][0] = Min.x; 	obj.surfaces[obj.surfaceCount-3].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-3].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-3].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-3].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-3].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-3].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-3].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-3].vertices[2][2] = -Max.y;

            // block bottom face
            obj.surfaces[obj.surfaceCount-2].vertices[0][0] = Min.x;	obj.surfaces[obj.surfaceCount-2].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-2].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-2].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-2].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-2].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-2].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-2].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-2].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-1].vertices[0][0] = Max.x; 	obj.surfaces[obj.surfaceCount-1].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-1].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-1].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-1].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-1].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-1].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-1].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-1].vertices[2][2] = -Max.y;
        }

        for (uint32_t j=0; j<obj.surfaceCount; j++)
        {
            obj.surfaces[j].type = SURFACE_DEFAULT;
            obj.surfaces[j].force = 0;
            obj.surfaces[j].terrain = terrainType;
        }

        for (int j=0; j<MAX_OBJS; j++)
        {
            if (loadedBuildings[j].ent) continue;
            loadedBuildings[j].ent = outEntities[i];
            loadedBuildings[j].cachedPos = ePos;
            loadedBuildings[j].cachedRotations[0] = orX;
            loadedBuildings[j].cachedRotations[1] = heading;
            loadedBuildings[j].cachedRotations[2] = orY;
            loadedBuildings[j].transform = obj.transform;
            loadedBuildings[j].ID = sm64_surface_object_create(&obj);
            break;
        }

        if (obj.surfaces) free(obj.surfaces);
    }
}

void deleteNonBuildings()
{
    for (int i=0; i<MAX_OBJS; i++)
    {
        if (loadedObjects[i].ent)
        {
            sm64_surface_object_delete(loadedObjects[i].ID);
            loadedObjects[i].ent = nullptr;
            loadedObjects[i].ID = UINT_MAX;
        }
        if (loadedPeds[i].ID != UINT_MAX)
        {
            sm64_object_delete(loadedPeds[i].ID);
            loadedPeds[i].ID = UINT_MAX;
            loadedPeds[i].ent = nullptr;
        }
    }
}

void loadNonBuildings(const CVector& pos)
{
    deleteNonBuildings();

    // look for non-static GTA surfaces (vehicles, objects, etc) nearby
    short foundObjs = 0;
    CEntity* outEntities[MAX_OBJS] = {0};
    CWorld::FindObjectsIntersectingCube(pos-CVector(16,16,16), pos+CVector(16,16,16), &foundObjs, MAX_OBJS, outEntities, false, true, true, true, false);
    //FindObjectsIntersectingCube(CVector const& cornerA, CVector const& cornerB, short* outCount, short maxCount, CEntity** outEntities, bool buildings, bool vehicles, bool peds, bool objects, bool dummies);
    printf("%d foundObjs (nonBuildings)\n", foundObjs);
    fflush(stdout);

    for (short i=0; i<foundObjs; i++)
    {
        if (outEntities[i]->m_bRemoveFromWorld || !outEntities[i]->m_bIsVisible ||
            (outEntities[i]->m_nType == ENTITY_TYPE_OBJECT &&
             ( ((CObject*)outEntities[i])->m_nPhysicalFlags.bDisableMoveForce || ((CObject*)outEntities[i])->m_nPhysicalFlags.bAttachedToEntity) // door, or being held by a ped
            )
           )
            continue;

        CVector ePos = outEntities[i]->GetPosition();

        if (outEntities[i]->m_nType == ENTITY_TYPE_PED)
        {
            // add peds as object collider instead of static surface
            CPed* entPed = (CPed*)outEntities[i];
            if (!entPed->IsPlayer() && entPed->IsAlive())
            {
                for (int32_t j=0; j<MAX_OBJS; j++)
                {
                    if (loadedPeds[j].ID != UINT_MAX) continue;

                    SM64ObjectCollider objCol;
                    objCol.position[0] = ePos.x/MARIO_SCALE;
                    objCol.position[1] = ePos.z/MARIO_SCALE;
                    objCol.position[2] = -ePos.y/MARIO_SCALE;
                    objCol.height = PED_HEIGHT;
                    objCol.radius = 30.f;

                    loadedPeds[j].ID = sm64_object_create(&objCol);
                    loadedPeds[j].ent = outEntities[i];
                    loadedPeds[j].cachedPos = ePos;
                    break;
                }
            }
            continue;
        }

        CCollisionData* colData = outEntities[i]->GetColModel()->m_pColData;
        if (!colData) continue;

        float heading = outEntities[i]->GetHeading();
        float orX = outEntities[i]->GetMatrix()->up.z;
        float orY = outEntities[i]->GetMatrix()->right.z;

        SM64SurfaceObject obj;
        memset(&obj, 0, sizeof(SM64SurfaceObject));
        obj.transform.position[0] = ePos.x/MARIO_SCALE;
        obj.transform.position[1] = ePos.z/MARIO_SCALE;
        obj.transform.position[2] = -ePos.y/MARIO_SCALE;
        obj.transform.eulerRotation[0] = -orX * 180.f / M_PI;
        obj.transform.eulerRotation[1] = -heading * 180.f / M_PI;
        obj.transform.eulerRotation[2] = -orY * 180.f / M_PI;

        for (uint16_t j=0; j<colData->m_nNumTriangles; j++)
        {
            CVector vertA, vertB, vertC;
            colData->GetTrianglePoint(vertA, colData->m_pTriangles[j].m_nVertA);
            colData->GetTrianglePoint(vertB, colData->m_pTriangles[j].m_nVertB);
            colData->GetTrianglePoint(vertC, colData->m_pTriangles[j].m_nVertC);
            vertA /= MARIO_SCALE; vertB /= MARIO_SCALE; vertC /= MARIO_SCALE;

            obj.surfaceCount++;
            obj.surfaces = (SM64Surface*)realloc(obj.surfaces, sizeof(SM64Surface) * obj.surfaceCount);

            obj.surfaces[obj.surfaceCount-1].vertices[0][0] = vertC.x;  obj.surfaces[obj.surfaceCount-1].vertices[0][1] = vertC.z;  obj.surfaces[obj.surfaceCount-1].vertices[0][2] = -vertC.y;
            obj.surfaces[obj.surfaceCount-1].vertices[1][0] = vertB.x;  obj.surfaces[obj.surfaceCount-1].vertices[1][1] = vertB.z;	obj.surfaces[obj.surfaceCount-1].vertices[1][2] = -vertB.y;
            obj.surfaces[obj.surfaceCount-1].vertices[2][0] = vertA.x;  obj.surfaces[obj.surfaceCount-1].vertices[2][1] = vertA.z;	obj.surfaces[obj.surfaceCount-1].vertices[2][2] = -vertA.y;
        }

        for (uint16_t j=0; j<colData->m_nNumBoxes; j++)
        {
            obj.surfaceCount += 12;
            obj.surfaces = (SM64Surface*)realloc(obj.surfaces, sizeof(SM64Surface) * obj.surfaceCount);

            CVector Min = colData->m_pBoxes[j].m_vecMin;
            CVector Max = colData->m_pBoxes[j].m_vecMax;
            Min /= MARIO_SCALE; Max /= MARIO_SCALE;

            // block ground face
            obj.surfaces[obj.surfaceCount-12].vertices[0][0] = Max.x;	obj.surfaces[obj.surfaceCount-12].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-12].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-12].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-12].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-12].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-12].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-12].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-12].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-11].vertices[0][0] = Min.x; 	obj.surfaces[obj.surfaceCount-11].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-11].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-11].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-11].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-11].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-11].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-11].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-11].vertices[2][2] = -Max.y;

            // left (Y+)
            obj.surfaces[obj.surfaceCount-10].vertices[0][0] = Min.x;	obj.surfaces[obj.surfaceCount-10].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-10].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-10].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-10].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-10].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-10].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-10].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-10].vertices[2][2] = -Max.y;

            obj.surfaces[obj.surfaceCount-9].vertices[0][0] = Max.x; 	obj.surfaces[obj.surfaceCount-9].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-9].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-9].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-9].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-9].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-9].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-9].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-9].vertices[2][2] = -Max.y;

            // right (Y-)
            obj.surfaces[obj.surfaceCount-8].vertices[0][0] = Max.x;	obj.surfaces[obj.surfaceCount-8].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-8].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-8].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-8].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-8].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-8].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-8].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-8].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-7].vertices[0][0] = Min.x; 	obj.surfaces[obj.surfaceCount-7].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-7].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-7].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-7].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-7].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-7].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-7].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-7].vertices[2][2] = -Min.y;

            // back (X+)
            obj.surfaces[obj.surfaceCount-6].vertices[0][0] = Max.x;	obj.surfaces[obj.surfaceCount-6].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-6].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-6].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-6].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-6].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-6].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-6].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-6].vertices[2][2] = -Max.y;

            obj.surfaces[obj.surfaceCount-5].vertices[0][0] = Max.x; 	obj.surfaces[obj.surfaceCount-5].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-5].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-5].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-5].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-5].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-5].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-5].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-5].vertices[2][2] = -Min.y;

            // front (X-)
            obj.surfaces[obj.surfaceCount-4].vertices[0][0] = Min.x;	obj.surfaces[obj.surfaceCount-4].vertices[0][1] = Max.z;	obj.surfaces[obj.surfaceCount-4].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-4].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-4].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-4].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-4].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-4].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-4].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-3].vertices[0][0] = Min.x; 	obj.surfaces[obj.surfaceCount-3].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-3].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-3].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-3].vertices[1][1] = Max.z;	obj.surfaces[obj.surfaceCount-3].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-3].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-3].vertices[2][1] = Max.z;	obj.surfaces[obj.surfaceCount-3].vertices[2][2] = -Max.y;

            // block bottom face
            obj.surfaces[obj.surfaceCount-2].vertices[0][0] = Min.x;	obj.surfaces[obj.surfaceCount-2].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-2].vertices[0][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-2].vertices[1][0] = Max.x;	obj.surfaces[obj.surfaceCount-2].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-2].vertices[1][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-2].vertices[2][0] = Max.x;	obj.surfaces[obj.surfaceCount-2].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-2].vertices[2][2] = -Min.y;

            obj.surfaces[obj.surfaceCount-1].vertices[0][0] = Max.x; 	obj.surfaces[obj.surfaceCount-1].vertices[0][1] = Min.z;	obj.surfaces[obj.surfaceCount-1].vertices[0][2] = -Max.y;
            obj.surfaces[obj.surfaceCount-1].vertices[1][0] = Min.x;	obj.surfaces[obj.surfaceCount-1].vertices[1][1] = Min.z;	obj.surfaces[obj.surfaceCount-1].vertices[1][2] = -Min.y;
            obj.surfaces[obj.surfaceCount-1].vertices[2][0] = Min.x;	obj.surfaces[obj.surfaceCount-1].vertices[2][1] = Min.z;	obj.surfaces[obj.surfaceCount-1].vertices[2][2] = -Max.y;
        }

        for (uint32_t j=0; j<obj.surfaceCount; j++)
        {
            obj.surfaces[j].type = SURFACE_DEFAULT;
            obj.surfaces[j].force = 0;
            obj.surfaces[j].terrain = TERRAIN_STONE;
        }

        if (obj.surfaceCount)
        {
            for (int j=0; j<MAX_OBJS; j++)
            {
                if (loadedObjects[j].ent) continue;
                loadedObjects[j].ent = outEntities[i];
                loadedObjects[j].cachedPos = ePos;
                loadedObjects[j].cachedRotations[0] = orX;
                loadedObjects[j].cachedRotations[1] = heading;
                loadedObjects[j].cachedRotations[2] = orY;
                loadedObjects[j].transform = obj.transform;
                loadedObjects[j].ID = sm64_surface_object_create(&obj);
                break;
            }
        }

        if (obj.surfaces) free(obj.surfaces);
    }
}

void loadCollisions(const CVector& pos, bool waterLevel = true)
{
    int width = 16384;
    SM64Surface surfaces[2];
    CVector sm64pos(pos.x / MARIO_SCALE, (waterLevel ? -70 : -105) / MARIO_SCALE, -pos.y / MARIO_SCALE);

    surfaces[0].vertices[0][0] = sm64pos.x + width;	surfaces[0].vertices[0][1] = sm64pos.y;	surfaces[0].vertices[0][2] = sm64pos.z + width;
    surfaces[0].vertices[1][0] = sm64pos.x - width;	surfaces[0].vertices[1][1] = sm64pos.y;	surfaces[0].vertices[1][2] = sm64pos.z - width;
    surfaces[0].vertices[2][0] = sm64pos.x - width;	surfaces[0].vertices[2][1] = sm64pos.y;	surfaces[0].vertices[2][2] = sm64pos.z + width;

    surfaces[1].vertices[0][0] = sm64pos.x - width;	surfaces[1].vertices[0][1] = sm64pos.y;	surfaces[1].vertices[0][2] = sm64pos.z - width;
    surfaces[1].vertices[1][0] = sm64pos.x + width;	surfaces[1].vertices[1][1] = sm64pos.y;	surfaces[1].vertices[1][2] = sm64pos.z + width;
    surfaces[1].vertices[2][0] = sm64pos.x + width;	surfaces[1].vertices[2][1] = sm64pos.y;	surfaces[1].vertices[2][2] = sm64pos.z - width;

    sm64_static_surfaces_load(surfaces, 2);

    loadBuildings(pos);
    loadNonBuildings(pos);
}

void marioSetPos(const CVector& pos, bool load)
{
    if (!marioSpawned()) return;
    if (load) loadCollisions(pos);

    marioLastPos = pos;
    marioCurrPos = pos;
    marioInterpPos = pos;

    CVector sm64pos(pos.x / MARIO_SCALE, pos.z / MARIO_SCALE, -pos.y / MARIO_SCALE);
    sm64_set_mario_position(marioId, sm64pos.x, sm64pos.y, sm64pos.z);
}

void onWallAttack(uint32_t surfaceObjectID)
{
    if (surfaceObjectID == UINT_MAX) return;

    for (int i=0; i<MAX_OBJS; i++)
    {
        if (!loadedObjects[i].ent || loadedObjects[i].ID != surfaceObjectID)
            continue;
        CPlayerPed* player = FindPlayerPed();
        CVector direction(0,0,0);

        if (loadedObjects[i].ent->m_nType == ENTITY_TYPE_OBJECT)
        {
            CObject* obj = (CObject*)loadedObjects[i].ent;
            obj->m_fHealth -= 500;
            if (obj->m_fHealth <= 0)
            {
                obj->ObjectDamage(1000.f, &marioInterpPos, &direction, player, WEAPON_UNARMED); // directly using this without "health -= 500" instantly destroys object
                sm64_play_sound_global(SOUND_GENERAL_BREAK_BOX);
            }
        }
        break;
    }
}

void marioSpawn()
{
    if (marioSpawned() || !FindPlayerPed()) return;
    char buf[256];

    // SM64 <--> GTA SA coordinates translation:
    // Y and Z coordinates must be swapped. SM64 up coord is Y+ and GTA SA is Z+
    // Mario model must also be unmirrored by making SM64 Z coordinate / GTA-SA Y coordinate negative
    // GTA SA -> SM64: divide scale
    // SM64 -> GTA SA: multiply scale
    CPlayerPed* ped = FindPlayerPed();
    CVector pos = ped->GetPosition();
    pos.z -= 1;

    loadCollisions(pos, ped->m_nPhysicalFlags.bTouchingWater);

    marioGeometry.position = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.normal   = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.color    = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.uv       = new float[6 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.numTrianglesUsed = 0;

    CVector sm64pos(pos.x / MARIO_SCALE, pos.z / MARIO_SCALE, -pos.y / MARIO_SCALE);
    marioId = sm64_mario_create(sm64pos.x, sm64pos.y, sm64pos.z);
    if (!marioSpawned())
    {
        sprintf(buf, "Failed to spawn Mario at %.2f %.2f %.2f", pos.x, pos.y, pos.z);
        CHud::SetHelpMessage(buf, false, false, true);
        return;
    }

    marioLastPos = pos;
    marioCurrPos = pos;
    marioInterpPos = pos;

    ticks = 0;
    elapsedTicks = 0;
    memset(&marioInput, 0, sizeof(marioInput));
    memset(headAngle, 0, sizeof(headAngle));

    sm64_set_mario_faceangle(marioId, ped->GetHeading() + M_PI);

    CPad* pad = ped->GetPadFromPlayer();
    bool cjHasControl = (pad->bPlayerSafe || ped->m_nPedFlags.bInVehicle);
    if (!cjHasControl)
    {
        safeTicks = 0;
        ped->m_nPhysicalFlags.bApplyGravity = 0;
        ped->m_nPhysicalFlags.bCanBeCollidedWith = 0;
        ped->m_nPhysicalFlags.bCollidable = 0;
        ped->m_nPhysicalFlags.bOnSolidSurface = 1;
        ped->m_nPhysicalFlags.bDisableMoveForce = 1;
        ped->m_nPhysicalFlags.bDisableTurnForce = 1;
        ped->m_nPhysicalFlags.bDontApplySpeed = 1;
        ped->m_nAllowedAttackMoves = 0;
    }

    ped->m_pedAudio.field_7C = 0; // disable footstep sounds

#ifdef PRE_RELEASE_BUILD
    static bool firstInit = false;
    if (!firstInit)
    {
        sprintf(buf, "sm64-san-andreas build date %s %s. Things are subject to change!", __DATE__, __TIME__);
        CHud::SetHelpMessage(buf, false,false,false);
        firstInit = true;
    }
#endif // PRE_RELEASE_BUILD
}

void marioDestroy()
{
    if (!marioSpawned()) return;

    sm64_mario_delete(marioId);
    marioId = -1;

    delete[] marioGeometry.position;
    delete[] marioGeometry.normal;
    delete[] marioGeometry.color;
    delete[] marioGeometry.uv;
    memset(&marioGeometry, 0, sizeof(marioGeometry));
    if (FindPlayerPed())
    {
        CPlayerPed* ped = FindPlayerPed();
        ped->GetPadFromPlayer()->bDisablePlayerDuck = 0;
        ped->GetPadFromPlayer()->bDisablePlayerJump = 0;
        //ped->SetPosn(marioInterpPos + CVector(0, 0, 0.5f));
        ped->m_nPhysicalFlags.bApplyGravity = 1;
        ped->m_nPhysicalFlags.bCanBeCollidedWith = 1;
        ped->m_nPhysicalFlags.bCollidable = 1;
        ped->m_nPhysicalFlags.bOnSolidSurface = 1;
        ped->m_nPhysicalFlags.bDisableMoveForce = 0;
        ped->m_nPhysicalFlags.bDisableTurnForce = 0;
        ped->m_nPhysicalFlags.bDontApplySpeed = 0;
        ped->m_nAllowedAttackMoves = 5;

        if (weaponObj)
        {
            ped->m_pWeaponObject = weaponObj;
            weaponObj = nullptr;
        }

        ped->m_pedAudio.field_7C = 1; // enable footstep sounds
    }
}

void marioTick(float dt)
{
    if (!marioSpawned() || !FindPlayerPed()) return;
    CPlayerPed* ped = FindPlayerPed();

    bool usingMobilePhone = ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_USE_MOBILE_PHONE);
    if (ped->m_pWeaponObject || ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_eWeaponType == WEAPON_UNARMED)
    {
        weaponObj = ped->m_pWeaponObject;
        if (usingMobilePhone && ped->m_pWeaponObject)
            phoneObj = ped->m_pWeaponObject;
        ped->m_pWeaponObject = nullptr;
    }
    if (!usingMobilePhone && phoneObj)
        phoneObj = nullptr;

    ped->m_pShadowData = nullptr;

    bool carDoor = ped->m_pIntelligence->IsPedGoingForCarDoor();
    float hp = ped->m_fHealth / ped->m_fMaxHealth;

    CPad* pad = ped->GetPadFromPlayer();
    if (ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_PHONE_IN) || ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_PHONE_OUT))
        pad->DisablePlayerControls = 1;
    else if (ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_PHONE_TALK))
        pad->DisablePlayerControls = 0;
    pad->bDisablePlayerDuck = 0;
    pad->bDisablePlayerJump = 0;
    int jumpState = pad->GetJump();
    int attackState = pad->GetMeleeAttack();
    int duckState = pad->GetDuck();
    int walkState = pad->NewState.m_bPedWalk;
    pad->bDisablePlayerDuck = 1;
    pad->bDisablePlayerJump = 1;
    if (attackState > 1) attackState = 0; // fix bug where pressing jump makes attackState above 1

    // bugfix for some cutscenes where mario appears to be floating in the air while sitting down and driving a car
    CTaskComplexLeaveCar* leaveCarTask = (CTaskComplexLeaveCar*)ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR);
    bool leavingCar = (leaveCarTask && leaveCarTask->m_nNumGettingInSet);

    CTaskSimpleUseGun* useGunTask = (CTaskSimpleUseGun*)ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_USE_GUN);
	bool aimingHeavyGun = (useGunTask && useGunTask->m_pWeaponInfo && !useGunTask->m_pWeaponInfo->m_nFlags.bAimWithArm);

    bool fallingToVoid = (ped->GetPosition().z <= -100);

    bool cjHasControl = (pad->bPlayerSafe || ped->m_nPedFlags.bInVehicle || hp <= 0 || CEntryExitManager::mp_Active || fallingToVoid || CCutsceneMgr::ms_running || safeTicks > 0);
    bool overrideWithCJPos = ((ped->m_nPedFlags.bInVehicle || hp <= 0) &&
                              !ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER) &&
                              !ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) &&
                              !leavingCar &&
                              !ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT));
    bool overrideWithCJAI = (cjHasControl || carDoor || aimingHeavyGun ||
                             TheCamera.m_aCams[TheCamera.m_nActiveCam].m_nMode == MODE_HELICANNON_1STPERSON ||
                             ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_ACHIEVE_HEADING) ||
                             ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL) ||
                             ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_USE_SEQUENCE) ||
                             ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_STEALTH_KILL) ||
                             ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_USE_MOBILE_PHONE));

    if (cjHasControl)
    {
        if (pad->bPlayerSafe || ped->m_nPedFlags.bInVehicle || hp <= 0 || CEntryExitManager::mp_Active || CCutsceneMgr::ms_running || fallingToVoid) safeTicks = 4;
        ped->m_nPhysicalFlags.bApplyGravity = 1;
        ped->m_nPhysicalFlags.bCanBeCollidedWith = 1;
        ped->m_nPhysicalFlags.bCollidable = 1;
        ped->m_nPhysicalFlags.bDisableMoveForce = 0;
        ped->m_nPhysicalFlags.bDisableTurnForce = 0;
        ped->m_nPhysicalFlags.bDontApplySpeed = 0;
        ped->m_nAllowedAttackMoves = 5;
    }
    else
    {
        if (marioState.animInfo.animID == MARIO_ANIM_CUSTOM_DANCE_LOOP)
            sm64_set_mario_action(marioId, ACT_IDLE);

        ped->m_nPhysicalFlags.bApplyGravity = 0;
        ped->m_nPhysicalFlags.bCanBeCollidedWith = 0;
        ped->m_nPhysicalFlags.bCollidable = 0;
        ped->m_nPhysicalFlags.bDisableMoveForce = 0;
        ped->m_nPhysicalFlags.bDisableTurnForce = !carDoor;
        ped->m_nPhysicalFlags.bDontApplySpeed = 0;
        ped->m_nAllowedAttackMoves = 0;
    }

    static bool lastCutsceneRunning = false;
    if (CCutsceneMgr::ms_running)
    {
        if (!lastCutsceneRunning)
        {
            lastCutsceneRunning = true;
            marioSetPos(ped->GetPosition());
        }
        /*
        if (marioState.action != ACT_CUTSCENE)
        {
            sm64_set_mario_action(marioId, ACT_CUTSCENE);
            sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_CUTSCENE);
        }

        ped->m_nPedFlags.bDontRender = 0;
        ped->m_bIsVisible = 1;

        for (uint32_t i=0; i<CCutsceneMgr::ms_numCutsceneObjs; i++)
        {
            if (CCutsceneMgr::ms_pCutsceneObjects[i]->m_nModelIndex == 1) // CJ
            {
                float orX, orY, orZ;
                auto getAngle = [](RwMatrix* m, float& orX, float& orY, float& orZ){
                    orX = asinf(m->up.z);
                    float cosx = cosf(orX);
                    float cosy = CLAMP(m->at.z / cosx, 0, 1);
                    orY = acosf(cosy);
                    float cosz = CLAMP(m->up.y / cosx, 0, 1);
                    orZ = acosf(cosz);
                    //if (m->right.z < 0) orZ = -orZ;
                };

                RpHAnimHierarchy* hier = GetAnimHierarchyFromSkinClump(CCutsceneMgr::ms_pCutsceneObjects[i]->m_pRwClump);
                RwMatrix* root = &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, 0)];
                ConvertToEulerAngles(root, &orX, &orY, &orZ, 0);
                float heading = -orZ - M_PI_2;
                while (heading < -M_PI) heading += M_PI*2;

                CVector cutscenePos;
                cutscenePos.FromRwV3d(*RwMatrixGetPos(root));
                ped->SetPosn(cutscenePos);
                ped->SetHeading(heading);

                RwMatrix* neck = &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_NECK)];
                getAngle(neck, orX, orY, orZ);
                sm64_set_mario_headangle(marioId, -neck->up.z, neck->at.z, -orY + M_PI_2);



                RwMatrix* lshoulder = &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_LEFTSHOULDER)];
                ConvertToEulerAngles(lshoulder, &orX, &orY, &orZ, EULER_ANGLES);
                float lshoulderX = orX - M_PI;
                float lshoulderY = -orY + M_PI_2;
                while (lshoulderX < -M_PI) lshoulderX += M_PI*2;
                while (lshoulderX > M_PI) lshoulderX -= M_PI*2;
                while (lshoulderY < -M_PI) lshoulderY += M_PI*2;
                while (lshoulderY > M_PI) lshoulderY -= M_PI*2;
                //sm64_set_mario_leftarm_angle(marioId, lshoulderX, lshoulderY, 0);

                RwMatrix* lelbow = &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_LEFTELBOW)];
                ConvertToEulerAngles(lelbow, &orX, &orY, &orZ, EULER_ANGLES);

                static int keyPressTime = 0;
                static int lelbowModify = 0;
                static int lelbowSigns[3] = {1,1,1};
                static float lelbowOffsets[3] = {0,0,0};
                static char lelbowNames[3] = {'X', 'Y', 'Z'};
                static char lelbowBuf[256];
                static int lelbowArgs[3] = {0,1,2};
                static bool lelbowOn[3] = {true,true,true};
                if (plugin::KeyPressed(VK_NUMPAD1))
                {
                    lelbowModify = 0;
                    CHud::SetMessage("modifying lelbowX");
                }
                if (plugin::KeyPressed(VK_NUMPAD2))
                {
                    lelbowModify = 1;
                    CHud::SetMessage("modifying lelbowY");
                }
                if (plugin::KeyPressed(VK_NUMPAD3))
                {
                    lelbowModify = 2;
                    CHud::SetMessage("modifying lelbowZ");
                }
                if (plugin::KeyPressed(VK_NUMPAD0) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowSigns[lelbowModify] = -lelbowSigns[lelbowModify];
                    sprintf(lelbowBuf, "changed sign for lelbow%c: %sor%c", lelbowNames[lelbowModify], (lelbowSigns[lelbowModify] < 0) ? "-" : "", lelbowNames[lelbowModify]);
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_ADD) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowOffsets[lelbowModify] += M_PI_4;
                    sprintf(lelbowBuf, "add offset for lelbow%c: %.3f", lelbowNames[lelbowModify], lelbowOffsets[lelbowModify]);
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_SUBTRACT) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowOffsets[lelbowModify] -= M_PI_4;
                    sprintf(lelbowBuf, "subtract offset for lelbow%c: %.3f", lelbowNames[lelbowModify], lelbowOffsets[lelbowModify]);
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_PRINT) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                }
                if (plugin::KeyPressed(VK_MULTIPLY))
                {
                    sprintf(lelbowBuf, "%sorX + %.3f, %sorY + %.3f, %sorZ + %.3f, lelbow%c %s, lelbow%c %s, lelbow%c %s", lelbowSigns[0] < 0 ? "-" : "", lelbowOffsets[0], lelbowSigns[1] < 0 ? "-" : "", lelbowOffsets[1], lelbowSigns[2] < 0 ? "-" : "", lelbowOffsets[2], lelbowNames[lelbowArgs[0]], lelbowOn[0]?"true":"false", lelbowNames[lelbowArgs[1]], lelbowOn[1]?"true":"false", lelbowNames[lelbowArgs[2]], lelbowOn[2]?"true":"false");
                    CHud::SetMessage(lelbowBuf);
                }
                if (plugin::KeyPressed(VK_NUMPAD7) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowArgs[0] = (lelbowArgs[0]+1) % 3;
                    sprintf(lelbowBuf, "sm64_set_mario_leftarm_angle argument 1 is now lelbow%c", lelbowNames[lelbowArgs[0]]);
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_NUMPAD8) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowArgs[1] = (lelbowArgs[1]+1) % 3;
                    sprintf(lelbowBuf, "sm64_set_mario_leftarm_angle argument 2 is now lelbow%c", lelbowNames[lelbowArgs[1]]);
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_NUMPAD9) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowArgs[2] = (lelbowArgs[2]+1) % 3;
                    sprintf(lelbowBuf, "sm64_set_mario_leftarm_angle argument 3 is now lelbow%c", lelbowNames[lelbowArgs[2]]);
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_NUMPAD4) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowOn[0] = !lelbowOn[0];
                    sprintf(lelbowBuf, "sm64_set_mario_leftarm_angle argument 1 is now %s", lelbowOn[0]?"true":"false");
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_NUMPAD5) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowOn[1] = !lelbowOn[1];
                    sprintf(lelbowBuf, "sm64_set_mario_leftarm_angle argument 2 is now %s", lelbowOn[1]?"true":"false");
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }
                if (plugin::KeyPressed(VK_NUMPAD6) && CTimer::m_snTimeInMilliseconds - keyPressTime > 250)
                {
                    keyPressTime = CTimer::m_snTimeInMilliseconds;
                    lelbowOn[2] = !lelbowOn[2];
                    sprintf(lelbowBuf, "sm64_set_mario_leftarm_angle argument 3 is now %s", lelbowOn[2]?"true":"false");
                    CMessages::AddMessageJumpQ(lelbowBuf, 2000, 0, false);
                }

                float lelbowX = (orX * lelbowSigns[0]) + lelbowOffsets[0];
                float lelbowY = (orY * lelbowSigns[1]) + lelbowOffsets[1];
                float lelbowZ = (orZ * lelbowSigns[2]) + lelbowOffsets[2];
                while (lelbowX < -M_PI) lelbowX += M_PI*2;
                while (lelbowX > M_PI) lelbowX -= M_PI*2;
                while (lelbowY < -M_PI) lelbowY += M_PI*2;
                while (lelbowY > M_PI) lelbowY -= M_PI*2;
                while (lelbowZ < -M_PI) lelbowZ += M_PI*2;
                while (lelbowZ > M_PI) lelbowZ -= M_PI*2;
                float args[3] = {
                    (lelbowArgs[0] == 0) ? lelbowX : (lelbowArgs[0] == 1) ? lelbowY : lelbowZ,
                    (lelbowArgs[1] == 0) ? lelbowX : (lelbowArgs[1] == 1) ? lelbowY : lelbowZ,
                    (lelbowArgs[2] == 0) ? lelbowX : (lelbowArgs[2] == 1) ? lelbowY : lelbowZ,
                };
                sm64_set_mario_leftarm_angle(marioId, (lelbowOn[0] ? args[0] : 0), (lelbowOn[1] ? args[1] : 0), (lelbowOn[2] ? args[2] : 0));
                //char buf[256];
                //sprintf(buf, "%.3f %.3f %.3f - %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f", (orX - M_PI), lelbowY, orZ, lelbow->right.x, lelbow->right.y, lelbow->right.z, lelbow->up.x, lelbow->up.y, lelbow->up.z, lelbow->at.x, lelbow->at.y, lelbow->at.z);
                //CHud::SetMessage(buf);

                RwMatrix* rshoulder = &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_RIGHTSHOULDER)];
                getAngle(rshoulder, orX, orY, orZ);
                //sm64_set_mario_rightarm_angle(marioId, -orX, -orZ + M_PI_2, -orY + M_PI_2);
                //sm64_set_mario_rightarm_angle(marioId, rshoulder->up.z, 0, 0);

                RwMatrix* relbow = &RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_RIGHTELBOW)];
                ConvertToEulerAngles(relbow, &orX, &orY, &orZ, EULER_ANGLES);
                float relbowX = orX - M_PI;
                float relbowY = -orY + M_PI_2;
                float relbowZ = orZ;
                while (relbowX < -M_PI) relbowX += M_PI*2;
                while (relbowX > M_PI) relbowX -= M_PI*2;
                while (relbowY < -M_PI) relbowY += M_PI*2;
                while (relbowY > M_PI) relbowY -= M_PI*2;
                while (relbowZ < -M_PI) relbowZ += M_PI*2;
                while (relbowZ > M_PI) relbowZ -= M_PI*2;
                //sm64_set_mario_rightarm_angle(marioId, relbowX, relbowY, relbowZ);

                break;
            }
        }
        */
    }
    else if (!CCutsceneMgr::ms_running && lastCutsceneRunning)
    {
        lastCutsceneRunning = false;
        sm64_set_mario_action(marioId, ACT_IDLE);
        sm64_set_mario_leftarm_angle(marioId, 0,0,0);
        sm64_set_mario_leftforearm_angle(marioId, 0,0,0);
        sm64_set_mario_lefthand_angle(marioId, 0,0,0);
        sm64_set_mario_leftleg_angle(marioId, 0,0,0);
        sm64_set_mario_leftankle_angle(marioId, 0,0,0);
        sm64_set_mario_leftfoot_angle(marioId, 0,0,0);
        sm64_set_mario_rightarm_angle(marioId, 0,0,0);
        sm64_set_mario_rightforearm_angle(marioId, 0,0,0);
        sm64_set_mario_righthand_angle(marioId, 0,0,0);
        sm64_set_mario_rightleg_angle(marioId, 0,0,0);
        sm64_set_mario_rightankle_angle(marioId, 0,0,0);
        sm64_set_mario_rightfoot_angle(marioId, 0,0,0);
    }

    static int lastFade = 0;
    if (lastFade != TheCamera.GetScreenFadeStatus())
    {
        lastFade = TheCamera.GetScreenFadeStatus();
        if (!CGame::CanSeeOutSideFromCurrArea())
            loadCollisions(ped->GetPosition(), ped->m_nPhysicalFlags.bTouchingWater);
    }

    ticks += dt;
    while (ticks >= 1.f/30)
    {
        ticks -= 1.f/30;
        elapsedTicks++;
        if (safeTicks > 0) safeTicks--;

        memcpy(&marioLastPos, &marioCurrPos, sizeof(marioCurrPos));

        // handle input
        float length = sqrtf(pad->GetPedWalkLeftRight() * pad->GetPedWalkLeftRight() + pad->GetPedWalkUpDown() * pad->GetPedWalkUpDown()) / 128.f;
        if (length > 1) length = 1;
        if (walkState) length *= 0.65f;

        float angle;
        if ((marioState.action & ACT_GROUP_MASK) == ACT_GROUP_SUBMERGED)
            angle = atan2(-pad->GetPedWalkUpDown(), -pad->GetPedWalkLeftRight());
        else
            angle = atan2(pad->GetPedWalkUpDown(), pad->GetPedWalkLeftRight());

        if (!overrideWithCJAI)
        {
            marioInput.stickX = -cosf(angle) * length;
            marioInput.stickY = -sinf(angle) * length;
            marioInput.buttonA = jumpState;
            marioInput.buttonB = attackState;
            marioInput.buttonZ = duckState;
            marioInput.camLookX = TheCamera.m_fCamFrontXNorm * (pad->GetLookBehindForPed() ? 1 : -1);
            marioInput.camLookZ = TheCamera.m_fCamFrontYNorm * (pad->GetLookBehindForPed() ? -1 : 1);

            if (marioState.action == ACT_DRIVING_VEHICLE) sm64_set_mario_action(marioId, ACT_FREEFALL);

            // control CJ crouch
            if (marioState.action == ACT_CROUCHING || marioState.action == ACT_START_CROUCHING ||
                marioState.action == ACT_CRAWLING || marioState.action == ACT_START_CRAWLING || marioState.action == ACT_STOP_CRAWLING)
            {
                if (CTaskSimpleDuck2::CanPedDuck(ped) && !ped->m_pIntelligence->m_TaskMgr.GetTaskSecondary(TASK_SECONDARY_DUCK))
                    ped->m_pIntelligence->SetTaskDuckSecondary(0);
            }
            else if (ped->m_pIntelligence->m_TaskMgr.GetTaskSecondary(TASK_SECONDARY_DUCK))
                ped->m_pIntelligence->ClearTaskDuckSecondary();
        }
        else
        {
            memset(&marioInput, 0, sizeof(marioInput));

            float faceangle = ped->GetHeading() + M_PI;
            if (faceangle > M_PI) faceangle -= M_PI*2;

            CVector pos = ped->GetPosition();
            if (!ped->m_nPedFlags.bInVehicle) pos.z -= 1.f;
            CVector sm64pos(pos.x / MARIO_SCALE, pos.z / MARIO_SCALE, -pos.y / MARIO_SCALE);

            if (ped->m_nPedFlags.bInVehicle && overrideWithCJPos)
            {
                bool tiltForward = ped->m_pVehicle->m_nNumPassengers && (ped->m_pVehicle->m_nVehicleClass == VEHICLE_BIKE || ped->m_pVehicle->m_nVehicleClass == VEHICLE_BMX);

                float orX = asinf(ped->GetMatrix()->up.z);
                float cosx = cosf(orX);
                float cosy = CLAMP(ped->GetMatrix()->at.z / cosx, 0, 1);
                float orY = acosf(cosy);
                if (ped->GetMatrix()->right.z < 0) orY = -orY;
                /*
                float orX = ped->GetMatrix()->up.z;
                float orY = ped->GetMatrix()->right.z;
                */


                sm64_set_mario_angle(marioId, -orX, faceangle, -orY);
                sm64_set_mario_torsoangle(marioId, (tiltForward ? 0.4f : 0), 0, 0);
                sm64_set_mario_position(marioId, sm64pos.x, sm64pos.y, sm64pos.z);
                if (marioState.action != ACT_DRIVING_VEHICLE) sm64_set_mario_action(marioId, ACT_DRIVING_VEHICLE);
            }
            else if (!ped->m_nPedFlags.bInVehicle)
            {
                // in a cutscene, or CJ walking towards a car
                CVector2D spd(ped->m_vecMoveSpeed);
                if (ped->m_pContactEntity && (ped->m_pContactEntity->m_nType == ENTITY_TYPE_OBJECT || ped->m_pContactEntity->m_nType == ENTITY_TYPE_VEHICLE))
                {
                    spd.x = (int)spd.x;
                    spd.y = (int)spd.y;
                    CVector2D spd2(((CObject*)ped->m_pContactEntity)->m_vecMoveSpeed);
                    spd2.x = (int)spd2.x;
                    spd2.y = (int)spd2.y;
                    spd -= spd2;
                }

                float spdMag = (!carDoor || cjHasControl) ? spd.Magnitude()*17.5f : 0.9f;
                if (spdMag >= 1) spdMag = 0.9f;

                marioInput.camLookX = sinf(faceangle);
                marioInput.camLookZ = cosf(faceangle);
                marioInput.stickX = 0;
                marioInput.stickY = -spdMag;
                sm64_set_mario_faceangle(marioId, faceangle);

                if (!carDoor || cjHasControl)
                {
                    sm64_set_mario_position(marioId, sm64pos.x, sm64pos.y, sm64pos.z);
                    if (marioState.action == ACT_DRIVING_VEHICLE) sm64_set_mario_action(marioId, ACT_FREEFALL);
                }
            }
        }

        // Mario head rotation
        if (ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[TASK_SECONDARY_IK])
        {
            overrideHeadAngle = true;
            CTaskSimpleIKManager* task = static_cast<CTaskSimpleIKManager*>(ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[TASK_SECONDARY_IK]);
            CTaskSimpleIKLookAt* lookAt = static_cast<CTaskSimpleIKLookAt*>(task->m_pIKChainTasks[0]);

            if (lookAt)
            {
                // the math here was a bitch to get right...
                CVector targetPos = (lookAt->m_bEntityExist) ? lookAt->m_pEntity->GetPosition() : lookAt->m_offsetPos;

                // part 1: yaw
                float atanRes = atan2f(targetPos.y - marioCurrPos.y, targetPos.x - marioCurrPos.x) + M_PI_2;
                if (atanRes > M_PI) atanRes -= M_PI*2;

                float ang = atanRes - marioState.angle[1];
                if (ang < -M_PI) ang += M_PI*2;
                if (ang > M_PI) ang -= M_PI*2;

                headAngleTarget[1] = std::clamp(ang, (float)-M_PI_2+0.35f, (float)M_PI_2-0.35f);

                // part 2: pitch
                if (!ped->m_nPedFlags.bInVehicle)
                {
                    RwV3d headPosRw = targetPos.ToRwV3d();
                    if (lookAt->m_bEntityExist && lookAt->m_pEntity->m_nType == ENTITY_TYPE_PED)
                    {
                        const auto hier = GetAnimHierarchyFromSkinClump(lookAt->m_pEntity->m_pRwClump);
                        if (hier)
                            headPosRw = *RwMatrixGetPos(&RpHAnimHierarchyGetMatrixArray(hier)[RpHAnimIDGetIndex(hier, BONE_NECK)]);
                        else
                            ((CPed*)lookAt->m_pEntity)->GetBonePosition(headPosRw, BONE_NECK, false);
                    }
                    float dist = DistanceBetweenPoints(CVector2D(marioCurrPos), CVector2D(headPosRw)) * 2.f;
                    int Zsign = -sign(headPosRw.z - marioCurrPos.z);
                    headAngleTarget[0] = (dist) ? (M_PI_2 * Zsign) / dist : 0;
                    if (headAngleTarget[0] < -M_PI || headAngleTarget[0] > M_PI) headAngleTarget[0] = 0;
                }
            }
        }
        else if (!ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_NAMED_ANIM) && !ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[TASK_SECONDARY_IK])
            overrideHeadAngle = false;

        if (!overrideHeadAngle)
            for (int i=0; i<2; i++) headAngleTarget[i] = 0;

        if (!CCutsceneMgr::ms_running)
        {
            for (int i=0; i<2; i++) headAngle[i] += (headAngleTarget[i] - headAngle[i])/5.f;
            sm64_set_mario_headangle(marioId, headAngle[0], headAngle[1], 0);
        }

        if (!cjHasControl)
        {
            bool modifiedAngle = (headAngleTarget[0] || headAngleTarget[1]);
            if (!modifiedAngle && marioState.action == ACT_FIRST_PERSON) sm64_set_mario_action(marioId, ACT_IDLE);
            else if (modifiedAngle && marioState.action == ACT_IDLE) sm64_set_mario_action(marioId, ACT_FIRST_PERSON);
        }
        else if (!ped->m_nPedFlags.bInVehicle && (marioState.action == ACT_IDLE || marioState.action == ACT_PANTING))
            sm64_set_mario_action(marioId, ACT_FIRST_PERSON);

        // health
        if (hp <= 0)
            sm64_mario_kill(marioId);
        else if ((marioState.action & ACT_GROUP_MASK) == ACT_GROUP_SUBMERGED)
            sm64_set_mario_health(marioId, 0x880);
        else
        {
            if (marioState.health < 0x100) sm64_set_mario_action(marioId, ACT_SPAWN_SPIN_AIRBORNE);
            sm64_set_mario_health(marioId, (hp <= 0.1f) ? 0x200 : 0x880);
        }

        // water level
        if (CGame::CanSeeWaterFromCurrArea())
            sm64_set_mario_water_level(marioId, (ped->m_nPhysicalFlags.bTouchingWater) ? ped->m_pPlayerData->m_fWaterHeight/MARIO_SCALE : INT16_MIN);
        else
            sm64_set_mario_water_level(marioId, INT16_MIN);

        // fire
        if (ped->m_pFire && hp > 0)
        {
            if (marioState.action != ACT_BURNING_GROUND && marioState.action != ACT_BURNING_FALL && marioState.action != ACT_BURNING_JUMP)
                sm64_set_mario_action_arg(marioId, ACT_BURNING_JUMP, 1);
            else if (marioState.burnTimer <= 1)
                ped->m_pFire->Extinguish();
        }

        // mario damage -> CJ
        static uint8_t lastHurtCounter = 0;
        if (marioState.hurtCounter && !lastHurtCounter && !overrideWithCJAI)
        {
            lastHurtCounter = marioState.hurtCounter;
            // 64 is 0x40, 2176 is 0x880 (full mario HP), from libsm64 decomp/game/mario.c
            float damage = (64.f * marioState.hurtCounter) / 2176.f * ped->m_fMaxHealth;

            ped->m_fHealth -= damage;
            if (ped->m_fHealth < 0) ped->m_fHealth = 0;
        }
        else if (!marioState.hurtCounter && lastHurtCounter)
            lastHurtCounter = 0;


        ////// handle CJ ped tasks //////
        marioPedTasks(ped, marioId);

        // handles loaded objects, vehicles and peds
        for (int i=0; i<MAX_OBJS; i++)
        {
            LoadedPed& objPed = loadedPeds[i];
            if (objPed.ID != UINT_MAX)
            {
                CPed* entPed = (CPed*)objPed.ent;
                CVector pedPos = entPed->GetPosition();

                if (entPed->m_bRemoveFromWorld || !entPed->m_bIsVisible || !entPed->IsAlive())
                {
                    sm64_object_delete(objPed.ID);
                    objPed.ent = nullptr;
                    objPed.ID = UINT_MAX;
                }
                else
                {
                    float dist = DistanceBetweenPoints(CVector2D(pedPos.x, pedPos.y), CVector2D(marioInterpPos.x, marioInterpPos.y));
                    float dist3D = DistanceBetweenPoints(pedPos, marioInterpPos);
                    if (objPed.cachedPos.x != pedPos.x || objPed.cachedPos.y != pedPos.y || objPed.cachedPos.z != pedPos.z)
                    {
                        sm64_object_move(objPed.ID, pedPos.x/MARIO_SCALE, pedPos.z/MARIO_SCALE, -pedPos.y/MARIO_SCALE);
                        objPed.cachedPos = pedPos;
                    }
                    if ( ((!(marioState.action & ACT_FLAG_AIR) && dist <= 1.25f) || (marioState.action & ACT_FLAG_AIR && dist3D <= 1.f)) && sm64_mario_attack(marioId, pedPos.x/MARIO_SCALE, pedPos.z/MARIO_SCALE, -pedPos.y/MARIO_SCALE, 0))
                    {
                        /**
                            dirs:
                            0 - backward
                            1 - forward right
                            2 - forward left
                            3 - backward left
                            8 - forward
                        */
                        int damage = 20;
                        int dir = 0;
                        eWeaponType weapon = WEAPON_UNARMED;
                        ePedPieceTypes piece = (ePedPieceTypes)0; // default

                        if (marioState.action == ACT_JUMP_KICK || marioState.action & ACT_FLAG_BUTT_OR_STOMACH_SLIDE)
                        {
                            // knock ped over to the ground
                            damage += 10;
                            weapon = WEAPON_SHOTGUN;
                            piece = (ePedPieceTypes)3; // torso piece
                        }
                        else if (marioState.action == ACT_GROUND_POUND)
                        {
                            // crack everything in this poor ped's body
                            damage = (marioState.velocity[1] < 0) ? damage+50 : 0;
                            weapon = (eWeaponType)54; // WEAPON_FALL
                        }
                        if (marioState.flags & MARIO_METAL_CAP)
                        {
                            damage += 200;
                            weapon = (eWeaponType)51; // WEAPON_EXPLOSION
                        }

                        if (damage)
                            CWeapon::GenerateDamageEvent(entPed, ped, weapon, damage, (ePedPieceTypes)3, dir);
                    }
                }
            }


            LoadedSurface& obj = loadedObjects[i];

            if (!obj.ent) continue;
            if (obj.ent->m_bRemoveFromWorld || !obj.ent->m_bIsVisible)
            {
                sm64_surface_object_delete(obj.ID);
                obj.ent = nullptr;
                obj.ID = UINT_MAX;
                continue;
            }

            CVector ePos = obj.ent->GetPosition();
            float heading = obj.ent->GetHeading();
            float orX = obj.ent->GetMatrix()->up.z;
            float orY = obj.ent->GetMatrix()->right.z;

            if (obj.cachedPos.x != ePos.x || obj.cachedPos.y != ePos.y || obj.cachedPos.z != ePos.z)
            {
                obj.cachedPos = ePos;
                obj.transform.position[0] = ePos.x/MARIO_SCALE;
                obj.transform.position[1] = ePos.z/MARIO_SCALE;
                obj.transform.position[2] = -ePos.y/MARIO_SCALE;
                sm64_surface_object_move(obj.ID, &obj.transform);
            }
            if (obj.cachedRotations[0] != orX || obj.cachedRotations[1] != heading || obj.cachedRotations[2] != orY)
            {
                obj.cachedRotations[0] = orX;
                obj.cachedRotations[1] = heading;
                obj.cachedRotations[2] = orY;
                obj.transform.eulerRotation[0] = -orX * 180.f / M_PI;
                obj.transform.eulerRotation[1] = -heading * 180.f / M_PI;
                obj.transform.eulerRotation[2] = -orY * 180.f / M_PI;
                sm64_surface_object_move(obj.ID, &obj.transform);
            }
        }

        sm64_mario_tick(marioId, &marioInput, &marioState, &marioGeometry);

        marioCurrPos = CVector(marioState.position[0] * MARIO_SCALE, -marioState.position[2] * MARIO_SCALE, marioState.position[1] * MARIO_SCALE);

        marioRenderUpdateGeometry(marioGeometry);

        memcpy(&marioInterpPos, &marioCurrPos, sizeof(marioCurrPos));

        if (!cjHasControl)
        {
            ped->SetHeading(marioState.angle[1] + M_PI);
            ped->m_fCurrentRotation = marioState.angle[1] + M_PI;
        }

        if (DistanceBetweenPoints(marioBlocksPos, marioCurrPos) > 64 || sm64_surface_find_floor_height(marioState.position[0], marioState.position[1], marioState.position[2]) == FLOOR_LOWER_LIMIT)
            loadCollisions(marioCurrPos, ped->m_nPhysicalFlags.bTouchingWater);
        if (elapsedTicks % 30 == 0)
            loadNonBuildings(marioCurrPos);
    }

    marioInterpPos.x = lerp(marioLastPos.x, marioCurrPos.x, ticks / (1.f/30));
    marioInterpPos.y = lerp(marioLastPos.y, marioCurrPos.y, ticks / (1.f/30));
    marioInterpPos.z = lerp(marioLastPos.z, marioCurrPos.z, ticks / (1.f/30));

    CVector pos = ped->GetPosition();
    if (!cjHasControl)
    {
        TheCamera.SetPosn(TheCamera.GetPosition() - CVector(0,0, 0.5f));
        if (!overrideWithCJAI ||
			ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER) ||
			ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER)
		)
			ped->SetPosn(marioInterpPos + CVector(0, 0, 1.f));
    }
    else if (overrideWithCJPos)
    {
        if (ped->m_nPedFlags.bInVehicle)
            pos.z -= (ped->m_pVehicle->m_pHandlingData->m_bIsBike) ? 0.3f : 0.35f;
        else
            pos.z -= 1;

        pos.x += cos(ped->GetHeading() + M_PI_2) * 0.3f;
        pos.y += sin(ped->GetHeading() + M_PI_2) * 0.3f;
    }

    marioRenderInterpolate(marioGeometry, ticks, overrideWithCJPos, pos);

    ////// handle CJ ped tasks that require not being inside the 1.f/30 loop //////
    marioPedTasksMaxFPS(ped, marioId);

    ////// process weapon (stealth, etc)
    marioProcessWeapon(ped);
}

void marioProcessWeapon(CPlayerPed* player)
{
    if (!marioSpawned()) return;

    CPlayerData* playerData = player->m_pPlayerData;
    CPedIntelligence* intelligence = player->m_pIntelligence;
    CTaskManager* taskManager = &intelligence->m_TaskMgr;
    CPad* pad = player->GetPadFromPlayer();

    CWeaponInfo* weaponInfo = CWeaponInfo::GetWeaponInfo(player->m_aWeapons[player->m_nActiveWeaponSlot].m_eWeaponType, player->GetWeaponSkill());

    if (taskManager->FindActiveTaskByType(TASK_SIMPLE_USE_GUN))
        return;

    CPed* targetEntity = nullptr;
    if (!player->m_pTargetedObject) {
        if (TheCamera.m_bUseMouse3rdPerson && player->m_pPlayerTargettedPed) {
            targetEntity = player->m_pPlayerTargettedPed;
        }
    } else {
        if (player->m_pTargetedObject->m_nType == ENTITY_TYPE_PED) {
            targetEntity = reinterpret_cast<CPed*>(player->m_pTargetedObject);
        }
    }

    static float rightArmAngle = 0;
    float targetAngle = 0;
    unsigned int animGroupID = weaponInfo->m_nAnimToPlay;
    if (targetEntity && pad->GetTarget() && playerData->m_fMoveBlendRatio < 1.9f && player->m_nMoveState != PEDMOVE_SPRINT &&
        !taskManager->GetTaskSecondary(TASK_SECONDARY_ATTACK) && animGroupID != ANIM_GROUP_DEFAULT && intelligence->TestForStealthKill(targetEntity, false))
    {
        targetAngle = -M_PI/1.5f;
    }

    rightArmAngle += (targetAngle - rightArmAngle) / 5.f;
    sm64_set_mario_rightarm_angle(marioId, rightArmAngle, 0, 0);
}

void marioTestAnim()
{
    if (!marioSpawned()) return;

    if (marioState.action == ACT_CUSTOM_ANIM)
        sm64_set_mario_action(marioId, ACT_IDLE);
    else if (marioState.action == ACT_IDLE || marioState.action == ACT_PANTING)
    {
        sm64_set_mario_action(marioId, ACT_CUSTOM_ANIM);
        sm64_set_mario_animation(marioId, MARIO_ANIM_CUSTOM_TEST);
    }
}
