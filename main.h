#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "RenderWare.h"

#include <libsm64.h>

extern bool loaded;
extern uint8_t* marioTexture;
extern RwImVertexIndex marioIndices[SM64_GEO_MAX_TRIANGLES * 3];

#endif // MAIN_H_INCLUDED
