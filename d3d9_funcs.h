#ifndef D3D9_FUNCS_H_INCLUDED
#define D3D9_FUNCS_H_INCLUDED

#include "RenderWare.h"

extern RwTexture* marioTextureRW;
extern RwTexture* marioShadowRW;

void initD3D();
void destroyD3D();
void draw();

#endif // D3D9_FUNCS_H_INCLUDED
