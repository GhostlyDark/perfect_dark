#ifndef IN_GAME_GAME_0B28D0_H
#define IN_GAME_GAME_0B28D0_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

bool func0f0b28d0(struct model *model, struct modelfiledata *filedata);
void func0f0b2904(void);
struct model *modelInstantiate(struct modelfiledata *modelfiledata, bool arg1);
struct model *modelInstantiateWithoutAnim(struct modelfiledata *modelfiledata);
void modelFree(struct model *model);
struct model *modelInstantiateWithAnim(struct modelfiledata *modelfiledata);
void func0f0b32a0(struct model *model, struct modelnode *node, struct modelfiledata *headfiledata);
struct anim *func0f0b32e4(void);
void animTurnOff(struct anim *anim);

#endif
