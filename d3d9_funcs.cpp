#include "d3d9_funcs.h"

#include <libsm64.h>
extern "C" {
    #include <decomp/include/PR/ultratypes.h>
    #include <decomp/include/audio_defines.h>
}

#include "main.h"
#include "mario_render.h"
#include "mario_shadow.raw.h"

RwRaster* marioRaster;
RwRaster* marioShadowRaster;
RwTexture* marioTextureRW = nullptr;
RwTexture* marioShadowRW = nullptr;

void initD3D()
{
    marioRaster = RwRasterCreate(SM64_TEXTURE_WIDTH, SM64_TEXTURE_HEIGHT, 32, rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
    RwUInt8* pixels = RwRasterLock(marioRaster, 0, 1);
    memcpy(pixels, marioTexture, 4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);
    RwRasterUnlock(marioRaster);

    RwRaster* marioShadowRaster = RwRasterCreate(80, 80, 32, rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
    pixels = RwRasterLock(marioShadowRaster, 0, 1);
    memcpy(pixels, marioShadow, 4 * 80 * 80);
    RwRasterUnlock(marioShadowRaster);

    marioTextureRW = RwTextureCreate(marioRaster);
    RwTextureSetFilterMode(marioTextureRW, rwFILTERLINEAR);

    marioShadowRW = RwTextureCreate(marioShadowRaster);
    RwTextureSetFilterMode(marioShadowRW, rwFILTERLINEAR);
}

void destroyD3D()
{
    if (marioTextureRW)
    {
        RwTextureDestroy(marioTextureRW);
        marioTextureRW = nullptr;
    }
    if (marioShadowRW)
    {
        RwTextureDestroy(marioShadowRW);
        marioShadowRW = nullptr;
    }
}

void draw()
{
    marioRender();
}
