#include "d3d9_funcs.h"

#include "CSprite2d.h"

extern "C" {
    #include <libsm64.h>
    #include <decomp/include/PR/ultratypes.h>
    #include <decomp/include/audio_defines.h>
}

#include "main.h"

RwTexture* marioTextureRW = nullptr;
RwIm2DVertex marioVertex[4];

void setVertex(int i, float x, float y, float u, float v)
{
    marioVertex[i].x = x;
    marioVertex[i].y = y;
    marioVertex[i].u = u;
    marioVertex[i].v = v;
    marioVertex[i].z = CSprite2d::NearScreenZ + 0.0001f;
    marioVertex[i].rhw = CSprite2d::RecipNearClip;
    marioVertex[i].emissiveColor = RWRGBALONG(255, 255, 255, 255);
}

void initD3D()
{
    RwRaster* raster = RwRasterCreate(SM64_TEXTURE_WIDTH, SM64_TEXTURE_HEIGHT, 32, rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
    RwUInt8* pixels = RwRasterLock(raster, 0, 1);
    memcpy(pixels, marioTexture, 4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);
    RwRasterUnlock(raster);

    marioTextureRW = RwTextureCreate(raster);

    setVertex(0, 0, 0, 0, 0);
    setVertex(1, 0, 64, 0, 1);
    setVertex(2, 704, 0, 1, 0);
    setVertex(3, 704, 64, 1, 1);
}

void destroyD3D()
{
    if (marioTextureRW)
    {
        RwTextureDestroy(marioTextureRW);
        marioTextureRW = nullptr;
    }
}

void draw()
{
    RwD3D9SetTexture(marioTextureRW, 0);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, marioVertex, 4);
}
