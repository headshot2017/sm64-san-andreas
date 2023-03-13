#include "d3d9_funcs.h"

#include <libsm64.h>
extern "C" {
    #include <decomp/include/PR/ultratypes.h>
    #include <decomp/include/audio_defines.h>
}

#include "main.h"
#include "mario_render.h"

RwTexture* marioTextureRW = nullptr;

void initD3D()
{
    RwRaster* raster = RwRasterCreate(SM64_TEXTURE_WIDTH, SM64_TEXTURE_HEIGHT, 32, rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
    RwUInt8* pixels = RwRasterLock(raster, 0, 1);
    memcpy(pixels, marioTexture, 4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);
    RwRasterUnlock(raster);

    marioTextureRW = RwTextureCreate(raster);
    RwTextureSetFilterMode(marioTextureRW, rwFILTERLINEAR);
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
    marioRender();
}
