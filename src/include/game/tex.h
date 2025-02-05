#ifndef IN_GAME_TEX_H
#define IN_GAME_TEX_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

void texInit(void);

void surfaceReset(void);

void texSetBitstring(u8 *arg0);
s32 texReadBits(s32 arg0);
void texReset(void);

s32 texGetMask(s32 value);
s32 tex0f0b33f8(s32 width, s32 height, s32 lod);
s32 tex0f0b3468(s32 width, s32 height, s32 lod);
s32 tex0f0b34d8(s32 width, s32 height, s32 lod);
s32 tex0f0b3548(s32 width, s32 height, s32 lod);
void texSetRenderMode(Gfx **gdlptr, s32 arg1, s32 numcycles, s32 arg3);
void texLoadFromConfig(struct textureconfig *config);
void texSelect(Gfx **gdl, struct textureconfig *tconfig, u32 arg2, s32 arg3, u32 arg4, bool arg5, struct texturething *arg6);

void tex0f173a08(void);
bool texSetLutMode(u32 value);
bool tex0f173a70(s32 index, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8, s32 arg9, s32 arg10);
bool tex0f173b8c(s32 index, s32 uls, s32 ult, s32 lrs, s32 lrt);
s32 texGetWidthAtLod(struct texloadthing *arg0, s32 lod);
s32 texGetHeightAtLod(struct texloadthing *arg0, s32 lod);
s32 texGetLineSizeInBytes(struct texloadthing *arg0, s32 lod);
s32 texGetSizeInBytes(struct texloadthing *arg0, s32 lod);
void tex0f173e50(struct texloadthing *arg0, s32 *arg1, s32 *arg2);
s32 tex0f173f18(s32 arg0);
s32 tex0f173f48(s32 arg0);
u32 tex0f173f78(void);
u32 tex0f1742e4(void);
Gfx *tex0f1743a0(Gfx *gdl, struct texloadthing *arg1, s32 arg2);
Gfx *tex0f1747a4(Gfx *gdl, struct texloadthing *arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, u32 arg7);
u32 tex0f174b54(void);
u32 tex0f174f30(void);
Gfx *tex0f1751e4(Gfx *gdl, struct texloadthing *arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
u32 tex0f175308(void);
u32 tex0f175490(void);
u32 tex0f1755dc(void);
u32 tex0f17563c(void);
s32 tex0f1756c0(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
void tex0f175ef4(s32 arg0, s32 arg1, s32 arg2);

#endif
