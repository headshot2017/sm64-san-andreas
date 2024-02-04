#include "mario_render.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "plugin.h"
#include "CCutsceneMgr.h"
#include "CPlayerPed.h"
#include "CPointLights.h"
#include "CScene.h"
#include "CTimeCycle.h"
#include "CHud.h"
#include "CWeapon.h"
#include "CWeaponInfo.h"
#include "Fx_c.h"

#include "d3d9_funcs.h"


// General rendering stuff
bool inited = false;
bool surfaceDebugger = false;
int marioTexturedCount = 0;
int triangles = 0;

// Immediate Mode API
RwIm3DVertex marioInterpGeo[SM64_GEO_MAX_TRIANGLES * 3];
RwIm3DVertex marioCurrGeoPos[SM64_GEO_MAX_TRIANGLES * 3];
RwIm3DVertex marioLastGeoPos[SM64_GEO_MAX_TRIANGLES * 3];
RwImVertexIndex marioTextureIndices[SM64_GEO_MAX_TRIANGLES * 3];
RwUInt32 marioOriginalColor[SM64_GEO_MAX_TRIANGLES * 3];

// Retained Mode API
RpClump* marioClump;
RpAtomic* marioAtomic;
//RwFrame* marioFrameClump;
RwFrame* marioFrameAtomic;
RwObject* weaponObj;
RwObject* phoneObj;
RpMaterial* marioMaterialTextured;
RpMaterial* marioMaterial;


void marioRenderToggleDebug()
{
    surfaceDebugger = !surfaceDebugger;
}

void marioRenderInit()
{
    if (inited) return;
    inited = true;
    triangles = SM64_GEO_MAX_TRIANGLES;

    memset(&marioTextureIndices, 0, sizeof(marioTextureIndices));
    memset(&marioOriginalColor, 0, sizeof(marioOriginalColor));
    marioTexturedCount = 0;

    // Retained Mode API
    weaponObj = nullptr;

    // create Mario RenderWare clump.
    // steps from RenderWare's "geometry" example
    RwRGBA noColor = {0,0,0,255};
    RwV3d noV3d = {0.f, 0.f, 0.f};
    RwTexCoords noTexCoord = {1.f, 1.f};

    // create materials
    marioMaterialTextured = RpMaterialCreate();
    marioMaterial = RpMaterialCreate();
    RpMaterialSetTexture(marioMaterialTextured, marioTextureRW);

    // create geometry, and get the vertices, normals, tex coords, triangle indexes list and colors
    // geometry is created in locked state so it can be modified
    RpGeometry* marioRpGeometry = RpGeometryCreate(SM64_GEO_MAX_TRIANGLES*3, SM64_GEO_MAX_TRIANGLES, rpGEOMETRYLIGHT | rpGEOMETRYNORMALS | rpGEOMETRYTEXTURED | rpGEOMETRYPRELIT | rpGEOMETRYMODULATEMATERIALCOLOR);
    RpMorphTarget* morphTarget = marioRpGeometry->morphTarget;
    RwV3d* vlist = morphTarget->verts;
    RwV3d* nlist = morphTarget->normals;
    RwTexCoords* texCoord = marioRpGeometry->texCoords[0];
    RpTriangle* tlist = marioRpGeometry->triangles;
    RwRGBA* colors = marioRpGeometry->preLitLum;

    // the geometry will have two materials
    // one for textured triangles (mario's face, buttons on his overalls, etc),
    // and one for solid-color triangles

    // initialize the geometry with default values
    for (int i=0; i<SM64_GEO_MAX_TRIANGLES*3; i++)
    {
        *vlist++ = noV3d;
        *nlist++ = noV3d;
        *texCoord++ = noTexCoord;
        *colors++ = noColor;

        if (i < SM64_GEO_MAX_TRIANGLES)
        {
            RpGeometryTriangleSetVertexIndices(marioRpGeometry, tlist, i*3+0, i*3+1, i*3+2);
            RpGeometryTriangleSetMaterial(marioRpGeometry, tlist++, (i > 0) ? marioMaterial : marioMaterialTextured);
        }
    }

    RwSphere boundingSphere;
    RpMorphTargetCalcBoundingSphere(morphTarget, &boundingSphere);
    morphTarget->boundingSphere = boundingSphere;

    // unlock the geometry when done
    RpGeometryUnlock(marioRpGeometry);

    // create the clump itself and assign a single frame to it
    marioClump = RpClumpCreate();
    //marioFrameClump = RwFrameCreate();
    //marioClump->object.parent = (void*)marioFrameClump; // RpClumpSetFrame

    // create the atomic and make a new separate frame for it
    marioAtomic = RpAtomicCreate();
    marioFrameAtomic = RwFrameCreate();
    RpAtomicSetFrame(marioAtomic, marioFrameAtomic);

    // assign the geometry to the atomic. this will make a new copy of the geometry
    // then assign the atomic to the clump
    RpAtomicSetGeometry(marioAtomic, marioRpGeometry, 0);
    RpClumpAddAtomic(marioClump, marioAtomic);

    // we can now delete the geometry
    RpGeometryDestroy(marioRpGeometry);

    // finally, add clump to world
    RpWorldAddClump(Scene.m_pRpWorld, marioClump);
}

