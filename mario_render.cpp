#include "mario_render.h"

#include "plugin.h"
#include "CCutsceneMgr.h"
#include "CPlayerPed.h"
#include "CPointLights.h"
#include "CScene.h"
#include "CTimeCycle.h"

#include "d3d9_funcs.h"


// General rendering stuff
bool surfaceDebugger = false;
int marioTexturedCount = 0;

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


void marioRenderToggleDebug()
{
    surfaceDebugger = !surfaceDebugger;
}

void marioRenderInit()
{
    memset(&marioTextureIndices, 0, sizeof(marioTextureIndices));
    memset(&marioOriginalColor, 0, sizeof(marioOriginalColor));
    marioTexturedCount = 0;

    // Immediate Mode API requires no steps here

    // Retained Mode API
    // create Mario RenderWare clump.
    // steps from RenderWare's "geometry" example
    RwRGBA noColor = {0,0,0,255};
    RwV3d noV3d = {0.f, 0.f, 0.f};
    RwTexCoords noTexCoord = {1.f, 1.f};

    // create materials
    RpMaterial* marioMaterialTextured = RpMaterialCreate();
    RpMaterial* marioMaterial = RpMaterialCreate();
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
    marioRpGeometry->matList.numMaterials = 2;
    marioRpGeometry->matList.materials = (RpMaterial**)malloc(sizeof(RpMaterial*) * marioRpGeometry->matList.numMaterials);
    marioRpGeometry->matList.materials[0] = RpMaterialClone(marioMaterialTextured);
    marioRpGeometry->matList.materials[1] = RpMaterialClone(marioMaterial);

    // initialize the geometry with default values
    for (int i=0; i<SM64_GEO_MAX_TRIANGLES*3; i++)
    {
        *vlist++ = noV3d;
        *nlist++ = noV3d;
        *texCoord++ = noTexCoord;
        *colors++ = noColor;

        if (i < SM64_GEO_MAX_TRIANGLES)
        {
            tlist->matIndex = 0;
            for (int j=0; j<3; j++) tlist->vertIndex[j] = i*3+j;
            *tlist++;
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

    // we can now delete the geometry and the materials
    RpGeometryDestroy(marioRpGeometry);
    RpMaterialDestroy(marioMaterialTextured);
    RpMaterialDestroy(marioMaterial);

    // finally, add clump to world
    RpWorldAddClump(Scene.m_pRpWorld, marioClump);
}

void marioRenderDestroy()
{
    // Retained Mode API only

    // remove clump from world, and remove atomic from clump
    RpWorldRemoveClump(Scene.m_pRpWorld, marioClump);
    RpClumpRemoveAtomic(marioClump, marioAtomic);
    RpAtomicSetFrame(marioAtomic, nullptr);

    //RwFrameDestroy(marioFrameClump);
    RwFrameDestroy(marioFrameAtomic);

    // destroy them
    RpClumpDestroy(marioClump);
    RpAtomicDestroy(marioAtomic);
}

void marioRenderUpdateGeometry(const SM64MarioGeometryBuffers& marioGeometry)
{
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

    for (uint16_t i=0; i<marioGeometry.numTrianglesUsed*3; i++)
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
            tlist->matIndex = 1;
            for (int j=0; j<3; j++) tlist->vertIndex[j] = i*3+j;
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
        tlist->matIndex = 0;
        for (int j=0; j<3; j++)
        {
            *vlist++ = marioCurrGeoPos[marioTextureIndices[i+j]].objVertex;
            *nlist++ = marioCurrGeoPos[marioTextureIndices[i+j]].objNormal;
            *colors++ = white;
            tlist->vertIndex[j] = texInd++;
        }
        *tlist++;
    }
    RpGeometryUnlock(marioAtomic->geometry);

    memcpy(marioInterpGeo, marioCurrGeoPos, sizeof(marioCurrGeoPos));
}

void marioRenderInterpolate(const SM64MarioGeometryBuffers& marioGeometry, float& ticks, bool useCJPos, const CVector& pos)
{
    RpGeometryLock(marioAtomic->geometry, rpGEOMETRYLOCKVERTICES);
    RpMorphTarget* morphTarget = marioAtomic->geometry->morphTarget;
    RwV3d* vlist = morphTarget->verts;
    for (int i=0; i<marioGeometry.numTrianglesUsed*3; i++)
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
    if (!marioSpawned() || CCutsceneMgr::ms_running) return;

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
    RpMaterialSetColor(marioAtomic->geometry->matList.materials[0], &newColor);
    RpMaterialSetColor(marioAtomic->geometry->matList.materials[1], &newColor);
    RpMaterialSetSurfaceProperties(marioAtomic->geometry->matList.materials[0], &surfProp);
    RpMaterialSetSurfaceProperties(marioAtomic->geometry->matList.materials[1], &surfProp);

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
RwObject* pedObject;

void marioRenderPed(CPed* ped)
{
    if (!marioSpawned() || !ped->IsPlayer()) return;

    pedAtomic = ped->m_pRwAtomic;
    pedClump = ped->m_pRwClump;
    pedObject = ped->m_pRwObject;
    ped->m_pRwAtomic = marioAtomic;
    ped->m_pRwClump = marioClump;
    ped->m_pRwObject = &marioClump->object;
}

void marioRenderPedReset(CPed* ped)
{
    if (!marioSpawned() || !ped->IsPlayer()) return;

    ped->m_pRwAtomic = pedAtomic;
    ped->m_pRwClump = pedClump;
    ped->m_pRwObject = pedObject;
}
