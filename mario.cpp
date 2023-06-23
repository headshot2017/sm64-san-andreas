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
#include "CTaskSimpleIKLookAt.h"
#include "CTaskSimpleIKManager.h"
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

SM64MarioState marioState;
SM64MarioInputs marioInput;
SM64MarioGeometryBuffers marioGeometry;

CVector marioLastPos, marioCurrPos, marioInterpPos, marioBlocksPos;
uint32_t elapsedTicks = 0;
uint32_t safeTicks = 0;
int marioId = -1;
float ticks = 0;
static float headAngle[2] = {0};


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

void loadCollisions(const CVector& pos)
{
    int width = 16384;
    SM64Surface surfaces[2];
    CVector sm64pos(pos.x / MARIO_SCALE, -70 / MARIO_SCALE, -pos.y / MARIO_SCALE);

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
    CVector pos = FindPlayerPed()->GetPosition();
    pos.z -= 1;

    loadCollisions(pos);

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
    marioGeometry.position = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.normal   = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.color    = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.uv       = new float[6 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.numTrianglesUsed = 0;
    memset(&marioInput, 0, sizeof(marioInput));
    memset(headAngle, 0, sizeof(headAngle));

    CPlayerPed* ped = FindPlayerPed();
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
    }
}

void marioTick(float dt)
{
    if (!marioSpawned() || !FindPlayerPed()) return;
    CPlayerPed* ped = FindPlayerPed();
    ped->m_pShadowData = nullptr;
    bool carDoor = ped->m_pIntelligence->IsPedGoingForCarDoor();
    float hp = ped->m_fHealth / ped->m_fMaxHealth;

    CPad* pad = ped->GetPadFromPlayer();
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

    bool cjHasControl = (pad->bPlayerSafe || ped->m_nPedFlags.bInVehicle || hp <= 0 || CEntryExitManager::mp_Active || safeTicks > 0);
    bool overrideWithCJPos = ((ped->m_nPedFlags.bInVehicle || hp <= 0) &&
                              !ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER) &&
                              !leavingCar &&
                              !ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_CAR_SLOW_BE_DRAGGED_OUT));
    bool overrideWithCJAI = (cjHasControl || carDoor ||
                             ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_ACHIEVE_HEADING) ||
                             ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_GO_TO_POINT_AND_STAND_STILL));

    if (cjHasControl)
    {
        if (pad->bPlayerSafe || ped->m_nPedFlags.bInVehicle || hp <= 0 || CEntryExitManager::mp_Active) safeTicks = 4;
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
        ped->m_nPhysicalFlags.bApplyGravity = 0;
        ped->m_nPhysicalFlags.bCanBeCollidedWith = 0;
        ped->m_nPhysicalFlags.bCollidable = 0;
        ped->m_nPhysicalFlags.bDisableMoveForce = 0;
        ped->m_nPhysicalFlags.bDisableTurnForce = !carDoor;
        ped->m_nPhysicalFlags.bDontApplySpeed = 0;
        ped->m_nAllowedAttackMoves = 0;
    }

    static bool lastCutsceneRunning = false;
    if (CCutsceneMgr::ms_running && !lastCutsceneRunning)
    {
        lastCutsceneRunning = true;
        marioSetPos(ped->GetPosition());
    }
    else if (!CCutsceneMgr::ms_running && lastCutsceneRunning)
        lastCutsceneRunning = false;

    static int lastFade = 0;
    if (lastFade != TheCamera.GetScreenFadeStatus())
    {
        lastFade = TheCamera.GetScreenFadeStatus();
        if (!CGame::CanSeeOutSideFromCurrArea())
            loadCollisions(ped->GetPosition());
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
            marioInput.camLookX = TheCamera.GetPosition().x/MARIO_SCALE - marioState.position[0];
            marioInput.camLookZ = -TheCamera.GetPosition().y/MARIO_SCALE - marioState.position[2];

            if (marioState.action == ACT_DRIVING_VEHICLE) sm64_set_mario_action(marioId, ACT_FREEFALL);
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
                float orX = ped->GetMatrix()->up.z;
                float orY = ped->GetMatrix()->right.z;
                sm64_set_mario_angle(marioId, -orX, faceangle, -orY);
                sm64_set_mario_torsoangle(marioId, (tiltForward ? 0.4f : 0), 0, 0);
                sm64_set_mario_position(marioId, sm64pos.x, sm64pos.y, sm64pos.z);
                if (marioState.action != ACT_DRIVING_VEHICLE) sm64_set_mario_action(marioId, ACT_DRIVING_VEHICLE);
            }
            else if (!ped->m_nPedFlags.bInVehicle)
            {
                // in a cutscene, or CJ walking towards a car
                CVector2D spd(ped->m_vecMoveSpeed);
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
        float headAngleTarget[2] = {0};
        if (ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[TASK_SECONDARY_IK])
        {
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
                    float dist = DistanceBetweenPoints(CVector2D(marioCurrPos), CVector2D(targetPos)) * 2.f;
                    int Zsign = -sign(targetPos.z - marioCurrPos.z);
                    headAngleTarget[0] = (dist) ? (M_PI_2 * Zsign) / dist : 0;
                    if (headAngleTarget[0] < -M_PI || headAngleTarget[0] > M_PI) headAngleTarget[0] = 0;
                }
            }
        }

        for (int i=0; i<2; i++) headAngle[i] += (headAngleTarget[i] - headAngle[i])/5.f;
        sm64_set_mario_headangle(marioId, headAngle[0], headAngle[1], 0);
        if (!cjHasControl)
        {
            bool modifiedAngle = (headAngleTarget[0] || headAngleTarget[1]);
            if (!modifiedAngle && marioState.action == ACT_FIRST_PERSON) sm64_set_mario_action(marioId, ACT_IDLE);
            else if (modifiedAngle && (marioState.action == ACT_IDLE || marioState.action == ACT_PANTING)) sm64_set_mario_action(marioId, ACT_FIRST_PERSON);
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
        if (marioState.hurtCounter && !lastHurtCounter)
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
            loadCollisions(marioCurrPos);
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