void marioRenderDestroy()
{
    if (!inited) return;
    inited = false;

    // Retained Mode API only

    RpWorldRemoveClump(Scene.m_pRpWorld, marioClump);
    RpClumpRemoveAtomic(marioClump, marioAtomic);
    RpAtomicDestroy(marioAtomic);
    RpClumpDestroy(marioClump);

    RpMaterialDestroy(marioMaterialTextured);
    RpMaterialDestroy(marioMaterial);
}

void marioRenderUpdateGeometry(const SM64MarioGeometryBuffers& marioGeometry)
{
    if (!inited) return;

    marioTexturedCount = 0;
    memcpy(marioLastGeoPos, marioCurrGeoPos, sizeof(marioCurrGeoPos));

    // lock the geometry in order to modify it
    RpGeometryLock(marioAtomic->geometry, rpGEOMETRYLOCKALL);

    RpMorphTarget* morphTarget = marioAtomic->geometry->morphTarget;
    RwV3d* vlist = morphTarget->verts;
    RwV3d* nlist = morphTarget->normals;
    RwTexCoords* texCoord = marioAtomic->geometry->texCoords[0];
    RwRGBA* colors = marioAtomic->geometry->preLitLum;
    RpTriangle* tlist = marioAtomic->geometry->triangles;
    memset(tlist, 0, sizeof(RpTriangle) * SM64_GEO_MAX_TRIANGLES);

    for (uint16_t i=0; i<marioGeometry.numTrianglesUsed*3 && i<triangles*3; i++)
    {
        bool hasTexture = (marioGeometry.uv[i*2+0] != 1 && marioGeometry.uv[i*2+1] != 1);

        RwUInt32 col = RWRGBALONG((int)(marioGeometry.color[i*3+0]*255), (int)(marioGeometry.color[i*3+1]*255), (int)(marioGeometry.color[i*3+2]*255), 255);
        if (hasTexture)
        {
            marioOriginalColor[marioTexturedCount] = col;
            marioTextureIndices[marioTexturedCount++] = i;
        }

        // Immediate Mode API
        marioCurrGeoPos[i].color = col;

        marioCurrGeoPos[i].u = marioGeometry.uv[i*2+0];
        marioCurrGeoPos[i].v = marioGeometry.uv[i*2+1];

        marioCurrGeoPos[i].objNormal.x = marioGeometry.normal[i*3+0];
        marioCurrGeoPos[i].objNormal.y = -marioGeometry.normal[i*3+2];
        marioCurrGeoPos[i].objNormal.z = marioGeometry.normal[i*3+1];

        marioCurrGeoPos[i].objVertex.x = marioGeometry.position[i*3+0] * MARIO_SCALE;
        marioCurrGeoPos[i].objVertex.y = -marioGeometry.position[i*3+2] * MARIO_SCALE;
        marioCurrGeoPos[i].objVertex.z = marioGeometry.position[i*3+1] * MARIO_SCALE;

        // Retained Mode API
        RwTexCoords texCoords = {1.f, 1.f};
        RwRGBA marioColor = {(RwUInt8)(marioGeometry.color[i*3+0]*255), (RwUInt8)(marioGeometry.color[i*3+1]*255), (RwUInt8)(marioGeometry.color[i*3+2]*255), 255};
        *vlist++ = marioCurrGeoPos[i].objVertex;
        *nlist++ = marioCurrGeoPos[i].objNormal;
        *texCoord++ = texCoords;
        *colors++ = marioColor;
        if (i < marioGeometry.numTrianglesUsed)
        {
            RpGeometryTriangleSetVertexIndices(marioAtomic->geometry, tlist, i*3+0, i*3+1, i*3+2);
            tlist->matIndex = 1;
            *tlist++;
        }
    }

    // add extra vertices for indices that have textures assigned to them
    uint16_t texInd = marioGeometry.numTrianglesUsed*3;
    for (int i=0; i<marioTexturedCount; i+=3)
    {
        RwRGBA white = {255,255,255,255};
        RwTexCoords texCoords1 = {marioCurrGeoPos[marioTextureIndices[i+0]].u, marioCurrGeoPos[marioTextureIndices[i+0]].v};
        RwTexCoords texCoords2 = {marioCurrGeoPos[marioTextureIndices[i+1]].u, marioCurrGeoPos[marioTextureIndices[i+1]].v};
        RwTexCoords texCoords3 = {marioCurrGeoPos[marioTextureIndices[i+2]].u, marioCurrGeoPos[marioTextureIndices[i+2]].v};

        *texCoord++ = texCoords1;
        *texCoord++ = texCoords2;
        *texCoord++ = texCoords3;
        for (int j=0; j<3; j++)
        {
            *vlist++ = marioCurrGeoPos[marioTextureIndices[i+j]].objVertex;
            *nlist++ = marioCurrGeoPos[marioTextureIndices[i+j]].objNormal;
            *colors++ = white;
        }
        RpGeometryTriangleSetVertexIndices(marioAtomic->geometry, tlist, texInd, texInd+1, texInd+2);
        tlist->matIndex = 0;
        *tlist++;
        texInd += 3;
    }
    RpGeometryUnlock(marioAtomic->geometry);

    memcpy(marioInterpGeo, marioCurrGeoPos, sizeof(marioCurrGeoPos));
}

