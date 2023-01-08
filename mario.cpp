#include "mario.h"
#include "plugin.h"
#include "CHud.h"
#include "CCamera.h"
#include "CPlayerPed.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

extern "C" {
    #include <decomp/include/surface_terrains.h>
}

#include "d3d9_funcs.h"
#include "main.h"

#define lerp(a, b, amnt) a + (b - a) * amnt

SM64MarioState marioState;
SM64MarioInputs marioInput;
SM64MarioGeometryBuffers marioGeometry;
CVector marioLastPos, marioCurrPos, marioInterpPos;
RwIm3DVertex marioInterpGeo[SM64_GEO_MAX_TRIANGLES * 3];
RwIm3DVertex marioCurrGeoPos[SM64_GEO_MAX_TRIANGLES * 3];
RwIm3DVertex marioLastGeoPos[SM64_GEO_MAX_TRIANGLES * 3];
RwImVertexIndex marioTextureIndices[SM64_GEO_MAX_TRIANGLES * 3];
RwUInt32 marioOriginalColor[SM64_GEO_MAX_TRIANGLES * 3];
int marioTexturedCount = 0;
int marioId = -1;
float ticks = 0;

bool marioSpawned()
{
    return marioId != -1;
}

void marioSpawn()
{
    if (marioSpawned() || !FindPlayerPed()) return;

    // SM64 <--> GTA SA coordinates translation:
    // Y and Z coordinates must be swapped. SM64 up coord is Y+ and GTA SA is Z+
    // Mario model must also be unmirrored by making SM64 Z coordinate / GTA-SA Y coordinate negative
    // GTA SA -> SM64: divide scale
    // SM64 -> GTA SA: multiply scale
    CVector pos = FindPlayerPed()->GetPosition();
    pos.z -= 1;
    CVector sm64pos(pos.x / MARIO_SCALE, pos.z / MARIO_SCALE, -pos.y / MARIO_SCALE);

    uint32_t surfaceCount = 2;
    SM64Surface surfaces[surfaceCount];

    for (uint32_t i=0; i<surfaceCount; i++)
    {
      surfaces[i].type = SURFACE_DEFAULT;
      surfaces[i].force = 0;
      surfaces[i].terrain = TERRAIN_STONE;
    }

    int width = 16384;

    surfaces[surfaceCount-2].vertices[0][0] = sm64pos.x + width;	surfaces[surfaceCount-2].vertices[0][1] = sm64pos.y;	surfaces[surfaceCount-2].vertices[0][2] = sm64pos.z + width;
    surfaces[surfaceCount-2].vertices[1][0] = sm64pos.x - width;	surfaces[surfaceCount-2].vertices[1][1] = sm64pos.y;	surfaces[surfaceCount-2].vertices[1][2] = sm64pos.z - width;
    surfaces[surfaceCount-2].vertices[2][0] = sm64pos.x - width;	surfaces[surfaceCount-2].vertices[2][1] = sm64pos.y;	surfaces[surfaceCount-2].vertices[2][2] = sm64pos.z + width;

    surfaces[surfaceCount-1].vertices[0][0] = sm64pos.x - width;	surfaces[surfaceCount-1].vertices[0][1] = sm64pos.y;	surfaces[surfaceCount-1].vertices[0][2] = sm64pos.z - width;
    surfaces[surfaceCount-1].vertices[1][0] = sm64pos.x + width;	surfaces[surfaceCount-1].vertices[1][1] = sm64pos.y;	surfaces[surfaceCount-1].vertices[1][2] = sm64pos.z + width;
    surfaces[surfaceCount-1].vertices[2][0] = sm64pos.x + width;	surfaces[surfaceCount-1].vertices[2][1] = sm64pos.y;	surfaces[surfaceCount-1].vertices[2][2] = sm64pos.z - width;

    sm64_static_surfaces_load(surfaces, surfaceCount);

    marioId = sm64_mario_create(sm64pos.x, sm64pos.y, sm64pos.z);
    if (!marioSpawned())
    {
        char buf[256];
        sprintf(buf, "Failed to spawn Mario at %.2f %.2f %.2f", pos.x, pos.y, pos.z);
        CHud::SetHelpMessage(buf, false, false, true);
        return;
    }

    ticks = 0;
    marioGeometry.position = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.normal   = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.color    = new float[9 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.uv       = new float[6 * SM64_GEO_MAX_TRIANGLES];
    marioGeometry.numTrianglesUsed = 0;
    memset(&marioInput, 0, sizeof(marioInput));
    memset(&marioTextureIndices, 0, sizeof(marioTextureIndices));
    memset(&marioOriginalColor, 0, sizeof(marioOriginalColor));
    marioTexturedCount = 0;
    CHud::SetHelpMessage("Mario spawned", false, false, true);
    FindPlayerPed()->DeactivatePlayerPed(0);
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
    CHud::SetHelpMessage("Mario destroyed", false, false, true);
    FindPlayerPed()->ReactivatePlayerPed(0);
}

void marioTick(float dt)
{
    if (!marioSpawned() || !FindPlayerPed()) return;
    CPlayerPed* ped = FindPlayerPed();
    CPad* pad = ped->GetPadFromPlayer();

    ticks += dt;
    while (ticks >= 1.f/30)
    {
        ticks -= 1.f/30;

        marioTexturedCount = 0;
        memcpy(&marioLastPos, &marioCurrPos, sizeof(marioCurrPos));
        memcpy(marioLastGeoPos, marioCurrGeoPos, sizeof(marioCurrGeoPos));

        float angle = atan2(pad->GetPedWalkUpDown(), pad->GetPedWalkLeftRight());
        float length = sqrtf(pad->GetPedWalkLeftRight() * pad->GetPedWalkLeftRight() + pad->GetPedWalkUpDown() * pad->GetPedWalkUpDown()) / 128.f;
        if (length > 1) length = 1;

        marioInput.stickX = -cosf(angle) * length;
        marioInput.stickY = -sinf(angle) * length;
        marioInput.buttonA = pad->GetJump();
        marioInput.buttonB = pad->GetMeleeAttack();
        marioInput.buttonZ = pad->GetDuck();
        marioInput.camLookX = TheCamera.GetPosition().x/MARIO_SCALE - marioState.position[0];
        marioInput.camLookZ = -TheCamera.GetPosition().y/MARIO_SCALE - marioState.position[2];

        sm64_mario_tick(marioId, &marioInput, &marioState, &marioGeometry);

        marioCurrPos = CVector(marioState.position[0] * MARIO_SCALE, -marioState.position[2] * MARIO_SCALE, marioState.position[1] * MARIO_SCALE);

        for (int i=0; i<marioGeometry.numTrianglesUsed*3; i++)
        {
            bool hasTexture = (marioGeometry.uv[i*2+0] != 1 && marioGeometry.uv[i*2+1] != 1);

            RwUInt32 col = RWRGBALONG((int)(marioGeometry.color[i*3+0]*255), (int)(marioGeometry.color[i*3+1]*255), (int)(marioGeometry.color[i*3+2]*255), 255);
            if (hasTexture)
            {
                marioOriginalColor[marioTexturedCount] = col;
                marioTextureIndices[marioTexturedCount++] = i;
            }
            marioCurrGeoPos[i].color = col;

            marioCurrGeoPos[i].u = marioGeometry.uv[i*2+0];
            marioCurrGeoPos[i].v = marioGeometry.uv[i*2+1];

            marioCurrGeoPos[i].objNormal.x = marioGeometry.normal[i*3+0];
            marioCurrGeoPos[i].objNormal.y = -marioGeometry.normal[i*3+2];
            marioCurrGeoPos[i].objNormal.z = marioGeometry.normal[i*3+1];

            marioCurrGeoPos[i].objVertex.x = marioGeometry.position[i*3+0] * MARIO_SCALE;
            marioCurrGeoPos[i].objVertex.y = -marioGeometry.position[i*3+2] * MARIO_SCALE;
            marioCurrGeoPos[i].objVertex.z = marioGeometry.position[i*3+1] * MARIO_SCALE;
        }

        memcpy(&marioInterpPos, &marioCurrPos, sizeof(marioCurrPos));
        memcpy(marioInterpGeo, marioCurrGeoPos, sizeof(marioCurrGeoPos));
    }

    marioInterpPos.x = lerp(marioLastPos.x, marioCurrPos.x, ticks / (1./30));
    marioInterpPos.y = lerp(marioLastPos.y, marioCurrPos.y, ticks / (1./30));
    marioInterpPos.z = lerp(marioLastPos.z, marioCurrPos.z, ticks / (1./30));
    ped->SetPosn(marioInterpPos + CVector(0, 0, 0.5f));
    for (int i=0; i<marioGeometry.numTrianglesUsed*3; i++)
    {
        marioInterpGeo[i].objVertex.x = lerp(marioLastGeoPos[i].objVertex.x, marioCurrGeoPos[i].objVertex.x, ticks / (1./30));
        marioInterpGeo[i].objVertex.y = lerp(marioLastGeoPos[i].objVertex.y, marioCurrGeoPos[i].objVertex.y, ticks / (1./30));
        marioInterpGeo[i].objVertex.z = lerp(marioLastGeoPos[i].objVertex.z, marioCurrGeoPos[i].objVertex.z, ticks / (1./30));
    }
}

void marioRender()
{
    if (!marioSpawned()) return;

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);

    if (RwIm3DTransform(marioInterpGeo, SM64_GEO_MAX_TRIANGLES*3, 0, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA | rwIM3D_VERTEXUV))
    {
        for (int i=0; i<marioTexturedCount; i++) marioInterpGeo[marioTextureIndices[i]].color = marioOriginalColor[i];
        RwD3D9SetTexture(0, 0);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, marioIndices, marioGeometry.numTrianglesUsed*3);

        for (int i=0; i<marioTexturedCount; i++) marioInterpGeo[marioTextureIndices[i]].color = RWRGBALONG(255, 255, 255, 255);
        RwD3D9SetTexture(marioTextureRW, 0);
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, marioTextureIndices, marioTexturedCount);

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwIm3DEnd();
    }
}