void marioRenderInterpolate(const SM64MarioGeometryBuffers& marioGeometry, float& ticks, bool useCJPos, const CVector& pos)
{
    if (!inited) return;

    RpGeometryLock(marioAtomic->geometry, rpGEOMETRYLOCKVERTICES);
    RpMorphTarget* morphTarget = marioAtomic->geometry->morphTarget;
    RwV3d* vlist = morphTarget->verts;
    for (int i=0; i<marioGeometry.numTrianglesUsed*3 && i<triangles*3; i++)
    {
        marioInterpGeo[i].objVertex.x = lerp(marioLastGeoPos[i].objVertex.x, marioCurrGeoPos[i].objVertex.x, ticks / (1.f/30));
        marioInterpGeo[i].objVertex.y = lerp(marioLastGeoPos[i].objVertex.y, marioCurrGeoPos[i].objVertex.y, ticks / (1.f/30));
        marioInterpGeo[i].objVertex.z = lerp(marioLastGeoPos[i].objVertex.z, marioCurrGeoPos[i].objVertex.z, ticks / (1.f/30));
        if (useCJPos)
        {
            marioInterpGeo[i].objVertex.x = marioInterpGeo[i].objVertex.x - marioInterpPos.x + pos.x;
            marioInterpGeo[i].objVertex.y = marioInterpGeo[i].objVertex.y - marioInterpPos.y + pos.y;
            marioInterpGeo[i].objVertex.z = marioInterpGeo[i].objVertex.z - marioInterpPos.z + pos.z;
        }

        *vlist++ = marioInterpGeo[i].objVertex;
    }
    for (int i=0; i<marioTexturedCount*3; i++)
        *vlist++ = marioInterpGeo[marioTextureIndices[i]].objVertex;
    RpGeometryUnlock(marioAtomic->geometry);
}

void marioRender()
{
    if (!marioSpawned() || CCutsceneMgr::ms_running || !inited) return;

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);

    CPlayerPed* ped = FindPlayerPed();
    CVector pos = ped->GetPosition();


    // Retained Mode API

    // get lighting values for Mario
    float dynLight = 0.f;
    float generatedLightings = CPointLights::GenerateLightsAffectingObject(&pos, &dynLight, ped);
    float lightingMultiplier = (ped->GetLightingFromCol(true) * (1.0f - 0.05f) + 0.05f) * generatedLightings;
    SetAmbientColours();
    ActivateDirectional();
    SetLightColoursForPedsCarsAndObjects(lightingMultiplier);

    // modify the materials' properties
    RwSurfaceProperties surfProp;
    memset(&surfProp, 0, sizeof(surfProp));
    surfProp.ambient = 1.f / (-lightingMultiplier*2.f);
    surfProp.diffuse = surfProp.ambient * -1.5f;
    surfProp.specular = 0;
    if (surfProp.ambient < -0.5f) surfProp.ambient = -0.5f;
    if (surfProp.diffuse > 1.f) surfProp.diffuse = 1.f;

    // modify the materials' colors depending on daylight and light multiplier
    RwUInt16 r = (RwUInt16)(255*CTimeCycle::GetAmbientRed_Obj() * lightingMultiplier);
    RwUInt16 g = (RwUInt16)(255*CTimeCycle::GetAmbientGreen_Obj() * lightingMultiplier);
    RwUInt16 b = (RwUInt16)(255*CTimeCycle::GetAmbientBlue_Obj() * lightingMultiplier);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    RwRGBA newColor = {
        (RwUInt8)r,
        (RwUInt8)g,
        (RwUInt8)b,
        255
    };

    // apply changes
    RpMaterialSetColor(marioMaterialTextured, &newColor);
    RpMaterialSetColor(marioMaterial, &newColor);
    RpMaterialSetSurfaceProperties(marioMaterialTextured, &surfProp);
    RpMaterialSetSurfaceProperties(marioMaterial, &surfProp);

    // Mario model is rendered by changing the CJ ped's RW atomic, clump and RWobject values below in marioRenderPed()


    // Immediate Mode API
    bool immediateDrawn = false;

    /*
    if (RwIm3DTransform(marioInterpGeo, SM64_GEO_MAX_TRIANGLES*3, 0, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA | rwIM3D_VERTEXUV))
    {
        immediateDrawn = true;
        for (int i=0; i<marioTexturedCount; i++) marioInterpGeo[marioTextureIndices[i]].color = marioOriginalColor[i];
        RwD3D9SetTexture(0, 0);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, marioIndices, marioGeometry.numTrianglesUsed*3);

        for (int i=0; i<marioTexturedCount; i++) marioInterpGeo[marioTextureIndices[i]].color = RWRGBALONG(255, 255, 255, 255);
        RwD3D9SetTexture(marioTextureRW, 0);
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, marioTextureIndices, marioTexturedCount);
    }
    */

#ifdef _DEBUG
    if (surfaceDebugger)
    {
        uint32_t objectCount = 0, vertexCount = 0;
        SM64LoadedSurfaceObject* objects = sm64_get_all_surface_objects(&objectCount);

        for (uint32_t i=0; i<objectCount; i++)
            vertexCount += objects[i].surfaceCount*3;

        RwIm3DVertex surfaceVertices[vertexCount];
        memset(surfaceVertices, 0, sizeof(RwIm3DVertex) * vertexCount);
        uint16_t surfaceIndices[vertexCount];

        uint32_t startInd = 0;

        for (uint32_t i=0; i<objectCount; i++)
        {
            for (uint32_t j=0; j<objects[i].surfaceCount; j++)
            {
                uint8_t r = ((0.5 + 0.25 * 1) * (.5+.5*objects[i].engineSurfaces[j].normal.x)) * 255;
                uint8_t g = ((0.5 + 0.25 * 1) * (.5+.5*objects[i].engineSurfaces[j].normal.y)) * 255;
                uint8_t b = ((0.5 + 0.25 * 1) * (.5+.5*objects[i].engineSurfaces[j].normal.z)) * 255;

                for (int k=0; k<3; k++)
                {
                    surfaceIndices[startInd + j*3+k] = startInd + j*3+k;

                    surfaceVertices[startInd + j*3+k].objNormal.x = objects[i].engineSurfaces[j].normal.x;
                    surfaceVertices[startInd + j*3+k].objNormal.y = objects[i].engineSurfaces[j].normal.y;
                    surfaceVertices[startInd + j*3+k].objNormal.z = objects[i].engineSurfaces[j].normal.z;

                    surfaceVertices[startInd + j*3+k].color = RWRGBALONG(r, g, b, 128);
                }

                surfaceVertices[startInd + j*3+0].objVertex.x = objects[i].engineSurfaces[j].vertex1[0] * MARIO_SCALE;
                surfaceVertices[startInd + j*3+0].objVertex.y = -objects[i].engineSurfaces[j].vertex1[2] * MARIO_SCALE;
                surfaceVertices[startInd + j*3+0].objVertex.z = objects[i].engineSurfaces[j].vertex1[1] * MARIO_SCALE;

                surfaceVertices[startInd + j*3+1].objVertex.x = objects[i].engineSurfaces[j].vertex2[0] * MARIO_SCALE;
                surfaceVertices[startInd + j*3+1].objVertex.y = -objects[i].engineSurfaces[j].vertex2[2] * MARIO_SCALE;
                surfaceVertices[startInd + j*3+1].objVertex.z = objects[i].engineSurfaces[j].vertex2[1] * MARIO_SCALE;

                surfaceVertices[startInd + j*3+2].objVertex.x = objects[i].engineSurfaces[j].vertex3[0] * MARIO_SCALE;
                surfaceVertices[startInd + j*3+2].objVertex.y = -objects[i].engineSurfaces[j].vertex3[2] * MARIO_SCALE;
                surfaceVertices[startInd + j*3+2].objVertex.z = objects[i].engineSurfaces[j].vertex3[1] * MARIO_SCALE;
            }

            startInd += objects[i].surfaceCount*3;
        }

        if (RwIm3DTransform(surfaceVertices, vertexCount, 0, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            immediateDrawn = true;

            RwD3D9SetTexture(0, 0);
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, surfaceIndices, vertexCount);
        }
    }
#endif // _DEBUG

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
    if (immediateDrawn) RwIm3DEnd();
}


RpAtomic* pedAtomic;
RpClump* pedClump;

void marioRenderPed(CPed* ped)
{
    if (!marioSpawned() || !ped->IsPlayer()) return;

    if (RwObjectGetType(ped->m_pRwObject) == rpCLUMP)
    {
        pedClump = ped->m_pRwClump;
        ped->m_pRwClump = marioClump;
    }
    else if (RwObjectGetType(ped->m_pRwObject) == rpATOMIC)
    {
        pedAtomic = ped->m_pRwAtomic;
        ped->m_pRwAtomic = marioAtomic;
    }
}

void marioRenderPedReset(CPed* ped)
{
    if (!marioSpawned() || !ped->IsPlayer()) return;

    if (RwObjectGetType(ped->m_pRwObject) == rpCLUMP)
        ped->m_pRwClump = pedClump;
    else if (RwObjectGetType(ped->m_pRwObject) == rpATOMIC)
        ped->m_pRwAtomic = pedAtomic;
}

FxQuality_e oldQuality;
void marioPreRender(CPed* ped)
{
    if (!marioSpawned() || !ped->IsPlayer()) return;

    oldQuality = g_fx.GetFxQuality();
    g_fx.SetFxQuality(FXQUALITY_LOW);
}

void marioPreRenderReset(CPed* ped)
{
    if (!marioSpawned() || !ped->IsPlayer()) return;

    g_fx.SetFxQuality(oldQuality);
}

void marioRenderWeapon()
{
    if (!marioSpawned() || !FindPlayerPed()) return;
    CPlayerPed* ped = FindPlayerPed();

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,          RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,         RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,            RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,             RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,            RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(20));

    float div = 45*3+1;

    bool usingPhone = phoneObj && ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_USE_MOBILE_PHONE);
    if (weaponObj || usingPhone)
    {
        ped->SetupLighting();
        RpClump* weaponClump = usingPhone ? (RpClump*)phoneObj : (RpClump*)weaponObj;
        RwFrame* weaponFrame = (RwFrame*)weaponClump->object.parent;
        const CWeapon& activeWeapon = ped->m_aWeapons[ped->m_nActiveWeaponSlot];
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(activeWeapon.m_eWeaponType, ped->GetWeaponSkill());
        bool aiming = !!(ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_USE_GUN)); // kinda hacky
        bool cantAim = (aiming && marioState.rightArmAngle[1] == 0); // kinda hacky

        RwV3d yawAxis = {0,0,1};
        RwV3d pitchAxisRw = {0, 1, 0};
        RwV3d rollAxisRw = {1, 0, 0};

        // mario geo triangle indexes:
        // 445 to 490: left hand
        // 555 to 600: right hand
        // multiply by 3 to get the individiual triangle's 3 vertices in marioInterpGeo

        // set position to center of mario's right hand
        RwV3d rwpos = {0};
        for (int i=555*3; i<=600*3; i++)
        {
            rwpos.x += marioInterpGeo[i].objVertex.x;
            rwpos.y += marioInterpGeo[i].objVertex.y;
            rwpos.z += marioInterpGeo[i].objVertex.z;
        }
        rwpos.x /= div;
        rwpos.y /= div;
        rwpos.z /= div;

        /*char abuf[256];
        sprintf(abuf, "%d %d %d %d %d %d %d %d %d %d",
				info->m_nFlags.bAimWithArm, info->m_nFlags.bTwinPistol, info->m_nFlags.b1stPerson, info->m_nFlags.bCanAim, info->m_nFlags.bContinuosFire, info->m_nFlags.bHeavy, info->m_nFlags.bExpands, info->m_nFlags.bLongReload, info->m_nFlags.bMoveAim, info->m_nFlags.bMoveFire);
		CHud::SetMessage(abuf);*/

        if (!info->m_nFlags.bAimWithArm)
		{
			// heavy weapon
			if (sideAnimWeaponIDs.count(activeWeapon.m_eWeaponType))
			{
				// weapon facing to the side
				RwV3d a = {-0.035f * cosf(marioState.angle[1] + (M_PI/2.f)), -0.035f * sinf(marioState.angle[1] + (M_PI/2.f)), 0.001f};
				RwFrameRotate(weaponFrame, &yawAxis, (marioState.angle[1])/M_PI*180 - (aiming ? 85 : 18), rwCOMBINEREPLACE);
				RwFrameRotate(weaponFrame, &pitchAxisRw, 6 + (marioState.rightArmAngle[1]/M_PI*90) + (marioState.torsoAngle[2]/M_PI*90), rwCOMBINEPRECONCAT);
				if (aiming) RwFrameTranslate(weaponFrame, &a, rwCOMBINEPOSTCONCAT);
			}
			else if (shoulderWeaponIDs.count(activeWeapon.m_eWeaponType))
			{
				// rocket launcher pointing downwards/propped on shoulder
				RwFrameRotate(weaponFrame, &yawAxis, (marioState.angle[1])/M_PI*180 - 90, rwCOMBINEREPLACE);
				if (!aiming) RwFrameRotate(weaponFrame, &pitchAxisRw, 20, rwCOMBINEPRECONCAT);
			}
			else if (heavyWeaponIDs.count(activeWeapon.m_eWeaponType))
			{
				// minigun, flamethrower, extinguisher
				RwFrameRotate(weaponFrame, &yawAxis, (marioState.angle[1])/M_PI*180 - 90, rwCOMBINEREPLACE);
				if (aiming) RwFrameRotate(weaponFrame, &pitchAxisRw, 30, rwCOMBINEPRECONCAT);
			}
			else
				RwFrameRotate(weaponFrame, &yawAxis, (marioState.angle[1])/M_PI*180 - 90, rwCOMBINEREPLACE);

            if (ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_SIMPLE_STEALTH_KILL))
                RwFrameRotate(weaponFrame, &rollAxisRw, -90-45, rwCOMBINEPRECONCAT);
		}
        else
        {
            RwFrameRotate(weaponFrame, &yawAxis, (marioState.angle[1] + marioState.rightArmAngle[2])/M_PI*180 - 90, rwCOMBINEREPLACE);
            if (aiming)
            {
                if (!cantAim) RwFrameRotate(weaponFrame, &yawAxis, 25, rwCOMBINEPRECONCAT);
                RwFrameRotate(weaponFrame, &pitchAxisRw, (cantAim) ? -90 : (-marioState.rightArmAngle[0]/M_PI*180 - 90), rwCOMBINEPRECONCAT);
            }
            else
                RwFrameRotate(weaponFrame, &pitchAxisRw, 90, rwCOMBINEPRECONCAT);
            RwFrameRotate(weaponFrame, &rollAxisRw, -90, rwCOMBINEPRECONCAT);
        }
        RwFrameTranslate(weaponFrame, &rwpos, rwCOMBINEPOSTCONCAT);

        ped->SetGunFlashAlpha(false);
        RwFrameUpdateObjects(weaponFrame);
        RpClumpRender(weaponClump);

        if (info->m_nFlags.bTwinPistol)
        {
            // render weapon on left hand

            // set position to center of mario's left hand
            memset(&rwpos, 0, sizeof(RwV3d));
            for (int i=445*3; i<=490*3; i++)
            {
                rwpos.x += marioInterpGeo[i].objVertex.x;
                rwpos.y += marioInterpGeo[i].objVertex.y;
                rwpos.z += marioInterpGeo[i].objVertex.z;
            }
            rwpos.x /= div;
            rwpos.y /= div;
            rwpos.z /= div;

            RwFrameRotate(weaponFrame, &yawAxis, (marioState.angle[1] + marioState.leftArmAngle[2])/M_PI*180 - 90, rwCOMBINEREPLACE);
            if (aiming)
            {
                if (!cantAim) RwFrameRotate(weaponFrame, &yawAxis, -25, rwCOMBINEPRECONCAT);
                RwFrameRotate(weaponFrame, &pitchAxisRw, (cantAim) ? 90 : (-marioState.leftArmAngle[0]/M_PI*180 - 90), rwCOMBINEPRECONCAT);
            }
            else
                RwFrameRotate(weaponFrame, &pitchAxisRw, 90, rwCOMBINEPRECONCAT);
            RwFrameRotate(weaponFrame, &rollAxisRw, 90, rwCOMBINEPRECONCAT);
            RwFrameTranslate(weaponFrame, &rwpos, rwCOMBINEPOSTCONCAT);

            ped->SetGunFlashAlpha(true);
            RwFrameUpdateObjects(weaponFrame);
            RpClumpRender(weaponClump);
        }

        ped->ResetGunFlashAlpha();
    }
}
