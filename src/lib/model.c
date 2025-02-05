#include <ultra64.h>
#include "constants.h"
#include "game/game_096700.h"
#include "game/acosfasinf.h"
#include "game/quaternion.h"
#include "game/camera.h"
#include "game/floor.h"
#include "game/ceil.h"
#include "game/tex.h"
#include "game/gfxmemory.h"
#include "game/bg.h"
#include "game/file.h"
#include "bss.h"
#include "lib/rng.h"
#include "lib/mtx.h"
#include "lib/anim.h"
#include "lib/model.h"
#include "data.h"
#include "types.h"

#if VERSION >= VERSION_PAL_BETA
u8 var8005efb0_2 = 0;
#endif

u32 var8005efb0 = 0;

bool g_ModelDistanceDisabled = false;
f32 g_ModelDistanceScale = 1;
bool var8005efbc = false;
f32 var8005efc0 = 0;
bool (*var8005efc4)(struct model *model, struct modelnode *node) = NULL;

#if VERSION >= VERSION_PAL_BETA
bool var8005efd8_2 = false;
#endif

struct gfxvtx *(*g_ModelVtxAllocatorFunc)(s32 numvertices) = NULL;
void *var8005efcc = NULL; // pointer to a function

void modelSetDistanceChecksDisabled(bool disabled)
{
	g_ModelDistanceDisabled = disabled;
}

void modelSetDistanceScale(f32 scale)
{
	g_ModelDistanceScale = scale;
}

void modelSetVtxAllocatorFunc(struct gfxvtx *(*fn)(s32 numvertices))
{
	g_ModelVtxAllocatorFunc = fn;
}

s32 model0001a524(struct modelnode *node, s32 arg1)
{
	s32 index;
	union modelrodata *rodata1;
	union modelrodata *rodata2;
	union modelrodata *rodata3;

	while (node) {
		switch (node->type & 0xff) {
		case MODELNODETYPE_CHRINFO:
			rodata1 = node->rodata;
			return rodata1->chrinfo.mtxindex;
		case MODELNODETYPE_POSITION:
			rodata2 = node->rodata;
			return rodata2->position.pieces[arg1 == 0x200 ? 2 : (arg1 == 0x100 ? 1 : 0)];
		case MODELNODETYPE_POSITIONHELD:
			rodata3 = node->rodata;
			return rodata3->positionheld.unk0c;
		}

		node = node->parent;
	}

	return -1;
}

Mtxf *model0001a5cc(struct model *model, struct modelnode *node, s32 arg2)
{
	s32 index = model0001a524(node, arg2);

	if (index >= 0) {
		return &model->matrices[index];
	}

	return NULL;
}

Mtxf *model0001a60c(struct model *model)
{
	return model0001a5cc(model, model->filedata->rootnode, 0);
}

struct modelnode *model0001a634(struct model *model, s32 mtxindex)
{
	struct modelnode *node = model->filedata->rootnode;
	union modelrodata *rodata1;
	union modelrodata *rodata2;
	union modelrodata *rodata3;

	while (node) {
		switch (node->type & 0xff) {
		case MODELNODETYPE_CHRINFO:
			rodata1 = node->rodata;
			if (mtxindex == rodata1->chrinfo.mtxindex) {
				return node;
			}
			break;
		case MODELNODETYPE_POSITION:
			rodata2 = node->rodata;
			if (mtxindex == rodata2->position.pieces[0]
					|| mtxindex == rodata2->position.pieces[1]
					|| mtxindex == rodata2->position.pieces[2]) {
				return node;
			}
			break;
		case MODELNODETYPE_POSITIONHELD:
			rodata3 = node->rodata;
			if (mtxindex == rodata3->positionheld.unk0c) {
				return node;
			}
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node && node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}

	return NULL;
}

struct modelnode *model0001a740(struct modelnode *node)
{
	while (node) {
		u32 type = node->type & 0xff;

		if (type == MODELNODETYPE_CHRINFO
				|| type == MODELNODETYPE_POSITION
				|| type == MODELNODETYPE_POSITIONHELD) {
			break;
		}

		node = node->parent;
	}

	return node;
}

struct modelnode *model0001a784(struct modelnode *node)
{
	while ((node = node->parent)) {
		u32 type = node->type & 0xff;

		if (type == MODELNODETYPE_CHRINFO
				|| type == MODELNODETYPE_POSITION
				|| type == MODELNODETYPE_POSITIONHELD) {
			break;
		}
	}

	return node;
}

struct modelnode *model0001a7cc(struct modelnode *basenode)
{
	struct modelnode *node = basenode->child;

	while (node) {
		u32 type = node->type & 0xff;

		if (type == MODELNODETYPE_CHRINFO
				|| type == MODELNODETYPE_POSITION
				|| type == MODELNODETYPE_POSITIONHELD) {
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node == basenode) {
					node = NULL;
					break;
				}

				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}

	return node;
}

struct modelnode *model0001a85c(struct modelnode *basenode)
{
	struct modelnode *node = basenode;
	struct modelnode *next;
	u32 type;

	while (node) {
		if (node != basenode && node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node != basenode) {
					type = node->type & 0xff;

					if (type == MODELNODETYPE_CHRINFO
							|| type == MODELNODETYPE_POSITION
							|| type == MODELNODETYPE_POSITIONHELD) {
						node = NULL;
						break;
					}
				}

				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}

			if (!node) {
				break;
			}
		}

		type = node->type & 0xff;

		if (type == MODELNODETYPE_CHRINFO
				|| type == MODELNODETYPE_POSITION
				|| type == MODELNODETYPE_POSITIONHELD) {
			break;
		}
	}

	return node;
}

struct modelnode *modelGetPart(struct modelfiledata *modelfiledata, s32 partnum)
{
	s32 upper;
	s32 lower;
	u32 i;
	s16 *partnums;

	if (modelfiledata->numparts == 0) {
		return NULL;
	}

	partnums = (s16 *)&modelfiledata->parts[modelfiledata->numparts];
	lower = 0;
	upper = modelfiledata->numparts;

	while (upper >= lower) {
		i = (lower + upper) / 2;

		if (partnum == partnums[i]) {
			return modelfiledata->parts[i];
		}

		if (partnum < partnums[i]) {
			upper = i - 1;
		} else {
			lower = i + 1;
		}
	}

	return NULL;
}

void *modelGetPartRodata(struct modelfiledata *modelfiledata, s32 partnum)
{
	struct modelnode *node = modelGetPart(modelfiledata, partnum);

	if (node) {
		return node->rodata;
	}

	return NULL;
}

f32 model0001a9e8(struct model *model)
{
	Mtxf *mtx = model0001a60c(model);

	if (mtx) {
		return -mtx->m[3][2];
	}

	return 0;
}

#if VERSION >= VERSION_NTSC_1_0
// ntsc-beta has this function in another file
void *modelGetNodeRwData(struct model *model, struct modelnode *node)
{
	u32 index = 0;
	union modelrwdata **rwdatas = model->rwdatas;

	switch (node->type & 0xff) {
	case MODELNODETYPE_CHRINFO:
		index = node->rodata->chrinfo.rwdataindex;
		break;
	case MODELNODETYPE_DL:
		index = node->rodata->dl.rwdataindex;
		break;
	case MODELNODETYPE_DISTANCE:
		index = node->rodata->distance.rwdataindex;
		break;
	case MODELNODETYPE_TOGGLE:
		index = node->rodata->toggle.rwdataindex;
		break;
	case MODELNODETYPE_REORDER:
		index = node->rodata->reorder.rwdataindex;
		break;
	case MODELNODETYPE_0B:
		index = node->rodata->type0b.rwdataindex;
		break;
	case MODELNODETYPE_GUNFIRE:
		index = node->rodata->gunfire.rwdataindex;
		break;
	case MODELNODETYPE_HEADSPOT:
		index = node->rodata->headspot.rwdataindex;
		break;
	}

	while (node->parent) {
		node = node->parent;

		if ((node->type & 0xff) == MODELNODETYPE_HEADSPOT) {
			struct modelrwdata_headspot *tmp = modelGetNodeRwData(model, node);
			rwdatas = tmp->rwdatas;
			break;
		}
	}

	return &rwdatas[index];
}
#endif

void modelNodeGetPosition(struct model *model, struct modelnode *node, struct coord *pos)
{
	switch (node->type & 0xff) {
	case MODELNODETYPE_CHRINFO:
		{
			struct modelrwdata_chrinfo *rwdata = modelGetNodeRwData(model, node);
			pos->x = rwdata->pos.x;
			pos->y = rwdata->pos.y;
			pos->z = rwdata->pos.z;
		}
		break;
	case MODELNODETYPE_POSITION:
		{
			struct modelrodata_position *rodata = &node->rodata->position;
			pos->x = rodata->pos.x;
			pos->y = rodata->pos.y;
			pos->z = rodata->pos.z;
		}
		break;
	case MODELNODETYPE_POSITIONHELD:
		{
			struct modelrodata_positionheld *rodata = &node->rodata->positionheld;
			pos->x = rodata->pos.x;
			pos->y = rodata->pos.y;
			pos->z = rodata->pos.z;
		}
		break;
	default:
		pos->x = 0;
		pos->y = 0;
		pos->z = 0;
		break;
	}
}

void modelNodeSetPosition(struct model *model, struct modelnode *node, struct coord *pos)
{
	switch (node->type & 0xff) {
	case MODELNODETYPE_CHRINFO:
		{
			struct modelrwdata_chrinfo *rwdata = modelGetNodeRwData(model, node);
			struct coord diff[1];

			diff[0].x = pos->x - rwdata->pos.x;
			diff[0].z = pos->z - rwdata->pos.z;

			rwdata->pos.x = pos->x;
			rwdata->pos.y = pos->y;
			rwdata->pos.z = pos->z;

			rwdata->unk24.x += diff[0].x; rwdata->unk24.z += diff[0].z;
			rwdata->unk34.x += diff[0].x; rwdata->unk34.z += diff[0].z;
			rwdata->unk40.x += diff[0].x; rwdata->unk40.z += diff[0].z;
			rwdata->unk4c.x += diff[0].x; rwdata->unk4c.z += diff[0].z;
		}
		break;
	case MODELNODETYPE_POSITION:
		{
			struct modelrodata_position *rodata = &node->rodata->position;
			rodata->pos.x = pos->x;
			rodata->pos.y = pos->y;
			rodata->pos.z = pos->z;
		}
		break;
	case MODELNODETYPE_POSITIONHELD:
		{
			struct modelrodata_positionheld *rodata = &node->rodata->positionheld;
			rodata->pos.x = pos->x;
			rodata->pos.y = pos->y;
			rodata->pos.z = pos->z;
		}
		break;
	}
}

void modelGetRootPosition(struct model *model, struct coord *pos)
{
	modelNodeGetPosition(model, model->filedata->rootnode, pos);
}

void modelSetRootPosition(struct model *model, struct coord *pos)
{
	modelNodeSetPosition(model, model->filedata->rootnode, pos);
}

void modelNodeGetModelRelativePosition(struct model *model, struct modelnode *node, struct coord *pos)
{
	pos->x = 0;
	pos->y = 0;
	pos->z = 0;

	while (node) {
		struct coord nodepos;
		u32 type = node->type & 0xff;

		if (type == MODELNODETYPE_CHRINFO
				|| type == MODELNODETYPE_POSITION
				|| type == MODELNODETYPE_POSITIONHELD) {
			modelNodeGetPosition(model, node, &nodepos);
			pos->x += nodepos.x;
			pos->y += nodepos.y;
			pos->z += nodepos.z;
		}

		node = node->parent;
	}
}

f32 model0001ae44(struct model *model)
{
	if ((model->filedata->rootnode->type & 0xff) == MODELNODETYPE_CHRINFO) {
		union modelrwdata *rwdata = modelGetNodeRwData(model, model->filedata->rootnode);
		return rwdata->chrinfo.unk14;
	}

	return 0;
}

void model0001ae90(struct model *model, f32 angle)
{
	if ((model->filedata->rootnode->type & 0xff) == MODELNODETYPE_CHRINFO) {
		struct modelrwdata_chrinfo *rwdata = modelGetNodeRwData(model, model->filedata->rootnode);
		f32 diff = angle - rwdata->unk14;

		if (diff < 0) {
			diff += M_BADTAU;
		}

		rwdata->unk30 += diff;

		if (rwdata->unk30 >= M_BADTAU) {
			rwdata->unk30 -= M_BADTAU;
		}

		rwdata->unk20 += diff;

		if (rwdata->unk20 >= M_BADTAU) {
			rwdata->unk20 -= M_BADTAU;
		}

		rwdata->unk14 = angle;
	}
}

void modelSetScale(struct model *model, f32 scale)
{
	model->scale = scale;
}

void modelSetAnimScale(struct model *model, f32 scale)
{
	if (model->anim) {
		model->anim->animscale = scale;
	}
}

f32 model0001af80(struct model *model)
{
	return model->filedata->unk10 * model->scale;
}

void model0001af98(struct coord *arg0, struct coord *arg1, f32 frac)
{
	arg0->x += (arg1->x - arg0->x) * frac;
	arg0->y += (arg1->y - arg0->y) * frac;
	arg0->z += (arg1->z - arg0->z) * frac;
}

f32 model0001afe8(f32 arg0, f32 angle, f32 mult)
{
	f32 value = angle - arg0;

	if (angle < arg0) {
		value += M_BADTAU;
	}

	if (value < M_PI) {
		arg0 += value * mult;

		if (arg0 >= M_BADTAU) {
			arg0 -= M_BADTAU;
		}
	} else {
		arg0 -= (M_BADTAU - value) * mult;

		if (arg0 < 0) {
			arg0 += M_BADTAU;
		}
	}

	return arg0;
}

void model0001b07c(struct coord *arg0, struct coord *arg1, f32 mult)
{
	arg0->x = model0001afe8(arg0->x, arg1->x, mult);
	arg0->y = model0001afe8(arg0->y, arg1->y, mult);
	arg0->z = model0001afe8(arg0->z, arg1->z, mult);
}

void model0001b0e8(struct model *model, struct modelnode *node)
{
	union modelrwdata *rwdata;
	struct anim *anim = model->anim;
	struct coord sp34;
	struct coord sp28;
	f32 frac;

	if (!anim) {
		return;
	}

	rwdata = modelGetNodeRwData(model, node);

	if (rwdata->chrinfo.unk00) {
		return;
	}

	sp34.x = rwdata->chrinfo.unk34.x;
	sp34.y = rwdata->chrinfo.unk34.y;
	sp34.z = rwdata->chrinfo.unk34.z;

	rwdata->chrinfo.unk14 = rwdata->chrinfo.unk30;

	if (g_Vars.in_cutscene && anim->speed > 0.0f) {
#if VERSION >= VERSION_PAL_BETA
		frac = floorf(anim->frac / anim->speed + 0.01f) * anim->speed;
#else
		frac = floorf(anim->frac / anim->speed) * anim->speed;
#endif
	} else {
		frac = anim->frac;
	}

	if (frac != 0.0f && rwdata->chrinfo.unk01) {
		model0001af98(&sp34, &rwdata->chrinfo.unk24, frac);

		rwdata->chrinfo.unk14 = model0001afe8(rwdata->chrinfo.unk30, rwdata->chrinfo.unk20, frac);
	}

	if (anim->animnum2 || anim->fracmerge) {
		if (rwdata->chrinfo.unk02) {
			f32 y = rwdata->chrinfo.unk4c.y;

			if (anim->frac2 != 0.0f) {
				y += (rwdata->chrinfo.unk40.y - y) * anim->frac2;
			}

			sp34.y += (y - sp34.y) * anim->fracmerge;
		}
	}

	if (anim->unk70 == NULL) {
		rwdata->chrinfo.pos.x = sp34.x;
		rwdata->chrinfo.pos.y = rwdata->chrinfo.ground + sp34.f[1];
		rwdata->chrinfo.pos.z = sp34.z;
	} else {
		sp28.x = sp34.x;
		sp28.y = sp34.y;
		sp28.z = sp34.z;

		if (anim->unk70(model, &rwdata->chrinfo.pos, &sp28, &rwdata->chrinfo.ground)) {
			rwdata->chrinfo.pos.x = sp28.x;
			rwdata->chrinfo.pos.y = rwdata->chrinfo.ground + sp28.f[1];
			rwdata->chrinfo.pos.z = sp28.z;

			sp34.x = sp28.x - sp34.x;
			sp34.z = sp28.z - sp34.z;

			rwdata->chrinfo.unk34.x += sp34.x;
			rwdata->chrinfo.unk34.z += sp34.z;

			if (rwdata->chrinfo.unk01) {
				rwdata->chrinfo.unk24.x += sp34.x;
				rwdata->chrinfo.unk24.z += sp34.z;
			}

			if (rwdata->chrinfo.unk02) {
				rwdata->chrinfo.unk4c.x += sp34.x;
				rwdata->chrinfo.unk4c.z += sp34.z;

				rwdata->chrinfo.unk40.x += sp34.x;
				rwdata->chrinfo.unk40.z += sp34.z;
			}
		}
	}
}

void model0001b3bc(struct model *model)
{
	struct modelnode *node = model->filedata->rootnode;

	if (node && (node->type & 0xff) == MODELNODETYPE_CHRINFO) {
		model0001b0e8(model, node);
	}
}

void model0001b400(struct modelrenderdata *arg0, struct model *model, struct modelnode *node)
{
	struct anim *anim = model->anim;
	union modelrodata *rodata = node->rodata;
	union modelrwdata *rwdata = modelGetNodeRwData(model, node);
	f32 scale = model->scale;
	struct coord *sp254 = &rwdata->chrinfo.pos;
	f32 sp250 = rwdata->chrinfo.unk14;
	Mtxf *sp24c;
	u32 stack1;
	Mtxf *mtx = &model->matrices[rodata->chrinfo.mtxindex];
	s32 sp240 = rodata->chrinfo.unk00;
	struct skeleton *skel = model->filedata->skel;
	struct coord sp230;
	struct coord sp224;
	struct coord sp218;
	Mtxf sp1d8;
	Mtxf sp198;
	Mtxf sp158;
	f32 sp154;
	struct coord sp148;
	struct coord sp13c;
	struct coord sp130;
	struct coord sp124;
	struct coord sp118;
	struct coord sp10c;
	f32 spfc[4];
	f32 spec[4];
	u8 stack4[0xc];
	f32 spdc;
	struct coord spd0;
	struct coord spc4;
	struct coord spb8;
	Mtxf sp78;
	Mtxf sp38;

	if (rodata->chrinfo.mtxindex);

	if (node->parent) {
		sp24c = model0001a5cc(model, node->parent, 0);
	} else {
		sp24c = arg0->unk00;
	}

	anim00024050(sp240, anim->flip, skel, anim->animnum, anim->unk04, &sp230, &sp224, &sp218);

	if (g_Vars.in_cutscene && anim->speed > 0) {
#if VERSION >= VERSION_PAL_BETA
		sp154 = floorf(anim->frac / anim->speed + 0.01f) * anim->speed;
#else
		sp154 = floorf(anim->frac / anim->speed) * anim->speed;
#endif
	} else {
		sp154 = anim->frac;
	}

	if (sp154 != 0.0f) {
		anim00024050(sp240, anim->flip, skel, anim->animnum, anim->unk05, &sp148, &sp13c, &sp130);
		model0001b07c(&sp230, &sp148, sp154);
	}

	if (anim->fracmerge != 0.0f) {
		anim00024050(sp240, anim->flip2, skel, anim->animnum2, anim->unk06, &sp124, &sp118, &sp10c);

		if (anim->frac2 != 0.0f) {
			anim00024050(sp240, anim->flip2, skel, anim->animnum2, anim->unk07, &spd0, &spc4, &spb8);
			model0001b07c(&sp124, &spd0, anim->frac2);
		}

		if ((g_Anims[anim->animnum].flags & ANIMFLAG_02) && (g_Anims[anim->animnum2].flags & ANIMFLAG_02) == 0) {
			mtx4LoadYRotation(rwdata->chrinfo.unk14, &sp78);
			mtx4LoadRotation(&sp124, &sp38);
			mtx00015be0(&sp78, &sp38);
			quaternion0f097044(&sp38, spec);
		} else {
			quaternion0f096ca0(&sp124, spec);
		}

		quaternion0f096ca0(&sp230, spfc);
		quaternion0f0976c0(spfc, spec);
		quaternionSlerp(spfc, spec, anim->fracmerge, &spdc);
		quaternionToMtx(&spdc, &sp1d8);
	} else {
		mtx4LoadRotation(&sp230, &sp1d8);
	}

	if (g_Anims[anim->animnum].flags & ANIMFLAG_02) {
		mtx4LoadTranslation(sp254, &sp198);
	} else {
		if (rwdata->chrinfo.unk18 != 0.0f) {
			sp250 = model0001afe8(sp250, rwdata->chrinfo.unk1c, rwdata->chrinfo.unk18);
		}

		mtx4LoadYRotationWithTranslation(sp254, sp250, &sp198);
	}

	mtx00015be4(&sp198, &sp1d8, &sp158);

	if (scale != 1.0f) {
		mtx00015f4c(scale, &sp158);
	}

	if (sp24c) {
		mtx00015be4(sp24c, &sp158, mtx);
	} else {
		mtx4Copy(&sp158, mtx);
	}
}

GLOBAL_ASM(
glabel model0001b80c
.late_rodata
glabel var700542f4
.word 0x40490fdb
glabel var700542f8
.word 0x40c907a9
glabel var700542fc
.word 0x40c907a9
glabel var70054300
.word 0x40490fdb
glabel var70054304
.word 0x40c907a9
glabel var70054308
.word 0x3f63dec6
.text
/*    1b80c:	27bdff48 */ 	addiu	$sp,$sp,-184
/*    1b810:	afbf0014 */ 	sw	$ra,0x14($sp)
/*    1b814:	afa500bc */ 	sw	$a1,0xbc($sp)
/*    1b818:	afa700c4 */ 	sw	$a3,0xc4($sp)
/*    1b81c:	94ce0000 */ 	lhu	$t6,0x0($a2)
/*    1b820:	afae00b4 */ 	sw	$t6,0xb4($sp)
/*    1b824:	8cc20004 */ 	lw	$v0,0x4($a2)
/*    1b828:	844f000e */ 	lh	$t7,0xe($v0)
/*    1b82c:	afaf0064 */ 	sw	$t7,0x64($sp)
/*    1b830:	84580010 */ 	lh	$t8,0x10($v0)
/*    1b834:	afb80060 */ 	sw	$t8,0x60($sp)
/*    1b838:	84590012 */ 	lh	$t9,0x12($v0)
/*    1b83c:	afb9005c */ 	sw	$t9,0x5c($sp)
/*    1b840:	8ca9000c */ 	lw	$t1,0xc($a1)
/*    1b844:	afa90058 */ 	sw	$t1,0x58($sp)
/*    1b848:	8cca0008 */ 	lw	$t2,0x8($a2)
/*    1b84c:	00003025 */ 	or	$a2,$zero,$zero
/*    1b850:	11400006 */ 	beqz	$t2,.L0001b86c
/*    1b854:	afaa001c */ 	sw	$t2,0x1c($sp)
/*    1b858:	00a02025 */ 	or	$a0,$a1,$zero
/*    1b85c:	0c006973 */ 	jal	model0001a5cc
/*    1b860:	01402825 */ 	or	$a1,$t2,$zero
/*    1b864:	10000003 */ 	b	.L0001b874
/*    1b868:	afa200ac */ 	sw	$v0,0xac($sp)
.L0001b86c:
/*    1b86c:	8c8c0000 */ 	lw	$t4,0x0($a0)
/*    1b870:	afac00ac */ 	sw	$t4,0xac($sp)
.L0001b874:
/*    1b874:	8fad00ac */ 	lw	$t5,0xac($sp)
/*    1b878:	8fa400c8 */ 	lw	$a0,0xc8($sp)
/*    1b87c:	8fac0064 */ 	lw	$t4,0x64($sp)
/*    1b880:	11a00042 */ 	beqz	$t5,.L0001b98c
/*    1b884:	8fae0058 */ 	lw	$t6,0x58($sp)
/*    1b888:	8fa400c8 */ 	lw	$a0,0xc8($sp)
/*    1b88c:	8fa500c4 */ 	lw	$a1,0xc4($sp)
/*    1b890:	0c0059a9 */ 	jal	mtx4LoadRotationAndTranslation
/*    1b894:	27a60068 */ 	addiu	$a2,$sp,0x68
/*    1b898:	8fae00cc */ 	lw	$t6,0xcc($sp)
/*    1b89c:	8faf00bc */ 	lw	$t7,0xbc($sp)
/*    1b8a0:	3c013f80 */ 	lui	$at,0x3f80
/*    1b8a4:	51c0000b */ 	beqzl	$t6,.L0001b8d4
/*    1b8a8:	8fb800d0 */ 	lw	$t8,0xd0($sp)
/*    1b8ac:	c5ec0014 */ 	lwc1	$f12,0x14($t7)
/*    1b8b0:	44812000 */ 	mtc1	$at,$f4
/*    1b8b4:	00000000 */ 	nop
/*    1b8b8:	460c2032 */ 	c.eq.s	$f4,$f12
/*    1b8bc:	00000000 */ 	nop
/*    1b8c0:	45030004 */ 	bc1tl	.L0001b8d4
/*    1b8c4:	8fb800d0 */ 	lw	$t8,0xd0($sp)
/*    1b8c8:	0c0057c1 */ 	jal	mtx00015f04
/*    1b8cc:	27a50068 */ 	addiu	$a1,$sp,0x68
/*    1b8d0:	8fb800d0 */ 	lw	$t8,0xd0($sp)
.L0001b8d4:
/*    1b8d4:	3c013f80 */ 	lui	$at,0x3f80
/*    1b8d8:	44813000 */ 	mtc1	$at,$f6
/*    1b8dc:	c70c0000 */ 	lwc1	$f12,0x0($t8)
/*    1b8e0:	460c3032 */ 	c.eq.s	$f6,$f12
/*    1b8e4:	00000000 */ 	nop
/*    1b8e8:	45030004 */ 	bc1tl	.L0001b8fc
/*    1b8ec:	8fb900d0 */ 	lw	$t9,0xd0($sp)
/*    1b8f0:	0c00577c */ 	jal	mtx00015df0
/*    1b8f4:	27a50068 */ 	addiu	$a1,$sp,0x68
/*    1b8f8:	8fb900d0 */ 	lw	$t9,0xd0($sp)
.L0001b8fc:
/*    1b8fc:	3c013f80 */ 	lui	$at,0x3f80
/*    1b900:	44814000 */ 	mtc1	$at,$f8
/*    1b904:	c72c0004 */ 	lwc1	$f12,0x4($t9)
/*    1b908:	460c4032 */ 	c.eq.s	$f8,$f12
/*    1b90c:	00000000 */ 	nop
/*    1b910:	45030004 */ 	bc1tl	.L0001b924
/*    1b914:	8fa900d0 */ 	lw	$t1,0xd0($sp)
/*    1b918:	0c005793 */ 	jal	mtx00015e4c
/*    1b91c:	27a50068 */ 	addiu	$a1,$sp,0x68
/*    1b920:	8fa900d0 */ 	lw	$t1,0xd0($sp)
.L0001b924:
/*    1b924:	3c013f80 */ 	lui	$at,0x3f80
/*    1b928:	44815000 */ 	mtc1	$at,$f10
/*    1b92c:	c52c0008 */ 	lwc1	$f12,0x8($t1)
/*    1b930:	460c5032 */ 	c.eq.s	$f10,$f12
/*    1b934:	00000000 */ 	nop
/*    1b938:	45030004 */ 	bc1tl	.L0001b94c
/*    1b93c:	8faa0064 */ 	lw	$t2,0x64($sp)
/*    1b940:	0c0057aa */ 	jal	mtx00015ea8
/*    1b944:	27a50068 */ 	addiu	$a1,$sp,0x68
/*    1b948:	8faa0064 */ 	lw	$t2,0x64($sp)
.L0001b94c:
/*    1b94c:	8fab0058 */ 	lw	$t3,0x58($sp)
/*    1b950:	8fa400ac */ 	lw	$a0,0xac($sp)
/*    1b954:	000a4180 */ 	sll	$t0,$t2,0x6
/*    1b958:	010b3021 */ 	addu	$a2,$t0,$t3
/*    1b95c:	afa6001c */ 	sw	$a2,0x1c($sp)
/*    1b960:	0c0056f9 */ 	jal	mtx00015be4
/*    1b964:	27a50068 */ 	addiu	$a1,$sp,0x68
/*    1b968:	3c028006 */ 	lui	$v0,%hi(var8005efcc)
/*    1b96c:	8c42efcc */ 	lw	$v0,%lo(var8005efcc)($v0)
/*    1b970:	8fa40064 */ 	lw	$a0,0x64($sp)
/*    1b974:	50400037 */ 	beqzl	$v0,.L0001ba54
/*    1b978:	8fa800b4 */ 	lw	$t0,0xb4($sp)
/*    1b97c:	0040f809 */ 	jalr	$v0
/*    1b980:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1b984:	10000033 */ 	b	.L0001ba54
/*    1b988:	8fa800b4 */ 	lw	$t0,0xb4($sp)
.L0001b98c:
/*    1b98c:	000c6980 */ 	sll	$t5,$t4,0x6
/*    1b990:	01ae3021 */ 	addu	$a2,$t5,$t6
/*    1b994:	afa6001c */ 	sw	$a2,0x1c($sp)
/*    1b998:	0c0059a9 */ 	jal	mtx4LoadRotationAndTranslation
/*    1b99c:	8fa500c4 */ 	lw	$a1,0xc4($sp)
/*    1b9a0:	8faf00cc */ 	lw	$t7,0xcc($sp)
/*    1b9a4:	8fb800bc */ 	lw	$t8,0xbc($sp)
/*    1b9a8:	3c013f80 */ 	lui	$at,0x3f80
/*    1b9ac:	51e0000b */ 	beqzl	$t7,.L0001b9dc
/*    1b9b0:	8fb900d0 */ 	lw	$t9,0xd0($sp)
/*    1b9b4:	c70c0014 */ 	lwc1	$f12,0x14($t8)
/*    1b9b8:	44818000 */ 	mtc1	$at,$f16
/*    1b9bc:	00000000 */ 	nop
/*    1b9c0:	460c8032 */ 	c.eq.s	$f16,$f12
/*    1b9c4:	00000000 */ 	nop
/*    1b9c8:	45030004 */ 	bc1tl	.L0001b9dc
/*    1b9cc:	8fb900d0 */ 	lw	$t9,0xd0($sp)
/*    1b9d0:	0c0057c1 */ 	jal	mtx00015f04
/*    1b9d4:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1b9d8:	8fb900d0 */ 	lw	$t9,0xd0($sp)
.L0001b9dc:
/*    1b9dc:	3c013f80 */ 	lui	$at,0x3f80
/*    1b9e0:	44819000 */ 	mtc1	$at,$f18
/*    1b9e4:	c72c0000 */ 	lwc1	$f12,0x0($t9)
/*    1b9e8:	460c9032 */ 	c.eq.s	$f18,$f12
/*    1b9ec:	00000000 */ 	nop
/*    1b9f0:	45030004 */ 	bc1tl	.L0001ba04
/*    1b9f4:	8fa900d0 */ 	lw	$t1,0xd0($sp)
/*    1b9f8:	0c00577c */ 	jal	mtx00015df0
/*    1b9fc:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1ba00:	8fa900d0 */ 	lw	$t1,0xd0($sp)
.L0001ba04:
/*    1ba04:	3c013f80 */ 	lui	$at,0x3f80
/*    1ba08:	44812000 */ 	mtc1	$at,$f4
/*    1ba0c:	c52c0004 */ 	lwc1	$f12,0x4($t1)
/*    1ba10:	460c2032 */ 	c.eq.s	$f4,$f12
/*    1ba14:	00000000 */ 	nop
/*    1ba18:	45030004 */ 	bc1tl	.L0001ba2c
/*    1ba1c:	8faa00d0 */ 	lw	$t2,0xd0($sp)
/*    1ba20:	0c005793 */ 	jal	mtx00015e4c
/*    1ba24:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1ba28:	8faa00d0 */ 	lw	$t2,0xd0($sp)
.L0001ba2c:
/*    1ba2c:	3c013f80 */ 	lui	$at,0x3f80
/*    1ba30:	44813000 */ 	mtc1	$at,$f6
/*    1ba34:	c54c0008 */ 	lwc1	$f12,0x8($t2)
/*    1ba38:	460c3032 */ 	c.eq.s	$f6,$f12
/*    1ba3c:	00000000 */ 	nop
/*    1ba40:	45030004 */ 	bc1tl	.L0001ba54
/*    1ba44:	8fa800b4 */ 	lw	$t0,0xb4($sp)
/*    1ba48:	0c0057aa */ 	jal	mtx00015ea8
/*    1ba4c:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1ba50:	8fa800b4 */ 	lw	$t0,0xb4($sp)
.L0001ba54:
/*    1ba54:	8fa400c4 */ 	lw	$a0,0xc4($sp)
/*    1ba58:	310b0100 */ 	andi	$t3,$t0,0x100
/*    1ba5c:	5160001d */ 	beqzl	$t3,.L0001bad4
/*    1ba60:	8faa00b4 */ 	lw	$t2,0xb4($sp)
/*    1ba64:	0fc25b28 */ 	jal	quaternion0f096ca0
/*    1ba68:	27a5003c */ 	addiu	$a1,$sp,0x3c
/*    1ba6c:	27a4003c */ 	addiu	$a0,$sp,0x3c
/*    1ba70:	3c053f00 */ 	lui	$a1,0x3f00
/*    1ba74:	0fc25d46 */ 	jal	quaternion0f097518
/*    1ba78:	27a6002c */ 	addiu	$a2,$sp,0x2c
/*    1ba7c:	8fac00ac */ 	lw	$t4,0xac($sp)
/*    1ba80:	8fb80060 */ 	lw	$t8,0x60($sp)
/*    1ba84:	27a5002c */ 	addiu	$a1,$sp,0x2c
/*    1ba88:	1180000d */ 	beqz	$t4,.L0001bac0
/*    1ba8c:	0018c980 */ 	sll	$t9,$t8,0x6
/*    1ba90:	8fa400c8 */ 	lw	$a0,0xc8($sp)
/*    1ba94:	0fc25c99 */ 	jal	quaternionToTransformMtx
/*    1ba98:	27a60068 */ 	addiu	$a2,$sp,0x68
/*    1ba9c:	8fad0060 */ 	lw	$t5,0x60($sp)
/*    1baa0:	8faf0058 */ 	lw	$t7,0x58($sp)
/*    1baa4:	8fa400ac */ 	lw	$a0,0xac($sp)
/*    1baa8:	000d7180 */ 	sll	$t6,$t5,0x6
/*    1baac:	27a50068 */ 	addiu	$a1,$sp,0x68
/*    1bab0:	0c0056f9 */ 	jal	mtx00015be4
/*    1bab4:	01cf3021 */ 	addu	$a2,$t6,$t7
/*    1bab8:	10000006 */ 	b	.L0001bad4
/*    1babc:	8faa00b4 */ 	lw	$t2,0xb4($sp)
.L0001bac0:
/*    1bac0:	8fa90058 */ 	lw	$t1,0x58($sp)
/*    1bac4:	8fa400c8 */ 	lw	$a0,0xc8($sp)
/*    1bac8:	0fc25c99 */ 	jal	quaternionToTransformMtx
/*    1bacc:	03293021 */ 	addu	$a2,$t9,$t1
/*    1bad0:	8faa00b4 */ 	lw	$t2,0xb4($sp)
.L0001bad4:
/*    1bad4:	8fab00ac */ 	lw	$t3,0xac($sp)
/*    1bad8:	31480200 */ 	andi	$t0,$t2,0x200
/*    1badc:	5100004a */ 	beqzl	$t0,.L0001bc08
/*    1bae0:	8fbf0014 */ 	lw	$ra,0x14($sp)
/*    1bae4:	11600004 */ 	beqz	$t3,.L0001baf8
/*    1bae8:	8fad005c */ 	lw	$t5,0x5c($sp)
/*    1baec:	27ac0068 */ 	addiu	$t4,$sp,0x68
/*    1baf0:	10000005 */ 	b	.L0001bb08
/*    1baf4:	afac0028 */ 	sw	$t4,0x28($sp)
.L0001baf8:
/*    1baf8:	8faf0058 */ 	lw	$t7,0x58($sp)
/*    1bafc:	000d7180 */ 	sll	$t6,$t5,0x6
/*    1bb00:	01cfc021 */ 	addu	$t8,$t6,$t7
/*    1bb04:	afb80028 */ 	sw	$t8,0x28($sp)
.L0001bb08:
/*    1bb08:	8fb900c4 */ 	lw	$t9,0xc4($sp)
/*    1bb0c:	3c017005 */ 	lui	$at,%hi(var700542f4)
/*    1bb10:	c42842f4 */ 	lwc1	$f8,%lo(var700542f4)($at)
/*    1bb14:	c72c0004 */ 	lwc1	$f12,0x4($t9)
/*    1bb18:	3c017005 */ 	lui	$at,%hi(var700542f8)
/*    1bb1c:	8fa50028 */ 	lw	$a1,0x28($sp)
/*    1bb20:	4608603c */ 	c.lt.s	$f12,$f8
/*    1bb24:	00000000 */ 	nop
/*    1bb28:	45000007 */ 	bc1f	.L0001bb48
/*    1bb2c:	00000000 */ 	nop
/*    1bb30:	3c013f00 */ 	lui	$at,0x3f00
/*    1bb34:	44815000 */ 	mtc1	$at,$f10
/*    1bb38:	00000000 */ 	nop
/*    1bb3c:	460a6302 */ 	mul.s	$f12,$f12,$f10
/*    1bb40:	10000009 */ 	b	.L0001bb68
/*    1bb44:	00000000 */ 	nop
.L0001bb48:
/*    1bb48:	c43042f8 */ 	lwc1	$f16,%lo(var700542f8)($at)
/*    1bb4c:	3c013f00 */ 	lui	$at,0x3f00
/*    1bb50:	44812000 */ 	mtc1	$at,$f4
/*    1bb54:	460c8481 */ 	sub.s	$f18,$f16,$f12
/*    1bb58:	3c017005 */ 	lui	$at,%hi(var700542fc)
/*    1bb5c:	c42842fc */ 	lwc1	$f8,%lo(var700542fc)($at)
/*    1bb60:	46049182 */ 	mul.s	$f6,$f18,$f4
/*    1bb64:	46064301 */ 	sub.s	$f12,$f8,$f6
.L0001bb68:
/*    1bb68:	0c0058dd */ 	jal	mtx4LoadYRotation
/*    1bb6c:	e7ac0024 */ 	swc1	$f12,0x24($sp)
/*    1bb70:	3c017005 */ 	lui	$at,%hi(var70054300)
/*    1bb74:	c7ac0024 */ 	lwc1	$f12,0x24($sp)
/*    1bb78:	c42a4300 */ 	lwc1	$f10,%lo(var70054300)($at)
/*    1bb7c:	3c017005 */ 	lui	$at,%hi(var70054304)
/*    1bb80:	460c503e */ 	c.le.s	$f10,$f12
/*    1bb84:	00000000 */ 	nop
/*    1bb88:	45000003 */ 	bc1f	.L0001bb98
/*    1bb8c:	00000000 */ 	nop
/*    1bb90:	c4304304 */ 	lwc1	$f16,%lo(var70054304)($at)
/*    1bb94:	460c8301 */ 	sub.s	$f12,$f16,$f12
.L0001bb98:
/*    1bb98:	3c017005 */ 	lui	$at,%hi(var70054308)
/*    1bb9c:	c4324308 */ 	lwc1	$f18,%lo(var70054308)($at)
/*    1bba0:	3c013fc0 */ 	lui	$at,0x3fc0
/*    1bba4:	4612603c */ 	c.lt.s	$f12,$f18
/*    1bba8:	00000000 */ 	nop
/*    1bbac:	45020006 */ 	bc1fl	.L0001bbc8
/*    1bbb0:	44816000 */ 	mtc1	$at,$f12
/*    1bbb4:	0fc259c0 */ 	jal	func0f096700
/*    1bbb8:	00000000 */ 	nop
/*    1bbbc:	10000003 */ 	b	.L0001bbcc
/*    1bbc0:	46000306 */ 	mov.s	$f12,$f0
/*    1bbc4:	44816000 */ 	mtc1	$at,$f12
.L0001bbc8:
/*    1bbc8:	00000000 */ 	nop
.L0001bbcc:
/*    1bbcc:	0c0057b7 */ 	jal	mtx00015edc
/*    1bbd0:	8fa50028 */ 	lw	$a1,0x28($sp)
/*    1bbd4:	8fa400c8 */ 	lw	$a0,0xc8($sp)
/*    1bbd8:	0c005775 */ 	jal	mtx4SetTranslation
/*    1bbdc:	8fa50028 */ 	lw	$a1,0x28($sp)
/*    1bbe0:	8fa900ac */ 	lw	$t1,0xac($sp)
/*    1bbe4:	8faa005c */ 	lw	$t2,0x5c($sp)
/*    1bbe8:	8fab0058 */ 	lw	$t3,0x58($sp)
/*    1bbec:	11200005 */ 	beqz	$t1,.L0001bc04
/*    1bbf0:	01202025 */ 	or	$a0,$t1,$zero
/*    1bbf4:	000a4180 */ 	sll	$t0,$t2,0x6
/*    1bbf8:	010b3021 */ 	addu	$a2,$t0,$t3
/*    1bbfc:	0c0056f9 */ 	jal	mtx00015be4
/*    1bc00:	8fa50028 */ 	lw	$a1,0x28($sp)
.L0001bc04:
/*    1bc04:	8fbf0014 */ 	lw	$ra,0x14($sp)
.L0001bc08:
/*    1bc08:	27bd00b8 */ 	addiu	$sp,$sp,0xb8
/*    1bc0c:	03e00008 */ 	jr	$ra
/*    1bc10:	00000000 */ 	nop
);

GLOBAL_ASM(
glabel model0001bc14
.late_rodata
glabel var7005430c
.word 0x40490fdb
glabel var70054310
.word 0x40c907a9
glabel var70054314
.word 0x40c907a9
glabel var70054318
.word 0x40490fdb
glabel var7005431c
.word 0x40c907a9
glabel var70054320
.word 0x3f63dec6
.text
/*    1bc14:	27bdff58 */ 	addiu	$sp,$sp,-168
/*    1bc18:	afbf0014 */ 	sw	$ra,0x14($sp)
/*    1bc1c:	afa500ac */ 	sw	$a1,0xac($sp)
/*    1bc20:	afa700b4 */ 	sw	$a3,0xb4($sp)
/*    1bc24:	94ce0000 */ 	lhu	$t6,0x0($a2)
/*    1bc28:	afae00a4 */ 	sw	$t6,0xa4($sp)
/*    1bc2c:	8cc20004 */ 	lw	$v0,0x4($a2)
/*    1bc30:	844f000e */ 	lh	$t7,0xe($v0)
/*    1bc34:	afaf0054 */ 	sw	$t7,0x54($sp)
/*    1bc38:	84580010 */ 	lh	$t8,0x10($v0)
/*    1bc3c:	afb80050 */ 	sw	$t8,0x50($sp)
/*    1bc40:	84590012 */ 	lh	$t9,0x12($v0)
/*    1bc44:	afb9004c */ 	sw	$t9,0x4c($sp)
/*    1bc48:	8ca9000c */ 	lw	$t1,0xc($a1)
/*    1bc4c:	afa90048 */ 	sw	$t1,0x48($sp)
/*    1bc50:	8cca0008 */ 	lw	$t2,0x8($a2)
/*    1bc54:	00003025 */ 	or	$a2,$zero,$zero
/*    1bc58:	11400006 */ 	beqz	$t2,.L0001bc74
/*    1bc5c:	afaa001c */ 	sw	$t2,0x1c($sp)
/*    1bc60:	00a02025 */ 	or	$a0,$a1,$zero
/*    1bc64:	0c006973 */ 	jal	model0001a5cc
/*    1bc68:	01402825 */ 	or	$a1,$t2,$zero
/*    1bc6c:	10000003 */ 	b	.L0001bc7c
/*    1bc70:	afa2009c */ 	sw	$v0,0x9c($sp)
.L0001bc74:
/*    1bc74:	8c8c0000 */ 	lw	$t4,0x0($a0)
/*    1bc78:	afac009c */ 	sw	$t4,0x9c($sp)
.L0001bc7c:
/*    1bc7c:	8fad009c */ 	lw	$t5,0x9c($sp)
/*    1bc80:	8fa400b8 */ 	lw	$a0,0xb8($sp)
/*    1bc84:	8fa80054 */ 	lw	$t0,0x54($sp)
/*    1bc88:	11a00034 */ 	beqz	$t5,.L0001bd5c
/*    1bc8c:	8fac0048 */ 	lw	$t4,0x48($sp)
/*    1bc90:	8fa400b8 */ 	lw	$a0,0xb8($sp)
/*    1bc94:	8fa500b4 */ 	lw	$a1,0xb4($sp)
/*    1bc98:	0fc25c99 */ 	jal	quaternionToTransformMtx
/*    1bc9c:	27a60058 */ 	addiu	$a2,$sp,0x58
/*    1bca0:	8fae00bc */ 	lw	$t6,0xbc($sp)
/*    1bca4:	3c013f80 */ 	lui	$at,0x3f80
/*    1bca8:	44812000 */ 	mtc1	$at,$f4
/*    1bcac:	c5cc0000 */ 	lwc1	$f12,0x0($t6)
/*    1bcb0:	460c2032 */ 	c.eq.s	$f4,$f12
/*    1bcb4:	00000000 */ 	nop
/*    1bcb8:	45030004 */ 	bc1tl	.L0001bccc
/*    1bcbc:	8faf00bc */ 	lw	$t7,0xbc($sp)
/*    1bcc0:	0c00577c */ 	jal	mtx00015df0
/*    1bcc4:	27a50058 */ 	addiu	$a1,$sp,0x58
/*    1bcc8:	8faf00bc */ 	lw	$t7,0xbc($sp)
.L0001bccc:
/*    1bccc:	3c013f80 */ 	lui	$at,0x3f80
/*    1bcd0:	44813000 */ 	mtc1	$at,$f6
/*    1bcd4:	c5ec0004 */ 	lwc1	$f12,0x4($t7)
/*    1bcd8:	460c3032 */ 	c.eq.s	$f6,$f12
/*    1bcdc:	00000000 */ 	nop
/*    1bce0:	45030004 */ 	bc1tl	.L0001bcf4
/*    1bce4:	8fb800bc */ 	lw	$t8,0xbc($sp)
/*    1bce8:	0c005793 */ 	jal	mtx00015e4c
/*    1bcec:	27a50058 */ 	addiu	$a1,$sp,0x58
/*    1bcf0:	8fb800bc */ 	lw	$t8,0xbc($sp)
.L0001bcf4:
/*    1bcf4:	3c013f80 */ 	lui	$at,0x3f80
/*    1bcf8:	44814000 */ 	mtc1	$at,$f8
/*    1bcfc:	c70c0008 */ 	lwc1	$f12,0x8($t8)
/*    1bd00:	460c4032 */ 	c.eq.s	$f8,$f12
/*    1bd04:	00000000 */ 	nop
/*    1bd08:	45030004 */ 	bc1tl	.L0001bd1c
/*    1bd0c:	8fb90054 */ 	lw	$t9,0x54($sp)
/*    1bd10:	0c0057aa */ 	jal	mtx00015ea8
/*    1bd14:	27a50058 */ 	addiu	$a1,$sp,0x58
/*    1bd18:	8fb90054 */ 	lw	$t9,0x54($sp)
.L0001bd1c:
/*    1bd1c:	8faa0048 */ 	lw	$t2,0x48($sp)
/*    1bd20:	8fa4009c */ 	lw	$a0,0x9c($sp)
/*    1bd24:	00194980 */ 	sll	$t1,$t9,0x6
/*    1bd28:	012a3021 */ 	addu	$a2,$t1,$t2
/*    1bd2c:	afa6001c */ 	sw	$a2,0x1c($sp)
/*    1bd30:	0c0056f9 */ 	jal	mtx00015be4
/*    1bd34:	27a50058 */ 	addiu	$a1,$sp,0x58
/*    1bd38:	3c028006 */ 	lui	$v0,%hi(var8005efcc)
/*    1bd3c:	8c42efcc */ 	lw	$v0,%lo(var8005efcc)($v0)
/*    1bd40:	8fa40054 */ 	lw	$a0,0x54($sp)
/*    1bd44:	50400029 */ 	beqzl	$v0,.L0001bdec
/*    1bd48:	8fb800a4 */ 	lw	$t8,0xa4($sp)
/*    1bd4c:	0040f809 */ 	jalr	$v0
/*    1bd50:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1bd54:	10000025 */ 	b	.L0001bdec
/*    1bd58:	8fb800a4 */ 	lw	$t8,0xa4($sp)
.L0001bd5c:
/*    1bd5c:	00085980 */ 	sll	$t3,$t0,0x6
/*    1bd60:	016c3021 */ 	addu	$a2,$t3,$t4
/*    1bd64:	afa6001c */ 	sw	$a2,0x1c($sp)
/*    1bd68:	0fc25c99 */ 	jal	quaternionToTransformMtx
/*    1bd6c:	8fa500b4 */ 	lw	$a1,0xb4($sp)
/*    1bd70:	8fad00bc */ 	lw	$t5,0xbc($sp)
/*    1bd74:	3c013f80 */ 	lui	$at,0x3f80
/*    1bd78:	44815000 */ 	mtc1	$at,$f10
/*    1bd7c:	c5ac0000 */ 	lwc1	$f12,0x0($t5)
/*    1bd80:	460c5032 */ 	c.eq.s	$f10,$f12
/*    1bd84:	00000000 */ 	nop
/*    1bd88:	45030004 */ 	bc1tl	.L0001bd9c
/*    1bd8c:	8fae00bc */ 	lw	$t6,0xbc($sp)
/*    1bd90:	0c00577c */ 	jal	mtx00015df0
/*    1bd94:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1bd98:	8fae00bc */ 	lw	$t6,0xbc($sp)
.L0001bd9c:
/*    1bd9c:	3c013f80 */ 	lui	$at,0x3f80
/*    1bda0:	44818000 */ 	mtc1	$at,$f16
/*    1bda4:	c5cc0004 */ 	lwc1	$f12,0x4($t6)
/*    1bda8:	460c8032 */ 	c.eq.s	$f16,$f12
/*    1bdac:	00000000 */ 	nop
/*    1bdb0:	45030004 */ 	bc1tl	.L0001bdc4
/*    1bdb4:	8faf00bc */ 	lw	$t7,0xbc($sp)
/*    1bdb8:	0c005793 */ 	jal	mtx00015e4c
/*    1bdbc:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1bdc0:	8faf00bc */ 	lw	$t7,0xbc($sp)
.L0001bdc4:
/*    1bdc4:	3c013f80 */ 	lui	$at,0x3f80
/*    1bdc8:	44819000 */ 	mtc1	$at,$f18
/*    1bdcc:	c5ec0008 */ 	lwc1	$f12,0x8($t7)
/*    1bdd0:	460c9032 */ 	c.eq.s	$f18,$f12
/*    1bdd4:	00000000 */ 	nop
/*    1bdd8:	45030004 */ 	bc1tl	.L0001bdec
/*    1bddc:	8fb800a4 */ 	lw	$t8,0xa4($sp)
/*    1bde0:	0c0057aa */ 	jal	mtx00015ea8
/*    1bde4:	8fa5001c */ 	lw	$a1,0x1c($sp)
/*    1bde8:	8fb800a4 */ 	lw	$t8,0xa4($sp)
.L0001bdec:
/*    1bdec:	8fa400b4 */ 	lw	$a0,0xb4($sp)
/*    1bdf0:	3c053f00 */ 	lui	$a1,0x3f00
/*    1bdf4:	33190100 */ 	andi	$t9,$t8,0x100
/*    1bdf8:	53200019 */ 	beqzl	$t9,.L0001be60
/*    1bdfc:	8faf00a4 */ 	lw	$t7,0xa4($sp)
/*    1be00:	0fc25d46 */ 	jal	quaternion0f097518
/*    1be04:	27a6002c */ 	addiu	$a2,$sp,0x2c
/*    1be08:	8fa9009c */ 	lw	$t1,0x9c($sp)
/*    1be0c:	8fac0050 */ 	lw	$t4,0x50($sp)
/*    1be10:	27a5002c */ 	addiu	$a1,$sp,0x2c
/*    1be14:	1120000d */ 	beqz	$t1,.L0001be4c
/*    1be18:	000c6980 */ 	sll	$t5,$t4,0x6
/*    1be1c:	8fa400b8 */ 	lw	$a0,0xb8($sp)
/*    1be20:	0fc25c99 */ 	jal	quaternionToTransformMtx
/*    1be24:	27a60058 */ 	addiu	$a2,$sp,0x58
/*    1be28:	8faa0050 */ 	lw	$t2,0x50($sp)
/*    1be2c:	8fab0048 */ 	lw	$t3,0x48($sp)
/*    1be30:	8fa4009c */ 	lw	$a0,0x9c($sp)
/*    1be34:	000a4180 */ 	sll	$t0,$t2,0x6
/*    1be38:	27a50058 */ 	addiu	$a1,$sp,0x58
/*    1be3c:	0c0056f9 */ 	jal	mtx00015be4
/*    1be40:	010b3021 */ 	addu	$a2,$t0,$t3
/*    1be44:	10000006 */ 	b	.L0001be60
/*    1be48:	8faf00a4 */ 	lw	$t7,0xa4($sp)
.L0001be4c:
/*    1be4c:	8fae0048 */ 	lw	$t6,0x48($sp)
/*    1be50:	8fa400b8 */ 	lw	$a0,0xb8($sp)
/*    1be54:	0fc25c99 */ 	jal	quaternionToTransformMtx
/*    1be58:	01ae3021 */ 	addu	$a2,$t5,$t6
/*    1be5c:	8faf00a4 */ 	lw	$t7,0xa4($sp)
.L0001be60:
/*    1be60:	8fb9009c */ 	lw	$t9,0x9c($sp)
/*    1be64:	31f80200 */ 	andi	$t8,$t7,0x200
/*    1be68:	5300004c */ 	beqzl	$t8,.L0001bf9c
/*    1be6c:	8fbf0014 */ 	lw	$ra,0x14($sp)
/*    1be70:	13200004 */ 	beqz	$t9,.L0001be84
/*    1be74:	8faa004c */ 	lw	$t2,0x4c($sp)
/*    1be78:	27a90058 */ 	addiu	$t1,$sp,0x58
/*    1be7c:	10000005 */ 	b	.L0001be94
/*    1be80:	afa90028 */ 	sw	$t1,0x28($sp)
.L0001be84:
/*    1be84:	8fab0048 */ 	lw	$t3,0x48($sp)
/*    1be88:	000a4180 */ 	sll	$t0,$t2,0x6
/*    1be8c:	010b6021 */ 	addu	$t4,$t0,$t3
/*    1be90:	afac0028 */ 	sw	$t4,0x28($sp)
.L0001be94:
/*    1be94:	8fad00b4 */ 	lw	$t5,0xb4($sp)
/*    1be98:	0fc25a74 */ 	jal	acosf
/*    1be9c:	c5ac0000 */ 	lwc1	$f12,0x0($t5)
/*    1bea0:	46000080 */ 	add.s	$f2,$f0,$f0
/*    1bea4:	3c017005 */ 	lui	$at,%hi(var7005430c)
/*    1bea8:	c424430c */ 	lwc1	$f4,%lo(var7005430c)($at)
/*    1beac:	3c017005 */ 	lui	$at,%hi(var70054310)
/*    1beb0:	8fa50028 */ 	lw	$a1,0x28($sp)
/*    1beb4:	4604103c */ 	c.lt.s	$f2,$f4
/*    1beb8:	00000000 */ 	nop
/*    1bebc:	45000007 */ 	bc1f	.L0001bedc
/*    1bec0:	00000000 */ 	nop
/*    1bec4:	3c013f00 */ 	lui	$at,0x3f00
/*    1bec8:	44813000 */ 	mtc1	$at,$f6
/*    1becc:	00000000 */ 	nop
/*    1bed0:	46061302 */ 	mul.s	$f12,$f2,$f6
/*    1bed4:	10000009 */ 	b	.L0001befc
/*    1bed8:	00000000 */ 	nop
.L0001bedc:
/*    1bedc:	c4284310 */ 	lwc1	$f8,%lo(var70054310)($at)
/*    1bee0:	3c013f00 */ 	lui	$at,0x3f00
/*    1bee4:	44818000 */ 	mtc1	$at,$f16
/*    1bee8:	46024281 */ 	sub.s	$f10,$f8,$f2
/*    1beec:	3c017005 */ 	lui	$at,%hi(var70054314)
/*    1bef0:	c4244314 */ 	lwc1	$f4,%lo(var70054314)($at)
/*    1bef4:	46105482 */ 	mul.s	$f18,$f10,$f16
/*    1bef8:	46122301 */ 	sub.s	$f12,$f4,$f18
.L0001befc:
/*    1befc:	0c0058dd */ 	jal	mtx4LoadYRotation
/*    1bf00:	e7ac0024 */ 	swc1	$f12,0x24($sp)
/*    1bf04:	3c017005 */ 	lui	$at,%hi(var70054318)
/*    1bf08:	c7ac0024 */ 	lwc1	$f12,0x24($sp)
/*    1bf0c:	c4264318 */ 	lwc1	$f6,%lo(var70054318)($at)
/*    1bf10:	3c017005 */ 	lui	$at,%hi(var7005431c)
/*    1bf14:	460c303e */ 	c.le.s	$f6,$f12
/*    1bf18:	00000000 */ 	nop
/*    1bf1c:	45000003 */ 	bc1f	.L0001bf2c
/*    1bf20:	00000000 */ 	nop
/*    1bf24:	c428431c */ 	lwc1	$f8,%lo(var7005431c)($at)
/*    1bf28:	460c4301 */ 	sub.s	$f12,$f8,$f12
.L0001bf2c:
/*    1bf2c:	3c017005 */ 	lui	$at,%hi(var70054320)
/*    1bf30:	c42a4320 */ 	lwc1	$f10,%lo(var70054320)($at)
/*    1bf34:	3c013fc0 */ 	lui	$at,0x3fc0
/*    1bf38:	460a603c */ 	c.lt.s	$f12,$f10
/*    1bf3c:	00000000 */ 	nop
/*    1bf40:	45020006 */ 	bc1fl	.L0001bf5c
/*    1bf44:	44816000 */ 	mtc1	$at,$f12
/*    1bf48:	0fc259c0 */ 	jal	func0f096700
/*    1bf4c:	00000000 */ 	nop
/*    1bf50:	10000003 */ 	b	.L0001bf60
/*    1bf54:	46000306 */ 	mov.s	$f12,$f0
/*    1bf58:	44816000 */ 	mtc1	$at,$f12
.L0001bf5c:
/*    1bf5c:	00000000 */ 	nop
.L0001bf60:
/*    1bf60:	0c0057b7 */ 	jal	mtx00015edc
/*    1bf64:	8fa50028 */ 	lw	$a1,0x28($sp)
/*    1bf68:	8fa400b8 */ 	lw	$a0,0xb8($sp)
/*    1bf6c:	0c005775 */ 	jal	mtx4SetTranslation
/*    1bf70:	8fa50028 */ 	lw	$a1,0x28($sp)
/*    1bf74:	8fae009c */ 	lw	$t6,0x9c($sp)
/*    1bf78:	8faf004c */ 	lw	$t7,0x4c($sp)
/*    1bf7c:	8fb90048 */ 	lw	$t9,0x48($sp)
/*    1bf80:	11c00005 */ 	beqz	$t6,.L0001bf98
/*    1bf84:	01c02025 */ 	or	$a0,$t6,$zero
/*    1bf88:	000fc180 */ 	sll	$t8,$t7,0x6
/*    1bf8c:	03193021 */ 	addu	$a2,$t8,$t9
/*    1bf90:	0c0056f9 */ 	jal	mtx00015be4
/*    1bf94:	8fa50028 */ 	lw	$a1,0x28($sp)
.L0001bf98:
/*    1bf98:	8fbf0014 */ 	lw	$ra,0x14($sp)
.L0001bf9c:
/*    1bf9c:	27bd00a8 */ 	addiu	$sp,$sp,0xa8
/*    1bfa0:	03e00008 */ 	jr	$ra
/*    1bfa4:	00000000 */ 	nop
);

#if VERSION >= VERSION_JPN_FINAL
GLOBAL_ASM(
glabel model0001bfa8
.late_rodata
glabel var70053fc0pf
.word 0x3c23d70a
.text
/*  1bf60:	27bdfea0 */ 	addiu	$sp,$sp,-352
/*  1bf64:	afbf002c */ 	sw	$ra,0x2c($sp)
/*  1bf68:	afb00028 */ 	sw	$s0,0x28($sp)
/*  1bf6c:	afa40160 */ 	sw	$a0,0x160($sp)
/*  1bf70:	afa50164 */ 	sw	$a1,0x164($sp)
/*  1bf74:	afa60168 */ 	sw	$a2,0x168($sp)
/*  1bf78:	8ccf0004 */ 	lw	$t7,0x4($a2)
/*  1bf7c:	afaf0158 */ 	sw	$t7,0x158($sp)
/*  1bf80:	8cb00020 */ 	lw	$s0,0x20($a1)
/*  1bf84:	5200015a */ 	beqzl	$s0,.JF0001c4f0
/*  1bf88:	8faf0168 */ 	lw	$t7,0x168($sp)
/*  1bf8c:	95f9000c */ 	lhu	$t9,0xc($t7)
/*  1bf90:	3c098006 */ 	lui	$t1,0x8006
/*  1bf94:	27b80144 */ 	addiu	$t8,$sp,0x144
/*  1bf98:	afb90154 */ 	sw	$t9,0x154($sp)
/*  1bf9c:	8ca20008 */ 	lw	$v0,0x8($a1)
/*  1bfa0:	3c013f80 */ 	lui	$at,0x3f80
/*  1bfa4:	8c480004 */ 	lw	$t0,0x4($v0)
/*  1bfa8:	afa80150 */ 	sw	$t0,0x150($sp)
/*  1bfac:	86070000 */ 	lh	$a3,0x0($s0)
/*  1bfb0:	27a80138 */ 	addiu	$t0,$sp,0x138
/*  1bfb4:	10e00054 */ 	beqz	$a3,.JF0001c108
/*  1bfb8:	00075080 */ 	sll	$t2,$a3,0x2
/*  1bfbc:	8d29eecc */ 	lw	$t1,-0x1134($t1)
/*  1bfc0:	01475023 */ 	subu	$t2,$t2,$a3
/*  1bfc4:	000a5080 */ 	sll	$t2,$t2,0x2
/*  1bfc8:	012a5821 */ 	addu	$t3,$t1,$t2
/*  1bfcc:	9163000b */ 	lbu	$v1,0xb($t3)
/*  1bfd0:	27a9012c */ 	addiu	$t1,$sp,0x12c
/*  1bfd4:	306c0002 */ 	andi	$t4,$v1,0x2
/*  1bfd8:	000c182b */ 	sltu	$v1,$zero,$t4
/*  1bfdc:	50600005 */ 	beqzl	$v1,.JF0001bff4
/*  1bfe0:	afa30128 */ 	sw	$v1,0x128($sp)
/*  1bfe4:	8c4f0000 */ 	lw	$t7,0x0($v0)
/*  1bfe8:	00cf1826 */ 	xor	$v1,$a2,$t7
/*  1bfec:	2c630001 */ 	sltiu	$v1,$v1,0x1
/*  1bff0:	afa30128 */ 	sw	$v1,0x128($sp)
.JF0001bff4:
/*  1bff4:	92190004 */ 	lbu	$t9,0x4($s0)
/*  1bff8:	82050008 */ 	lb	$a1,0x8($s0)
/*  1bffc:	afa3003c */ 	sw	$v1,0x3c($sp)
/*  1c000:	afa9001c */ 	sw	$t1,0x1c($sp)
/*  1c004:	afa80018 */ 	sw	$t0,0x18($sp)
/*  1c008:	afb80014 */ 	sw	$t8,0x14($sp)
/*  1c00c:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1c010:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1c014:	0c008ed8 */ 	jal	anim00024050
/*  1c018:	afb90010 */ 	sw	$t9,0x10($sp)
/*  1c01c:	3c0a800a */ 	lui	$t2,0x800a
/*  1c020:	8d4aaafc */ 	lw	$t2,-0x5504($t2)
/*  1c024:	51400013 */ 	beqzl	$t2,.JF0001c074
/*  1c028:	44807000 */ 	mtc1	$zero,$f14
/*  1c02c:	44807000 */ 	mtc1	$zero,$f14
/*  1c030:	c600001c */ 	lwc1	$f0,0x1c($s0)
/*  1c034:	4600703c */ 	c.lt.s	$f14,$f0
/*  1c038:	00000000 */ 	nop
/*  1c03c:	4502000d */ 	bc1fl	.JF0001c074
/*  1c040:	44807000 */ 	mtc1	$zero,$f14
/*  1c044:	c6040010 */ 	lwc1	$f4,0x10($s0)
/*  1c048:	3c017005 */ 	lui	$at,0x7005
/*  1c04c:	c42841dc */ 	lwc1	$f8,0x41dc($at)
/*  1c050:	46002183 */ 	div.s	$f6,$f4,$f0
/*  1c054:	0fc2618c */ 	jal	floorf
/*  1c058:	46083300 */ 	add.s	$f12,$f6,$f8
/*  1c05c:	c60a001c */ 	lwc1	$f10,0x1c($s0)
/*  1c060:	44807000 */ 	mtc1	$zero,$f14
/*  1c064:	460a0082 */ 	mul.s	$f2,$f0,$f10
/*  1c068:	10000004 */ 	b	.JF0001c07c
/*  1c06c:	460e1032 */ 	c.eq.s	$f2,$f14
/*  1c070:	44807000 */ 	mtc1	$zero,$f14
.JF0001c074:
/*  1c074:	c6020010 */ 	lwc1	$f2,0x10($s0)
/*  1c078:	460e1032 */ 	c.eq.s	$f2,$f14
.JF0001c07c:
/*  1c07c:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1c080:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1c084:	27ac00d4 */ 	addiu	$t4,$sp,0xd4
/*  1c088:	4501001c */ 	bc1t	.JF0001c0fc
/*  1c08c:	27ad00c8 */ 	addiu	$t5,$sp,0xc8
/*  1c090:	920b0005 */ 	lbu	$t3,0x5($s0)
/*  1c094:	82050008 */ 	lb	$a1,0x8($s0)
/*  1c098:	86070000 */ 	lh	$a3,0x0($s0)
/*  1c09c:	27ae00bc */ 	addiu	$t6,$sp,0xbc
/*  1c0a0:	afae001c */ 	sw	$t6,0x1c($sp)
/*  1c0a4:	e7a200e0 */ 	swc1	$f2,0xe0($sp)
/*  1c0a8:	afad0018 */ 	sw	$t5,0x18($sp)
/*  1c0ac:	afac0014 */ 	sw	$t4,0x14($sp)
/*  1c0b0:	0c008ed8 */ 	jal	anim00024050
/*  1c0b4:	afab0010 */ 	sw	$t3,0x10($sp)
/*  1c0b8:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*  1c0bc:	27a40144 */ 	addiu	$a0,$sp,0x144
/*  1c0c0:	27a500d4 */ 	addiu	$a1,$sp,0xd4
/*  1c0c4:	44061000 */ 	mfc1	$a2,$f2
/*  1c0c8:	0c006c07 */ 	jal	model0001b07c
/*  1c0cc:	00000000 */ 	nop
/*  1c0d0:	8faf003c */ 	lw	$t7,0x3c($sp)
/*  1c0d4:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*  1c0d8:	3c198006 */ 	lui	$t9,0x8006
/*  1c0dc:	15e00004 */ 	bnez	$t7,.JF0001c0f0
/*  1c0e0:	27a40138 */ 	addiu	$a0,$sp,0x138
/*  1c0e4:	8f39ee7c */ 	lw	$t9,-0x1184($t9)
/*  1c0e8:	53200005 */ 	beqzl	$t9,.JF0001c100
/*  1c0ec:	44807000 */ 	mtc1	$zero,$f14
.JF0001c0f0:
/*  1c0f0:	44061000 */ 	mfc1	$a2,$f2
/*  1c0f4:	0c006bce */ 	jal	model0001af98
/*  1c0f8:	27a500c8 */ 	addiu	$a1,$sp,0xc8
.JF0001c0fc:
/*  1c0fc:	44807000 */ 	mtc1	$zero,$f14
.JF0001c100:
/*  1c100:	10000010 */ 	b	.JF0001c144
/*  1c104:	c6120054 */ 	lwc1	$f18,0x54($s0)
.JF0001c108:
/*  1c108:	44800000 */ 	mtc1	$zero,$f0
/*  1c10c:	44801000 */ 	mtc1	$zero,$f2
/*  1c110:	44807000 */ 	mtc1	$zero,$f14
/*  1c114:	e7a00140 */ 	swc1	$f0,0x140($sp)
/*  1c118:	e7a0013c */ 	swc1	$f0,0x13c($sp)
/*  1c11c:	e7a00138 */ 	swc1	$f0,0x138($sp)
/*  1c120:	44810000 */ 	mtc1	$at,$f0
/*  1c124:	afa00128 */ 	sw	$zero,0x128($sp)
/*  1c128:	e7a20148 */ 	swc1	$f2,0x148($sp)
/*  1c12c:	e7a20144 */ 	swc1	$f2,0x144($sp)
/*  1c130:	e7a2014c */ 	swc1	$f2,0x14c($sp)
/*  1c134:	e7a00130 */ 	swc1	$f0,0x130($sp)
/*  1c138:	e7a0012c */ 	swc1	$f0,0x12c($sp)
/*  1c13c:	e7a00134 */ 	swc1	$f0,0x134($sp)
/*  1c140:	c6120054 */ 	lwc1	$f18,0x54($s0)
.JF0001c144:
/*  1c144:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1c148:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1c14c:	46127032 */ 	c.eq.s	$f14,$f18
/*  1c150:	27a800b0 */ 	addiu	$t0,$sp,0xb0
/*  1c154:	27a900a4 */ 	addiu	$t1,$sp,0xa4
/*  1c158:	27aa0098 */ 	addiu	$t2,$sp,0x98
/*  1c15c:	4501007a */ 	bc1t	.JF0001c348
/*  1c160:	8fb90128 */ 	lw	$t9,0x128($sp)
/*  1c164:	92180006 */ 	lbu	$t8,0x6($s0)
/*  1c168:	82050009 */ 	lb	$a1,0x9($s0)
/*  1c16c:	86070002 */ 	lh	$a3,0x2($s0)
/*  1c170:	afaa001c */ 	sw	$t2,0x1c($sp)
/*  1c174:	afa90018 */ 	sw	$t1,0x18($sp)
/*  1c178:	afa80014 */ 	sw	$t0,0x14($sp)
/*  1c17c:	0c008ed8 */ 	jal	anim00024050
/*  1c180:	afb80010 */ 	sw	$t8,0x10($sp)
/*  1c184:	44802000 */ 	mtc1	$zero,$f4
/*  1c188:	c6060034 */ 	lwc1	$f6,0x34($s0)
/*  1c18c:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1c190:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1c194:	46062032 */ 	c.eq.s	$f4,$f6
/*  1c198:	27ac005c */ 	addiu	$t4,$sp,0x5c
/*  1c19c:	27ad0050 */ 	addiu	$t5,$sp,0x50
/*  1c1a0:	27ae0044 */ 	addiu	$t6,$sp,0x44
/*  1c1a4:	4503000e */ 	bc1tl	.JF0001c1e0
/*  1c1a8:	27a40144 */ 	addiu	$a0,$sp,0x144
/*  1c1ac:	920b0007 */ 	lbu	$t3,0x7($s0)
/*  1c1b0:	82050009 */ 	lb	$a1,0x9($s0)
/*  1c1b4:	86070002 */ 	lh	$a3,0x2($s0)
/*  1c1b8:	afae001c */ 	sw	$t6,0x1c($sp)
/*  1c1bc:	afad0018 */ 	sw	$t5,0x18($sp)
/*  1c1c0:	afac0014 */ 	sw	$t4,0x14($sp)
/*  1c1c4:	0c008ed8 */ 	jal	anim00024050
/*  1c1c8:	afab0010 */ 	sw	$t3,0x10($sp)
/*  1c1cc:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*  1c1d0:	27a5005c */ 	addiu	$a1,$sp,0x5c
/*  1c1d4:	0c006c07 */ 	jal	model0001b07c
/*  1c1d8:	8e060034 */ 	lw	$a2,0x34($s0)
/*  1c1dc:	27a40144 */ 	addiu	$a0,$sp,0x144
.JF0001c1e0:
/*  1c1e0:	0fc25e90 */ 	jal	quaternion0f096ca0
/*  1c1e4:	27a50088 */ 	addiu	$a1,$sp,0x88
/*  1c1e8:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*  1c1ec:	0fc25e90 */ 	jal	quaternion0f096ca0
/*  1c1f0:	27a50078 */ 	addiu	$a1,$sp,0x78
/*  1c1f4:	27a40088 */ 	addiu	$a0,$sp,0x88
/*  1c1f8:	0fc26118 */ 	jal	quaternion0f0976c0
/*  1c1fc:	27a50078 */ 	addiu	$a1,$sp,0x78
/*  1c200:	27a70068 */ 	addiu	$a3,$sp,0x68
/*  1c204:	27a40088 */ 	addiu	$a0,$sp,0x88
/*  1c208:	27a50078 */ 	addiu	$a1,$sp,0x78
/*  1c20c:	0fc26016 */ 	jal	quaternionSlerp
/*  1c210:	8e060054 */ 	lw	$a2,0x54($s0)
/*  1c214:	44804000 */ 	mtc1	$zero,$f8
/*  1c218:	c7aa0138 */ 	lwc1	$f10,0x138($sp)
/*  1c21c:	27a70068 */ 	addiu	$a3,$sp,0x68
/*  1c220:	c7b2013c */ 	lwc1	$f18,0x13c($sp)
/*  1c224:	460a4032 */ 	c.eq.s	$f8,$f10
/*  1c228:	c7a80138 */ 	lwc1	$f8,0x138($sp)
/*  1c22c:	27b80138 */ 	addiu	$t8,$sp,0x138
/*  1c230:	27a8012c */ 	addiu	$t0,$sp,0x12c
/*  1c234:	4502000e */ 	bc1fl	.JF0001c270
/*  1c238:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1c23c:	44808000 */ 	mtc1	$zero,$f16
/*  1c240:	c7a60140 */ 	lwc1	$f6,0x140($sp)
/*  1c244:	46128032 */ 	c.eq.s	$f16,$f18
/*  1c248:	00000000 */ 	nop
/*  1c24c:	45020008 */ 	bc1fl	.JF0001c270
/*  1c250:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1c254:	44802000 */ 	mtc1	$zero,$f4
/*  1c258:	8fa90164 */ 	lw	$t1,0x164($sp)
/*  1c25c:	46062032 */ 	c.eq.s	$f4,$f6
/*  1c260:	00000000 */ 	nop
/*  1c264:	45030023 */ 	bc1tl	.JF0001c2f4
/*  1c268:	8d2a0008 */ 	lw	$t2,0x8($t1)
/*  1c26c:	c60a0088 */ 	lwc1	$f10,0x88($s0)
.JF0001c270:
/*  1c270:	c7b2013c */ 	lwc1	$f18,0x13c($sp)
/*  1c274:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1c278:	460a4402 */ 	mul.s	$f16,$f8,$f10
/*  1c27c:	c7a80140 */ 	lwc1	$f8,0x140($sp)
/*  1c280:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1c284:	8fa20158 */ 	lw	$v0,0x158($sp)
/*  1c288:	e7b00138 */ 	swc1	$f16,0x138($sp)
/*  1c28c:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*  1c290:	46049182 */ 	mul.s	$f6,$f18,$f4
/*  1c294:	e7a6013c */ 	swc1	$f6,0x13c($sp)
/*  1c298:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1c29c:	460a4482 */ 	mul.s	$f18,$f8,$f10
/*  1c2a0:	e7b20140 */ 	swc1	$f18,0x140($sp)
/*  1c2a4:	8caf0008 */ 	lw	$t7,0x8($a1)
/*  1c2a8:	8df90000 */ 	lw	$t9,0x0($t7)
/*  1c2ac:	50d9000b */ 	beql	$a2,$t9,.JF0001c2dc
/*  1c2b0:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1c2b4:	c4440000 */ 	lwc1	$f4,0x0($v0)
/*  1c2b8:	46048200 */ 	add.s	$f8,$f16,$f4
/*  1c2bc:	e7a80138 */ 	swc1	$f8,0x138($sp)
/*  1c2c0:	c44a0004 */ 	lwc1	$f10,0x4($v0)
/*  1c2c4:	460a3400 */ 	add.s	$f16,$f6,$f10
/*  1c2c8:	e7b0013c */ 	swc1	$f16,0x13c($sp)
/*  1c2cc:	c4440008 */ 	lwc1	$f4,0x8($v0)
/*  1c2d0:	46049200 */ 	add.s	$f8,$f18,$f4
/*  1c2d4:	e7a80140 */ 	swc1	$f8,0x140($sp)
/*  1c2d8:	8fa40160 */ 	lw	$a0,0x160($sp)
.JF0001c2dc:
/*  1c2dc:	afb80010 */ 	sw	$t8,0x10($sp)
/*  1c2e0:	0c006ef3 */ 	jal	model0001bc14
/*  1c2e4:	afa80014 */ 	sw	$t0,0x14($sp)
/*  1c2e8:	100000a2 */ 	b	.JF0001c574
/*  1c2ec:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  1c2f0:	8d2a0008 */ 	lw	$t2,0x8($t1)
.JF0001c2f4:
/*  1c2f4:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1c2f8:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1c2fc:	8d4b0000 */ 	lw	$t3,0x0($t2)
/*  1c300:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1c304:	27ae0138 */ 	addiu	$t6,$sp,0x138
/*  1c308:	10cb000a */ 	beq	$a2,$t3,.JF0001c334
/*  1c30c:	27af012c */ 	addiu	$t7,$sp,0x12c
/*  1c310:	8fac0158 */ 	lw	$t4,0x158($sp)
/*  1c314:	27ad012c */ 	addiu	$t5,$sp,0x12c
/*  1c318:	afad0014 */ 	sw	$t5,0x14($sp)
/*  1c31c:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1c320:	01202825 */ 	move	$a1,$t1
/*  1c324:	0c006ef3 */ 	jal	model0001bc14
/*  1c328:	afac0010 */ 	sw	$t4,0x10($sp)
/*  1c32c:	10000091 */ 	b	.JF0001c574
/*  1c330:	8fbf002c */ 	lw	$ra,0x2c($sp)
.JF0001c334:
/*  1c334:	afae0010 */ 	sw	$t6,0x10($sp)
/*  1c338:	0c006ef3 */ 	jal	model0001bc14
/*  1c33c:	afaf0014 */ 	sw	$t7,0x14($sp)
/*  1c340:	1000008c */ 	b	.JF0001c574
/*  1c344:	8fbf002c */ 	lw	$ra,0x2c($sp)
.JF0001c348:
/*  1c348:	13200019 */ 	beqz	$t9,.JF0001c3b0
/*  1c34c:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*  1c350:	0fc57326 */ 	jal	func0f15c888
/*  1c354:	00000000 */ 	nop
/*  1c358:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*  1c35c:	c7b0013c */ 	lwc1	$f16,0x13c($sp)
/*  1c360:	c7a40140 */ 	lwc1	$f4,0x140($sp)
/*  1c364:	46003282 */ 	mul.s	$f10,$f6,$f0
/*  1c368:	27b80138 */ 	addiu	$t8,$sp,0x138
/*  1c36c:	24080001 */ 	li	$t0,0x1
/*  1c370:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  1c374:	27aa012c */ 	addiu	$t2,$sp,0x12c
/*  1c378:	afaa0018 */ 	sw	$t2,0x18($sp)
/*  1c37c:	46002202 */ 	mul.s	$f8,$f4,$f0
/*  1c380:	e7aa0138 */ 	swc1	$f10,0x138($sp)
/*  1c384:	afa80014 */ 	sw	$t0,0x14($sp)
/*  1c388:	afb80010 */ 	sw	$t8,0x10($sp)
/*  1c38c:	e7b2013c */ 	swc1	$f18,0x13c($sp)
/*  1c390:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1c394:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1c398:	e7a80140 */ 	swc1	$f8,0x140($sp)
/*  1c39c:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1c3a0:	0c006df1 */ 	jal	model0001b80c
/*  1c3a4:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1c3a8:	10000072 */ 	b	.JF0001c574
/*  1c3ac:	8fbf002c */ 	lw	$ra,0x2c($sp)
.JF0001c3b0:
/*  1c3b0:	46067032 */ 	c.eq.s	$f14,$f6
/*  1c3b4:	c7aa013c */ 	lwc1	$f10,0x13c($sp)
/*  1c3b8:	c7b20138 */ 	lwc1	$f18,0x138($sp)
/*  1c3bc:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1c3c0:	45000009 */ 	bc1f	.JF0001c3e8
/*  1c3c4:	27af0138 */ 	addiu	$t7,$sp,0x138
/*  1c3c8:	460a7032 */ 	c.eq.s	$f14,$f10
/*  1c3cc:	c7b00140 */ 	lwc1	$f16,0x140($sp)
/*  1c3d0:	45020006 */ 	bc1fl	.JF0001c3ec
/*  1c3d4:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*  1c3d8:	46107032 */ 	c.eq.s	$f14,$f16
/*  1c3dc:	8fa80164 */ 	lw	$t0,0x164($sp)
/*  1c3e0:	45030027 */ 	bc1tl	.JF0001c480
/*  1c3e4:	8d0a0008 */ 	lw	$t2,0x8($t0)
.JF0001c3e8:
/*  1c3e8:	c6040088 */ 	lwc1	$f4,0x88($s0)
.JF0001c3ec:
/*  1c3ec:	c7a6013c */ 	lwc1	$f6,0x13c($sp)
/*  1c3f0:	8fa90164 */ 	lw	$t1,0x164($sp)
/*  1c3f4:	46049202 */ 	mul.s	$f8,$f18,$f4
/*  1c3f8:	c7b20140 */ 	lwc1	$f18,0x140($sp)
/*  1c3fc:	8fab0168 */ 	lw	$t3,0x168($sp)
/*  1c400:	8fae0158 */ 	lw	$t6,0x158($sp)
/*  1c404:	27b9012c */ 	addiu	$t9,$sp,0x12c
/*  1c408:	e7a80138 */ 	swc1	$f8,0x138($sp)
/*  1c40c:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1c410:	460a3402 */ 	mul.s	$f16,$f6,$f10
/*  1c414:	e7b0013c */ 	swc1	$f16,0x13c($sp)
/*  1c418:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*  1c41c:	46049182 */ 	mul.s	$f6,$f18,$f4
/*  1c420:	e7a60140 */ 	swc1	$f6,0x140($sp)
/*  1c424:	8d2c0008 */ 	lw	$t4,0x8($t1)
/*  1c428:	8d8d0000 */ 	lw	$t5,0x0($t4)
/*  1c42c:	516d000b */ 	beql	$t3,$t5,.JF0001c45c
/*  1c430:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1c434:	c5ca0000 */ 	lwc1	$f10,0x0($t6)
/*  1c438:	460a4480 */ 	add.s	$f18,$f8,$f10
/*  1c43c:	e7b20138 */ 	swc1	$f18,0x138($sp)
/*  1c440:	c5c40004 */ 	lwc1	$f4,0x4($t6)
/*  1c444:	46048200 */ 	add.s	$f8,$f16,$f4
/*  1c448:	e7a8013c */ 	swc1	$f8,0x13c($sp)
/*  1c44c:	c5ca0008 */ 	lwc1	$f10,0x8($t6)
/*  1c450:	460a3480 */ 	add.s	$f18,$f6,$f10
/*  1c454:	e7b20140 */ 	swc1	$f18,0x140($sp)
/*  1c458:	8fa40160 */ 	lw	$a0,0x160($sp)
.JF0001c45c:
/*  1c45c:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1c460:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1c464:	afaf0010 */ 	sw	$t7,0x10($sp)
/*  1c468:	afa00014 */ 	sw	$zero,0x14($sp)
/*  1c46c:	0c006df1 */ 	jal	model0001b80c
/*  1c470:	afb90018 */ 	sw	$t9,0x18($sp)
/*  1c474:	1000003f */ 	b	.JF0001c574
/*  1c478:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  1c47c:	8d0a0008 */ 	lw	$t2,0x8($t0)
.JF0001c480:
/*  1c480:	8fb80168 */ 	lw	$t8,0x168($sp)
/*  1c484:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1c488:	8d490000 */ 	lw	$t1,0x0($t2)
/*  1c48c:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1c490:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1c494:	1309000d */ 	beq	$t8,$t1,.JF0001c4cc
/*  1c498:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1c49c:	8fac0158 */ 	lw	$t4,0x158($sp)
/*  1c4a0:	27ab012c */ 	addiu	$t3,$sp,0x12c
/*  1c4a4:	afab0018 */ 	sw	$t3,0x18($sp)
/*  1c4a8:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1c4ac:	01002825 */ 	move	$a1,$t0
/*  1c4b0:	03003025 */ 	move	$a2,$t8
/*  1c4b4:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1c4b8:	afa00014 */ 	sw	$zero,0x14($sp)
/*  1c4bc:	0c006df1 */ 	jal	model0001b80c
/*  1c4c0:	afac0010 */ 	sw	$t4,0x10($sp)
/*  1c4c4:	1000002b */ 	b	.JF0001c574
/*  1c4c8:	8fbf002c */ 	lw	$ra,0x2c($sp)
.JF0001c4cc:
/*  1c4cc:	27ad0138 */ 	addiu	$t5,$sp,0x138
/*  1c4d0:	27ae012c */ 	addiu	$t6,$sp,0x12c
/*  1c4d4:	afae0018 */ 	sw	$t6,0x18($sp)
/*  1c4d8:	afad0010 */ 	sw	$t5,0x10($sp)
/*  1c4dc:	0c006df1 */ 	jal	model0001b80c
/*  1c4e0:	afa00014 */ 	sw	$zero,0x14($sp)
/*  1c4e4:	10000023 */ 	b	.JF0001c574
/*  1c4e8:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  1c4ec:	8faf0168 */ 	lw	$t7,0x168($sp)
.JF0001c4f0:
/*  1c4f0:	8fa40164 */ 	lw	$a0,0x164($sp)
/*  1c4f4:	8fb90160 */ 	lw	$t9,0x160($sp)
/*  1c4f8:	8de50008 */ 	lw	$a1,0x8($t7)
/*  1c4fc:	50a00006 */ 	beqzl	$a1,.JF0001c518
/*  1c500:	8f300000 */ 	lw	$s0,0x0($t9)
/*  1c504:	0c00695b */ 	jal	model0001a5cc
/*  1c508:	00003025 */ 	move	$a2,$zero
/*  1c50c:	10000002 */ 	b	.JF0001c518
/*  1c510:	00408025 */ 	move	$s0,$v0
/*  1c514:	8f300000 */ 	lw	$s0,0x0($t9)
.JF0001c518:
/*  1c518:	1200000f */ 	beqz	$s0,.JF0001c558
/*  1c51c:	8fa40158 */ 	lw	$a0,0x158($sp)
/*  1c520:	8fa40158 */ 	lw	$a0,0x158($sp)
/*  1c524:	0c00599f */ 	jal	mtx4LoadTranslation
/*  1c528:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*  1c52c:	8fa80158 */ 	lw	$t0,0x158($sp)
/*  1c530:	8faa0164 */ 	lw	$t2,0x164($sp)
/*  1c534:	02002025 */ 	move	$a0,$s0
/*  1c538:	8518000e */ 	lh	$t8,0xe($t0)
/*  1c53c:	8d49000c */ 	lw	$t1,0xc($t2)
/*  1c540:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*  1c544:	00186180 */ 	sll	$t4,$t8,0x6
/*  1c548:	0c0056e1 */ 	jal	mtx00015be4
/*  1c54c:	012c3021 */ 	addu	$a2,$t1,$t4
/*  1c550:	10000008 */ 	b	.JF0001c574
/*  1c554:	8fbf002c */ 	lw	$ra,0x2c($sp)
.JF0001c558:
/*  1c558:	8fad0164 */ 	lw	$t5,0x164($sp)
/*  1c55c:	848f000e */ 	lh	$t7,0xe($a0)
/*  1c560:	8dae000c */ 	lw	$t6,0xc($t5)
/*  1c564:	000fc980 */ 	sll	$t9,$t7,0x6
/*  1c568:	0c00599f */ 	jal	mtx4LoadTranslation
/*  1c56c:	01d92821 */ 	addu	$a1,$t6,$t9
/*  1c570:	8fbf002c */ 	lw	$ra,0x2c($sp)
.JF0001c574:
/*  1c574:	8fb00028 */ 	lw	$s0,0x28($sp)
/*  1c578:	27bd0160 */ 	addiu	$sp,$sp,0x160
/*  1c57c:	03e00008 */ 	jr	$ra
/*  1c580:	00000000 */ 	nop
);
#elif VERSION >= VERSION_PAL_FINAL
GLOBAL_ASM(
glabel model0001bfa8
.late_rodata
glabel var70053fc0pf
.word 0x3c23d70a
.text
/*    1bd50:	27bdfea0 */ 	addiu	$sp,$sp,-352
/*    1bd54:	afbf002c */ 	sw	$ra,0x2c($sp)
/*    1bd58:	afb00028 */ 	sw	$s0,0x28($sp)
/*    1bd5c:	afa40160 */ 	sw	$a0,0x160($sp)
/*    1bd60:	afa50164 */ 	sw	$a1,0x164($sp)
/*    1bd64:	afa60168 */ 	sw	$a2,0x168($sp)
/*    1bd68:	8ccf0004 */ 	lw	$t7,0x4($a2)
/*    1bd6c:	afaf0158 */ 	sw	$t7,0x158($sp)
/*    1bd70:	8cb00020 */ 	lw	$s0,0x20($a1)
/*    1bd74:	5200015a */ 	beqzl	$s0,.PF0001c2e0
/*    1bd78:	8faf0168 */ 	lw	$t7,0x168($sp)
/*    1bd7c:	95f9000c */ 	lhu	$t9,0xc($t7)
/*    1bd80:	3c098006 */ 	lui	$t1,0x8006
/*    1bd84:	27b80144 */ 	addiu	$t8,$sp,0x144
/*    1bd88:	afb90154 */ 	sw	$t9,0x154($sp)
/*    1bd8c:	8ca20008 */ 	lw	$v0,0x8($a1)
/*    1bd90:	3c013f80 */ 	lui	$at,0x3f80
/*    1bd94:	8c480004 */ 	lw	$t0,0x4($v0)
/*    1bd98:	afa80150 */ 	sw	$t0,0x150($sp)
/*    1bd9c:	86070000 */ 	lh	$a3,0x0($s0)
/*    1bda0:	27a80138 */ 	addiu	$t0,$sp,0x138
/*    1bda4:	10e00054 */ 	beqz	$a3,.PF0001bef8
/*    1bda8:	00075080 */ 	sll	$t2,$a3,0x2
/*    1bdac:	8d29ecbc */ 	lw	$t1,-0x1344($t1)
/*    1bdb0:	01475023 */ 	subu	$t2,$t2,$a3
/*    1bdb4:	000a5080 */ 	sll	$t2,$t2,0x2
/*    1bdb8:	012a5821 */ 	addu	$t3,$t1,$t2
/*    1bdbc:	9163000b */ 	lbu	$v1,0xb($t3)
/*    1bdc0:	27a9012c */ 	addiu	$t1,$sp,0x12c
/*    1bdc4:	306c0002 */ 	andi	$t4,$v1,0x2
/*    1bdc8:	000c182b */ 	sltu	$v1,$zero,$t4
/*    1bdcc:	50600005 */ 	beqzl	$v1,.PF0001bde4
/*    1bdd0:	afa30128 */ 	sw	$v1,0x128($sp)
/*    1bdd4:	8c4f0000 */ 	lw	$t7,0x0($v0)
/*    1bdd8:	00cf1826 */ 	xor	$v1,$a2,$t7
/*    1bddc:	2c630001 */ 	sltiu	$v1,$v1,0x1
/*    1bde0:	afa30128 */ 	sw	$v1,0x128($sp)
.PF0001bde4:
/*    1bde4:	92190004 */ 	lbu	$t9,0x4($s0)
/*    1bde8:	82050008 */ 	lb	$a1,0x8($s0)
/*    1bdec:	afa3003c */ 	sw	$v1,0x3c($sp)
/*    1bdf0:	afa9001c */ 	sw	$t1,0x1c($sp)
/*    1bdf4:	afa80018 */ 	sw	$t0,0x18($sp)
/*    1bdf8:	afb80014 */ 	sw	$t8,0x14($sp)
/*    1bdfc:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1be00:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1be04:	0c008e54 */ 	jal	anim00024050
/*    1be08:	afb90010 */ 	sw	$t9,0x10($sp)
/*    1be0c:	3c0a800a */ 	lui	$t2,0x800a
/*    1be10:	8d4aa9dc */ 	lw	$t2,-0x5624($t2)
/*    1be14:	51400013 */ 	beqzl	$t2,.PF0001be64
/*    1be18:	44807000 */ 	mtc1	$zero,$f14
/*    1be1c:	44807000 */ 	mtc1	$zero,$f14
/*    1be20:	c600001c */ 	lwc1	$f0,0x1c($s0)
/*    1be24:	4600703c */ 	c.lt.s	$f14,$f0
/*    1be28:	00000000 */ 	nop
/*    1be2c:	4502000d */ 	bc1fl	.PF0001be64
/*    1be30:	44807000 */ 	mtc1	$zero,$f14
/*    1be34:	c6040010 */ 	lwc1	$f4,0x10($s0)
/*    1be38:	3c017005 */ 	lui	$at,0x7005
/*    1be3c:	c4283fcc */ 	lwc1	$f8,0x3fcc($at)
/*    1be40:	46002183 */ 	div.s	$f6,$f4,$f0
/*    1be44:	0fc25dec */ 	jal	floorf
/*    1be48:	46083300 */ 	add.s	$f12,$f6,$f8
/*    1be4c:	c60a001c */ 	lwc1	$f10,0x1c($s0)
/*    1be50:	44807000 */ 	mtc1	$zero,$f14
/*    1be54:	460a0082 */ 	mul.s	$f2,$f0,$f10
/*    1be58:	10000004 */ 	b	.PF0001be6c
/*    1be5c:	460e1032 */ 	c.eq.s	$f2,$f14
/*    1be60:	44807000 */ 	mtc1	$zero,$f14
.PF0001be64:
/*    1be64:	c6020010 */ 	lwc1	$f2,0x10($s0)
/*    1be68:	460e1032 */ 	c.eq.s	$f2,$f14
.PF0001be6c:
/*    1be6c:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1be70:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1be74:	27ac00d4 */ 	addiu	$t4,$sp,0xd4
/*    1be78:	4501001c */ 	bc1t	.PF0001beec
/*    1be7c:	27ad00c8 */ 	addiu	$t5,$sp,0xc8
/*    1be80:	920b0005 */ 	lbu	$t3,0x5($s0)
/*    1be84:	82050008 */ 	lb	$a1,0x8($s0)
/*    1be88:	86070000 */ 	lh	$a3,0x0($s0)
/*    1be8c:	27ae00bc */ 	addiu	$t6,$sp,0xbc
/*    1be90:	afae001c */ 	sw	$t6,0x1c($sp)
/*    1be94:	e7a200e0 */ 	swc1	$f2,0xe0($sp)
/*    1be98:	afad0018 */ 	sw	$t5,0x18($sp)
/*    1be9c:	afac0014 */ 	sw	$t4,0x14($sp)
/*    1bea0:	0c008e54 */ 	jal	anim00024050
/*    1bea4:	afab0010 */ 	sw	$t3,0x10($sp)
/*    1bea8:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*    1beac:	27a40144 */ 	addiu	$a0,$sp,0x144
/*    1beb0:	27a500d4 */ 	addiu	$a1,$sp,0xd4
/*    1beb4:	44061000 */ 	mfc1	$a2,$f2
/*    1beb8:	0c006b83 */ 	jal	model0001b07c
/*    1bebc:	00000000 */ 	nop
/*    1bec0:	8faf003c */ 	lw	$t7,0x3c($sp)
/*    1bec4:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*    1bec8:	3c198006 */ 	lui	$t9,0x8006
/*    1becc:	15e00004 */ 	bnez	$t7,.PF0001bee0
/*    1bed0:	27a40138 */ 	addiu	$a0,$sp,0x138
/*    1bed4:	8f39ec6c */ 	lw	$t9,-0x1394($t9)
/*    1bed8:	53200005 */ 	beqzl	$t9,.PF0001bef0
/*    1bedc:	44807000 */ 	mtc1	$zero,$f14
.PF0001bee0:
/*    1bee0:	44061000 */ 	mfc1	$a2,$f2
/*    1bee4:	0c006b4a */ 	jal	model0001af98
/*    1bee8:	27a500c8 */ 	addiu	$a1,$sp,0xc8
.PF0001beec:
/*    1beec:	44807000 */ 	mtc1	$zero,$f14
.PF0001bef0:
/*    1bef0:	10000010 */ 	b	.PF0001bf34
/*    1bef4:	c6120054 */ 	lwc1	$f18,0x54($s0)
.PF0001bef8:
/*    1bef8:	44800000 */ 	mtc1	$zero,$f0
/*    1befc:	44801000 */ 	mtc1	$zero,$f2
/*    1bf00:	44807000 */ 	mtc1	$zero,$f14
/*    1bf04:	e7a00140 */ 	swc1	$f0,0x140($sp)
/*    1bf08:	e7a0013c */ 	swc1	$f0,0x13c($sp)
/*    1bf0c:	e7a00138 */ 	swc1	$f0,0x138($sp)
/*    1bf10:	44810000 */ 	mtc1	$at,$f0
/*    1bf14:	afa00128 */ 	sw	$zero,0x128($sp)
/*    1bf18:	e7a20148 */ 	swc1	$f2,0x148($sp)
/*    1bf1c:	e7a20144 */ 	swc1	$f2,0x144($sp)
/*    1bf20:	e7a2014c */ 	swc1	$f2,0x14c($sp)
/*    1bf24:	e7a00130 */ 	swc1	$f0,0x130($sp)
/*    1bf28:	e7a0012c */ 	swc1	$f0,0x12c($sp)
/*    1bf2c:	e7a00134 */ 	swc1	$f0,0x134($sp)
/*    1bf30:	c6120054 */ 	lwc1	$f18,0x54($s0)
.PF0001bf34:
/*    1bf34:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1bf38:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1bf3c:	46127032 */ 	c.eq.s	$f14,$f18
/*    1bf40:	27a800b0 */ 	addiu	$t0,$sp,0xb0
/*    1bf44:	27a900a4 */ 	addiu	$t1,$sp,0xa4
/*    1bf48:	27aa0098 */ 	addiu	$t2,$sp,0x98
/*    1bf4c:	4501007a */ 	bc1t	.PF0001c138
/*    1bf50:	8fb90128 */ 	lw	$t9,0x128($sp)
/*    1bf54:	92180006 */ 	lbu	$t8,0x6($s0)
/*    1bf58:	82050009 */ 	lb	$a1,0x9($s0)
/*    1bf5c:	86070002 */ 	lh	$a3,0x2($s0)
/*    1bf60:	afaa001c */ 	sw	$t2,0x1c($sp)
/*    1bf64:	afa90018 */ 	sw	$t1,0x18($sp)
/*    1bf68:	afa80014 */ 	sw	$t0,0x14($sp)
/*    1bf6c:	0c008e54 */ 	jal	anim00024050
/*    1bf70:	afb80010 */ 	sw	$t8,0x10($sp)
/*    1bf74:	44802000 */ 	mtc1	$zero,$f4
/*    1bf78:	c6060034 */ 	lwc1	$f6,0x34($s0)
/*    1bf7c:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1bf80:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1bf84:	46062032 */ 	c.eq.s	$f4,$f6
/*    1bf88:	27ac005c */ 	addiu	$t4,$sp,0x5c
/*    1bf8c:	27ad0050 */ 	addiu	$t5,$sp,0x50
/*    1bf90:	27ae0044 */ 	addiu	$t6,$sp,0x44
/*    1bf94:	4503000e */ 	bc1tl	.PF0001bfd0
/*    1bf98:	27a40144 */ 	addiu	$a0,$sp,0x144
/*    1bf9c:	920b0007 */ 	lbu	$t3,0x7($s0)
/*    1bfa0:	82050009 */ 	lb	$a1,0x9($s0)
/*    1bfa4:	86070002 */ 	lh	$a3,0x2($s0)
/*    1bfa8:	afae001c */ 	sw	$t6,0x1c($sp)
/*    1bfac:	afad0018 */ 	sw	$t5,0x18($sp)
/*    1bfb0:	afac0014 */ 	sw	$t4,0x14($sp)
/*    1bfb4:	0c008e54 */ 	jal	anim00024050
/*    1bfb8:	afab0010 */ 	sw	$t3,0x10($sp)
/*    1bfbc:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*    1bfc0:	27a5005c */ 	addiu	$a1,$sp,0x5c
/*    1bfc4:	0c006b83 */ 	jal	model0001b07c
/*    1bfc8:	8e060034 */ 	lw	$a2,0x34($s0)
/*    1bfcc:	27a40144 */ 	addiu	$a0,$sp,0x144
.PF0001bfd0:
/*    1bfd0:	0fc25af0 */ 	jal	quaternion0f096ca0
/*    1bfd4:	27a50088 */ 	addiu	$a1,$sp,0x88
/*    1bfd8:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*    1bfdc:	0fc25af0 */ 	jal	quaternion0f096ca0
/*    1bfe0:	27a50078 */ 	addiu	$a1,$sp,0x78
/*    1bfe4:	27a40088 */ 	addiu	$a0,$sp,0x88
/*    1bfe8:	0fc25d78 */ 	jal	quaternion0f0976c0
/*    1bfec:	27a50078 */ 	addiu	$a1,$sp,0x78
/*    1bff0:	27a70068 */ 	addiu	$a3,$sp,0x68
/*    1bff4:	27a40088 */ 	addiu	$a0,$sp,0x88
/*    1bff8:	27a50078 */ 	addiu	$a1,$sp,0x78
/*    1bffc:	0fc25c76 */ 	jal	quaternionSlerp
/*    1c000:	8e060054 */ 	lw	$a2,0x54($s0)
/*    1c004:	44804000 */ 	mtc1	$zero,$f8
/*    1c008:	c7aa0138 */ 	lwc1	$f10,0x138($sp)
/*    1c00c:	27a70068 */ 	addiu	$a3,$sp,0x68
/*    1c010:	c7b2013c */ 	lwc1	$f18,0x13c($sp)
/*    1c014:	460a4032 */ 	c.eq.s	$f8,$f10
/*    1c018:	c7a80138 */ 	lwc1	$f8,0x138($sp)
/*    1c01c:	27b80138 */ 	addiu	$t8,$sp,0x138
/*    1c020:	27a8012c */ 	addiu	$t0,$sp,0x12c
/*    1c024:	4502000e */ 	bc1fl	.PF0001c060
/*    1c028:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*    1c02c:	44808000 */ 	mtc1	$zero,$f16
/*    1c030:	c7a60140 */ 	lwc1	$f6,0x140($sp)
/*    1c034:	46128032 */ 	c.eq.s	$f16,$f18
/*    1c038:	00000000 */ 	nop
/*    1c03c:	45020008 */ 	bc1fl	.PF0001c060
/*    1c040:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*    1c044:	44802000 */ 	mtc1	$zero,$f4
/*    1c048:	8fa90164 */ 	lw	$t1,0x164($sp)
/*    1c04c:	46062032 */ 	c.eq.s	$f4,$f6
/*    1c050:	00000000 */ 	nop
/*    1c054:	45030023 */ 	bc1tl	.PF0001c0e4
/*    1c058:	8d2a0008 */ 	lw	$t2,0x8($t1)
/*    1c05c:	c60a0088 */ 	lwc1	$f10,0x88($s0)
.PF0001c060:
/*    1c060:	c7b2013c */ 	lwc1	$f18,0x13c($sp)
/*    1c064:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c068:	460a4402 */ 	mul.s	$f16,$f8,$f10
/*    1c06c:	c7a80140 */ 	lwc1	$f8,0x140($sp)
/*    1c070:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c074:	8fa20158 */ 	lw	$v0,0x158($sp)
/*    1c078:	e7b00138 */ 	swc1	$f16,0x138($sp)
/*    1c07c:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*    1c080:	46049182 */ 	mul.s	$f6,$f18,$f4
/*    1c084:	e7a6013c */ 	swc1	$f6,0x13c($sp)
/*    1c088:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*    1c08c:	460a4482 */ 	mul.s	$f18,$f8,$f10
/*    1c090:	e7b20140 */ 	swc1	$f18,0x140($sp)
/*    1c094:	8caf0008 */ 	lw	$t7,0x8($a1)
/*    1c098:	8df90000 */ 	lw	$t9,0x0($t7)
/*    1c09c:	50d9000b */ 	beql	$a2,$t9,.PF0001c0cc
/*    1c0a0:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c0a4:	c4440000 */ 	lwc1	$f4,0x0($v0)
/*    1c0a8:	46048200 */ 	add.s	$f8,$f16,$f4
/*    1c0ac:	e7a80138 */ 	swc1	$f8,0x138($sp)
/*    1c0b0:	c44a0004 */ 	lwc1	$f10,0x4($v0)
/*    1c0b4:	460a3400 */ 	add.s	$f16,$f6,$f10
/*    1c0b8:	e7b0013c */ 	swc1	$f16,0x13c($sp)
/*    1c0bc:	c4440008 */ 	lwc1	$f4,0x8($v0)
/*    1c0c0:	46049200 */ 	add.s	$f8,$f18,$f4
/*    1c0c4:	e7a80140 */ 	swc1	$f8,0x140($sp)
/*    1c0c8:	8fa40160 */ 	lw	$a0,0x160($sp)
.PF0001c0cc:
/*    1c0cc:	afb80010 */ 	sw	$t8,0x10($sp)
/*    1c0d0:	0c006e6f */ 	jal	model0001bc14
/*    1c0d4:	afa80014 */ 	sw	$t0,0x14($sp)
/*    1c0d8:	100000a2 */ 	b	.PF0001c364
/*    1c0dc:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*    1c0e0:	8d2a0008 */ 	lw	$t2,0x8($t1)
.PF0001c0e4:
/*    1c0e4:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c0e8:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c0ec:	8d4b0000 */ 	lw	$t3,0x0($t2)
/*    1c0f0:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c0f4:	27ae0138 */ 	addiu	$t6,$sp,0x138
/*    1c0f8:	10cb000a */ 	beq	$a2,$t3,.PF0001c124
/*    1c0fc:	27af012c */ 	addiu	$t7,$sp,0x12c
/*    1c100:	8fac0158 */ 	lw	$t4,0x158($sp)
/*    1c104:	27ad012c */ 	addiu	$t5,$sp,0x12c
/*    1c108:	afad0014 */ 	sw	$t5,0x14($sp)
/*    1c10c:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c110:	01202825 */ 	move	$a1,$t1
/*    1c114:	0c006e6f */ 	jal	model0001bc14
/*    1c118:	afac0010 */ 	sw	$t4,0x10($sp)
/*    1c11c:	10000091 */ 	b	.PF0001c364
/*    1c120:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PF0001c124:
/*    1c124:	afae0010 */ 	sw	$t6,0x10($sp)
/*    1c128:	0c006e6f */ 	jal	model0001bc14
/*    1c12c:	afaf0014 */ 	sw	$t7,0x14($sp)
/*    1c130:	1000008c */ 	b	.PF0001c364
/*    1c134:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PF0001c138:
/*    1c138:	13200019 */ 	beqz	$t9,.PF0001c1a0
/*    1c13c:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*    1c140:	0fc57602 */ 	jal	func0f15c888
/*    1c144:	00000000 */ 	nop
/*    1c148:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*    1c14c:	c7b0013c */ 	lwc1	$f16,0x13c($sp)
/*    1c150:	c7a40140 */ 	lwc1	$f4,0x140($sp)
/*    1c154:	46003282 */ 	mul.s	$f10,$f6,$f0
/*    1c158:	27b80138 */ 	addiu	$t8,$sp,0x138
/*    1c15c:	24080001 */ 	li	$t0,0x1
/*    1c160:	46008482 */ 	mul.s	$f18,$f16,$f0
/*    1c164:	27aa012c */ 	addiu	$t2,$sp,0x12c
/*    1c168:	afaa0018 */ 	sw	$t2,0x18($sp)
/*    1c16c:	46002202 */ 	mul.s	$f8,$f4,$f0
/*    1c170:	e7aa0138 */ 	swc1	$f10,0x138($sp)
/*    1c174:	afa80014 */ 	sw	$t0,0x14($sp)
/*    1c178:	afb80010 */ 	sw	$t8,0x10($sp)
/*    1c17c:	e7b2013c */ 	swc1	$f18,0x13c($sp)
/*    1c180:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c184:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c188:	e7a80140 */ 	swc1	$f8,0x140($sp)
/*    1c18c:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c190:	0c006d6d */ 	jal	model0001b80c
/*    1c194:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c198:	10000072 */ 	b	.PF0001c364
/*    1c19c:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PF0001c1a0:
/*    1c1a0:	46067032 */ 	c.eq.s	$f14,$f6
/*    1c1a4:	c7aa013c */ 	lwc1	$f10,0x13c($sp)
/*    1c1a8:	c7b20138 */ 	lwc1	$f18,0x138($sp)
/*    1c1ac:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c1b0:	45000009 */ 	bc1f	.PF0001c1d8
/*    1c1b4:	27af0138 */ 	addiu	$t7,$sp,0x138
/*    1c1b8:	460a7032 */ 	c.eq.s	$f14,$f10
/*    1c1bc:	c7b00140 */ 	lwc1	$f16,0x140($sp)
/*    1c1c0:	45020006 */ 	bc1fl	.PF0001c1dc
/*    1c1c4:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*    1c1c8:	46107032 */ 	c.eq.s	$f14,$f16
/*    1c1cc:	8fa80164 */ 	lw	$t0,0x164($sp)
/*    1c1d0:	45030027 */ 	bc1tl	.PF0001c270
/*    1c1d4:	8d0a0008 */ 	lw	$t2,0x8($t0)
.PF0001c1d8:
/*    1c1d8:	c6040088 */ 	lwc1	$f4,0x88($s0)
.PF0001c1dc:
/*    1c1dc:	c7a6013c */ 	lwc1	$f6,0x13c($sp)
/*    1c1e0:	8fa90164 */ 	lw	$t1,0x164($sp)
/*    1c1e4:	46049202 */ 	mul.s	$f8,$f18,$f4
/*    1c1e8:	c7b20140 */ 	lwc1	$f18,0x140($sp)
/*    1c1ec:	8fab0168 */ 	lw	$t3,0x168($sp)
/*    1c1f0:	8fae0158 */ 	lw	$t6,0x158($sp)
/*    1c1f4:	27b9012c */ 	addiu	$t9,$sp,0x12c
/*    1c1f8:	e7a80138 */ 	swc1	$f8,0x138($sp)
/*    1c1fc:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*    1c200:	460a3402 */ 	mul.s	$f16,$f6,$f10
/*    1c204:	e7b0013c */ 	swc1	$f16,0x13c($sp)
/*    1c208:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*    1c20c:	46049182 */ 	mul.s	$f6,$f18,$f4
/*    1c210:	e7a60140 */ 	swc1	$f6,0x140($sp)
/*    1c214:	8d2c0008 */ 	lw	$t4,0x8($t1)
/*    1c218:	8d8d0000 */ 	lw	$t5,0x0($t4)
/*    1c21c:	516d000b */ 	beql	$t3,$t5,.PF0001c24c
/*    1c220:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c224:	c5ca0000 */ 	lwc1	$f10,0x0($t6)
/*    1c228:	460a4480 */ 	add.s	$f18,$f8,$f10
/*    1c22c:	e7b20138 */ 	swc1	$f18,0x138($sp)
/*    1c230:	c5c40004 */ 	lwc1	$f4,0x4($t6)
/*    1c234:	46048200 */ 	add.s	$f8,$f16,$f4
/*    1c238:	e7a8013c */ 	swc1	$f8,0x13c($sp)
/*    1c23c:	c5ca0008 */ 	lwc1	$f10,0x8($t6)
/*    1c240:	460a3480 */ 	add.s	$f18,$f6,$f10
/*    1c244:	e7b20140 */ 	swc1	$f18,0x140($sp)
/*    1c248:	8fa40160 */ 	lw	$a0,0x160($sp)
.PF0001c24c:
/*    1c24c:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c250:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c254:	afaf0010 */ 	sw	$t7,0x10($sp)
/*    1c258:	afa00014 */ 	sw	$zero,0x14($sp)
/*    1c25c:	0c006d6d */ 	jal	model0001b80c
/*    1c260:	afb90018 */ 	sw	$t9,0x18($sp)
/*    1c264:	1000003f */ 	b	.PF0001c364
/*    1c268:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*    1c26c:	8d0a0008 */ 	lw	$t2,0x8($t0)
.PF0001c270:
/*    1c270:	8fb80168 */ 	lw	$t8,0x168($sp)
/*    1c274:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c278:	8d490000 */ 	lw	$t1,0x0($t2)
/*    1c27c:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c280:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c284:	1309000d */ 	beq	$t8,$t1,.PF0001c2bc
/*    1c288:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c28c:	8fac0158 */ 	lw	$t4,0x158($sp)
/*    1c290:	27ab012c */ 	addiu	$t3,$sp,0x12c
/*    1c294:	afab0018 */ 	sw	$t3,0x18($sp)
/*    1c298:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c29c:	01002825 */ 	move	$a1,$t0
/*    1c2a0:	03003025 */ 	move	$a2,$t8
/*    1c2a4:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c2a8:	afa00014 */ 	sw	$zero,0x14($sp)
/*    1c2ac:	0c006d6d */ 	jal	model0001b80c
/*    1c2b0:	afac0010 */ 	sw	$t4,0x10($sp)
/*    1c2b4:	1000002b */ 	b	.PF0001c364
/*    1c2b8:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PF0001c2bc:
/*    1c2bc:	27ad0138 */ 	addiu	$t5,$sp,0x138
/*    1c2c0:	27ae012c */ 	addiu	$t6,$sp,0x12c
/*    1c2c4:	afae0018 */ 	sw	$t6,0x18($sp)
/*    1c2c8:	afad0010 */ 	sw	$t5,0x10($sp)
/*    1c2cc:	0c006d6d */ 	jal	model0001b80c
/*    1c2d0:	afa00014 */ 	sw	$zero,0x14($sp)
/*    1c2d4:	10000023 */ 	b	.PF0001c364
/*    1c2d8:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*    1c2dc:	8faf0168 */ 	lw	$t7,0x168($sp)
.PF0001c2e0:
/*    1c2e0:	8fa40164 */ 	lw	$a0,0x164($sp)
/*    1c2e4:	8fb90160 */ 	lw	$t9,0x160($sp)
/*    1c2e8:	8de50008 */ 	lw	$a1,0x8($t7)
/*    1c2ec:	50a00006 */ 	beqzl	$a1,.PF0001c308
/*    1c2f0:	8f300000 */ 	lw	$s0,0x0($t9)
/*    1c2f4:	0c0068d7 */ 	jal	model0001a5cc
/*    1c2f8:	00003025 */ 	move	$a2,$zero
/*    1c2fc:	10000002 */ 	b	.PF0001c308
/*    1c300:	00408025 */ 	move	$s0,$v0
/*    1c304:	8f300000 */ 	lw	$s0,0x0($t9)
.PF0001c308:
/*    1c308:	1200000f */ 	beqz	$s0,.PF0001c348
/*    1c30c:	8fa40158 */ 	lw	$a0,0x158($sp)
/*    1c310:	8fa40158 */ 	lw	$a0,0x158($sp)
/*    1c314:	0c00591b */ 	jal	mtx4LoadTranslation
/*    1c318:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*    1c31c:	8fa80158 */ 	lw	$t0,0x158($sp)
/*    1c320:	8faa0164 */ 	lw	$t2,0x164($sp)
/*    1c324:	02002025 */ 	move	$a0,$s0
/*    1c328:	8518000e */ 	lh	$t8,0xe($t0)
/*    1c32c:	8d49000c */ 	lw	$t1,0xc($t2)
/*    1c330:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*    1c334:	00186180 */ 	sll	$t4,$t8,0x6
/*    1c338:	0c00565d */ 	jal	mtx00015be4
/*    1c33c:	012c3021 */ 	addu	$a2,$t1,$t4
/*    1c340:	10000008 */ 	b	.PF0001c364
/*    1c344:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PF0001c348:
/*    1c348:	8fad0164 */ 	lw	$t5,0x164($sp)
/*    1c34c:	848f000e */ 	lh	$t7,0xe($a0)
/*    1c350:	8dae000c */ 	lw	$t6,0xc($t5)
/*    1c354:	000fc980 */ 	sll	$t9,$t7,0x6
/*    1c358:	0c00591b */ 	jal	mtx4LoadTranslation
/*    1c35c:	01d92821 */ 	addu	$a1,$t6,$t9
/*    1c360:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PF0001c364:
/*    1c364:	8fb00028 */ 	lw	$s0,0x28($sp)
/*    1c368:	27bd0160 */ 	addiu	$sp,$sp,0x160
/*    1c36c:	03e00008 */ 	jr	$ra
/*    1c370:	00000000 */ 	nop
);
#elif VERSION >= VERSION_PAL_BETA
GLOBAL_ASM(
glabel model0001bfa8
.late_rodata
glabel var70053fc0pf
.word 0x3c23d70a
.text
/*  1ca10:	27bdfea0 */ 	addiu	$sp,$sp,-352
/*  1ca14:	afbf002c */ 	sw	$ra,0x2c($sp)
/*  1ca18:	afb00028 */ 	sw	$s0,0x28($sp)
/*  1ca1c:	afa40160 */ 	sw	$a0,0x160($sp)
/*  1ca20:	afa50164 */ 	sw	$a1,0x164($sp)
/*  1ca24:	afa60168 */ 	sw	$a2,0x168($sp)
/*  1ca28:	8ccf0004 */ 	lw	$t7,0x4($a2)
/*  1ca2c:	afaf0158 */ 	sw	$t7,0x158($sp)
/*  1ca30:	8cb00020 */ 	lw	$s0,0x20($a1)
/*  1ca34:	5200015a */ 	beqzl	$s0,.PB0001cfa0
/*  1ca38:	8faf0168 */ 	lw	$t7,0x168($sp)
/*  1ca3c:	95f9000c */ 	lhu	$t9,0xc($t7)
/*  1ca40:	3c098006 */ 	lui	$t1,0x8006
/*  1ca44:	27b80144 */ 	addiu	$t8,$sp,0x144
/*  1ca48:	afb90154 */ 	sw	$t9,0x154($sp)
/*  1ca4c:	8ca20008 */ 	lw	$v0,0x8($a1)
/*  1ca50:	3c013f80 */ 	lui	$at,0x3f80
/*  1ca54:	8c480004 */ 	lw	$t0,0x4($v0)
/*  1ca58:	afa80150 */ 	sw	$t0,0x150($sp)
/*  1ca5c:	86070000 */ 	lh	$a3,0x0($s0)
/*  1ca60:	27a80138 */ 	addiu	$t0,$sp,0x138
/*  1ca64:	10e00054 */ 	beqz	$a3,.PB0001cbb8
/*  1ca68:	00075080 */ 	sll	$t2,$a3,0x2
/*  1ca6c:	8d2906ac */ 	lw	$t1,0x6ac($t1)
/*  1ca70:	01475023 */ 	subu	$t2,$t2,$a3
/*  1ca74:	000a5080 */ 	sll	$t2,$t2,0x2
/*  1ca78:	012a5821 */ 	addu	$t3,$t1,$t2
/*  1ca7c:	9163000b */ 	lbu	$v1,0xb($t3)
/*  1ca80:	27a9012c */ 	addiu	$t1,$sp,0x12c
/*  1ca84:	306c0002 */ 	andi	$t4,$v1,0x2
/*  1ca88:	000c182b */ 	sltu	$v1,$zero,$t4
/*  1ca8c:	50600005 */ 	beqzl	$v1,.PB0001caa4
/*  1ca90:	afa30128 */ 	sw	$v1,0x128($sp)
/*  1ca94:	8c4f0000 */ 	lw	$t7,0x0($v0)
/*  1ca98:	00cf1826 */ 	xor	$v1,$a2,$t7
/*  1ca9c:	2c630001 */ 	sltiu	$v1,$v1,0x1
/*  1caa0:	afa30128 */ 	sw	$v1,0x128($sp)
.PB0001caa4:
/*  1caa4:	92190004 */ 	lbu	$t9,0x4($s0)
/*  1caa8:	82050008 */ 	lb	$a1,0x8($s0)
/*  1caac:	afa3003c */ 	sw	$v1,0x3c($sp)
/*  1cab0:	afa9001c */ 	sw	$t1,0x1c($sp)
/*  1cab4:	afa80018 */ 	sw	$t0,0x18($sp)
/*  1cab8:	afb80014 */ 	sw	$t8,0x14($sp)
/*  1cabc:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1cac0:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1cac4:	0c009184 */ 	jal	anim00024050
/*  1cac8:	afb90010 */ 	sw	$t9,0x10($sp)
/*  1cacc:	3c0a800a */ 	lui	$t2,0x800a
/*  1cad0:	8d4ae99c */ 	lw	$t2,-0x1664($t2)
/*  1cad4:	51400013 */ 	beqzl	$t2,.PB0001cb24
/*  1cad8:	44807000 */ 	mtc1	$zero,$f14
/*  1cadc:	44807000 */ 	mtc1	$zero,$f14
/*  1cae0:	c600001c */ 	lwc1	$f0,0x1c($s0)
/*  1cae4:	4600703c */ 	c.lt.s	$f14,$f0
/*  1cae8:	00000000 */ 	nop
/*  1caec:	4502000d */ 	bc1fl	.PB0001cb24
/*  1caf0:	44807000 */ 	mtc1	$zero,$f14
/*  1caf4:	c6040010 */ 	lwc1	$f4,0x10($s0)
/*  1caf8:	3c017005 */ 	lui	$at,0x7005
/*  1cafc:	c4284cbc */ 	lwc1	$f8,0x4cbc($at)
/*  1cb00:	46002183 */ 	div.s	$f6,$f4,$f0
/*  1cb04:	0fc25dc0 */ 	jal	floorf
/*  1cb08:	46083300 */ 	add.s	$f12,$f6,$f8
/*  1cb0c:	c60a001c */ 	lwc1	$f10,0x1c($s0)
/*  1cb10:	44807000 */ 	mtc1	$zero,$f14
/*  1cb14:	460a0082 */ 	mul.s	$f2,$f0,$f10
/*  1cb18:	10000004 */ 	b	.PB0001cb2c
/*  1cb1c:	460e1032 */ 	c.eq.s	$f2,$f14
/*  1cb20:	44807000 */ 	mtc1	$zero,$f14
.PB0001cb24:
/*  1cb24:	c6020010 */ 	lwc1	$f2,0x10($s0)
/*  1cb28:	460e1032 */ 	c.eq.s	$f2,$f14
.PB0001cb2c:
/*  1cb2c:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1cb30:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1cb34:	27ac00d4 */ 	addiu	$t4,$sp,0xd4
/*  1cb38:	4501001c */ 	bc1t	.PB0001cbac
/*  1cb3c:	27ad00c8 */ 	addiu	$t5,$sp,0xc8
/*  1cb40:	920b0005 */ 	lbu	$t3,0x5($s0)
/*  1cb44:	82050008 */ 	lb	$a1,0x8($s0)
/*  1cb48:	86070000 */ 	lh	$a3,0x0($s0)
/*  1cb4c:	27ae00bc */ 	addiu	$t6,$sp,0xbc
/*  1cb50:	afae001c */ 	sw	$t6,0x1c($sp)
/*  1cb54:	e7a200e0 */ 	swc1	$f2,0xe0($sp)
/*  1cb58:	afad0018 */ 	sw	$t5,0x18($sp)
/*  1cb5c:	afac0014 */ 	sw	$t4,0x14($sp)
/*  1cb60:	0c009184 */ 	jal	anim00024050
/*  1cb64:	afab0010 */ 	sw	$t3,0x10($sp)
/*  1cb68:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*  1cb6c:	27a40144 */ 	addiu	$a0,$sp,0x144
/*  1cb70:	27a500d4 */ 	addiu	$a1,$sp,0xd4
/*  1cb74:	44061000 */ 	mfc1	$a2,$f2
/*  1cb78:	0c006eb3 */ 	jal	model0001b07c
/*  1cb7c:	00000000 */ 	nop
/*  1cb80:	8faf003c */ 	lw	$t7,0x3c($sp)
/*  1cb84:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*  1cb88:	3c198006 */ 	lui	$t9,0x8006
/*  1cb8c:	15e00004 */ 	bnez	$t7,.PB0001cba0
/*  1cb90:	27a40138 */ 	addiu	$a0,$sp,0x138
/*  1cb94:	8f39065c */ 	lw	$t9,0x65c($t9)
/*  1cb98:	53200005 */ 	beqzl	$t9,.PB0001cbb0
/*  1cb9c:	44807000 */ 	mtc1	$zero,$f14
.PB0001cba0:
/*  1cba0:	44061000 */ 	mfc1	$a2,$f2
/*  1cba4:	0c006e7a */ 	jal	model0001af98
/*  1cba8:	27a500c8 */ 	addiu	$a1,$sp,0xc8
.PB0001cbac:
/*  1cbac:	44807000 */ 	mtc1	$zero,$f14
.PB0001cbb0:
/*  1cbb0:	10000010 */ 	b	.PB0001cbf4
/*  1cbb4:	c6120054 */ 	lwc1	$f18,0x54($s0)
.PB0001cbb8:
/*  1cbb8:	44800000 */ 	mtc1	$zero,$f0
/*  1cbbc:	44801000 */ 	mtc1	$zero,$f2
/*  1cbc0:	44807000 */ 	mtc1	$zero,$f14
/*  1cbc4:	e7a00140 */ 	swc1	$f0,0x140($sp)
/*  1cbc8:	e7a0013c */ 	swc1	$f0,0x13c($sp)
/*  1cbcc:	e7a00138 */ 	swc1	$f0,0x138($sp)
/*  1cbd0:	44810000 */ 	mtc1	$at,$f0
/*  1cbd4:	afa00128 */ 	sw	$zero,0x128($sp)
/*  1cbd8:	e7a20148 */ 	swc1	$f2,0x148($sp)
/*  1cbdc:	e7a20144 */ 	swc1	$f2,0x144($sp)
/*  1cbe0:	e7a2014c */ 	swc1	$f2,0x14c($sp)
/*  1cbe4:	e7a00130 */ 	swc1	$f0,0x130($sp)
/*  1cbe8:	e7a0012c */ 	swc1	$f0,0x12c($sp)
/*  1cbec:	e7a00134 */ 	swc1	$f0,0x134($sp)
/*  1cbf0:	c6120054 */ 	lwc1	$f18,0x54($s0)
.PB0001cbf4:
/*  1cbf4:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1cbf8:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1cbfc:	46127032 */ 	c.eq.s	$f14,$f18
/*  1cc00:	27a800b0 */ 	addiu	$t0,$sp,0xb0
/*  1cc04:	27a900a4 */ 	addiu	$t1,$sp,0xa4
/*  1cc08:	27aa0098 */ 	addiu	$t2,$sp,0x98
/*  1cc0c:	4501007a */ 	bc1t	.PB0001cdf8
/*  1cc10:	8fb90128 */ 	lw	$t9,0x128($sp)
/*  1cc14:	92180006 */ 	lbu	$t8,0x6($s0)
/*  1cc18:	82050009 */ 	lb	$a1,0x9($s0)
/*  1cc1c:	86070002 */ 	lh	$a3,0x2($s0)
/*  1cc20:	afaa001c */ 	sw	$t2,0x1c($sp)
/*  1cc24:	afa90018 */ 	sw	$t1,0x18($sp)
/*  1cc28:	afa80014 */ 	sw	$t0,0x14($sp)
/*  1cc2c:	0c009184 */ 	jal	anim00024050
/*  1cc30:	afb80010 */ 	sw	$t8,0x10($sp)
/*  1cc34:	44802000 */ 	mtc1	$zero,$f4
/*  1cc38:	c6060034 */ 	lwc1	$f6,0x34($s0)
/*  1cc3c:	8fa40154 */ 	lw	$a0,0x154($sp)
/*  1cc40:	8fa60150 */ 	lw	$a2,0x150($sp)
/*  1cc44:	46062032 */ 	c.eq.s	$f4,$f6
/*  1cc48:	27ac005c */ 	addiu	$t4,$sp,0x5c
/*  1cc4c:	27ad0050 */ 	addiu	$t5,$sp,0x50
/*  1cc50:	27ae0044 */ 	addiu	$t6,$sp,0x44
/*  1cc54:	4503000e */ 	bc1tl	.PB0001cc90
/*  1cc58:	27a40144 */ 	addiu	$a0,$sp,0x144
/*  1cc5c:	920b0007 */ 	lbu	$t3,0x7($s0)
/*  1cc60:	82050009 */ 	lb	$a1,0x9($s0)
/*  1cc64:	86070002 */ 	lh	$a3,0x2($s0)
/*  1cc68:	afae001c */ 	sw	$t6,0x1c($sp)
/*  1cc6c:	afad0018 */ 	sw	$t5,0x18($sp)
/*  1cc70:	afac0014 */ 	sw	$t4,0x14($sp)
/*  1cc74:	0c009184 */ 	jal	anim00024050
/*  1cc78:	afab0010 */ 	sw	$t3,0x10($sp)
/*  1cc7c:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*  1cc80:	27a5005c */ 	addiu	$a1,$sp,0x5c
/*  1cc84:	0c006eb3 */ 	jal	model0001b07c
/*  1cc88:	8e060034 */ 	lw	$a2,0x34($s0)
/*  1cc8c:	27a40144 */ 	addiu	$a0,$sp,0x144
.PB0001cc90:
/*  1cc90:	0fc25ac4 */ 	jal	quaternion0f096ca0
/*  1cc94:	27a50088 */ 	addiu	$a1,$sp,0x88
/*  1cc98:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*  1cc9c:	0fc25ac4 */ 	jal	quaternion0f096ca0
/*  1cca0:	27a50078 */ 	addiu	$a1,$sp,0x78
/*  1cca4:	27a40088 */ 	addiu	$a0,$sp,0x88
/*  1cca8:	0fc25d4c */ 	jal	quaternion0f0976c0
/*  1ccac:	27a50078 */ 	addiu	$a1,$sp,0x78
/*  1ccb0:	27a70068 */ 	addiu	$a3,$sp,0x68
/*  1ccb4:	27a40088 */ 	addiu	$a0,$sp,0x88
/*  1ccb8:	27a50078 */ 	addiu	$a1,$sp,0x78
/*  1ccbc:	0fc25c4a */ 	jal	quaternionSlerp
/*  1ccc0:	8e060054 */ 	lw	$a2,0x54($s0)
/*  1ccc4:	44804000 */ 	mtc1	$zero,$f8
/*  1ccc8:	c7aa0138 */ 	lwc1	$f10,0x138($sp)
/*  1cccc:	27a70068 */ 	addiu	$a3,$sp,0x68
/*  1ccd0:	c7b2013c */ 	lwc1	$f18,0x13c($sp)
/*  1ccd4:	460a4032 */ 	c.eq.s	$f8,$f10
/*  1ccd8:	c7a80138 */ 	lwc1	$f8,0x138($sp)
/*  1ccdc:	27b80138 */ 	addiu	$t8,$sp,0x138
/*  1cce0:	27a8012c */ 	addiu	$t0,$sp,0x12c
/*  1cce4:	4502000e */ 	bc1fl	.PB0001cd20
/*  1cce8:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1ccec:	44808000 */ 	mtc1	$zero,$f16
/*  1ccf0:	c7a60140 */ 	lwc1	$f6,0x140($sp)
/*  1ccf4:	46128032 */ 	c.eq.s	$f16,$f18
/*  1ccf8:	00000000 */ 	nop
/*  1ccfc:	45020008 */ 	bc1fl	.PB0001cd20
/*  1cd00:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1cd04:	44802000 */ 	mtc1	$zero,$f4
/*  1cd08:	8fa90164 */ 	lw	$t1,0x164($sp)
/*  1cd0c:	46062032 */ 	c.eq.s	$f4,$f6
/*  1cd10:	00000000 */ 	nop
/*  1cd14:	45030023 */ 	bc1tl	.PB0001cda4
/*  1cd18:	8d2a0008 */ 	lw	$t2,0x8($t1)
/*  1cd1c:	c60a0088 */ 	lwc1	$f10,0x88($s0)
.PB0001cd20:
/*  1cd20:	c7b2013c */ 	lwc1	$f18,0x13c($sp)
/*  1cd24:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1cd28:	460a4402 */ 	mul.s	$f16,$f8,$f10
/*  1cd2c:	c7a80140 */ 	lwc1	$f8,0x140($sp)
/*  1cd30:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1cd34:	8fa20158 */ 	lw	$v0,0x158($sp)
/*  1cd38:	e7b00138 */ 	swc1	$f16,0x138($sp)
/*  1cd3c:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*  1cd40:	46049182 */ 	mul.s	$f6,$f18,$f4
/*  1cd44:	e7a6013c */ 	swc1	$f6,0x13c($sp)
/*  1cd48:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1cd4c:	460a4482 */ 	mul.s	$f18,$f8,$f10
/*  1cd50:	e7b20140 */ 	swc1	$f18,0x140($sp)
/*  1cd54:	8caf0008 */ 	lw	$t7,0x8($a1)
/*  1cd58:	8df90000 */ 	lw	$t9,0x0($t7)
/*  1cd5c:	50d9000b */ 	beql	$a2,$t9,.PB0001cd8c
/*  1cd60:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1cd64:	c4440000 */ 	lwc1	$f4,0x0($v0)
/*  1cd68:	46048200 */ 	add.s	$f8,$f16,$f4
/*  1cd6c:	e7a80138 */ 	swc1	$f8,0x138($sp)
/*  1cd70:	c44a0004 */ 	lwc1	$f10,0x4($v0)
/*  1cd74:	460a3400 */ 	add.s	$f16,$f6,$f10
/*  1cd78:	e7b0013c */ 	swc1	$f16,0x13c($sp)
/*  1cd7c:	c4440008 */ 	lwc1	$f4,0x8($v0)
/*  1cd80:	46049200 */ 	add.s	$f8,$f18,$f4
/*  1cd84:	e7a80140 */ 	swc1	$f8,0x140($sp)
/*  1cd88:	8fa40160 */ 	lw	$a0,0x160($sp)
.PB0001cd8c:
/*  1cd8c:	afb80010 */ 	sw	$t8,0x10($sp)
/*  1cd90:	0c00719f */ 	jal	model0001bc14
/*  1cd94:	afa80014 */ 	sw	$t0,0x14($sp)
/*  1cd98:	100000a2 */ 	b	.PB0001d024
/*  1cd9c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  1cda0:	8d2a0008 */ 	lw	$t2,0x8($t1)
.PB0001cda4:
/*  1cda4:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1cda8:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1cdac:	8d4b0000 */ 	lw	$t3,0x0($t2)
/*  1cdb0:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1cdb4:	27ae0138 */ 	addiu	$t6,$sp,0x138
/*  1cdb8:	10cb000a */ 	beq	$a2,$t3,.PB0001cde4
/*  1cdbc:	27af012c */ 	addiu	$t7,$sp,0x12c
/*  1cdc0:	8fac0158 */ 	lw	$t4,0x158($sp)
/*  1cdc4:	27ad012c */ 	addiu	$t5,$sp,0x12c
/*  1cdc8:	afad0014 */ 	sw	$t5,0x14($sp)
/*  1cdcc:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1cdd0:	01202825 */ 	move	$a1,$t1
/*  1cdd4:	0c00719f */ 	jal	model0001bc14
/*  1cdd8:	afac0010 */ 	sw	$t4,0x10($sp)
/*  1cddc:	10000091 */ 	b	.PB0001d024
/*  1cde0:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PB0001cde4:
/*  1cde4:	afae0010 */ 	sw	$t6,0x10($sp)
/*  1cde8:	0c00719f */ 	jal	model0001bc14
/*  1cdec:	afaf0014 */ 	sw	$t7,0x14($sp)
/*  1cdf0:	1000008c */ 	b	.PB0001d024
/*  1cdf4:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PB0001cdf8:
/*  1cdf8:	13200019 */ 	beqz	$t9,.PB0001ce60
/*  1cdfc:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*  1ce00:	0fc577be */ 	jal	func0f15c888
/*  1ce04:	00000000 */ 	nop
/*  1ce08:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*  1ce0c:	c7b0013c */ 	lwc1	$f16,0x13c($sp)
/*  1ce10:	c7a40140 */ 	lwc1	$f4,0x140($sp)
/*  1ce14:	46003282 */ 	mul.s	$f10,$f6,$f0
/*  1ce18:	27b80138 */ 	addiu	$t8,$sp,0x138
/*  1ce1c:	24080001 */ 	li	$t0,0x1
/*  1ce20:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  1ce24:	27aa012c */ 	addiu	$t2,$sp,0x12c
/*  1ce28:	afaa0018 */ 	sw	$t2,0x18($sp)
/*  1ce2c:	46002202 */ 	mul.s	$f8,$f4,$f0
/*  1ce30:	e7aa0138 */ 	swc1	$f10,0x138($sp)
/*  1ce34:	afa80014 */ 	sw	$t0,0x14($sp)
/*  1ce38:	afb80010 */ 	sw	$t8,0x10($sp)
/*  1ce3c:	e7b2013c */ 	swc1	$f18,0x13c($sp)
/*  1ce40:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1ce44:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1ce48:	e7a80140 */ 	swc1	$f8,0x140($sp)
/*  1ce4c:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1ce50:	0c00709d */ 	jal	model0001b80c
/*  1ce54:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1ce58:	10000072 */ 	b	.PB0001d024
/*  1ce5c:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PB0001ce60:
/*  1ce60:	46067032 */ 	c.eq.s	$f14,$f6
/*  1ce64:	c7aa013c */ 	lwc1	$f10,0x13c($sp)
/*  1ce68:	c7b20138 */ 	lwc1	$f18,0x138($sp)
/*  1ce6c:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1ce70:	45000009 */ 	bc1f	.PB0001ce98
/*  1ce74:	27af0138 */ 	addiu	$t7,$sp,0x138
/*  1ce78:	460a7032 */ 	c.eq.s	$f14,$f10
/*  1ce7c:	c7b00140 */ 	lwc1	$f16,0x140($sp)
/*  1ce80:	45020006 */ 	bc1fl	.PB0001ce9c
/*  1ce84:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*  1ce88:	46107032 */ 	c.eq.s	$f14,$f16
/*  1ce8c:	8fa80164 */ 	lw	$t0,0x164($sp)
/*  1ce90:	45030027 */ 	bc1tl	.PB0001cf30
/*  1ce94:	8d0a0008 */ 	lw	$t2,0x8($t0)
.PB0001ce98:
/*  1ce98:	c6040088 */ 	lwc1	$f4,0x88($s0)
.PB0001ce9c:
/*  1ce9c:	c7a6013c */ 	lwc1	$f6,0x13c($sp)
/*  1cea0:	8fa90164 */ 	lw	$t1,0x164($sp)
/*  1cea4:	46049202 */ 	mul.s	$f8,$f18,$f4
/*  1cea8:	c7b20140 */ 	lwc1	$f18,0x140($sp)
/*  1ceac:	8fab0168 */ 	lw	$t3,0x168($sp)
/*  1ceb0:	8fae0158 */ 	lw	$t6,0x158($sp)
/*  1ceb4:	27b9012c */ 	addiu	$t9,$sp,0x12c
/*  1ceb8:	e7a80138 */ 	swc1	$f8,0x138($sp)
/*  1cebc:	c60a0088 */ 	lwc1	$f10,0x88($s0)
/*  1cec0:	460a3402 */ 	mul.s	$f16,$f6,$f10
/*  1cec4:	e7b0013c */ 	swc1	$f16,0x13c($sp)
/*  1cec8:	c6040088 */ 	lwc1	$f4,0x88($s0)
/*  1cecc:	46049182 */ 	mul.s	$f6,$f18,$f4
/*  1ced0:	e7a60140 */ 	swc1	$f6,0x140($sp)
/*  1ced4:	8d2c0008 */ 	lw	$t4,0x8($t1)
/*  1ced8:	8d8d0000 */ 	lw	$t5,0x0($t4)
/*  1cedc:	516d000b */ 	beql	$t3,$t5,.PB0001cf0c
/*  1cee0:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1cee4:	c5ca0000 */ 	lwc1	$f10,0x0($t6)
/*  1cee8:	460a4480 */ 	add.s	$f18,$f8,$f10
/*  1ceec:	e7b20138 */ 	swc1	$f18,0x138($sp)
/*  1cef0:	c5c40004 */ 	lwc1	$f4,0x4($t6)
/*  1cef4:	46048200 */ 	add.s	$f8,$f16,$f4
/*  1cef8:	e7a8013c */ 	swc1	$f8,0x13c($sp)
/*  1cefc:	c5ca0008 */ 	lwc1	$f10,0x8($t6)
/*  1cf00:	460a3480 */ 	add.s	$f18,$f6,$f10
/*  1cf04:	e7b20140 */ 	swc1	$f18,0x140($sp)
/*  1cf08:	8fa40160 */ 	lw	$a0,0x160($sp)
.PB0001cf0c:
/*  1cf0c:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1cf10:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1cf14:	afaf0010 */ 	sw	$t7,0x10($sp)
/*  1cf18:	afa00014 */ 	sw	$zero,0x14($sp)
/*  1cf1c:	0c00709d */ 	jal	model0001b80c
/*  1cf20:	afb90018 */ 	sw	$t9,0x18($sp)
/*  1cf24:	1000003f */ 	b	.PB0001d024
/*  1cf28:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  1cf2c:	8d0a0008 */ 	lw	$t2,0x8($t0)
.PB0001cf30:
/*  1cf30:	8fb80168 */ 	lw	$t8,0x168($sp)
/*  1cf34:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1cf38:	8d490000 */ 	lw	$t1,0x0($t2)
/*  1cf3c:	8fa50164 */ 	lw	$a1,0x164($sp)
/*  1cf40:	8fa60168 */ 	lw	$a2,0x168($sp)
/*  1cf44:	1309000d */ 	beq	$t8,$t1,.PB0001cf7c
/*  1cf48:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1cf4c:	8fac0158 */ 	lw	$t4,0x158($sp)
/*  1cf50:	27ab012c */ 	addiu	$t3,$sp,0x12c
/*  1cf54:	afab0018 */ 	sw	$t3,0x18($sp)
/*  1cf58:	8fa40160 */ 	lw	$a0,0x160($sp)
/*  1cf5c:	01002825 */ 	move	$a1,$t0
/*  1cf60:	03003025 */ 	move	$a2,$t8
/*  1cf64:	27a70144 */ 	addiu	$a3,$sp,0x144
/*  1cf68:	afa00014 */ 	sw	$zero,0x14($sp)
/*  1cf6c:	0c00709d */ 	jal	model0001b80c
/*  1cf70:	afac0010 */ 	sw	$t4,0x10($sp)
/*  1cf74:	1000002b */ 	b	.PB0001d024
/*  1cf78:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PB0001cf7c:
/*  1cf7c:	27ad0138 */ 	addiu	$t5,$sp,0x138
/*  1cf80:	27ae012c */ 	addiu	$t6,$sp,0x12c
/*  1cf84:	afae0018 */ 	sw	$t6,0x18($sp)
/*  1cf88:	afad0010 */ 	sw	$t5,0x10($sp)
/*  1cf8c:	0c00709d */ 	jal	model0001b80c
/*  1cf90:	afa00014 */ 	sw	$zero,0x14($sp)
/*  1cf94:	10000023 */ 	b	.PB0001d024
/*  1cf98:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  1cf9c:	8faf0168 */ 	lw	$t7,0x168($sp)
.PB0001cfa0:
/*  1cfa0:	8fa40164 */ 	lw	$a0,0x164($sp)
/*  1cfa4:	8fb90160 */ 	lw	$t9,0x160($sp)
/*  1cfa8:	8de50008 */ 	lw	$a1,0x8($t7)
/*  1cfac:	50a00006 */ 	beqzl	$a1,.PB0001cfc8
/*  1cfb0:	8f300000 */ 	lw	$s0,0x0($t9)
/*  1cfb4:	0c006c07 */ 	jal	model0001a5cc
/*  1cfb8:	00003025 */ 	move	$a2,$zero
/*  1cfbc:	10000002 */ 	b	.PB0001cfc8
/*  1cfc0:	00408025 */ 	move	$s0,$v0
/*  1cfc4:	8f300000 */ 	lw	$s0,0x0($t9)
.PB0001cfc8:
/*  1cfc8:	1200000f */ 	beqz	$s0,.PB0001d008
/*  1cfcc:	8fa40158 */ 	lw	$a0,0x158($sp)
/*  1cfd0:	8fa40158 */ 	lw	$a0,0x158($sp)
/*  1cfd4:	0c005c4b */ 	jal	mtx4LoadTranslation
/*  1cfd8:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*  1cfdc:	8fa80158 */ 	lw	$t0,0x158($sp)
/*  1cfe0:	8faa0164 */ 	lw	$t2,0x164($sp)
/*  1cfe4:	02002025 */ 	move	$a0,$s0
/*  1cfe8:	8518000e */ 	lh	$t8,0xe($t0)
/*  1cfec:	8d49000c */ 	lw	$t1,0xc($t2)
/*  1cff0:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*  1cff4:	00186180 */ 	sll	$t4,$t8,0x6
/*  1cff8:	0c00598d */ 	jal	mtx00015be4
/*  1cffc:	012c3021 */ 	addu	$a2,$t1,$t4
/*  1d000:	10000008 */ 	b	.PB0001d024
/*  1d004:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PB0001d008:
/*  1d008:	8fad0164 */ 	lw	$t5,0x164($sp)
/*  1d00c:	848f000e */ 	lh	$t7,0xe($a0)
/*  1d010:	8dae000c */ 	lw	$t6,0xc($t5)
/*  1d014:	000fc980 */ 	sll	$t9,$t7,0x6
/*  1d018:	0c005c4b */ 	jal	mtx4LoadTranslation
/*  1d01c:	01d92821 */ 	addu	$a1,$t6,$t9
/*  1d020:	8fbf002c */ 	lw	$ra,0x2c($sp)
.PB0001d024:
/*  1d024:	8fb00028 */ 	lw	$s0,0x28($sp)
/*  1d028:	27bd0160 */ 	addiu	$sp,$sp,0x160
/*  1d02c:	03e00008 */ 	jr	$ra
/*  1d030:	00000000 */ 	nop
);
#else
GLOBAL_ASM(
glabel model0001bfa8
/*    1bfa8:	27bdfea0 */ 	addiu	$sp,$sp,-352
/*    1bfac:	afbf002c */ 	sw	$ra,0x2c($sp)
/*    1bfb0:	afb00028 */ 	sw	$s0,0x28($sp)
/*    1bfb4:	afa40160 */ 	sw	$a0,0x160($sp)
/*    1bfb8:	afa50164 */ 	sw	$a1,0x164($sp)
/*    1bfbc:	afa60168 */ 	sw	$a2,0x168($sp)
/*    1bfc0:	8ccf0004 */ 	lw	$t7,0x4($a2)
/*    1bfc4:	8fae0168 */ 	lw	$t6,0x168($sp)
/*    1bfc8:	afaf0158 */ 	sw	$t7,0x158($sp)
/*    1bfcc:	8cb00020 */ 	lw	$s0,0x20($a1)
/*    1bfd0:	52000154 */ 	beqzl	$s0,.L0001c524
/*    1bfd4:	8dc50008 */ 	lw	$a1,0x8($t6)
/*    1bfd8:	95f9000c */ 	lhu	$t9,0xc($t7)
/*    1bfdc:	3c098006 */ 	lui	$t1,%hi(g_Anims)
/*    1bfe0:	27b80144 */ 	addiu	$t8,$sp,0x144
/*    1bfe4:	afb90154 */ 	sw	$t9,0x154($sp)
/*    1bfe8:	8ca20008 */ 	lw	$v0,0x8($a1)
/*    1bfec:	3c013f80 */ 	lui	$at,0x3f80
/*    1bff0:	8c480004 */ 	lw	$t0,0x4($v0)
/*    1bff4:	afa80150 */ 	sw	$t0,0x150($sp)
/*    1bff8:	86070000 */ 	lh	$a3,0x0($s0)
/*    1bffc:	27a80138 */ 	addiu	$t0,$sp,0x138
/*    1c000:	10e0004e */ 	beqz	$a3,.L0001c13c
/*    1c004:	00075080 */ 	sll	$t2,$a3,0x2
/*    1c008:	8d29f00c */ 	lw	$t1,%lo(g_Anims)($t1)
/*    1c00c:	01475023 */ 	subu	$t2,$t2,$a3
/*    1c010:	000a5080 */ 	sll	$t2,$t2,0x2
/*    1c014:	012a5821 */ 	addu	$t3,$t1,$t2
/*    1c018:	9163000b */ 	lbu	$v1,0xb($t3)
/*    1c01c:	27a9012c */ 	addiu	$t1,$sp,0x12c
/*    1c020:	306c0002 */ 	andi	$t4,$v1,0x2
/*    1c024:	000c182b */ 	sltu	$v1,$zero,$t4
/*    1c028:	50600005 */ 	beqzl	$v1,.L0001c040
/*    1c02c:	afa30128 */ 	sw	$v1,0x128($sp)
/*    1c030:	8c4f0000 */ 	lw	$t7,0x0($v0)
/*    1c034:	00cf1826 */ 	xor	$v1,$a2,$t7
/*    1c038:	2c630001 */ 	sltiu	$v1,$v1,0x1
/*    1c03c:	afa30128 */ 	sw	$v1,0x128($sp)
.L0001c040:
/*    1c040:	92190004 */ 	lbu	$t9,0x4($s0)
/*    1c044:	82050008 */ 	lb	$a1,0x8($s0)
/*    1c048:	afa3003c */ 	sw	$v1,0x3c($sp)
/*    1c04c:	afa9001c */ 	sw	$t1,0x1c($sp)
/*    1c050:	afa80018 */ 	sw	$t0,0x18($sp)
/*    1c054:	afb80014 */ 	sw	$t8,0x14($sp)
/*    1c058:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1c05c:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1c060:	0c009014 */ 	jal	anim00024050
/*    1c064:	afb90010 */ 	sw	$t9,0x10($sp)
/*    1c068:	3c0a800a */ 	lui	$t2,%hi(g_Vars+0x4cc)
/*    1c06c:	8d4aa48c */ 	lw	$t2,%lo(g_Vars+0x4cc)($t2)
/*    1c070:	51400010 */ 	beqzl	$t2,.L0001c0b4
/*    1c074:	44807000 */ 	mtc1	$zero,$f14
/*    1c078:	44807000 */ 	mtc1	$zero,$f14
/*    1c07c:	c600001c */ 	lwc1	$f0,0x1c($s0)
/*    1c080:	4600703c */ 	c.lt.s	$f14,$f0
/*    1c084:	00000000 */ 	nop
/*    1c088:	4502000a */ 	bc1fl	.L0001c0b4
/*    1c08c:	44807000 */ 	mtc1	$zero,$f14
/*    1c090:	c6040010 */ 	lwc1	$f4,0x10($s0)
/*    1c094:	0fc25e24 */ 	jal	floorf
/*    1c098:	46002303 */ 	div.s	$f12,$f4,$f0
/*    1c09c:	c606001c */ 	lwc1	$f6,0x1c($s0)
/*    1c0a0:	44807000 */ 	mtc1	$zero,$f14
/*    1c0a4:	46060082 */ 	mul.s	$f2,$f0,$f6
/*    1c0a8:	10000004 */ 	b	.L0001c0bc
/*    1c0ac:	460e1032 */ 	c.eq.s	$f2,$f14
/*    1c0b0:	44807000 */ 	mtc1	$zero,$f14
.L0001c0b4:
/*    1c0b4:	c6020010 */ 	lwc1	$f2,0x10($s0)
/*    1c0b8:	460e1032 */ 	c.eq.s	$f2,$f14
.L0001c0bc:
/*    1c0bc:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1c0c0:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1c0c4:	27ac00d4 */ 	addiu	$t4,$sp,0xd4
/*    1c0c8:	45010019 */ 	bc1t	.L0001c130
/*    1c0cc:	27ad00c8 */ 	addiu	$t5,$sp,0xc8
/*    1c0d0:	920b0005 */ 	lbu	$t3,0x5($s0)
/*    1c0d4:	82050008 */ 	lb	$a1,0x8($s0)
/*    1c0d8:	86070000 */ 	lh	$a3,0x0($s0)
/*    1c0dc:	27ae00bc */ 	addiu	$t6,$sp,0xbc
/*    1c0e0:	afae001c */ 	sw	$t6,0x1c($sp)
/*    1c0e4:	e7a200e0 */ 	swc1	$f2,0xe0($sp)
/*    1c0e8:	afad0018 */ 	sw	$t5,0x18($sp)
/*    1c0ec:	afac0014 */ 	sw	$t4,0x14($sp)
/*    1c0f0:	0c009014 */ 	jal	anim00024050
/*    1c0f4:	afab0010 */ 	sw	$t3,0x10($sp)
/*    1c0f8:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*    1c0fc:	27a40144 */ 	addiu	$a0,$sp,0x144
/*    1c100:	27a500d4 */ 	addiu	$a1,$sp,0xd4
/*    1c104:	44061000 */ 	mfc1	$a2,$f2
/*    1c108:	0c006c1f */ 	jal	model0001b07c
/*    1c10c:	00000000 */ 	nop
/*    1c110:	8faf003c */ 	lw	$t7,0x3c($sp)
/*    1c114:	c7a200e0 */ 	lwc1	$f2,0xe0($sp)
/*    1c118:	27a40138 */ 	addiu	$a0,$sp,0x138
/*    1c11c:	51e00005 */ 	beqzl	$t7,.L0001c134
/*    1c120:	44807000 */ 	mtc1	$zero,$f14
/*    1c124:	44061000 */ 	mfc1	$a2,$f2
/*    1c128:	0c006be6 */ 	jal	model0001af98
/*    1c12c:	27a500c8 */ 	addiu	$a1,$sp,0xc8
.L0001c130:
/*    1c130:	44807000 */ 	mtc1	$zero,$f14
.L0001c134:
/*    1c134:	10000010 */ 	b	.L0001c178
/*    1c138:	c60a0054 */ 	lwc1	$f10,0x54($s0)
.L0001c13c:
/*    1c13c:	44800000 */ 	mtc1	$zero,$f0
/*    1c140:	44801000 */ 	mtc1	$zero,$f2
/*    1c144:	44807000 */ 	mtc1	$zero,$f14
/*    1c148:	e7a00140 */ 	swc1	$f0,0x140($sp)
/*    1c14c:	e7a0013c */ 	swc1	$f0,0x13c($sp)
/*    1c150:	e7a00138 */ 	swc1	$f0,0x138($sp)
/*    1c154:	44810000 */ 	mtc1	$at,$f0
/*    1c158:	afa00128 */ 	sw	$zero,0x128($sp)
/*    1c15c:	e7a20148 */ 	swc1	$f2,0x148($sp)
/*    1c160:	e7a20144 */ 	swc1	$f2,0x144($sp)
/*    1c164:	e7a2014c */ 	swc1	$f2,0x14c($sp)
/*    1c168:	e7a00130 */ 	swc1	$f0,0x130($sp)
/*    1c16c:	e7a0012c */ 	swc1	$f0,0x12c($sp)
/*    1c170:	e7a00134 */ 	swc1	$f0,0x134($sp)
/*    1c174:	c60a0054 */ 	lwc1	$f10,0x54($s0)
.L0001c178:
/*    1c178:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1c17c:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1c180:	460a7032 */ 	c.eq.s	$f14,$f10
/*    1c184:	27b800b0 */ 	addiu	$t8,$sp,0xb0
/*    1c188:	27a800a4 */ 	addiu	$t0,$sp,0xa4
/*    1c18c:	27a90098 */ 	addiu	$t1,$sp,0x98
/*    1c190:	4501007a */ 	bc1t	.L0001c37c
/*    1c194:	8faf0128 */ 	lw	$t7,0x128($sp)
/*    1c198:	92190006 */ 	lbu	$t9,0x6($s0)
/*    1c19c:	82050009 */ 	lb	$a1,0x9($s0)
/*    1c1a0:	86070002 */ 	lh	$a3,0x2($s0)
/*    1c1a4:	afa9001c */ 	sw	$t1,0x1c($sp)
/*    1c1a8:	afa80018 */ 	sw	$t0,0x18($sp)
/*    1c1ac:	afb80014 */ 	sw	$t8,0x14($sp)
/*    1c1b0:	0c009014 */ 	jal	anim00024050
/*    1c1b4:	afb90010 */ 	sw	$t9,0x10($sp)
/*    1c1b8:	44808000 */ 	mtc1	$zero,$f16
/*    1c1bc:	c6120034 */ 	lwc1	$f18,0x34($s0)
/*    1c1c0:	8fa40154 */ 	lw	$a0,0x154($sp)
/*    1c1c4:	8fa60150 */ 	lw	$a2,0x150($sp)
/*    1c1c8:	46128032 */ 	c.eq.s	$f16,$f18
/*    1c1cc:	27ab005c */ 	addiu	$t3,$sp,0x5c
/*    1c1d0:	27ac0050 */ 	addiu	$t4,$sp,0x50
/*    1c1d4:	27ad0044 */ 	addiu	$t5,$sp,0x44
/*    1c1d8:	4503000e */ 	bc1tl	.L0001c214
/*    1c1dc:	27a40144 */ 	addiu	$a0,$sp,0x144
/*    1c1e0:	920a0007 */ 	lbu	$t2,0x7($s0)
/*    1c1e4:	82050009 */ 	lb	$a1,0x9($s0)
/*    1c1e8:	86070002 */ 	lh	$a3,0x2($s0)
/*    1c1ec:	afad001c */ 	sw	$t5,0x1c($sp)
/*    1c1f0:	afac0018 */ 	sw	$t4,0x18($sp)
/*    1c1f4:	afab0014 */ 	sw	$t3,0x14($sp)
/*    1c1f8:	0c009014 */ 	jal	anim00024050
/*    1c1fc:	afaa0010 */ 	sw	$t2,0x10($sp)
/*    1c200:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*    1c204:	27a5005c */ 	addiu	$a1,$sp,0x5c
/*    1c208:	0c006c1f */ 	jal	model0001b07c
/*    1c20c:	8e060034 */ 	lw	$a2,0x34($s0)
/*    1c210:	27a40144 */ 	addiu	$a0,$sp,0x144
.L0001c214:
/*    1c214:	0fc25b28 */ 	jal	quaternion0f096ca0
/*    1c218:	27a50088 */ 	addiu	$a1,$sp,0x88
/*    1c21c:	27a400b0 */ 	addiu	$a0,$sp,0xb0
/*    1c220:	0fc25b28 */ 	jal	quaternion0f096ca0
/*    1c224:	27a50078 */ 	addiu	$a1,$sp,0x78
/*    1c228:	27a40088 */ 	addiu	$a0,$sp,0x88
/*    1c22c:	0fc25db0 */ 	jal	quaternion0f0976c0
/*    1c230:	27a50078 */ 	addiu	$a1,$sp,0x78
/*    1c234:	27a70068 */ 	addiu	$a3,$sp,0x68
/*    1c238:	27a40088 */ 	addiu	$a0,$sp,0x88
/*    1c23c:	27a50078 */ 	addiu	$a1,$sp,0x78
/*    1c240:	0fc25cae */ 	jal	quaternionSlerp
/*    1c244:	8e060054 */ 	lw	$a2,0x54($s0)
/*    1c248:	44802000 */ 	mtc1	$zero,$f4
/*    1c24c:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*    1c250:	27a70068 */ 	addiu	$a3,$sp,0x68
/*    1c254:	c7aa013c */ 	lwc1	$f10,0x13c($sp)
/*    1c258:	46062032 */ 	c.eq.s	$f4,$f6
/*    1c25c:	c7a40138 */ 	lwc1	$f4,0x138($sp)
/*    1c260:	27b90138 */ 	addiu	$t9,$sp,0x138
/*    1c264:	27b8012c */ 	addiu	$t8,$sp,0x12c
/*    1c268:	4502000e */ 	bc1fl	.L0001c2a4
/*    1c26c:	c6060088 */ 	lwc1	$f6,0x88($s0)
/*    1c270:	44804000 */ 	mtc1	$zero,$f8
/*    1c274:	c7b20140 */ 	lwc1	$f18,0x140($sp)
/*    1c278:	460a4032 */ 	c.eq.s	$f8,$f10
/*    1c27c:	00000000 */ 	nop
/*    1c280:	45020008 */ 	bc1fl	.L0001c2a4
/*    1c284:	c6060088 */ 	lwc1	$f6,0x88($s0)
/*    1c288:	44808000 */ 	mtc1	$zero,$f16
/*    1c28c:	8fa80164 */ 	lw	$t0,0x164($sp)
/*    1c290:	46128032 */ 	c.eq.s	$f16,$f18
/*    1c294:	00000000 */ 	nop
/*    1c298:	45030023 */ 	bc1tl	.L0001c328
/*    1c29c:	8d090008 */ 	lw	$t1,0x8($t0)
/*    1c2a0:	c6060088 */ 	lwc1	$f6,0x88($s0)
.L0001c2a4:
/*    1c2a4:	c7aa013c */ 	lwc1	$f10,0x13c($sp)
/*    1c2a8:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c2ac:	46062202 */ 	mul.s	$f8,$f4,$f6
/*    1c2b0:	c7a40140 */ 	lwc1	$f4,0x140($sp)
/*    1c2b4:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c2b8:	8fa20158 */ 	lw	$v0,0x158($sp)
/*    1c2bc:	e7a80138 */ 	swc1	$f8,0x138($sp)
/*    1c2c0:	c6100088 */ 	lwc1	$f16,0x88($s0)
/*    1c2c4:	46105482 */ 	mul.s	$f18,$f10,$f16
/*    1c2c8:	e7b2013c */ 	swc1	$f18,0x13c($sp)
/*    1c2cc:	c6060088 */ 	lwc1	$f6,0x88($s0)
/*    1c2d0:	46062282 */ 	mul.s	$f10,$f4,$f6
/*    1c2d4:	e7aa0140 */ 	swc1	$f10,0x140($sp)
/*    1c2d8:	8cae0008 */ 	lw	$t6,0x8($a1)
/*    1c2dc:	8dcf0000 */ 	lw	$t7,0x0($t6)
/*    1c2e0:	50cf000b */ 	beql	$a2,$t7,.L0001c310
/*    1c2e4:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c2e8:	c4500000 */ 	lwc1	$f16,0x0($v0)
/*    1c2ec:	46104100 */ 	add.s	$f4,$f8,$f16
/*    1c2f0:	e7a40138 */ 	swc1	$f4,0x138($sp)
/*    1c2f4:	c4460004 */ 	lwc1	$f6,0x4($v0)
/*    1c2f8:	46069200 */ 	add.s	$f8,$f18,$f6
/*    1c2fc:	e7a8013c */ 	swc1	$f8,0x13c($sp)
/*    1c300:	c4500008 */ 	lwc1	$f16,0x8($v0)
/*    1c304:	46105100 */ 	add.s	$f4,$f10,$f16
/*    1c308:	e7a40140 */ 	swc1	$f4,0x140($sp)
/*    1c30c:	8fa40160 */ 	lw	$a0,0x160($sp)
.L0001c310:
/*    1c310:	afb90010 */ 	sw	$t9,0x10($sp)
/*    1c314:	0c006f05 */ 	jal	model0001bc14
/*    1c318:	afb80014 */ 	sw	$t8,0x14($sp)
/*    1c31c:	100000a1 */ 	b	.L0001c5a4
/*    1c320:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*    1c324:	8d090008 */ 	lw	$t1,0x8($t0)
.L0001c328:
/*    1c328:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c32c:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c330:	8d2a0000 */ 	lw	$t2,0x0($t1)
/*    1c334:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c338:	27ad0138 */ 	addiu	$t5,$sp,0x138
/*    1c33c:	10ca000a */ 	beq	$a2,$t2,.L0001c368
/*    1c340:	27ae012c */ 	addiu	$t6,$sp,0x12c
/*    1c344:	8fab0158 */ 	lw	$t3,0x158($sp)
/*    1c348:	27ac012c */ 	addiu	$t4,$sp,0x12c
/*    1c34c:	afac0014 */ 	sw	$t4,0x14($sp)
/*    1c350:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c354:	01002825 */ 	or	$a1,$t0,$zero
/*    1c358:	0c006f05 */ 	jal	model0001bc14
/*    1c35c:	afab0010 */ 	sw	$t3,0x10($sp)
/*    1c360:	10000090 */ 	b	.L0001c5a4
/*    1c364:	8fbf002c */ 	lw	$ra,0x2c($sp)
.L0001c368:
/*    1c368:	afad0010 */ 	sw	$t5,0x10($sp)
/*    1c36c:	0c006f05 */ 	jal	model0001bc14
/*    1c370:	afae0014 */ 	sw	$t6,0x14($sp)
/*    1c374:	1000008b */ 	b	.L0001c5a4
/*    1c378:	8fbf002c */ 	lw	$ra,0x2c($sp)
.L0001c37c:
/*    1c37c:	11e00019 */ 	beqz	$t7,.L0001c3e4
/*    1c380:	c7b20138 */ 	lwc1	$f18,0x138($sp)
/*    1c384:	0fc57222 */ 	jal	func0f15c888
/*    1c388:	00000000 */ 	nop
/*    1c38c:	c7b20138 */ 	lwc1	$f18,0x138($sp)
/*    1c390:	c7a8013c */ 	lwc1	$f8,0x13c($sp)
/*    1c394:	c7b00140 */ 	lwc1	$f16,0x140($sp)
/*    1c398:	46009182 */ 	mul.s	$f6,$f18,$f0
/*    1c39c:	27b90138 */ 	addiu	$t9,$sp,0x138
/*    1c3a0:	24180001 */ 	addiu	$t8,$zero,0x1
/*    1c3a4:	46004282 */ 	mul.s	$f10,$f8,$f0
/*    1c3a8:	27a9012c */ 	addiu	$t1,$sp,0x12c
/*    1c3ac:	afa90018 */ 	sw	$t1,0x18($sp)
/*    1c3b0:	46008102 */ 	mul.s	$f4,$f16,$f0
/*    1c3b4:	e7a60138 */ 	swc1	$f6,0x138($sp)
/*    1c3b8:	afb80014 */ 	sw	$t8,0x14($sp)
/*    1c3bc:	afb90010 */ 	sw	$t9,0x10($sp)
/*    1c3c0:	e7aa013c */ 	swc1	$f10,0x13c($sp)
/*    1c3c4:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c3c8:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c3cc:	e7a40140 */ 	swc1	$f4,0x140($sp)
/*    1c3d0:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c3d4:	0c006e03 */ 	jal	model0001b80c
/*    1c3d8:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c3dc:	10000071 */ 	b	.L0001c5a4
/*    1c3e0:	8fbf002c */ 	lw	$ra,0x2c($sp)
.L0001c3e4:
/*    1c3e4:	46127032 */ 	c.eq.s	$f14,$f18
/*    1c3e8:	c7a6013c */ 	lwc1	$f6,0x13c($sp)
/*    1c3ec:	c7aa0138 */ 	lwc1	$f10,0x138($sp)
/*    1c3f0:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c3f4:	45000009 */ 	bc1f	.L0001c41c
/*    1c3f8:	27ae0138 */ 	addiu	$t6,$sp,0x138
/*    1c3fc:	46067032 */ 	c.eq.s	$f14,$f6
/*    1c400:	c7a80140 */ 	lwc1	$f8,0x140($sp)
/*    1c404:	45020006 */ 	bc1fl	.L0001c420
/*    1c408:	c6100088 */ 	lwc1	$f16,0x88($s0)
/*    1c40c:	46087032 */ 	c.eq.s	$f14,$f8
/*    1c410:	8fb80164 */ 	lw	$t8,0x164($sp)
/*    1c414:	45030027 */ 	bc1tl	.L0001c4b4
/*    1c418:	8f090008 */ 	lw	$t1,0x8($t8)
.L0001c41c:
/*    1c41c:	c6100088 */ 	lwc1	$f16,0x88($s0)
.L0001c420:
/*    1c420:	c7b2013c */ 	lwc1	$f18,0x13c($sp)
/*    1c424:	8fa80164 */ 	lw	$t0,0x164($sp)
/*    1c428:	46105102 */ 	mul.s	$f4,$f10,$f16
/*    1c42c:	c7aa0140 */ 	lwc1	$f10,0x140($sp)
/*    1c430:	8faa0168 */ 	lw	$t2,0x168($sp)
/*    1c434:	8fad0158 */ 	lw	$t5,0x158($sp)
/*    1c438:	27af012c */ 	addiu	$t7,$sp,0x12c
/*    1c43c:	e7a40138 */ 	swc1	$f4,0x138($sp)
/*    1c440:	c6060088 */ 	lwc1	$f6,0x88($s0)
/*    1c444:	46069202 */ 	mul.s	$f8,$f18,$f6
/*    1c448:	e7a8013c */ 	swc1	$f8,0x13c($sp)
/*    1c44c:	c6100088 */ 	lwc1	$f16,0x88($s0)
/*    1c450:	46105482 */ 	mul.s	$f18,$f10,$f16
/*    1c454:	e7b20140 */ 	swc1	$f18,0x140($sp)
/*    1c458:	8d0b0008 */ 	lw	$t3,0x8($t0)
/*    1c45c:	8d6c0000 */ 	lw	$t4,0x0($t3)
/*    1c460:	514c000b */ 	beql	$t2,$t4,.L0001c490
/*    1c464:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c468:	c5a60000 */ 	lwc1	$f6,0x0($t5)
/*    1c46c:	46062280 */ 	add.s	$f10,$f4,$f6
/*    1c470:	e7aa0138 */ 	swc1	$f10,0x138($sp)
/*    1c474:	c5b00004 */ 	lwc1	$f16,0x4($t5)
/*    1c478:	46104100 */ 	add.s	$f4,$f8,$f16
/*    1c47c:	e7a4013c */ 	swc1	$f4,0x13c($sp)
/*    1c480:	c5a60008 */ 	lwc1	$f6,0x8($t5)
/*    1c484:	46069280 */ 	add.s	$f10,$f18,$f6
/*    1c488:	e7aa0140 */ 	swc1	$f10,0x140($sp)
/*    1c48c:	8fa40160 */ 	lw	$a0,0x160($sp)
.L0001c490:
/*    1c490:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c494:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c498:	afae0010 */ 	sw	$t6,0x10($sp)
/*    1c49c:	afa00014 */ 	sw	$zero,0x14($sp)
/*    1c4a0:	0c006e03 */ 	jal	model0001b80c
/*    1c4a4:	afaf0018 */ 	sw	$t7,0x18($sp)
/*    1c4a8:	1000003e */ 	b	.L0001c5a4
/*    1c4ac:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*    1c4b0:	8f090008 */ 	lw	$t1,0x8($t8)
.L0001c4b4:
/*    1c4b4:	8fb90168 */ 	lw	$t9,0x168($sp)
/*    1c4b8:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c4bc:	8d280000 */ 	lw	$t0,0x0($t1)
/*    1c4c0:	8fa50164 */ 	lw	$a1,0x164($sp)
/*    1c4c4:	8fa60168 */ 	lw	$a2,0x168($sp)
/*    1c4c8:	1328000d */ 	beq	$t9,$t0,.L0001c500
/*    1c4cc:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c4d0:	8fab0158 */ 	lw	$t3,0x158($sp)
/*    1c4d4:	27aa012c */ 	addiu	$t2,$sp,0x12c
/*    1c4d8:	afaa0018 */ 	sw	$t2,0x18($sp)
/*    1c4dc:	8fa40160 */ 	lw	$a0,0x160($sp)
/*    1c4e0:	03002825 */ 	or	$a1,$t8,$zero
/*    1c4e4:	03203025 */ 	or	$a2,$t9,$zero
/*    1c4e8:	27a70144 */ 	addiu	$a3,$sp,0x144
/*    1c4ec:	afa00014 */ 	sw	$zero,0x14($sp)
/*    1c4f0:	0c006e03 */ 	jal	model0001b80c
/*    1c4f4:	afab0010 */ 	sw	$t3,0x10($sp)
/*    1c4f8:	1000002a */ 	b	.L0001c5a4
/*    1c4fc:	8fbf002c */ 	lw	$ra,0x2c($sp)
.L0001c500:
/*    1c500:	27ac0138 */ 	addiu	$t4,$sp,0x138
/*    1c504:	27ad012c */ 	addiu	$t5,$sp,0x12c
/*    1c508:	afad0018 */ 	sw	$t5,0x18($sp)
/*    1c50c:	afac0010 */ 	sw	$t4,0x10($sp)
/*    1c510:	0c006e03 */ 	jal	model0001b80c
/*    1c514:	afa00014 */ 	sw	$zero,0x14($sp)
/*    1c518:	10000022 */ 	b	.L0001c5a4
/*    1c51c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*    1c520:	8dc50008 */ 	lw	$a1,0x8($t6)
.L0001c524:
/*    1c524:	8fa40164 */ 	lw	$a0,0x164($sp)
/*    1c528:	8faf0160 */ 	lw	$t7,0x160($sp)
/*    1c52c:	50a00006 */ 	beqzl	$a1,.L0001c548
/*    1c530:	8df00000 */ 	lw	$s0,0x0($t7)
/*    1c534:	0c006973 */ 	jal	model0001a5cc
/*    1c538:	00003025 */ 	or	$a2,$zero,$zero
/*    1c53c:	10000002 */ 	b	.L0001c548
/*    1c540:	00408025 */ 	or	$s0,$v0,$zero
/*    1c544:	8df00000 */ 	lw	$s0,0x0($t7)
.L0001c548:
/*    1c548:	1200000f */ 	beqz	$s0,.L0001c588
/*    1c54c:	8fa40158 */ 	lw	$a0,0x158($sp)
/*    1c550:	8fa40158 */ 	lw	$a0,0x158($sp)
/*    1c554:	0c0059b7 */ 	jal	mtx4LoadTranslation
/*    1c558:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*    1c55c:	8fb80158 */ 	lw	$t8,0x158($sp)
/*    1c560:	8fa90164 */ 	lw	$t1,0x164($sp)
/*    1c564:	02002025 */ 	or	$a0,$s0,$zero
/*    1c568:	8719000e */ 	lh	$t9,0xe($t8)
/*    1c56c:	8d28000c */ 	lw	$t0,0xc($t1)
/*    1c570:	27a500e8 */ 	addiu	$a1,$sp,0xe8
/*    1c574:	00195980 */ 	sll	$t3,$t9,0x6
/*    1c578:	0c0056f9 */ 	jal	mtx00015be4
/*    1c57c:	010b3021 */ 	addu	$a2,$t0,$t3
/*    1c580:	10000008 */ 	b	.L0001c5a4
/*    1c584:	8fbf002c */ 	lw	$ra,0x2c($sp)
.L0001c588:
/*    1c588:	8fac0164 */ 	lw	$t4,0x164($sp)
/*    1c58c:	848e000e */ 	lh	$t6,0xe($a0)
/*    1c590:	8d8d000c */ 	lw	$t5,0xc($t4)
/*    1c594:	000e7980 */ 	sll	$t7,$t6,0x6
/*    1c598:	0c0059b7 */ 	jal	mtx4LoadTranslation
/*    1c59c:	01af2821 */ 	addu	$a1,$t5,$t7
/*    1c5a0:	8fbf002c */ 	lw	$ra,0x2c($sp)
.L0001c5a4:
/*    1c5a4:	8fb00028 */ 	lw	$s0,0x28($sp)
/*    1c5a8:	27bd0160 */ 	addiu	$sp,$sp,0x160
/*    1c5ac:	03e00008 */ 	jr	$ra
/*    1c5b0:	00000000 */ 	nop
);
#endif

void model0001c5b4(struct modelrenderdata *arg0, struct model *model, struct modelnode *node)
{
	union modelrodata *rodata = node->rodata;
	Mtxf *sp68;
	Mtxf sp28;
	s32 mtxindex = rodata->positionheld.unk0c;
	Mtxf *matrices = model->matrices;

	if (node->parent) {
		sp68 = model0001a5cc(model, node->parent, 0);
	} else {
		sp68 = arg0->unk00;
	}

	if (sp68) {
		mtx4LoadTranslation(&rodata->positionheld.pos, &sp28);
		mtx00015be4(sp68, &sp28, &matrices[mtxindex]);
	} else {
		mtx4LoadTranslation(&rodata->positionheld.pos, &matrices[mtxindex]);
	}
}

/**
 * For a distance node, set its target to visible based on distance.
 */
void model0001c664(struct model *model, struct modelnode *node)
{
	union modelrodata *rodata = node->rodata;
	union modelrwdata *rwdata = modelGetNodeRwData(model, node);
	Mtxf *mtx = model0001a5cc(model, node, 0);
	f32 distance;

	if (g_ModelDistanceDisabled || !mtx) {
		distance = 0;
	} else {
		distance = -mtx->m[3][2] * camGetLodScaleZ();

		if (g_ModelDistanceScale != 1) {
			distance *= g_ModelDistanceScale;
		}
	}

	if (distance > rodata->distance.near * model->scale || rodata->distance.near == 0) {
		if (distance <= rodata->distance.far * model->scale) {
			rwdata->distance.visible = true;
			node->child = rodata->distance.target;
			return;
		}
	}

	rwdata->distance.visible = false;
	node->child = NULL;
}

void model0001c784(struct model *model, struct modelnode *node)
{
	struct modelrodata_distance *rodata = &node->rodata->distance;
	struct modelrwdata_distance *rwdata = modelGetNodeRwData(model, node);

	if (rwdata->visible) {
		node->child = rodata->target;
	} else {
		node->child = NULL;
	}
}

void model0001c7d0(struct model *model, struct modelnode *node)
{
	struct modelrodata_toggle *rodata = &node->rodata->toggle;
	struct modelrwdata_toggle *rwdata = modelGetNodeRwData(model, node);

	if (rwdata->visible) {
		node->child = rodata->target;
	} else {
		node->child = NULL;
	}
}

/**
 * Attach a head model to its placeholder on the body model.
 *
 * The given modelnode is assumed to be of type MODELNODETYPE_HEADSPOT.
 */
void modelAttachHead(struct model *model, struct modelnode *bodynode)
{
	struct modelrwdata_headspot *rwdata = modelGetNodeRwData(model, bodynode);

	if (rwdata->modelfiledata) {
		struct modelnode *headnode = rwdata->modelfiledata->rootnode;

		bodynode->child = headnode;

		while (headnode) {
			headnode->parent = bodynode;
			headnode = headnode->next;
		}
	}
}

void model0001c868(struct modelnode *basenode, bool visible)
{
	union modelrodata *rodata = basenode->rodata;
	struct modelnode *node1;
	struct modelnode *node2;
	struct modelnode *loopnode;

	if (visible) {
		node1 = rodata->reorder.unk18;
		node2 = rodata->reorder.unk1c;
	} else {
		node1 = rodata->reorder.unk1c;
		node2 = rodata->reorder.unk18;
	}

	if (node1) {
		// I think what's happening here is there's two groups of siblings,
		// where node1 and node2 are the head nodes. Either group can be first,
		// and this is ensuring the node1 group is first.
		// Note that node2 might be NULL.

		basenode->child = node1;
		node1->prev = NULL;

		// Skip through node1's siblings until node2 is found or the end is
		// reached
		loopnode = node1;

		while (loopnode->next && loopnode->next != node2) {
			loopnode = loopnode->next;
		}

		loopnode->next = node2;

		if (node2) {
			// Append node2 and its siblings to node1's siblings
			node2->prev = loopnode;
			loopnode = node2;

			while (loopnode->next && loopnode->next != node1) {
				loopnode = loopnode->next;
			}

			loopnode->next = NULL;
		}
	} else {
		basenode->child = node2;

		if (node2) {
			node2->prev = NULL;
		}
	}
}

void modelRenderNodeReorder(struct model *model, struct modelnode *node)
{
	union modelrwdata *rwdata = modelGetNodeRwData(model, node);

	model0001c868(node, rwdata->reorder.visible);
}

void model0001c950(struct model *model, struct modelnode *node)
{
	union modelrodata *rodata = node->rodata;
	union modelrwdata *rwdata = modelGetNodeRwData(model, node);
	Mtxf *mtx = model0001a5cc(model, node, 0);
	struct coord sp38;
	struct coord sp2c;
	f32 tmp;

	if (rodata->reorder.unk20 == 0) {
		sp38.x = rodata->reorder.unk0c[0];
		sp38.y = rodata->reorder.unk0c[1];
		sp38.z = rodata->reorder.unk0c[2];
		mtx4RotateVecInPlace(mtx, &sp38);
	} else if (rodata->reorder.unk20 == 2) {
		sp38.x = mtx->m[1][0] * rodata->reorder.unk0c[1];
		sp38.y = mtx->m[1][1] * rodata->reorder.unk0c[1];
		sp38.z = mtx->m[1][2] * rodata->reorder.unk0c[1];
	} else if (rodata->reorder.unk20 == 3) {
		sp38.x = mtx->m[2][0] * rodata->reorder.unk0c[2];
		sp38.y = mtx->m[2][1] * rodata->reorder.unk0c[2];
		sp38.z = mtx->m[2][2] * rodata->reorder.unk0c[2];
	} else if (rodata->reorder.unk20 == 1) {
		sp38.x = mtx->m[0][0] * rodata->reorder.unk0c[0];
		sp38.y = mtx->m[0][1] * rodata->reorder.unk0c[0];
		sp38.z = mtx->m[0][2] * rodata->reorder.unk0c[0];
	}

	sp2c.x = rodata->reorder.unk00;
	sp2c.y = rodata->reorder.unk04;
	sp2c.z = rodata->reorder.unk08;

	mtx4TransformVecInPlace(mtx, &sp2c);

	tmp = sp38.f[0] * sp2c.f[0] + sp38.f[1] * sp2c.f[1] + sp38.f[2] * sp2c.f[2];

	if (tmp < 0) {
		rwdata->reorder.visible = true;
	} else {
		rwdata->reorder.visible = false;
	}

	modelRenderNodeReorder(model, node);
}

void model0001cb0c(struct model *model, struct modelnode *parent)
{
	struct modelnode *node = parent->child;

	if (parent);

	while (node) {
		s32 type = node->type & 0xff;
		bool dochildren = true;

		switch (type) {
		case MODELNODETYPE_CHRINFO:
		case MODELNODETYPE_POSITION:
		case MODELNODETYPE_0B:
		case MODELNODETYPE_GUNFIRE:
		case MODELNODETYPE_0D:
		case MODELNODETYPE_0E:
		case MODELNODETYPE_0F:
		case MODELNODETYPE_POSITIONHELD:
			dochildren = false;
			break;
		case MODELNODETYPE_DISTANCE:
			model0001c664(model, node);
			break;
		case MODELNODETYPE_REORDER:
			model0001c950(model, node);
			break;
		case MODELNODETYPE_HEADSPOT:
			modelAttachHead(model, node);
			break;
		case MODELNODETYPE_DL:
			break;
		}

		if (dochildren && node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node == parent->parent) {
					node = NULL;
					break;
				}

				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}
}

void model0001cc20(struct model *model)
{
	struct modelnode *node = model->filedata->rootnode;

	while (node) {
		u32 type = node->type & 0xff;

		switch (type) {
		case MODELNODETYPE_DISTANCE:
			model0001c664(model, node);
			break;
		case MODELNODETYPE_REORDER:
			model0001c950(model, node);
			break;
		case MODELNODETYPE_TOGGLE:
			model0001c7d0(model, node);
			break;
		case MODELNODETYPE_HEADSPOT:
			modelAttachHead(model, node);
			break;
		case MODELNODETYPE_CHRINFO:
		case MODELNODETYPE_DL:
		default:
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}
}

void model0001cd18(struct modelrenderdata *arg0, struct model *model)
{
	struct modelnode *node = model->filedata->rootnode;

	while (node) {
		u32 type = node->type & 0xff;

		switch (type) {
		case MODELNODETYPE_CHRINFO:
			model0001b400(arg0, model, node);
			break;
		case MODELNODETYPE_POSITION:
			model0001bfa8(arg0, model, node);
			break;
		case MODELNODETYPE_POSITIONHELD:
			model0001c5b4(arg0, model, node);
			break;
		case MODELNODETYPE_DISTANCE:
			model0001c664(model, node);
			break;
		case MODELNODETYPE_REORDER:
			model0001c950(model, node);
			break;
		case MODELNODETYPE_TOGGLE:
			model0001c7d0(model, node);
			break;
		case MODELNODETYPE_HEADSPOT:
			modelAttachHead(model, node);
			break;
		case MODELNODETYPE_DL:
		default:
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}
}

void model0001ce64(struct modelrenderdata *arg0, struct model *model)
{
	model->matrices = arg0->unk10;

	arg0->unk10 += model->filedata->nummatrices;

#if VERSION >= VERSION_PAL_BETA
	if (var8005efb0_2 || !model00018680()) {
		model0001cd18(arg0, model);
	}
#else
	if (!model00018680()) {
		model0001cd18(arg0, model);
	}
#endif
}

void model0001cebc(struct modelrenderdata *arg0, struct model *model)
{
	struct anim *anim = model->anim;
	f32 speed;
	f32 frac;
	f32 frac2;

	if (anim && anim->animnum) {
		if (PLAYERCOUNT() >= 2) {
			frac = anim->frac;
			frac2 = anim->frac2;
			speed = anim->speed;

			if (speed < 0) {
				speed = -speed;
			}

			if (speed > 0.5f) {
				anim->frac = 0;
				anim->frac2 = 0;
			}
		}

		anim00023d38(anim->animnum);
		anim->unk04 = anim00023ab0(anim->animnum, anim->framea);

		if (anim->frac != 0) {
			anim->unk05 = anim00023ab0(anim->animnum, anim->frameb);
		}

		if (anim->animnum2) {
			anim00023d38(anim->animnum2);
			anim->unk06 = anim00023ab0(anim->animnum2, anim->frame2a);

			if (anim->frac2 != 0) {
				anim->unk07 = anim00023ab0(anim->animnum2, anim->frame2b);
			}
		}

		anim00023d0c();
	}

	model0001ce64(arg0, model);

	if (PLAYERCOUNT() >= 2 && anim && anim->animnum) {
		anim->frac = frac;
		anim->frac2 = frac2;
	}
}

s16 modelGetAnimNum(struct model *model)
{
	if (model->anim) {
		return model->anim->animnum;
	}

	return 0;
}

bool modelIsFlipped(struct model *model)
{
	if (model->anim) {
		return model->anim->flip;
	}

	return false;
}

f32 modelGetCurAnimFrame(struct model *model)
{
	if (model->anim) {
		return model->anim->frame;
	}

	return 0;
}

f32 modelGetAnimEndFrame(struct model *model)
{
	struct anim *anim = model->anim;

	if (anim) {
		if (anim->endframe >= 0) {
			return anim->endframe;
		}

		if (anim->animnum) {
			return animGetNumFrames(anim->animnum) - 1;
		} else {
			return 0;
		}
	}

	return 0;
}

s32 modelGetNumAnimFrames(struct model *model)
{
	if (model->anim) {
		return animGetNumFrames(modelGetAnimNum(model));
	}

	return 0;
}

f32 modelGetAnimSpeed(struct model *model)
{
	if (model->anim) {
		return model->anim->speed;
	}

	return 1;
}

f32 modelGetAbsAnimSpeed(struct model *model)
{
	f32 speed;

	if (model->anim) {
		speed = model->anim->speed;

		if (speed < 0) {
			speed = -speed;
		}

		return speed;
	}

	return 1;
}

f32 modelGetEffectiveAnimSpeed(struct model *model)
{
	if (model->anim) {
		return modelGetAnimSpeed(model) * model->anim->playspeed;
	}

	return 1;
}

/**
 * Constrain the given frame number to the bounds of the animation, unless the
 * animation is looping in which case wrap it to the other side.
 */
s32 modelConstrainOrWrapAnimFrame(s32 frame, s16 animnum, f32 endframe)
{
	if (frame < 0) {
		if (var8005efbc || (g_Anims[animnum].flags & ANIMFLAG_LOOP)) {
			frame = animGetNumFrames(animnum) - (-frame % animGetNumFrames(animnum));
		} else {
			frame = 0;
		}
	} else if (endframe >= 0 && frame > (s32)endframe) {
		frame = ceil(endframe);
	} else if (frame >= animGetNumFrames(animnum)) {
		if (var8005efbc || (g_Anims[animnum].flags & ANIMFLAG_LOOP)) {
			frame = frame % animGetNumFrames(animnum);
		} else {
			frame = animGetNumFrames(animnum) - 1;
		}
	}

	return frame;
}

void modelCopyAnimForMerge(struct model *model, f32 merge)
{
	struct anim *anim = model->anim;
	struct modelnode *node;
	u32 nodetype;

	if (anim) {
		if (merge > 0 && anim->animnum) {
			if (anim->animnum2 && anim->fracmerge == 1) {
				return;
			}

			node = model->filedata->rootnode;
			nodetype = node->type & 0xff;

			anim->frame2 = anim->frame;
			anim->frac2 = anim->frac;
			anim->animnum2 = anim->animnum;
			anim->flip2 = anim->flip;
			anim->frame2a = anim->framea;
			anim->frame2b = anim->frameb;
			anim->speed2 = anim->speed;
			anim->newspeed2 = anim->newspeed;
			anim->oldspeed2 = anim->oldspeed;
			anim->timespeed2 = anim->timespeed;
			anim->elapsespeed2 = anim->elapsespeed;
			anim->endframe2 = anim->endframe;

			if (nodetype == MODELNODETYPE_CHRINFO) {
				struct modelrwdata_chrinfo *rwdata = modelGetNodeRwData(model, node);
				rwdata->unk02 = 1;
				rwdata->unk4c.x = rwdata->unk34.x;
				rwdata->unk4c.y = rwdata->unk34.y;
				rwdata->unk4c.z = rwdata->unk34.z;
				rwdata->unk40.x = rwdata->unk24.x;
				rwdata->unk40.y = rwdata->unk24.y;
				rwdata->unk40.z = rwdata->unk24.z;
			}
		} else {
			anim->animnum2 = 0;
		}
	}
}

void model0001d62c(struct model *model, s16 animnum, s32 flip, f32 fstartframe, f32 speed, f32 merge)
{
	struct anim *anim = model->anim;

	if (anim) {
		s32 isfirstanim = !anim->animnum;
		s32 type;

		if (anim->animnum2) {
			anim->timemerge = merge;
			anim->elapsemerge = 0;
			anim->fracmerge = 1;
		} else {
			anim->timemerge = 0;
			anim->fracmerge = 0;
		}

		anim->animnum = animnum;
		anim->flip = flip;
		anim->endframe = -1;
		anim->speed = speed;
		anim->timespeed = 0;

		model0001e018(model, fstartframe);

		anim->looping = false;

		type = model->filedata->rootnode->type & 0xff;

		if (type == MODELNODETYPE_CHRINFO) {
			u32 stack;
			struct modelrodata_chrinfo *rodata = &model->filedata->rootnode->rodata->chrinfo;
			struct modelrwdata_chrinfo *rwdata = (struct modelrwdata_chrinfo *) modelGetNodeRwData(model, model->filedata->rootnode);
			s32 spa4 = rodata->unk00;
			struct skeleton *skel = model->filedata->skel;
			f32 scale;
			f32 sp98;
			f32 sp94;
			struct coord sp88 = {0, 0, 0};
			f32 sp84;
			u8 sp83;
			struct coord sp74;
			struct coord sp68;
			f32 sp64;
			struct coord sp58;
			struct coord sp4c;
			f32 angle;
			f32 y;
			f32 x;
			f32 z;

			if (g_Anims[anim->animnum].flags & ANIMFLAG_02) {
				sp64 = func0f15c888();
				anim00023d38(anim->animnum);
				sp83 = anim00023ab0(anim->animnum, anim->framea);
				anim00023d0c();
				anim00024050(spa4, anim->flip, skel, anim->animnum, sp83, &sp74, &sp88, &sp68);

				rwdata->unk34.x = sp88.x * sp64;
				rwdata->unk34.y = sp88.y * sp64;
				rwdata->unk34.z = sp88.z * sp64;
				rwdata->unk30 = rwdata->unk14;

				if (anim->frac == 0) {
					rwdata->unk01 = 0;
				} else {
					anim00023d38(anim->animnum);
					sp83 = anim00023ab0(anim->animnum, anim->frameb);
					anim00023d0c();
					anim00024050(spa4, anim->flip, skel, anim->animnum, sp83, &sp74, &sp88, &sp68);

					rwdata->unk24.x = sp88.x * sp64;
					rwdata->unk24.y = sp88.y * sp64;
					rwdata->unk24.z = sp88.z * sp64;
					rwdata->unk20 = rwdata->unk14;

					rwdata->unk01 = 1;
				}
			} else {
				sp84 = anim00024b64(spa4, anim->flip, skel, anim->animnum, anim->frameb, &sp88, anim->average);
				scale = model->scale * anim->animscale;

				if (scale != 1) {
					sp88.x *= scale;
					sp88.y *= scale;
					sp88.z *= scale;
				}

				if (anim->average) {
					sp88.y = rwdata->pos.y - rwdata->ground;
				}

				sp98 = cosf(rwdata->unk14);
				sp94 = sinf(rwdata->unk14);

				if (anim->frac == 0) {
					rwdata->unk34.x = rwdata->pos.f[0];
					rwdata->unk34.y = rwdata->pos.f[1] - rwdata->ground;
					rwdata->unk34.z = rwdata->pos.f[2];

					rwdata->unk30 = rwdata->unk14;

					sp58.x = rwdata->unk34.f[0] + sp88.f[0] * sp98 + sp88.f[2] * sp94;
					sp58.y = sp88.f[1];
					sp58.z = rwdata->unk34.f[2] - sp88.f[0] * sp94 + sp88.f[2] * sp98;

					rwdata->unk24.x = sp58.f[0];
					rwdata->unk24.y = sp58.f[1];
					rwdata->unk24.z = sp58.f[2];

					if (rwdata->unk18 == 0) {
						rwdata->unk20 = rwdata->unk30 + sp84;

						if (rwdata->unk20 >= M_BADTAU) {
							rwdata->unk20 -= M_BADTAU;
						}
					}

					rwdata->unk01 = 1;
				} else {
					x = sp88.f[0] * sp98 + sp88.f[2] * sp94;
					y = sp88.f[1];
					z = -sp88.f[0] * sp94 + sp88.f[2] * sp98;

					sp4c.f[0] = rwdata->pos.f[0] + x * (1 - anim->frac);
					sp4c.f[1] = y;
					sp4c.f[2] = rwdata->pos.f[2] + z * (1 - anim->frac);

					rwdata->unk24.f[0] = sp4c.f[0];
					rwdata->unk24.f[1] = sp4c.f[1];
					rwdata->unk24.f[2] = sp4c.f[2];

					rwdata->unk34.f[0] = rwdata->unk24.f[0] - x;
					rwdata->unk34.f[1] = (rwdata->pos.f[1] - rwdata->ground) - (y - (rwdata->pos.f[1] - rwdata->ground)) * anim->frac / (1 - anim->frac);
					rwdata->unk34.f[2] = rwdata->unk24.f[2] - z;

					angle = rwdata->unk14 - sp84;

					if (angle < 0) {
						angle += M_BADTAU;
					}

					rwdata->unk30 = model0001afe8(rwdata->unk14, angle, anim->frac);

					if (rwdata->unk18 == 0) {
						rwdata->unk20 = rwdata->unk30 + sp84;

						if (rwdata->unk20 >= M_BADTAU) {
							rwdata->unk20 -= M_BADTAU;
						}
					}

					rwdata->unk01 = 1;
				}

				if (isfirstanim) {
					rwdata->unk34.f[1] = rwdata->unk24.f[1];
				}
			}
		}
	}
}

bool modelIsAnimMerging(struct model *model)
{
	if (model && model->anim && model->anim->animnum2
			&& model->anim->fracmerge != 0 && model->anim->fracmerge != 1) {
		return true;
	}

	return false;
}

void modelSetAnimationWithMerge(struct model *model, s16 animnum, u32 flip, f32 startframe, f32 speed, f32 timemerge, bool domerge)
{
	if (model) {
		if (model->anim && model->anim->animnum
				&& (g_Anims[model->anim->animnum].flags & ANIMFLAG_02)
				&& (g_Anims[animnum].flags & ANIMFLAG_02) == 0) {
			timemerge = 0;
		}

		if (domerge) {
			modelCopyAnimForMerge(model, timemerge);
		}

		model0001d62c(model, animnum, flip, startframe, speed, timemerge);
	}
}

void modelSetAnimation(struct model *model, s16 animnum, s32 flip, f32 startframe, f32 speed, f32 merge)
{
	if (model) {
		if (model->anim && model->anim->animnum
				&& (g_Anims[model->anim->animnum].flags & ANIMFLAG_02)
				&& (g_Anims[animnum].flags & ANIMFLAG_02) == 0) {
			merge = 0;
		}

		modelCopyAnimForMerge(model, merge);
		model0001d62c(model, animnum, flip, startframe, speed, merge);
	}
}

void modelCopyAnimData(struct model *src, struct model *dst)
{
	if (src->anim && dst->anim) {
		*dst->anim = *src->anim;
	}
}

void modelSetAnimLooping(struct model *model, f32 loopframe, f32 loopmerge)
{
	if (model->anim) {
		model->anim->looping = true;
		model->anim->loopframe = loopframe;
		model->anim->loopmerge = loopmerge;
	}
}

void modelSetAnimEndFrame(struct model *model, f32 endframe)
{
	struct anim *anim = model->anim;

	if (anim) {
		if (anim->animnum && endframe < animGetNumFrames(anim->animnum) - 1) {
			anim->endframe = endframe;
		} else {
			anim->endframe = -1;
		}
	}
}

void modelSetAnimFlipFunction(struct model *model, void *callback)
{
	if (model->anim) {
		model->anim->flipfunc = callback;
	}
}

#if VERSION < VERSION_NTSC_1_0
void modelSetAnimUnk6c(struct model *model, s32 value)
{
	if (model->anim) {
		model->anim->unk6c = value;
	}
}
#endif

void modelSetAnimSpeed(struct model *model, f32 speed, f32 startframe)
{
	struct anim *anim = model->anim;

	if (anim) {
		if (startframe > 0) {
			anim->timespeed = startframe;
			anim->newspeed = speed;
			anim->elapsespeed = 0;
			anim->oldspeed = anim->speed;
		} else {
			anim->speed = speed;
			anim->timespeed = 0;
		}
	}
}

void modelSetAnimSpeedAuto(struct model *model, f32 arg1, f32 startframe)
{
	struct anim *anim = model->anim;
	f32 tmp;
	f32 speed;

	if (anim) {
		if (anim->frame <= arg1) {
			tmp = arg1 - anim->frame;
		} else {
			tmp = animGetNumFrames(anim->animnum) - anim->frame + arg1;
		}

		speed = anim->speed + (tmp + tmp) / startframe;

		modelSetAnimSpeed(model, speed, startframe);
	}
}

void modelSetAnimPlaySpeed(struct model *model, f32 speed, f32 startframe)
{
	struct anim *anim = model->anim;

	if (anim) {
		if (startframe > 0) {
			anim->timeplay = startframe;
			anim->newplay = speed;
			anim->elapseplay = 0;
			anim->oldplay = anim->playspeed;
		} else {
			anim->playspeed = speed;
			anim->timeplay = 0;
		}
	}
}

void modelSetAnim70(struct model *model, void *callback)
{
	if (model->anim) {
		model->anim->unk70 = callback;
	}
}

void model0001e018(struct model *model, f32 arg1)
{
	s32 sp28;
	s32 sp24;
	bool sp20;
	struct anim *anim = model->anim;

	if (anim) {
		sp28 = floor(arg1);
		sp20 = anim->speed >= 0;

		sp24 = (sp20 ? sp28 + 1 : sp28 - 1);

		anim->framea = modelConstrainOrWrapAnimFrame(sp28, anim->animnum, anim->endframe);
		anim->frameb = modelConstrainOrWrapAnimFrame(sp24, anim->animnum, anim->endframe);

		if (anim->framea == anim->frameb) {
			anim->frac = 0;
			anim->frame = anim->framea;
		} else if (sp20) {
			anim->frac = arg1 - sp28;
			anim->frame = anim->framea + anim->frac;
		} else {
			anim->frac = 1 - (arg1 - sp24);
			anim->frame = anim->frameb + (1 - anim->frac);
		}
	}
}

void model0001e14c(struct model *model, f32 arg1, f32 arg2)
{
	struct anim *anim = model->anim;

	if (anim) {
		model0001e018(model, arg1);

		if (anim->animnum2) {
			s32 sp28 = floor(arg2);
			s32 sp24;
			bool sp20 = anim->speed2 >= 0;

			sp24 = (sp20 ? sp28 + 1 : sp28 - 1);

			anim->frame2a = modelConstrainOrWrapAnimFrame(sp28, anim->animnum2, anim->endframe2);
			anim->frame2b = modelConstrainOrWrapAnimFrame(sp24, anim->animnum2, anim->endframe2);

			if (anim->frame2a == anim->frame2b) {
				anim->frac2 = 0;
				anim->frame2 = anim->frame2a;
			} else if (sp20) {
				anim->frac2 = arg2 - sp28;
				anim->frame2 = anim->frame2a + anim->frac2;
			} else {
				anim->frac2 = 1 - (arg2 - sp24);
				anim->frame2 = anim->frame2b + (1 - anim->frac2);
			}
		}
	}
}

bool var8005efdc = true;
u32 var8005efe0 = 0x00000000;
u32 var8005efe4 = 0x00000000;
u32 var8005efe8 = 0x00000000;
u32 var8005efec = 0x00000000;
u32 var8005eff0 = 0x00000000;
u32 var8005eff4 = 0x00000000;
u32 var8005eff8 = 0xffffffff;

void model0001e29c(bool value)
{
	var8005efdc = value;
}

bool model0001e2a8(void)
{
	return var8005efdc;
}

GLOBAL_ASM(
glabel model0001e2b4
.late_rodata
glabel var70054450
.word 0x40c907a9
.text
/*    1e2b4:	27bdfed0 */ 	addiu	$sp,$sp,-304
/*    1e2b8:	afbf006c */ 	sw	$ra,0x6c($sp)
/*    1e2bc:	afb40068 */ 	sw	$s4,0x68($sp)
/*    1e2c0:	afb30064 */ 	sw	$s3,0x64($sp)
/*    1e2c4:	afb20060 */ 	sw	$s2,0x60($sp)
/*    1e2c8:	afb1005c */ 	sw	$s1,0x5c($sp)
/*    1e2cc:	afb00058 */ 	sw	$s0,0x58($sp)
/*    1e2d0:	f7be0050 */ 	sdc1	$f30,0x50($sp)
/*    1e2d4:	f7bc0048 */ 	sdc1	$f28,0x48($sp)
/*    1e2d8:	f7ba0040 */ 	sdc1	$f26,0x40($sp)
/*    1e2dc:	f7b80038 */ 	sdc1	$f24,0x38($sp)
/*    1e2e0:	f7b60030 */ 	sdc1	$f22,0x30($sp)
/*    1e2e4:	f7b40028 */ 	sdc1	$f20,0x28($sp)
/*    1e2e8:	afa60138 */ 	sw	$a2,0x138($sp)
/*    1e2ec:	afa7013c */ 	sw	$a3,0x13c($sp)
/*    1e2f0:	8c910020 */ 	lw	$s1,0x20($a0)
/*    1e2f4:	4485a000 */ 	mtc1	$a1,$f20
/*    1e2f8:	00808025 */ 	or	$s0,$a0,$zero
/*    1e2fc:	522002b9 */ 	beqzl	$s1,.L0001ede4
/*    1e300:	8fbf006c */ 	lw	$ra,0x6c($sp)
/*    1e304:	8c8e0008 */ 	lw	$t6,0x8($a0)
/*    1e308:	24010001 */ 	addiu	$at,$zero,0x1
/*    1e30c:	8dc50000 */ 	lw	$a1,0x0($t6)
/*    1e310:	94a20000 */ 	lhu	$v0,0x0($a1)
/*    1e314:	304f00ff */ 	andi	$t7,$v0,0xff
/*    1e318:	55e102ac */ 	bnel	$t7,$at,.L0001edcc
/*    1e31c:	c7b40140 */ 	lwc1	$f20,0x140($sp)
/*    1e320:	0c006a87 */ 	jal	modelGetNodeRwData
/*    1e324:	8cb30004 */ 	lw	$s3,0x4($a1)
/*    1e328:	80580000 */ 	lb	$t8,0x0($v0)
/*    1e32c:	00409025 */ 	or	$s2,$v0,$zero
/*    1e330:	5700029e */ 	bnezl	$t8,.L0001edac
/*    1e334:	c7b40140 */ 	lwc1	$f20,0x140($sp)
/*    1e338:	96790000 */ 	lhu	$t9,0x0($s3)
/*    1e33c:	3c0b8006 */ 	lui	$t3,%hi(var8005efe0)
/*    1e340:	256befe0 */ 	addiu	$t3,$t3,%lo(var8005efe0)
/*    1e344:	afb90118 */ 	sw	$t9,0x118($sp)
/*    1e348:	8e080008 */ 	lw	$t0,0x8($s0)
/*    1e34c:	27aa00fc */ 	addiu	$t2,$sp,0xfc
/*    1e350:	4480d000 */ 	mtc1	$zero,$f26
/*    1e354:	8d090004 */ 	lw	$t1,0x4($t0)
/*    1e358:	afa90114 */ 	sw	$t1,0x114($sp)
/*    1e35c:	c6260088 */ 	lwc1	$f6,0x88($s1)
/*    1e360:	c6040014 */ 	lwc1	$f4,0x14($s0)
/*    1e364:	46062202 */ 	mul.s	$f8,$f4,$f6
/*    1e368:	e7a80110 */ 	swc1	$f8,0x110($sp)
/*    1e36c:	8d610000 */ 	lw	$at,0x0($t3)
/*    1e370:	8d6e0004 */ 	lw	$t6,0x4($t3)
/*    1e374:	ad410000 */ 	sw	$at,0x0($t2)
/*    1e378:	8d610008 */ 	lw	$at,0x8($t3)
/*    1e37c:	ad4e0004 */ 	sw	$t6,0x4($t2)
/*    1e380:	ad410008 */ 	sw	$at,0x8($t2)
/*    1e384:	c44a0034 */ 	lwc1	$f10,0x34($v0)
/*    1e388:	e7aa00e0 */ 	swc1	$f10,0xe0($sp)
/*    1e38c:	c4500038 */ 	lwc1	$f16,0x38($v0)
/*    1e390:	e7b000e4 */ 	swc1	$f16,0xe4($sp)
/*    1e394:	c452003c */ 	lwc1	$f18,0x3c($v0)
/*    1e398:	c7b00138 */ 	lwc1	$f16,0x138($sp)
/*    1e39c:	e7b200e8 */ 	swc1	$f18,0xe8($sp)
/*    1e3a0:	c4440024 */ 	lwc1	$f4,0x24($v0)
/*    1e3a4:	c45e0030 */ 	lwc1	$f30,0x30($v0)
/*    1e3a8:	e7a400d0 */ 	swc1	$f4,0xd0($sp)
/*    1e3ac:	c4460028 */ 	lwc1	$f6,0x28($v0)
/*    1e3b0:	e7a600d4 */ 	swc1	$f6,0xd4($sp)
/*    1e3b4:	c448002c */ 	lwc1	$f8,0x2c($v0)
/*    1e3b8:	e7a800d8 */ 	swc1	$f8,0xd8($sp)
/*    1e3bc:	c44a0020 */ 	lwc1	$f10,0x20($v0)
/*    1e3c0:	e7aa00cc */ 	swc1	$f10,0xcc($sp)
/*    1e3c4:	804f0001 */ 	lb	$t7,0x1($v0)
/*    1e3c8:	afaf00c8 */ 	sw	$t7,0xc8($sp)
/*    1e3cc:	c63c001c */ 	lwc1	$f28,0x1c($s1)
/*    1e3d0:	461ae03c */ 	c.lt.s	$f28,$f26
/*    1e3d4:	00000000 */ 	nop
/*    1e3d8:	45020003 */ 	bc1fl	.L0001e3e8
/*    1e3dc:	c6200040 */ 	lwc1	$f0,0x40($s1)
/*    1e3e0:	4600e707 */ 	neg.s	$f28,$f28
/*    1e3e4:	c6200040 */ 	lwc1	$f0,0x40($s1)
.L0001e3e8:
/*    1e3e8:	461a003c */ 	c.lt.s	$f0,$f26
/*    1e3ec:	00000000 */ 	nop
/*    1e3f0:	45020003 */ 	bc1fl	.L0001e400
/*    1e3f4:	4610a03e */ 	c.le.s	$f20,$f16
/*    1e3f8:	46000007 */ 	neg.s	$f0,$f0
/*    1e3fc:	4610a03e */ 	c.le.s	$f20,$f16
.L0001e400:
/*    1e400:	0000a025 */ 	or	$s4,$zero,$zero
/*    1e404:	45000002 */ 	bc1f	.L0001e410
/*    1e408:	00000000 */ 	nop
/*    1e40c:	24140001 */ 	addiu	$s4,$zero,0x1
.L0001e410:
/*    1e410:	5280000a */ 	beqzl	$s4,.L0001e43c
/*    1e414:	4600a306 */ 	mov.s	$f12,$f20
/*    1e418:	4600a306 */ 	mov.s	$f12,$f20
/*    1e41c:	0fc25e42 */ 	jal	floor
/*    1e420:	e7a000bc */ 	swc1	$f0,0xbc($sp)
/*    1e424:	24530001 */ 	addiu	$s3,$v0,0x1
/*    1e428:	0fc25e42 */ 	jal	floor
/*    1e42c:	c7ac0138 */ 	lwc1	$f12,0x138($sp)
/*    1e430:	10000009 */ 	b	.L0001e458
/*    1e434:	00403825 */ 	or	$a3,$v0,$zero
/*    1e438:	4600a306 */ 	mov.s	$f12,$f20
.L0001e43c:
/*    1e43c:	0fc25e7a */ 	jal	ceil
/*    1e440:	e7a000bc */ 	swc1	$f0,0xbc($sp)
/*    1e444:	2453ffff */ 	addiu	$s3,$v0,-1
/*    1e448:	0fc25e7a */ 	jal	ceil
/*    1e44c:	c7ac0138 */ 	lwc1	$f12,0x138($sp)
/*    1e450:	afa200ec */ 	sw	$v0,0xec($sp)
/*    1e454:	00403825 */ 	or	$a3,$v0,$zero
.L0001e458:
/*    1e458:	86390000 */ 	lh	$t9,0x0($s1)
/*    1e45c:	3c188006 */ 	lui	$t8,%hi(g_Anims)
/*    1e460:	8f18f00c */ 	lw	$t8,%lo(g_Anims)($t8)
/*    1e464:	00194080 */ 	sll	$t0,$t9,0x2
/*    1e468:	01194023 */ 	subu	$t0,$t0,$t9
/*    1e46c:	00084080 */ 	sll	$t0,$t0,0x2
/*    1e470:	03084821 */ 	addu	$t1,$t8,$t0
/*    1e474:	912d000b */ 	lbu	$t5,0xb($t1)
/*    1e478:	3c017005 */ 	lui	$at,%hi(var70054450)
/*    1e47c:	31ac0002 */ 	andi	$t4,$t5,0x2
/*    1e480:	11800067 */ 	beqz	$t4,.L0001e620
/*    1e484:	00000000 */ 	nop
/*    1e488:	0fc57222 */ 	jal	func0f15c888
/*    1e48c:	afa700ec */ 	sw	$a3,0xec($sp)
/*    1e490:	8faa00ec */ 	lw	$t2,0xec($sp)
/*    1e494:	862b0014 */ 	lh	$t3,0x14($s1)
/*    1e498:	46000506 */ 	mov.s	$f20,$f0
/*    1e49c:	01402025 */ 	or	$a0,$t2,$zero
/*    1e4a0:	114b005d */ 	beq	$t2,$t3,.L0001e618
/*    1e4a4:	00000000 */ 	nop
/*    1e4a8:	86250000 */ 	lh	$a1,0x0($s1)
/*    1e4ac:	0c0074c8 */ 	jal	modelConstrainOrWrapAnimFrame
/*    1e4b0:	8e260018 */ 	lw	$a2,0x18($s1)
/*    1e4b4:	a6220014 */ 	sh	$v0,0x14($s1)
/*    1e4b8:	8fae00c8 */ 	lw	$t6,0xc8($sp)
/*    1e4bc:	00408025 */ 	or	$s0,$v0,$zero
/*    1e4c0:	8faf00ec */ 	lw	$t7,0xec($sp)
/*    1e4c4:	11c0000b */ 	beqz	$t6,.L0001e4f4
/*    1e4c8:	00000000 */ 	nop
/*    1e4cc:	86390016 */ 	lh	$t9,0x16($s1)
/*    1e4d0:	c7b200d0 */ 	lwc1	$f18,0xd0($sp)
/*    1e4d4:	15f90007 */ 	bne	$t7,$t9,.L0001e4f4
/*    1e4d8:	00000000 */ 	nop
/*    1e4dc:	c7a400d4 */ 	lwc1	$f4,0xd4($sp)
/*    1e4e0:	c7a600d8 */ 	lwc1	$f6,0xd8($sp)
/*    1e4e4:	e7b200e0 */ 	swc1	$f18,0xe0($sp)
/*    1e4e8:	e7a400e4 */ 	swc1	$f4,0xe4($sp)
/*    1e4ec:	1000001f */ 	b	.L0001e56c
/*    1e4f0:	e7a600e8 */ 	swc1	$f6,0xe8($sp)
.L0001e4f4:
/*    1e4f4:	0c008f4e */ 	jal	anim00023d38
/*    1e4f8:	86240000 */ 	lh	$a0,0x0($s1)
/*    1e4fc:	86240000 */ 	lh	$a0,0x0($s1)
/*    1e500:	0c008eac */ 	jal	anim00023ab0
/*    1e504:	02002825 */ 	or	$a1,$s0,$zero
/*    1e508:	0c008f43 */ 	jal	anim00023d0c
/*    1e50c:	305000ff */ 	andi	$s0,$v0,0xff
/*    1e510:	82250008 */ 	lb	$a1,0x8($s1)
/*    1e514:	86270000 */ 	lh	$a3,0x0($s1)
/*    1e518:	27b800a8 */ 	addiu	$t8,$sp,0xa8
/*    1e51c:	27a800fc */ 	addiu	$t0,$sp,0xfc
/*    1e520:	27a9009c */ 	addiu	$t1,$sp,0x9c
/*    1e524:	afa9001c */ 	sw	$t1,0x1c($sp)
/*    1e528:	afa80018 */ 	sw	$t0,0x18($sp)
/*    1e52c:	afb80014 */ 	sw	$t8,0x14($sp)
/*    1e530:	afb00010 */ 	sw	$s0,0x10($sp)
/*    1e534:	8fa40118 */ 	lw	$a0,0x118($sp)
/*    1e538:	0c009014 */ 	jal	anim00024050
/*    1e53c:	8fa60114 */ 	lw	$a2,0x114($sp)
/*    1e540:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e544:	c7aa0100 */ 	lwc1	$f10,0x100($sp)
/*    1e548:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e54c:	46141202 */ 	mul.s	$f8,$f2,$f20
/*    1e550:	00000000 */ 	nop
/*    1e554:	46145402 */ 	mul.s	$f16,$f10,$f20
/*    1e558:	00000000 */ 	nop
/*    1e55c:	46146482 */ 	mul.s	$f18,$f12,$f20
/*    1e560:	e7a800e0 */ 	swc1	$f8,0xe0($sp)
/*    1e564:	e7b000e4 */ 	swc1	$f16,0xe4($sp)
/*    1e568:	e7b200e8 */ 	swc1	$f18,0xe8($sp)
.L0001e56c:
/*    1e56c:	12800004 */ 	beqz	$s4,.L0001e580
/*    1e570:	8fb300ec */ 	lw	$s3,0xec($sp)
/*    1e574:	8fb300ec */ 	lw	$s3,0xec($sp)
/*    1e578:	10000002 */ 	b	.L0001e584
/*    1e57c:	26730001 */ 	addiu	$s3,$s3,0x1
.L0001e580:
/*    1e580:	2673ffff */ 	addiu	$s3,$s3,-1
.L0001e584:
/*    1e584:	02602025 */ 	or	$a0,$s3,$zero
/*    1e588:	86250000 */ 	lh	$a1,0x0($s1)
/*    1e58c:	0c0074c8 */ 	jal	modelConstrainOrWrapAnimFrame
/*    1e590:	8e260018 */ 	lw	$a2,0x18($s1)
/*    1e594:	00408025 */ 	or	$s0,$v0,$zero
/*    1e598:	a6220016 */ 	sh	$v0,0x16($s1)
/*    1e59c:	0c008f4e */ 	jal	anim00023d38
/*    1e5a0:	86240000 */ 	lh	$a0,0x0($s1)
/*    1e5a4:	86240000 */ 	lh	$a0,0x0($s1)
/*    1e5a8:	0c008eac */ 	jal	anim00023ab0
/*    1e5ac:	02002825 */ 	or	$a1,$s0,$zero
/*    1e5b0:	0c008f43 */ 	jal	anim00023d0c
/*    1e5b4:	305000ff */ 	andi	$s0,$v0,0xff
/*    1e5b8:	82250008 */ 	lb	$a1,0x8($s1)
/*    1e5bc:	86270000 */ 	lh	$a3,0x0($s1)
/*    1e5c0:	27ad00a8 */ 	addiu	$t5,$sp,0xa8
/*    1e5c4:	27ac00fc */ 	addiu	$t4,$sp,0xfc
/*    1e5c8:	27ab009c */ 	addiu	$t3,$sp,0x9c
/*    1e5cc:	afab001c */ 	sw	$t3,0x1c($sp)
/*    1e5d0:	afac0018 */ 	sw	$t4,0x18($sp)
/*    1e5d4:	afad0014 */ 	sw	$t5,0x14($sp)
/*    1e5d8:	afb00010 */ 	sw	$s0,0x10($sp)
/*    1e5dc:	8fa40118 */ 	lw	$a0,0x118($sp)
/*    1e5e0:	0c009014 */ 	jal	anim00024050
/*    1e5e4:	8fa60114 */ 	lw	$a2,0x114($sp)
/*    1e5e8:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e5ec:	c7a60100 */ 	lwc1	$f6,0x100($sp)
/*    1e5f0:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e5f4:	46141102 */ 	mul.s	$f4,$f2,$f20
/*    1e5f8:	240a0001 */ 	addiu	$t2,$zero,0x1
/*    1e5fc:	afaa00c8 */ 	sw	$t2,0xc8($sp)
/*    1e600:	46143202 */ 	mul.s	$f8,$f6,$f20
/*    1e604:	00000000 */ 	nop
/*    1e608:	46146282 */ 	mul.s	$f10,$f12,$f20
/*    1e60c:	e7a400d0 */ 	swc1	$f4,0xd0($sp)
/*    1e610:	e7a800d4 */ 	swc1	$f8,0xd4($sp)
/*    1e614:	e7aa00d8 */ 	swc1	$f10,0xd8($sp)
.L0001e618:
/*    1e618:	10000147 */ 	b	.L0001eb38
/*    1e61c:	8fa700ec */ 	lw	$a3,0xec($sp)
.L0001e620:
/*    1e620:	c4384450 */ 	lwc1	$f24,%lo(var70054450)($at)
.L0001e624:
/*    1e624:	12800006 */ 	beqz	$s4,.L0001e640
/*    1e628:	0267082a */ 	slt	$at,$s3,$a3
/*    1e62c:	00f3082a */ 	slt	$at,$a3,$s3
/*    1e630:	50200006 */ 	beqzl	$at,.L0001e64c
/*    1e634:	86250000 */ 	lh	$a1,0x0($s1)
/*    1e638:	10000140 */ 	b	.L0001eb3c
/*    1e63c:	c7a400e0 */ 	lwc1	$f4,0xe0($sp)
.L0001e640:
/*    1e640:	5420013e */ 	bnezl	$at,.L0001eb3c
/*    1e644:	c7a400e0 */ 	lwc1	$f4,0xe0($sp)
/*    1e648:	86250000 */ 	lh	$a1,0x0($s1)
.L0001e64c:
/*    1e64c:	8e260018 */ 	lw	$a2,0x18($s1)
/*    1e650:	afa700ec */ 	sw	$a3,0xec($sp)
/*    1e654:	0c0074c8 */ 	jal	modelConstrainOrWrapAnimFrame
/*    1e658:	02602025 */ 	or	$a0,$s3,$zero
/*    1e65c:	a6220014 */ 	sh	$v0,0x14($s1)
/*    1e660:	8fae00c8 */ 	lw	$t6,0xc8($sp)
/*    1e664:	00408025 */ 	or	$s0,$v0,$zero
/*    1e668:	8fa40118 */ 	lw	$a0,0x118($sp)
/*    1e66c:	11c0000e */ 	beqz	$t6,.L0001e6a8
/*    1e670:	27af00fc */ 	addiu	$t7,$sp,0xfc
/*    1e674:	c7b000d0 */ 	lwc1	$f16,0xd0($sp)
/*    1e678:	c7b200d4 */ 	lwc1	$f18,0xd4($sp)
/*    1e67c:	c7a400d8 */ 	lwc1	$f4,0xd8($sp)
/*    1e680:	e7b000e0 */ 	swc1	$f16,0xe0($sp)
/*    1e684:	e7b200e4 */ 	swc1	$f18,0xe4($sp)
/*    1e688:	e7a400e8 */ 	swc1	$f4,0xe8($sp)
/*    1e68c:	c6460018 */ 	lwc1	$f6,0x18($s2)
/*    1e690:	4606d032 */ 	c.eq.s	$f26,$f6
/*    1e694:	00000000 */ 	nop
/*    1e698:	4500004f */ 	bc1f	.L0001e7d8
/*    1e69c:	00000000 */ 	nop
/*    1e6a0:	1000004d */ 	b	.L0001e7d8
/*    1e6a4:	c7be00cc */ 	lwc1	$f30,0xcc($sp)
.L0001e6a8:
/*    1e6a8:	82250008 */ 	lb	$a1,0x8($s1)
/*    1e6ac:	86270000 */ 	lh	$a3,0x0($s1)
/*    1e6b0:	afaf0014 */ 	sw	$t7,0x14($sp)
/*    1e6b4:	afb00010 */ 	sw	$s0,0x10($sp)
/*    1e6b8:	8239000b */ 	lb	$t9,0xb($s1)
/*    1e6bc:	8fa60114 */ 	lw	$a2,0x114($sp)
/*    1e6c0:	0c0092d9 */ 	jal	anim00024b64
/*    1e6c4:	afb90018 */ 	sw	$t9,0x18($sp)
/*    1e6c8:	3c013f80 */ 	lui	$at,0x3f80
/*    1e6cc:	44815000 */ 	mtc1	$at,$f10
/*    1e6d0:	c7a80110 */ 	lwc1	$f8,0x110($sp)
/*    1e6d4:	46000586 */ 	mov.s	$f22,$f0
/*    1e6d8:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e6dc:	460a4032 */ 	c.eq.s	$f8,$f10
/*    1e6e0:	c7b00100 */ 	lwc1	$f16,0x100($sp)
/*    1e6e4:	4503000a */ 	bc1tl	.L0001e710
/*    1e6e8:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e6ec:	46081082 */ 	mul.s	$f2,$f2,$f8
/*    1e6f0:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e6f4:	46088482 */ 	mul.s	$f18,$f16,$f8
/*    1e6f8:	00000000 */ 	nop
/*    1e6fc:	46086302 */ 	mul.s	$f12,$f12,$f8
/*    1e700:	e7a200fc */ 	swc1	$f2,0xfc($sp)
/*    1e704:	e7b20100 */ 	swc1	$f18,0x100($sp)
/*    1e708:	e7ac0104 */ 	swc1	$f12,0x104($sp)
/*    1e70c:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
.L0001e710:
/*    1e710:	16800008 */ 	bnez	$s4,.L0001e734
/*    1e714:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e718:	4600d03c */ 	c.lt.s	$f26,$f0
/*    1e71c:	46001087 */ 	neg.s	$f2,$f2
/*    1e720:	46006307 */ 	neg.s	$f12,$f12
/*    1e724:	e7a200fc */ 	swc1	$f2,0xfc($sp)
/*    1e728:	45000002 */ 	bc1f	.L0001e734
/*    1e72c:	e7ac0104 */ 	swc1	$f12,0x104($sp)
/*    1e730:	4600c581 */ 	sub.s	$f22,$f24,$f0
.L0001e734:
/*    1e734:	8238000b */ 	lb	$t8,0xb($s1)
/*    1e738:	13000005 */ 	beqz	$t8,.L0001e750
/*    1e73c:	00000000 */ 	nop
/*    1e740:	c644000c */ 	lwc1	$f4,0xc($s2)
/*    1e744:	c6460004 */ 	lwc1	$f6,0x4($s2)
/*    1e748:	46062281 */ 	sub.s	$f10,$f4,$f6
/*    1e74c:	e7aa0100 */ 	swc1	$f10,0x100($sp)
.L0001e750:
/*    1e750:	0c0068f4 */ 	jal	cosf
/*    1e754:	c64c0014 */ 	lwc1	$f12,0x14($s2)
/*    1e758:	46000506 */ 	mov.s	$f20,$f0
/*    1e75c:	0c0068f7 */ 	jal	sinf
/*    1e760:	c64c0014 */ 	lwc1	$f12,0x14($s2)
/*    1e764:	c7b000fc */ 	lwc1	$f16,0xfc($sp)
/*    1e768:	c7a80104 */ 	lwc1	$f8,0x104($sp)
/*    1e76c:	c7aa00e0 */ 	lwc1	$f10,0xe0($sp)
/*    1e770:	46148482 */ 	mul.s	$f18,$f16,$f20
/*    1e774:	00000000 */ 	nop
/*    1e778:	46004102 */ 	mul.s	$f4,$f8,$f0
/*    1e77c:	46049180 */ 	add.s	$f6,$f18,$f4
/*    1e780:	c7a40100 */ 	lwc1	$f4,0x100($sp)
/*    1e784:	46065480 */ 	add.s	$f18,$f10,$f6
/*    1e788:	e7a400e4 */ 	swc1	$f4,0xe4($sp)
/*    1e78c:	46008287 */ 	neg.s	$f10,$f16
/*    1e790:	e7b200e0 */ 	swc1	$f18,0xe0($sp)
/*    1e794:	46005182 */ 	mul.s	$f6,$f10,$f0
/*    1e798:	c7b000e8 */ 	lwc1	$f16,0xe8($sp)
/*    1e79c:	46144482 */ 	mul.s	$f18,$f8,$f20
/*    1e7a0:	46123100 */ 	add.s	$f4,$f6,$f18
/*    1e7a4:	46048280 */ 	add.s	$f10,$f16,$f4
/*    1e7a8:	e7aa00e8 */ 	swc1	$f10,0xe8($sp)
/*    1e7ac:	c6480018 */ 	lwc1	$f8,0x18($s2)
/*    1e7b0:	4608d032 */ 	c.eq.s	$f26,$f8
/*    1e7b4:	00000000 */ 	nop
/*    1e7b8:	45000007 */ 	bc1f	.L0001e7d8
/*    1e7bc:	00000000 */ 	nop
/*    1e7c0:	4616f780 */ 	add.s	$f30,$f30,$f22
/*    1e7c4:	461ec03e */ 	c.le.s	$f24,$f30
/*    1e7c8:	00000000 */ 	nop
/*    1e7cc:	45000002 */ 	bc1f	.L0001e7d8
/*    1e7d0:	00000000 */ 	nop
/*    1e7d4:	4618f781 */ 	sub.s	$f30,$f30,$f24
.L0001e7d8:
/*    1e7d8:	52800004 */ 	beqzl	$s4,.L0001e7ec
/*    1e7dc:	2673ffff */ 	addiu	$s3,$s3,-1
/*    1e7e0:	10000002 */ 	b	.L0001e7ec
/*    1e7e4:	26730001 */ 	addiu	$s3,$s3,0x1
/*    1e7e8:	2673ffff */ 	addiu	$s3,$s3,-1
.L0001e7ec:
/*    1e7ec:	02602025 */ 	or	$a0,$s3,$zero
/*    1e7f0:	86250000 */ 	lh	$a1,0x0($s1)
/*    1e7f4:	0c0074c8 */ 	jal	modelConstrainOrWrapAnimFrame
/*    1e7f8:	8e260018 */ 	lw	$a2,0x18($s1)
/*    1e7fc:	a6220016 */ 	sh	$v0,0x16($s1)
/*    1e800:	86290016 */ 	lh	$t1,0x16($s1)
/*    1e804:	86280014 */ 	lh	$t0,0x14($s1)
/*    1e808:	8fa60114 */ 	lw	$a2,0x114($sp)
/*    1e80c:	8fa40118 */ 	lw	$a0,0x118($sp)
/*    1e810:	110900c7 */ 	beq	$t0,$t1,.L0001eb30
/*    1e814:	27ad00fc */ 	addiu	$t5,$sp,0xfc
/*    1e818:	82250008 */ 	lb	$a1,0x8($s1)
/*    1e81c:	86270000 */ 	lh	$a3,0x0($s1)
/*    1e820:	afad0014 */ 	sw	$t5,0x14($sp)
/*    1e824:	afa20010 */ 	sw	$v0,0x10($sp)
/*    1e828:	822c000b */ 	lb	$t4,0xb($s1)
/*    1e82c:	240b0001 */ 	addiu	$t3,$zero,0x1
/*    1e830:	afab00c8 */ 	sw	$t3,0xc8($sp)
/*    1e834:	0c0092d9 */ 	jal	anim00024b64
/*    1e838:	afac0018 */ 	sw	$t4,0x18($sp)
/*    1e83c:	3c013f80 */ 	lui	$at,0x3f80
/*    1e840:	44819000 */ 	mtc1	$at,$f18
/*    1e844:	c7a60110 */ 	lwc1	$f6,0x110($sp)
/*    1e848:	46000586 */ 	mov.s	$f22,$f0
/*    1e84c:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e850:	46123032 */ 	c.eq.s	$f6,$f18
/*    1e854:	c7b00100 */ 	lwc1	$f16,0x100($sp)
/*    1e858:	4503000a */ 	bc1tl	.L0001e884
/*    1e85c:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e860:	46061082 */ 	mul.s	$f2,$f2,$f6
/*    1e864:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e868:	46068102 */ 	mul.s	$f4,$f16,$f6
/*    1e86c:	00000000 */ 	nop
/*    1e870:	46066302 */ 	mul.s	$f12,$f12,$f6
/*    1e874:	e7a200fc */ 	swc1	$f2,0xfc($sp)
/*    1e878:	e7a40100 */ 	swc1	$f4,0x100($sp)
/*    1e87c:	e7ac0104 */ 	swc1	$f12,0x104($sp)
/*    1e880:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
.L0001e884:
/*    1e884:	16800008 */ 	bnez	$s4,.L0001e8a8
/*    1e888:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e88c:	4600d03c */ 	c.lt.s	$f26,$f0
/*    1e890:	46001087 */ 	neg.s	$f2,$f2
/*    1e894:	46006307 */ 	neg.s	$f12,$f12
/*    1e898:	e7a200fc */ 	swc1	$f2,0xfc($sp)
/*    1e89c:	45000002 */ 	bc1f	.L0001e8a8
/*    1e8a0:	e7ac0104 */ 	swc1	$f12,0x104($sp)
/*    1e8a4:	4600c581 */ 	sub.s	$f22,$f24,$f0
.L0001e8a8:
/*    1e8a8:	822a000b */ 	lb	$t2,0xb($s1)
/*    1e8ac:	11400003 */ 	beqz	$t2,.L0001e8bc
/*    1e8b0:	00000000 */ 	nop
/*    1e8b4:	c64a0038 */ 	lwc1	$f10,0x38($s2)
/*    1e8b8:	e7aa0100 */ 	swc1	$f10,0x100($sp)
.L0001e8bc:
/*    1e8bc:	0c0068f4 */ 	jal	cosf
/*    1e8c0:	c64c0030 */ 	lwc1	$f12,0x30($s2)
/*    1e8c4:	46000506 */ 	mov.s	$f20,$f0
/*    1e8c8:	0c0068f7 */ 	jal	sinf
/*    1e8cc:	c64c0030 */ 	lwc1	$f12,0x30($s2)
/*    1e8d0:	3c0e8006 */ 	lui	$t6,%hi(var8005efdc)
/*    1e8d4:	8dceefdc */ 	lw	$t6,%lo(var8005efdc)($t6)
/*    1e8d8:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e8dc:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e8e0:	11c00056 */ 	beqz	$t6,.L0001ea3c
/*    1e8e4:	00000000 */ 	nop
/*    1e8e8:	862f0002 */ 	lh	$t7,0x2($s1)
/*    1e8ec:	11e00053 */ 	beqz	$t7,.L0001ea3c
/*    1e8f0:	00000000 */ 	nop
/*    1e8f4:	c7ac0104 */ 	lwc1	$f12,0x104($sp)
/*    1e8f8:	c7a200fc */ 	lwc1	$f2,0xfc($sp)
/*    1e8fc:	461cd03c */ 	c.lt.s	$f26,$f28
/*    1e900:	46006202 */ 	mul.s	$f8,$f12,$f0
/*    1e904:	46001107 */ 	neg.s	$f4,$f2
/*    1e908:	3c013f00 */ 	lui	$at,0x3f00
/*    1e90c:	46141482 */ 	mul.s	$f18,$f2,$f20
/*    1e910:	00000000 */ 	nop
/*    1e914:	46146402 */ 	mul.s	$f16,$f12,$f20
/*    1e918:	00000000 */ 	nop
/*    1e91c:	46002282 */ 	mul.s	$f10,$f4,$f0
/*    1e920:	46124180 */ 	add.s	$f6,$f8,$f18
/*    1e924:	e7a600d0 */ 	swc1	$f6,0xd0($sp)
/*    1e928:	460a8200 */ 	add.s	$f8,$f16,$f10
/*    1e92c:	45000028 */ 	bc1f	.L0001e9d0
/*    1e930:	e7a800d8 */ 	swc1	$f8,0xd8($sp)
/*    1e934:	c6260058 */ 	lwc1	$f6,0x58($s1)
/*    1e938:	c6320074 */ 	lwc1	$f18,0x74($s1)
/*    1e93c:	c6220054 */ 	lwc1	$f2,0x54($s1)
/*    1e940:	4606e102 */ 	mul.s	$f4,$f28,$f6
/*    1e944:	46049403 */ 	div.s	$f16,$f18,$f4
/*    1e948:	46101001 */ 	sub.s	$f0,$f2,$f16
/*    1e94c:	461a003c */ 	c.lt.s	$f0,$f26
/*    1e950:	00000000 */ 	nop
/*    1e954:	45020003 */ 	bc1fl	.L0001e964
/*    1e958:	46001280 */ 	add.s	$f10,$f2,$f0
/*    1e95c:	4600d006 */ 	mov.s	$f0,$f26
/*    1e960:	46001280 */ 	add.s	$f10,$f2,$f0
.L0001e964:
/*    1e964:	c6460040 */ 	lwc1	$f6,0x40($s2)
/*    1e968:	c652004c */ 	lwc1	$f18,0x4c($s2)
/*    1e96c:	44814000 */ 	mtc1	$at,$f8
/*    1e970:	c7b000bc */ 	lwc1	$f16,0xbc($sp)
/*    1e974:	46123101 */ 	sub.s	$f4,$f6,$f18
/*    1e978:	46085002 */ 	mul.s	$f0,$f10,$f8
/*    1e97c:	00000000 */ 	nop
/*    1e980:	46102282 */ 	mul.s	$f10,$f4,$f16
/*    1e984:	461c5203 */ 	div.s	$f8,$f10,$f28
/*    1e988:	e7a80090 */ 	swc1	$f8,0x90($sp)
/*    1e98c:	c6520054 */ 	lwc1	$f18,0x54($s2)
/*    1e990:	c6460048 */ 	lwc1	$f6,0x48($s2)
/*    1e994:	46123101 */ 	sub.s	$f4,$f6,$f18
/*    1e998:	c7b200d0 */ 	lwc1	$f18,0xd0($sp)
/*    1e99c:	46102282 */ 	mul.s	$f10,$f4,$f16
/*    1e9a0:	46124101 */ 	sub.s	$f4,$f8,$f18
/*    1e9a4:	c7a800d8 */ 	lwc1	$f8,0xd8($sp)
/*    1e9a8:	46002402 */ 	mul.s	$f16,$f4,$f0
/*    1e9ac:	461c5183 */ 	div.s	$f6,$f10,$f28
/*    1e9b0:	46109280 */ 	add.s	$f10,$f18,$f16
/*    1e9b4:	e7aa00d0 */ 	swc1	$f10,0xd0($sp)
/*    1e9b8:	46083101 */ 	sub.s	$f4,$f6,$f8
/*    1e9bc:	e7a60098 */ 	swc1	$f6,0x98($sp)
/*    1e9c0:	46002482 */ 	mul.s	$f18,$f4,$f0
/*    1e9c4:	46124400 */ 	add.s	$f16,$f8,$f18
/*    1e9c8:	10000011 */ 	b	.L0001ea10
/*    1e9cc:	e7b000d8 */ 	swc1	$f16,0xd8($sp)
.L0001e9d0:
/*    1e9d0:	c64a0040 */ 	lwc1	$f10,0x40($s2)
/*    1e9d4:	c646004c */ 	lwc1	$f6,0x4c($s2)
/*    1e9d8:	c6280054 */ 	lwc1	$f8,0x54($s1)
/*    1e9dc:	c7b000d0 */ 	lwc1	$f16,0xd0($sp)
/*    1e9e0:	46065101 */ 	sub.s	$f4,$f10,$f6
/*    1e9e4:	46082482 */ 	mul.s	$f18,$f4,$f8
/*    1e9e8:	46128280 */ 	add.s	$f10,$f16,$f18
/*    1e9ec:	e7aa00d0 */ 	swc1	$f10,0xd0($sp)
/*    1e9f0:	c6440054 */ 	lwc1	$f4,0x54($s2)
/*    1e9f4:	c6460048 */ 	lwc1	$f6,0x48($s2)
/*    1e9f8:	c6300054 */ 	lwc1	$f16,0x54($s1)
/*    1e9fc:	c7aa00d8 */ 	lwc1	$f10,0xd8($sp)
/*    1ea00:	46043201 */ 	sub.s	$f8,$f6,$f4
/*    1ea04:	46104482 */ 	mul.s	$f18,$f8,$f16
/*    1ea08:	46125180 */ 	add.s	$f6,$f10,$f18
/*    1ea0c:	e7a600d8 */ 	swc1	$f6,0xd8($sp)
.L0001ea10:
/*    1ea10:	c7a400d0 */ 	lwc1	$f4,0xd0($sp)
/*    1ea14:	c7a800e0 */ 	lwc1	$f8,0xe0($sp)
/*    1ea18:	c7aa00d8 */ 	lwc1	$f10,0xd8($sp)
/*    1ea1c:	c7b200e8 */ 	lwc1	$f18,0xe8($sp)
/*    1ea20:	46082400 */ 	add.s	$f16,$f4,$f8
/*    1ea24:	c7a40100 */ 	lwc1	$f4,0x100($sp)
/*    1ea28:	46125180 */ 	add.s	$f6,$f10,$f18
/*    1ea2c:	e7b000d0 */ 	swc1	$f16,0xd0($sp)
/*    1ea30:	e7a400d4 */ 	swc1	$f4,0xd4($sp)
/*    1ea34:	1000000f */ 	b	.L0001ea74
/*    1ea38:	e7a600d8 */ 	swc1	$f6,0xd8($sp)
.L0001ea3c:
/*    1ea3c:	46141402 */ 	mul.s	$f16,$f2,$f20
/*    1ea40:	c7a800e0 */ 	lwc1	$f8,0xe0($sp)
/*    1ea44:	c7a40100 */ 	lwc1	$f4,0x100($sp)
/*    1ea48:	46006482 */ 	mul.s	$f18,$f12,$f0
/*    1ea4c:	e7a400d4 */ 	swc1	$f4,0xd4($sp)
/*    1ea50:	46104280 */ 	add.s	$f10,$f8,$f16
/*    1ea54:	46001402 */ 	mul.s	$f16,$f2,$f0
/*    1ea58:	c7a800e8 */ 	lwc1	$f8,0xe8($sp)
/*    1ea5c:	460a9180 */ 	add.s	$f6,$f18,$f10
/*    1ea60:	46146282 */ 	mul.s	$f10,$f12,$f20
/*    1ea64:	46104481 */ 	sub.s	$f18,$f8,$f16
/*    1ea68:	e7a600d0 */ 	swc1	$f6,0xd0($sp)
/*    1ea6c:	46125180 */ 	add.s	$f6,$f10,$f18
/*    1ea70:	e7a600d8 */ 	swc1	$f6,0xd8($sp)
.L0001ea74:
/*    1ea74:	c640005c */ 	lwc1	$f0,0x5c($s2)
/*    1ea78:	4600d03c */ 	c.lt.s	$f26,$f0
/*    1ea7c:	00000000 */ 	nop
/*    1ea80:	45020020 */ 	bc1fl	.L0001eb04
/*    1ea84:	c6520018 */ 	lwc1	$f18,0x18($s2)
/*    1ea88:	461cd03c */ 	c.lt.s	$f26,$f28
/*    1ea8c:	3c013f80 */ 	lui	$at,0x3f80
/*    1ea90:	4502001c */ 	bc1fl	.L0001eb04
/*    1ea94:	c6520018 */ 	lwc1	$f18,0x18($s2)
/*    1ea98:	44812000 */ 	mtc1	$at,$f4
/*    1ea9c:	00000000 */ 	nop
/*    1eaa0:	461c2083 */ 	div.s	$f2,$f4,$f28
/*    1eaa4:	4602003c */ 	c.lt.s	$f0,$f2
/*    1eaa8:	46001306 */ 	mov.s	$f12,$f2
/*    1eaac:	45020005 */ 	bc1fl	.L0001eac4
/*    1eab0:	46020201 */ 	sub.s	$f8,$f0,$f2
/*    1eab4:	46000306 */ 	mov.s	$f12,$f0
/*    1eab8:	10000003 */ 	b	.L0001eac8
/*    1eabc:	e65a005c */ 	swc1	$f26,0x5c($s2)
/*    1eac0:	46020201 */ 	sub.s	$f8,$f0,$f2
.L0001eac4:
/*    1eac4:	e648005c */ 	swc1	$f8,0x5c($s2)
.L0001eac8:
/*    1eac8:	c6500058 */ 	lwc1	$f16,0x58($s2)
/*    1eacc:	460c8282 */ 	mul.s	$f10,$f16,$f12
/*    1ead0:	460ab580 */ 	add.s	$f22,$f22,$f10
/*    1ead4:	461ab03c */ 	c.lt.s	$f22,$f26
/*    1ead8:	00000000 */ 	nop
/*    1eadc:	45020004 */ 	bc1fl	.L0001eaf0
/*    1eae0:	4616c03e */ 	c.le.s	$f24,$f22
/*    1eae4:	10000006 */ 	b	.L0001eb00
/*    1eae8:	4618b580 */ 	add.s	$f22,$f22,$f24
/*    1eaec:	4616c03e */ 	c.le.s	$f24,$f22
.L0001eaf0:
/*    1eaf0:	00000000 */ 	nop
/*    1eaf4:	45020003 */ 	bc1fl	.L0001eb04
/*    1eaf8:	c6520018 */ 	lwc1	$f18,0x18($s2)
/*    1eafc:	4618b581 */ 	sub.s	$f22,$f22,$f24
.L0001eb00:
/*    1eb00:	c6520018 */ 	lwc1	$f18,0x18($s2)
.L0001eb04:
/*    1eb04:	4612d032 */ 	c.eq.s	$f26,$f18
/*    1eb08:	00000000 */ 	nop
/*    1eb0c:	45000008 */ 	bc1f	.L0001eb30
/*    1eb10:	00000000 */ 	nop
/*    1eb14:	4616f000 */ 	add.s	$f0,$f30,$f22
/*    1eb18:	4600c03e */ 	c.le.s	$f24,$f0
/*    1eb1c:	e7a000cc */ 	swc1	$f0,0xcc($sp)
/*    1eb20:	45000003 */ 	bc1f	.L0001eb30
/*    1eb24:	00000000 */ 	nop
/*    1eb28:	46180181 */ 	sub.s	$f6,$f0,$f24
/*    1eb2c:	e7a600cc */ 	swc1	$f6,0xcc($sp)
.L0001eb30:
/*    1eb30:	1000febc */ 	b	.L0001e624
/*    1eb34:	8fa700ec */ 	lw	$a3,0xec($sp)
.L0001eb38:
/*    1eb38:	c7a400e0 */ 	lwc1	$f4,0xe0($sp)
.L0001eb3c:
/*    1eb3c:	e6440034 */ 	swc1	$f4,0x34($s2)
/*    1eb40:	c7a800e4 */ 	lwc1	$f8,0xe4($sp)
/*    1eb44:	e6480038 */ 	swc1	$f8,0x38($s2)
/*    1eb48:	c7b000e8 */ 	lwc1	$f16,0xe8($sp)
/*    1eb4c:	e65e0030 */ 	swc1	$f30,0x30($s2)
/*    1eb50:	e650003c */ 	swc1	$f16,0x3c($s2)
/*    1eb54:	c7aa00d0 */ 	lwc1	$f10,0xd0($sp)
/*    1eb58:	e64a0024 */ 	swc1	$f10,0x24($s2)
/*    1eb5c:	c7b200d4 */ 	lwc1	$f18,0xd4($sp)
/*    1eb60:	e6520028 */ 	swc1	$f18,0x28($s2)
/*    1eb64:	c7a600d8 */ 	lwc1	$f6,0xd8($sp)
/*    1eb68:	e646002c */ 	swc1	$f6,0x2c($s2)
/*    1eb6c:	c7a400cc */ 	lwc1	$f4,0xcc($sp)
/*    1eb70:	e6440020 */ 	swc1	$f4,0x20($s2)
/*    1eb74:	8fb900c8 */ 	lw	$t9,0xc8($sp)
/*    1eb78:	a2590001 */ 	sb	$t9,0x1($s2)
/*    1eb7c:	86220014 */ 	lh	$v0,0x14($s1)
/*    1eb80:	86230016 */ 	lh	$v1,0x16($s1)
/*    1eb84:	14620006 */ 	bne	$v1,$v0,.L0001eba0
/*    1eb88:	00000000 */ 	nop
/*    1eb8c:	44824000 */ 	mtc1	$v0,$f8
/*    1eb90:	e63a0010 */ 	swc1	$f26,0x10($s1)
/*    1eb94:	46804420 */ 	cvt.s.w	$f16,$f8
/*    1eb98:	10000019 */ 	b	.L0001ec00
/*    1eb9c:	e630000c */ 	swc1	$f16,0xc($s1)
.L0001eba0:
/*    1eba0:	5280000c */ 	beqzl	$s4,.L0001ebd4
/*    1eba4:	44879000 */ 	mtc1	$a3,$f18
/*    1eba8:	44879000 */ 	mtc1	$a3,$f18
/*    1ebac:	44822000 */ 	mtc1	$v0,$f4
/*    1ebb0:	c7aa0138 */ 	lwc1	$f10,0x138($sp)
/*    1ebb4:	468091a0 */ 	cvt.s.w	$f6,$f18
/*    1ebb8:	46802220 */ 	cvt.s.w	$f8,$f4
/*    1ebbc:	46065001 */ 	sub.s	$f0,$f10,$f6
/*    1ebc0:	46004400 */ 	add.s	$f16,$f8,$f0
/*    1ebc4:	e6200010 */ 	swc1	$f0,0x10($s1)
/*    1ebc8:	1000000d */ 	b	.L0001ec00
/*    1ebcc:	e630000c */ 	swc1	$f16,0xc($s1)
/*    1ebd0:	44879000 */ 	mtc1	$a3,$f18
.L0001ebd4:
/*    1ebd4:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*    1ebd8:	44832000 */ 	mtc1	$v1,$f4
/*    1ebdc:	468092a0 */ 	cvt.s.w	$f10,$f18
/*    1ebe0:	3c013f80 */ 	lui	$at,0x3f80
/*    1ebe4:	44818000 */ 	mtc1	$at,$f16
/*    1ebe8:	46802220 */ 	cvt.s.w	$f8,$f4
/*    1ebec:	46065001 */ 	sub.s	$f0,$f10,$f6
/*    1ebf0:	46008481 */ 	sub.s	$f18,$f16,$f0
/*    1ebf4:	e6200010 */ 	swc1	$f0,0x10($s1)
/*    1ebf8:	46124280 */ 	add.s	$f10,$f8,$f18
/*    1ebfc:	e62a000c */ 	swc1	$f10,0xc($s1)
.L0001ec00:
/*    1ec00:	86380002 */ 	lh	$t8,0x2($s1)
/*    1ec04:	13000066 */ 	beqz	$t8,.L0001eda0
/*    1ec08:	00000000 */ 	nop
/*    1ec0c:	86290000 */ 	lh	$t1,0x0($s1)
/*    1ec10:	3c088006 */ 	lui	$t0,%hi(g_Anims)
/*    1ec14:	8d08f00c */ 	lw	$t0,%lo(g_Anims)($t0)
/*    1ec18:	00096880 */ 	sll	$t5,$t1,0x2
/*    1ec1c:	01a96823 */ 	subu	$t5,$t5,$t1
/*    1ec20:	000d6880 */ 	sll	$t5,$t5,0x2
/*    1ec24:	010d6021 */ 	addu	$t4,$t0,$t5
/*    1ec28:	918b000b */ 	lbu	$t3,0xb($t4)
/*    1ec2c:	316a0002 */ 	andi	$t2,$t3,0x2
/*    1ec30:	1540005b */ 	bnez	$t2,.L0001eda0
/*    1ec34:	00000000 */ 	nop
/*    1ec38:	0fc25e42 */ 	jal	floor
/*    1ec3c:	c7ac013c */ 	lwc1	$f12,0x13c($sp)
/*    1ec40:	c7b40140 */ 	lwc1	$f20,0x140($sp)
/*    1ec44:	00408025 */ 	or	$s0,$v0,$zero
/*    1ec48:	0fc25e42 */ 	jal	floor
/*    1ec4c:	4600a306 */ 	mov.s	$f12,$f20
/*    1ec50:	12800004 */ 	beqz	$s4,.L0001ec64
/*    1ec54:	00409825 */ 	or	$s3,$v0,$zero
/*    1ec58:	0202082a */ 	slt	$at,$s0,$v0
/*    1ec5c:	54200006 */ 	bnezl	$at,.L0001ec78
/*    1ec60:	824e0002 */ 	lb	$t6,0x2($s2)
.L0001ec64:
/*    1ec64:	16800032 */ 	bnez	$s4,.L0001ed30
/*    1ec68:	0050082a */ 	slt	$at,$v0,$s0
/*    1ec6c:	50200031 */ 	beqzl	$at,.L0001ed34
/*    1ec70:	3c013f80 */ 	lui	$at,0x3f80
/*    1ec74:	824e0002 */ 	lb	$t6,0x2($s2)
.L0001ec78:
/*    1ec78:	02602025 */ 	or	$a0,$s3,$zero
/*    1ec7c:	51c00005 */ 	beqzl	$t6,.L0001ec94
/*    1ec80:	c6440038 */ 	lwc1	$f4,0x38($s2)
/*    1ec84:	c6460044 */ 	lwc1	$f6,0x44($s2)
/*    1ec88:	10000003 */ 	b	.L0001ec98
/*    1ec8c:	e6460050 */ 	swc1	$f6,0x50($s2)
/*    1ec90:	c6440038 */ 	lwc1	$f4,0x38($s2)
.L0001ec94:
/*    1ec94:	e6440050 */ 	swc1	$f4,0x50($s2)
.L0001ec98:
/*    1ec98:	86250002 */ 	lh	$a1,0x2($s1)
/*    1ec9c:	0c0074c8 */ 	jal	modelConstrainOrWrapAnimFrame
/*    1eca0:	8e26003c */ 	lw	$a2,0x3c($s1)
/*    1eca4:	a6220038 */ 	sh	$v0,0x38($s1)
/*    1eca8:	26640001 */ 	addiu	$a0,$s3,0x1
/*    1ecac:	86250002 */ 	lh	$a1,0x2($s1)
/*    1ecb0:	0c0074c8 */ 	jal	modelConstrainOrWrapAnimFrame
/*    1ecb4:	8e26003c */ 	lw	$a2,0x3c($s1)
/*    1ecb8:	a622003a */ 	sh	$v0,0x3a($s1)
/*    1ecbc:	82250009 */ 	lb	$a1,0x9($s1)
/*    1ecc0:	86270002 */ 	lh	$a3,0x2($s1)
/*    1ecc4:	27af00fc */ 	addiu	$t7,$sp,0xfc
/*    1ecc8:	afaf0014 */ 	sw	$t7,0x14($sp)
/*    1eccc:	afa20010 */ 	sw	$v0,0x10($sp)
/*    1ecd0:	8239000b */ 	lb	$t9,0xb($s1)
/*    1ecd4:	8fa60114 */ 	lw	$a2,0x114($sp)
/*    1ecd8:	8fa40118 */ 	lw	$a0,0x118($sp)
/*    1ecdc:	0c0092d9 */ 	jal	anim00024b64
/*    1ece0:	afb90018 */ 	sw	$t9,0x18($sp)
/*    1ece4:	3c013f80 */ 	lui	$at,0x3f80
/*    1ece8:	44816000 */ 	mtc1	$at,$f12
/*    1ecec:	c7a20110 */ 	lwc1	$f2,0x110($sp)
/*    1ecf0:	c7b00100 */ 	lwc1	$f16,0x100($sp)
/*    1ecf4:	460c1032 */ 	c.eq.s	$f2,$f12
/*    1ecf8:	00000000 */ 	nop
/*    1ecfc:	45030004 */ 	bc1tl	.L0001ed10
/*    1ed00:	8238000b */ 	lb	$t8,0xb($s1)
/*    1ed04:	46028202 */ 	mul.s	$f8,$f16,$f2
/*    1ed08:	e7a80100 */ 	swc1	$f8,0x100($sp)
/*    1ed0c:	8238000b */ 	lb	$t8,0xb($s1)
.L0001ed10:
/*    1ed10:	53000004 */ 	beqzl	$t8,.L0001ed24
/*    1ed14:	c7aa0100 */ 	lwc1	$f10,0x100($sp)
/*    1ed18:	c6520050 */ 	lwc1	$f18,0x50($s2)
/*    1ed1c:	e7b20100 */ 	swc1	$f18,0x100($sp)
/*    1ed20:	c7aa0100 */ 	lwc1	$f10,0x100($sp)
.L0001ed24:
/*    1ed24:	24090001 */ 	addiu	$t1,$zero,0x1
/*    1ed28:	a2490002 */ 	sb	$t1,0x2($s2)
/*    1ed2c:	e64a0044 */ 	swc1	$f10,0x44($s2)
.L0001ed30:
/*    1ed30:	3c013f80 */ 	lui	$at,0x3f80
.L0001ed34:
/*    1ed34:	44816000 */ 	mtc1	$at,$f12
/*    1ed38:	5280000d */ 	beqzl	$s4,.L0001ed70
/*    1ed3c:	44935000 */ 	mtc1	$s3,$f10
/*    1ed40:	44933000 */ 	mtc1	$s3,$f6
/*    1ed44:	86280038 */ 	lh	$t0,0x38($s1)
/*    1ed48:	46803120 */ 	cvt.s.w	$f4,$f6
/*    1ed4c:	44888000 */ 	mtc1	$t0,$f16
/*    1ed50:	00000000 */ 	nop
/*    1ed54:	46808220 */ 	cvt.s.w	$f8,$f16
/*    1ed58:	4604a001 */ 	sub.s	$f0,$f20,$f4
/*    1ed5c:	46004480 */ 	add.s	$f18,$f8,$f0
/*    1ed60:	e6200034 */ 	swc1	$f0,0x34($s1)
/*    1ed64:	1000001e */ 	b	.L0001ede0
/*    1ed68:	e6320030 */ 	swc1	$f18,0x30($s1)
/*    1ed6c:	44935000 */ 	mtc1	$s3,$f10
.L0001ed70:
/*    1ed70:	862d003a */ 	lh	$t5,0x3a($s1)
/*    1ed74:	468051a0 */ 	cvt.s.w	$f6,$f10
/*    1ed78:	448d8000 */ 	mtc1	$t5,$f16
/*    1ed7c:	00000000 */ 	nop
/*    1ed80:	46808220 */ 	cvt.s.w	$f8,$f16
/*    1ed84:	4606a101 */ 	sub.s	$f4,$f20,$f6
/*    1ed88:	46046001 */ 	sub.s	$f0,$f12,$f4
/*    1ed8c:	46006481 */ 	sub.s	$f18,$f12,$f0
/*    1ed90:	e6200034 */ 	swc1	$f0,0x34($s1)
/*    1ed94:	46124280 */ 	add.s	$f10,$f8,$f18
/*    1ed98:	10000011 */ 	b	.L0001ede0
/*    1ed9c:	e62a0030 */ 	swc1	$f10,0x30($s1)
.L0001eda0:
/*    1eda0:	1000000f */ 	b	.L0001ede0
/*    1eda4:	a2400002 */ 	sb	$zero,0x2($s2)
/*    1eda8:	c7b40140 */ 	lwc1	$f20,0x140($sp)
.L0001edac:
/*    1edac:	02002025 */ 	or	$a0,$s0,$zero
/*    1edb0:	8fa50138 */ 	lw	$a1,0x138($sp)
/*    1edb4:	4406a000 */ 	mfc1	$a2,$f20
/*    1edb8:	0c007853 */ 	jal	model0001e14c
/*    1edbc:	00000000 */ 	nop
/*    1edc0:	10000008 */ 	b	.L0001ede4
/*    1edc4:	8fbf006c */ 	lw	$ra,0x6c($sp)
/*    1edc8:	c7b40140 */ 	lwc1	$f20,0x140($sp)
.L0001edcc:
/*    1edcc:	02002025 */ 	or	$a0,$s0,$zero
/*    1edd0:	8fa50138 */ 	lw	$a1,0x138($sp)
/*    1edd4:	4406a000 */ 	mfc1	$a2,$f20
/*    1edd8:	0c007853 */ 	jal	model0001e14c
/*    1eddc:	00000000 */ 	nop
.L0001ede0:
/*    1ede0:	8fbf006c */ 	lw	$ra,0x6c($sp)
.L0001ede4:
/*    1ede4:	d7b40028 */ 	ldc1	$f20,0x28($sp)
/*    1ede8:	d7b60030 */ 	ldc1	$f22,0x30($sp)
/*    1edec:	d7b80038 */ 	ldc1	$f24,0x38($sp)
/*    1edf0:	d7ba0040 */ 	ldc1	$f26,0x40($sp)
/*    1edf4:	d7bc0048 */ 	ldc1	$f28,0x48($sp)
/*    1edf8:	d7be0050 */ 	ldc1	$f30,0x50($sp)
/*    1edfc:	8fb00058 */ 	lw	$s0,0x58($sp)
/*    1ee00:	8fb1005c */ 	lw	$s1,0x5c($sp)
/*    1ee04:	8fb20060 */ 	lw	$s2,0x60($sp)
/*    1ee08:	8fb30064 */ 	lw	$s3,0x64($sp)
/*    1ee0c:	8fb40068 */ 	lw	$s4,0x68($sp)
/*    1ee10:	03e00008 */ 	jr	$ra
/*    1ee14:	27bd0130 */ 	addiu	$sp,$sp,0x130
);

void model0001ee18(struct model *model, s32 lvupdate240, bool arg2)
{
	f32 frame;
	f32 frame2;
	f32 speed;
	f32 speed2;
	f32 startframe;
	f32 endframe;
	f32 realendframe;
	struct anim *anim = model->anim;

	if (anim && lvupdate240 > 0) {
		frame = anim->frame;
		frame2 = anim->frame2;

		for (; lvupdate240 > 0; lvupdate240--) {
			if (anim->timeplay > 0) {
				anim->elapseplay += 0.25f;

				if (anim->elapseplay < anim->timeplay) {
					anim->playspeed = anim->oldplay + (anim->newplay - anim->oldplay) * anim->elapseplay / anim->timeplay;
				} else {
					anim->timeplay = 0;
					anim->playspeed = anim->newplay;
				}
			}

			if (anim->timemerge > 0) {
				anim->elapsemerge += anim->playspeed * 0.25f;

				if (anim->elapsemerge == 0) {
					anim->fracmerge = 1;
				} else {
					if (anim->elapsemerge < anim->timemerge) {
						anim->fracmerge = (anim->timemerge - anim->elapsemerge) / anim->timemerge;
					} else {
						anim->timemerge = 0;
						anim->fracmerge = 0;
						anim->animnum2 = 0;
					}
				}
			}

			if (anim->timespeed > 0) {
				anim->elapsespeed += anim->playspeed * 0.25f;

				if (anim->elapsespeed < anim->timespeed) {
					anim->speed = anim->oldspeed + (anim->newspeed - anim->oldspeed) * anim->elapsespeed / anim->timespeed;
				} else {
					anim->timespeed = 0;
					anim->speed = anim->newspeed;
				}
			}

			speed = anim->speed;
			frame += anim->playspeed * speed * 0.25f;

			if (anim->animnum2) {
				if (anim->timespeed2 > 0) {
					anim->elapsespeed2 += anim->playspeed * 0.25f;

					if (anim->elapsespeed2 < anim->timespeed2) {
						anim->speed2 = anim->oldspeed2 + (anim->newspeed2 - anim->oldspeed2) * anim->elapsespeed2 / anim->timespeed2;
					} else {
						anim->timespeed2 = 0;
						anim->speed2 = anim->newspeed2;
					}
				}

				speed2 = anim->speed2;
				frame2 += anim->playspeed * speed2 * 0.25f;
			}

			if (anim->looping) {
				realendframe = anim->endframe;

				if (speed >= 0) {
					endframe = animGetNumFrames(anim->animnum) - 1;
					startframe = anim->loopframe;

					if (realendframe >= 0 && endframe > realendframe) {
						endframe = realendframe;
					}
				} else {
					endframe = anim->loopframe;
					startframe = animGetNumFrames(anim->animnum) - 1;

					if (realendframe >= 0 && startframe > realendframe) {
						startframe = realendframe;
					}
				}

				if ((speed >= 0 && frame >= endframe) || (speed < 0 && frame <= endframe)) {
					f32 prevnewspeed = anim->newspeed;
					f32 prevoldspeed = anim->oldspeed;
					f32 prevtimespeed = anim->timespeed;
					f32 prevelapsespeed = anim->elapsespeed;

					if (arg2) {
						model0001e2b4(model, anim->frame, endframe, 0, 0);
					} else {
						model0001e14c(model, endframe, 0);
					}

					modelSetAnimation(model, anim->animnum, anim->flip, startframe, anim->speed, anim->loopmerge);

					anim->looping = true;
					anim->endframe = realendframe;

					anim->newspeed = prevnewspeed;
					anim->oldspeed = prevoldspeed;
					anim->timespeed = prevtimespeed;
					anim->elapsespeed = prevelapsespeed;

					frame2 = frame;
					frame = startframe + frame - endframe;

					if (anim->flipfunc) {
						anim->flipfunc();
					}
				}
			}
		}

		if (arg2) {
			if (anim->animnum2) {
				model0001e2b4(model, anim->frame, frame, anim->frame2, frame2);
			} else {
				model0001e2b4(model, anim->frame, frame, 0, 0);
			}
		} else {
			if (anim->animnum2) {
				model0001e14c(model, frame, frame2);
			} else {
				model0001e14c(model, frame, 0);
			}
		}
	}
}

#if VERSION < VERSION_PAL_BETA
/**
 * This is identical to the above function but removes the 0.25f multipliers.
 */
void model0001f314(struct model *model, s32 lvupdate240, bool arg2)
{
	f32 frame;
	f32 frame2;
	f32 speed;
	f32 speed2;
	f32 startframe;
	f32 endframe;
	f32 realendframe;
	struct anim *anim = model->anim;

	if (anim && lvupdate240 > 0) {
		frame = anim->frame;
		frame2 = anim->frame2;

		for (; lvupdate240 > 0; lvupdate240--) {
			if (anim->timeplay > 0) {
				anim->elapseplay++;

				if (anim->elapseplay < anim->timeplay) {
					anim->playspeed = anim->oldplay + (anim->newplay - anim->oldplay) * anim->elapseplay / anim->timeplay;
				} else {
					anim->timeplay = 0;
					anim->playspeed = anim->newplay;
				}
			}

			if (anim->timemerge > 0) {
				anim->elapsemerge += anim->playspeed;

				if (anim->elapsemerge == 0) {
					anim->fracmerge = 1;
				} else {
					if (anim->elapsemerge < anim->timemerge) {
						anim->fracmerge = (anim->timemerge - anim->elapsemerge) / anim->timemerge;
					} else {
						anim->timemerge = 0;
						anim->fracmerge = 0;
						anim->animnum2 = 0;
					}
				}
			}

			if (anim->timespeed > 0) {
				anim->elapsespeed += anim->playspeed;

				if (anim->elapsespeed < anim->timespeed) {
					anim->speed = anim->oldspeed + (anim->newspeed - anim->oldspeed) * anim->elapsespeed / anim->timespeed;
				} else {
					anim->timespeed = 0;
					anim->speed = anim->newspeed;
				}
			}

			speed = anim->speed;
			frame += anim->playspeed * speed;

			if (anim->animnum2) {
				if (anim->timespeed2 > 0) {
					anim->elapsespeed2 += anim->playspeed;

					if (anim->elapsespeed2 < anim->timespeed2) {
						anim->speed2 = anim->oldspeed2 + (anim->newspeed2 - anim->oldspeed2) * anim->elapsespeed2 / anim->timespeed2;
					} else {
						anim->timespeed2 = 0;
						anim->speed2 = anim->newspeed2;
					}
				}

				speed2 = anim->speed2;
				frame2 += anim->playspeed * speed2;
			}

			if (anim->looping) {
				realendframe = anim->endframe;

				if (speed >= 0) {
					endframe = animGetNumFrames(anim->animnum) - 1;
					startframe = anim->loopframe;

					if (realendframe >= 0 && endframe > realendframe) {
						endframe = realendframe;
					}
				} else {
					endframe = anim->loopframe;
					startframe = animGetNumFrames(anim->animnum) - 1;

					if (realendframe >= 0 && startframe > realendframe) {
						startframe = realendframe;
					}
				}

				if ((speed >= 0 && frame >= endframe) || (speed < 0 && frame <= endframe)) {
					f32 prevnewspeed = anim->newspeed;
					f32 prevoldspeed = anim->oldspeed;
					f32 prevtimespeed = anim->timespeed;
					f32 prevelapsespeed = anim->elapsespeed;

					if (arg2) {
						model0001e2b4(model, anim->frame, endframe, 0, 0);
					} else {
						model0001e14c(model, endframe, 0);
					}

					modelSetAnimation(model, anim->animnum, anim->flip, startframe, anim->speed, anim->loopmerge);

					anim->looping = true;
					anim->endframe = realendframe;

					anim->newspeed = prevnewspeed;
					anim->oldspeed = prevoldspeed;
					anim->timespeed = prevtimespeed;
					anim->elapsespeed = prevelapsespeed;

					frame2 = frame;
					frame = startframe + frame - endframe;

					if (anim->flipfunc) {
						anim->flipfunc();
					}
				}
			}
		}

		if (arg2) {
			if (anim->animnum2) {
				model0001e2b4(model, anim->frame, frame, anim->frame2, frame2);
			} else {
				model0001e2b4(model, anim->frame, frame, 0, 0);
			}
		} else {
			if (anim->animnum2) {
				model0001e14c(model, frame, frame2);
			} else {
				model0001e14c(model, frame, 0);
			}
		}
	}
}
#endif

void model0001f7e0(struct modelrenderdata *renderdata)
{
	gDPPipeSync(renderdata->gdl++);
	gDPSetCycleType(renderdata->gdl++, G_CYC_1CYCLE);

	if (renderdata->zbufferenabled) {
		gDPSetRenderMode(renderdata->gdl++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
	} else {
		gDPSetRenderMode(renderdata->gdl++, G_RM_AA_OPA_SURF, G_RM_AA_OPA_SURF2);
	}

	gDPSetCombineMode(renderdata->gdl++, G_CC_MODULATEIA, G_CC_MODULATEIA);
}

void model0001f890(struct modelrenderdata *renderdata, bool arg1)
{
	if (renderdata->unk30 == 7) {
		if (arg1) {
			gDPPipeSync(renderdata->gdl++);
			gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
			gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
			gDPSetEnvColorViaWord(renderdata->gdl++, renderdata->envcolour | 0xff);
			gDPSetCombineLERP(renderdata->gdl++, TEXEL0, ENVIRONMENT, SHADE_ALPHA, ENVIRONMENT, TEXEL0, ENVIRONMENT, SHADE, ENVIRONMENT, COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED);

			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_OPA_SURF2);
			}
		} else {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
			}
		}
	} else if (renderdata->unk30 == 8) {
		if (arg1) {
			gDPPipeSync(renderdata->gdl++);
			gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
			gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
			gDPSetEnvColorViaWord(renderdata->gdl++, renderdata->envcolour);
			gDPSetCombineLERP(renderdata->gdl++, TEXEL0, ENVIRONMENT, SHADE_ALPHA, ENVIRONMENT, TEXEL0, 0, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED);

			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
			}
		}
	} else if (renderdata->unk30 == 9) {
		if ((renderdata->envcolour & 0xff) == 0) {
			if (arg1) {
				gDPPipeSync(renderdata->gdl++);
				gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
				gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
				gDPSetEnvColorViaWord(renderdata->gdl++, 0xffffffff);
				gDPSetPrimColor(renderdata->gdl++, 0, 0, 0, 0, 0, (renderdata->envcolour >> 8) & 0xff);
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, PRIMITIVE);

				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_OPA_SURF2);
				}
			} else {
				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
				}
			}
		} else {
			if (arg1) {
				gDPPipeSync(renderdata->gdl++);
				gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
				gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
				gDPSetEnvColor(renderdata->gdl++, 0, 0, 0, renderdata->envcolour);
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, 1, 0, SHADE, ENVIRONMENT, COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED);

				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_TEX_EDGE2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_TEX_EDGE2);
				}
			} else {
				gDPSetPrimColor(renderdata->gdl++, 0, 0, 0, 0, 0, (renderdata->envcolour >> 8) & 0xff);
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, SHADE, ENVIRONMENT, TEXEL0, 0, COMBINED, 0, SHADE, 0, 1, 0, PRIMITIVE, COMBINED);

				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_TEX_EDGE2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_TEX_EDGE2);
				}
			}
		}
	} else if (renderdata->unk30 == 4) {
		if (arg1) {
			gDPPipeSync(renderdata->gdl++);
			gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
			gDPSetFogColorViaWord(renderdata->gdl++, renderdata->envcolour);
			gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);

			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_OPA_SURF2);
			}
		} else {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
			}
		}
	} else if (renderdata->unk30 == 5) {
		u8 alpha;

		if (arg1) {
			gDPPipeSync(renderdata->gdl++);
			gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
			gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);

			alpha = renderdata->envcolour & 0xff;

			if (alpha < 255) {
				gDPSetEnvColor(renderdata->gdl++, 0xff, 0xff, 0xff, alpha);

				if (renderdata->envcolour & 0xff00) {
					gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, 1, SHADE, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0);
				} else {
					gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, 1, 0, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0);
				}
			} else {
				gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);
			}

			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
			}
		} else {
			alpha = renderdata->envcolour & 0xff;

			if (alpha < 255) {
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL0, 0, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0);
			} else {
				gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);
			}
		}
	} else {
		if (arg1) {
			gDPPipeSync(renderdata->gdl++);
			gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
			gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);

			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_PASS, G_RM_AA_ZB_OPA_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_PASS, G_RM_AA_OPA_SURF2);
			}
		} else {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_PASS, G_RM_AA_XLU_SURF2);
			}
		}
	}
}

void model00020248(struct modelrenderdata *renderdata, bool arg1)
{
	if (renderdata->unk30 == 7) {
		gDPPipeSync(renderdata->gdl++);
		gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
		gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
		gDPSetEnvColorViaWord(renderdata->gdl++, renderdata->envcolour | 0x000000ff);
		gDPSetCombineLERP(renderdata->gdl++, TEXEL0, ENVIRONMENT, SHADE_ALPHA, ENVIRONMENT, TEXEL0, ENVIRONMENT, SHADE, ENVIRONMENT, COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED);

		if (arg1) {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_OPA_SURF2);
			}
		} else {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
			}
		}
	} else if (renderdata->unk30 == 8) {
		gDPPipeSync(renderdata->gdl++);
		gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
		gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
		gDPSetEnvColorViaWord(renderdata->gdl++, renderdata->envcolour);
		gDPSetCombineLERP(renderdata->gdl++, TEXEL0, ENVIRONMENT, SHADE_ALPHA, ENVIRONMENT, TEXEL0, 0, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED);

		if (renderdata->zbufferenabled) {
			gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
		} else {
			gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
		}
	} else if (renderdata->unk30 == 9) {
		if ((renderdata->envcolour & 0xff) == 0) {
			gDPPipeSync(renderdata->gdl++);
			gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
			gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
			gDPSetEnvColorViaWord(renderdata->gdl++, 0xffffffff);
			gDPSetPrimColor(renderdata->gdl++, 0, 0, 0, 0, 0, (renderdata->envcolour >> 8) & 0xff);

			if (arg1) {
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, PRIMITIVE);

				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_OPA_SURF2);
				}
			} else {
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, PRIMITIVE);

				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
				}
			}
		} else {
			gDPPipeSync(renderdata->gdl++);
			gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
			gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);
			gDPSetEnvColorViaWord(renderdata->gdl++, renderdata->envcolour & 0xff);

			if (arg1) {
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, 1, 0, SHADE, ENVIRONMENT, COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED);

				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_TEX_EDGE2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_TEX_EDGE2);
				}
			} else {
				gDPSetPrimColor(renderdata->gdl++, 0, 0, 0, 0, 0, (renderdata->envcolour >> 8) & 0xff);
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, SHADE, ENVIRONMENT, TEXEL0, 0, COMBINED, 0, SHADE, 0, 1, 0, PRIMITIVE, COMBINED);

				if (renderdata->zbufferenabled) {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_TEX_EDGE2);
				} else {
					gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_TEX_EDGE2);
				}
			}
		}
	} else if (renderdata->unk30 == 4) {
		gDPPipeSync(renderdata->gdl++);
		gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
		gDPSetFogColorViaWord(renderdata->gdl++, renderdata->envcolour);
		gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);

		if (arg1) {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_OPA_SURF2);
			}
		} else {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
			}
		}
	} else if (renderdata->unk30 == 5) {
		u8 alpha;

		gDPPipeSync(renderdata->gdl++);
		gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
		gDPSetFogColorViaWord(renderdata->gdl++, renderdata->fogcolour);

		alpha = renderdata->envcolour & 0xff;

		if (alpha < 255) {
			gDPSetEnvColor(renderdata->gdl++, 0xff, 0xff, 0xff, alpha);

			if (arg1) {
				if (renderdata->envcolour & 0xff00) {
					gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, 1, SHADE, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0);
				} else {
					gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, 1, 0, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0);
				}
			} else {
				gDPSetCombineLERP(renderdata->gdl++, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL0, 0, ENVIRONMENT, 0, COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0);
			}
		} else {
			gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);
		}

		if (renderdata->zbufferenabled) {
			gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
		} else {
			gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
		}
	} else {
		gDPPipeSync(renderdata->gdl++);
		gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);
		gDPSetFogColorViaWord(renderdata->gdl++, 0xffffff00);
		gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);

		if (arg1) {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_OPA_SURF2);
			}
		} else {
			if (renderdata->zbufferenabled) {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_XLU_SURF2);
			} else {
				gDPSetRenderMode(renderdata->gdl++, G_RM_FOG_PRIM_A, G_RM_AA_XLU_SURF2);
			}
		}
	}
}

void model00020bdc(struct modelrenderdata *renderdata)
{
	gDPPipeSync(renderdata->gdl++);
	gDPSetCycleType(renderdata->gdl++, G_CYC_2CYCLE);

	if (renderdata->zbufferenabled) {
		gDPSetRenderMode(renderdata->gdl++, G_RM_PASS, G_RM_AA_ZB_OPA_SURF2);
	} else {
		gDPSetRenderMode(renderdata->gdl++, G_RM_PASS, G_RM_AA_OPA_SURF2);
	}

	gDPSetCombineMode(renderdata->gdl++, G_CC_TRILERP, G_CC_MODULATEIA2);
}

void modelApplyCullMode(struct modelrenderdata *renderdata)
{
	if (renderdata->cullmode == CULLMODE_NONE) {
		gSPClearGeometryMode(renderdata->gdl++, G_CULL_BOTH);
	} else if (renderdata->cullmode == CULLMODE_FRONT) {
		gSPSetGeometryMode(renderdata->gdl++, G_CULL_FRONT);
	} else if (renderdata->cullmode == CULLMODE_BACK) {
		gSPSetGeometryMode(renderdata->gdl++, G_CULL_BACK);
	}
}

void modelRenderNodeGundl(struct modelrenderdata *renderdata, struct model *model, struct modelnode *node)
{
	struct modelrodata_gundl *rodata = &node->rodata->gundl;

	if (var8005efc4 && !var8005efc4(model, node)) {
		return;
	}

	if ((renderdata->flags & 1) && rodata->primary) {
		gSPSegment(renderdata->gdl++, 0x05, osVirtualToPhysical(rodata->baseaddr));

		if (renderdata->cullmode) {
			modelApplyCullMode(renderdata);
		}

		switch (rodata->unk12) {
		case 1:
			model0001f7e0(renderdata);
			break;
		case 3:
			model0001f890(renderdata, true);
			break;
		case 4:
			model00020248(renderdata, true);
			break;
		case 2:
			model00020bdc(renderdata);
			break;
		}

		gSPDisplayList(renderdata->gdl++, rodata->primary);

		if (rodata->unk12 == 3 && rodata->secondary) {
			model0001f890(renderdata, false);

			gSPDisplayList(renderdata->gdl++, rodata->secondary);
		}
	}

	if ((renderdata->flags & 2) && rodata->primary && rodata->unk12 == 4 && rodata->secondary) {
		gSPSegment(renderdata->gdl++, 0x05, osVirtualToPhysical(rodata->baseaddr));

		if (renderdata->cullmode) {
			modelApplyCullMode(renderdata);
		}

		model00020248(renderdata, false);

		gSPDisplayList(renderdata->gdl++, rodata->secondary);
	}
}

void modelRenderNodeDl(struct modelrenderdata *renderdata, struct model *model, struct modelnode *node)
{
	union modelrodata *rodata = node->rodata;

	if (var8005efc4 && !var8005efc4(model, node)) {
		return;
	}

	if (renderdata->flags & 1) {
		union modelrwdata *rwdata = modelGetNodeRwData(model, node);

		if (rwdata->dl.gdl) {
			gSPSegment(renderdata->gdl++, 0x05, osVirtualToPhysical(rodata->dl.colourtable));

			if (renderdata->cullmode) {
				modelApplyCullMode(renderdata);
			}

			switch (rodata->dl.mcount) {
			case 1:
				model0001f7e0(renderdata);
				break;
			case 3:
				model0001f890(renderdata, true);
				break;
			case 4:
				model00020248(renderdata, true);
				break;
			case 2:
				model00020bdc(renderdata);
				break;
			}

			gSPSegment(renderdata->gdl++, 0x04, osVirtualToPhysical(rwdata->dl.vertices));
			gSPSegment(renderdata->gdl++, 0x06, osVirtualToPhysical(rwdata->dl.colours));

			gSPDisplayList(renderdata->gdl++, rwdata->dl.gdl);

			if (rodata->dl.mcount == 3 && rodata->dl.secondary) {
				model0001f890(renderdata, false);

				gSPDisplayList(renderdata->gdl++, rodata->dl.secondary);
			}
		}
	}

	if (renderdata->flags & 2) {
		union modelrwdata *rwdata = modelGetNodeRwData(model, node);

		if (rwdata->dl.gdl && rodata->dl.mcount == 4 && rodata->dl.secondary) {
			gSPSegment(renderdata->gdl++, 0x05, osVirtualToPhysical(rodata->dl.colourtable));

			if (renderdata->cullmode) {
				modelApplyCullMode(renderdata);
			}

			gSPSegment(renderdata->gdl++, 0x04, osVirtualToPhysical(rwdata->dl.vertices));
			gSPSegment(renderdata->gdl++, 0x06, osVirtualToPhysical(rwdata->dl.colours));

			model00020248(renderdata, false);

			gSPDisplayList(renderdata->gdl++, rodata->dl.secondary);
		}
	}
}

GLOBAL_ASM(
glabel modelRenderNodeType16
/*    2124c:	27bdffb8 */ 	addiu	$sp,$sp,-72
/*    21250:	afbf003c */ 	sw	$ra,0x3c($sp)
/*    21254:	afbe0038 */ 	sw	$s8,0x38($sp)
/*    21258:	afb70034 */ 	sw	$s7,0x34($sp)
/*    2125c:	afb60030 */ 	sw	$s6,0x30($sp)
/*    21260:	afb5002c */ 	sw	$s5,0x2c($sp)
/*    21264:	afb40028 */ 	sw	$s4,0x28($sp)
/*    21268:	afb30024 */ 	sw	$s3,0x24($sp)
/*    2126c:	afb20020 */ 	sw	$s2,0x20($sp)
/*    21270:	afb1001c */ 	sw	$s1,0x1c($sp)
/*    21274:	afb00018 */ 	sw	$s0,0x18($sp)
/*    21278:	8c8e0008 */ 	lw	$t6,0x8($a0)
/*    2127c:	00809825 */ 	or	$s3,$a0,$zero
/*    21280:	31cf0002 */ 	andi	$t7,$t6,0x2
/*    21284:	51e00106 */ 	beqzl	$t7,.L000216a0
/*    21288:	8fbf003c */ 	lw	$ra,0x3c($sp)
/*    2128c:	8ca20004 */ 	lw	$v0,0x4($a1)
/*    21290:	8c580008 */ 	lw	$t8,0x8($v0)
/*    21294:	53000102 */ 	beqzl	$t8,.L000216a0
/*    21298:	8fbf003c */ 	lw	$ra,0x3c($sp)
/*    2129c:	8c440000 */ 	lw	$a0,0x0($v0)
/*    212a0:	8c520004 */ 	lw	$s2,0x4($v0)
/*    212a4:	afa20044 */ 	sw	$v0,0x44($sp)
/*    212a8:	0004c880 */ 	sll	$t9,$a0,0x2
/*    212ac:	03202025 */ 	or	$a0,$t9,$zero
/*    212b0:	3c198006 */ 	lui	$t9,%hi(g_ModelVtxAllocatorFunc)
/*    212b4:	8f39efc8 */ 	lw	$t9,%lo(g_ModelVtxAllocatorFunc)($t9)
/*    212b8:	0320f809 */ 	jalr	$t9
/*    212bc:	00000000 */ 	nop
/*    212c0:	8e74000c */ 	lw	$s4,0xc($s3)
/*    212c4:	3c0abc00 */ 	lui	$t2,0xbc00
/*    212c8:	354a1006 */ 	ori	$t2,$t2,0x1006
/*    212cc:	26890008 */ 	addiu	$t1,$s4,0x8
/*    212d0:	ae69000c */ 	sw	$t1,0xc($s3)
/*    212d4:	00408025 */ 	or	$s0,$v0,$zero
/*    212d8:	00402025 */ 	or	$a0,$v0,$zero
/*    212dc:	0c012d20 */ 	jal	osVirtualToPhysical
/*    212e0:	ae8a0000 */ 	sw	$t2,0x0($s4)
/*    212e4:	8fa30044 */ 	lw	$v1,0x44($sp)
/*    212e8:	ae820004 */ 	sw	$v0,0x4($s4)
/*    212ec:	8e76000c */ 	lw	$s6,0xc($s3)
/*    212f0:	3c0cbc00 */ 	lui	$t4,0xbc00
/*    212f4:	358c1806 */ 	ori	$t4,$t4,0x1806
/*    212f8:	26cb0008 */ 	addiu	$t3,$s6,0x8
/*    212fc:	ae6b000c */ 	sw	$t3,0xc($s3)
/*    21300:	aecc0000 */ 	sw	$t4,0x0($s6)
/*    21304:	8c6e0000 */ 	lw	$t6,0x0($v1)
/*    21308:	2415000c */ 	addiu	$s5,$zero,0xc
/*    2130c:	8c6d0004 */ 	lw	$t5,0x4($v1)
/*    21310:	000e7880 */ 	sll	$t7,$t6,0x2
/*    21314:	01f50019 */ 	multu	$t7,$s5
/*    21318:	0000c012 */ 	mflo	$t8
/*    2131c:	01b82021 */ 	addu	$a0,$t5,$t8
/*    21320:	24840007 */ 	addiu	$a0,$a0,0x7
/*    21324:	34990007 */ 	ori	$t9,$a0,0x7
/*    21328:	0c012d20 */ 	jal	osVirtualToPhysical
/*    2132c:	3b240007 */ 	xori	$a0,$t9,0x7
/*    21330:	aec20004 */ 	sw	$v0,0x4($s6)
/*    21334:	8e74000c */ 	lw	$s4,0xc($s3)
/*    21338:	3c0bbc00 */ 	lui	$t3,0xbc00
/*    2133c:	356b1406 */ 	ori	$t3,$t3,0x1406
/*    21340:	268a0008 */ 	addiu	$t2,$s4,0x8
/*    21344:	ae6a000c */ 	sw	$t2,0xc($s3)
/*    21348:	ae8b0000 */ 	sw	$t3,0x0($s4)
/*    2134c:	8fac0044 */ 	lw	$t4,0x44($sp)
/*    21350:	0c012d20 */ 	jal	osVirtualToPhysical
/*    21354:	8d84000c */ 	lw	$a0,0xc($t4)
/*    21358:	8fa50044 */ 	lw	$a1,0x44($sp)
/*    2135c:	ae820004 */ 	sw	$v0,0x4($s4)
/*    21360:	8e71000c */ 	lw	$s1,0xc($s3)
/*    21364:	3c0ff800 */ 	lui	$t7,0xf800
/*    21368:	3c180600 */ 	lui	$t8,0x600
/*    2136c:	262e0008 */ 	addiu	$t6,$s1,0x8
/*    21370:	ae6e000c */ 	sw	$t6,0xc($s3)
/*    21374:	ae200004 */ 	sw	$zero,0x4($s1)
/*    21378:	ae2f0000 */ 	sw	$t7,0x0($s1)
/*    2137c:	8e71000c */ 	lw	$s1,0xc($s3)
/*    21380:	0000b025 */ 	or	$s6,$zero,$zero
/*    21384:	241e0200 */ 	addiu	$s8,$zero,0x200
/*    21388:	262d0008 */ 	addiu	$t5,$s1,0x8
/*    2138c:	ae6d000c */ 	sw	$t5,0xc($s3)
/*    21390:	ae380000 */ 	sw	$t8,0x0($s1)
/*    21394:	8cb90008 */ 	lw	$t9,0x8($a1)
/*    21398:	241700b5 */ 	addiu	$s7,$zero,0xb5
/*    2139c:	ae390004 */ 	sw	$t9,0x4($s1)
/*    213a0:	8ca90000 */ 	lw	$t1,0x0($a1)
/*    213a4:	592000be */ 	blezl	$t1,.L000216a0
/*    213a8:	8fbf003c */ 	lw	$ra,0x3c($sp)
.L000213ac:
/*    213ac:	0c004b70 */ 	jal	random
/*    213b0:	00000000 */ 	nop
/*    213b4:	00025280 */ 	sll	$t2,$v0,0xa
/*    213b8:	3151ffff */ 	andi	$s1,$t2,0xffff
/*    213bc:	0c013ef0 */ 	jal	coss
/*    213c0:	3144ffff */ 	andi	$a0,$t2,0xffff
/*    213c4:	00025940 */ 	sll	$t3,$v0,0x5
/*    213c8:	01770019 */ 	multu	$t3,$s7
/*    213cc:	3224ffff */ 	andi	$a0,$s1,0xffff
/*    213d0:	0000a012 */ 	mflo	$s4
/*    213d4:	00146483 */ 	sra	$t4,$s4,0x12
/*    213d8:	0c013efc */ 	jal	sins
/*    213dc:	0180a025 */ 	or	$s4,$t4,$zero
/*    213e0:	00027140 */ 	sll	$t6,$v0,0x5
/*    213e4:	01d70019 */ 	multu	$t6,$s7
/*    213e8:	00009812 */ 	mflo	$s3
/*    213ec:	00137c83 */ 	sra	$t7,$s3,0x12
/*    213f0:	0c004b70 */ 	jal	random
/*    213f4:	01e09825 */ 	or	$s3,$t7,$zero
/*    213f8:	0c004b70 */ 	jal	random
/*    213fc:	00028fc2 */ 	srl	$s1,$v0,0x1f
/*    21400:	8a410000 */ 	lwl	$at,0x0($s2)
/*    21404:	9a410003 */ 	lwr	$at,0x3($s2)
/*    21408:	304d3fff */ 	andi	$t5,$v0,0x3fff
/*    2140c:	3c180001 */ 	lui	$t8,0x1
/*    21410:	aa010000 */ 	swl	$at,0x0($s0)
/*    21414:	ba010003 */ 	swr	$at,0x3($s0)
/*    21418:	8a490004 */ 	lwl	$t1,0x4($s2)
/*    2141c:	9a490007 */ 	lwr	$t1,0x7($s2)
/*    21420:	030d2023 */ 	subu	$a0,$t8,$t5
/*    21424:	03d44023 */ 	subu	$t0,$s8,$s4
/*    21428:	aa090004 */ 	swl	$t1,0x4($s0)
/*    2142c:	ba090007 */ 	swr	$t1,0x7($s0)
/*    21430:	8a410008 */ 	lwl	$at,0x8($s2)
/*    21434:	9a41000b */ 	lwr	$at,0xb($s2)
/*    21438:	03d33823 */ 	subu	$a3,$s8,$s3
/*    2143c:	00801825 */ 	or	$v1,$a0,$zero
/*    21440:	aa010008 */ 	swl	$at,0x8($s0)
/*    21444:	ba01000b */ 	swr	$at,0xb($s0)
/*    21448:	8a41000c */ 	lwl	$at,0xc($s2)
/*    2144c:	9a41000f */ 	lwr	$at,0xf($s2)
/*    21450:	26660200 */ 	addiu	$a2,$s3,0x200
/*    21454:	26d60001 */ 	addiu	$s6,$s6,0x1
/*    21458:	aa01000c */ 	swl	$at,0xc($s0)
/*    2145c:	ba01000f */ 	swr	$at,0xf($s0)
/*    21460:	8a4b0010 */ 	lwl	$t3,0x10($s2)
/*    21464:	9a4b0013 */ 	lwr	$t3,0x13($s2)
/*    21468:	26100030 */ 	addiu	$s0,$s0,0x30
/*    2146c:	aa0bffe0 */ 	swl	$t3,-0x20($s0)
/*    21470:	ba0bffe3 */ 	swr	$t3,-0x1d($s0)
/*    21474:	8a410014 */ 	lwl	$at,0x14($s2)
/*    21478:	9a410017 */ 	lwr	$at,0x17($s2)
/*    2147c:	aa01ffe4 */ 	swl	$at,-0x1c($s0)
/*    21480:	ba01ffe7 */ 	swr	$at,-0x19($s0)
/*    21484:	8a410018 */ 	lwl	$at,0x18($s2)
/*    21488:	9a41001b */ 	lwr	$at,0x1b($s2)
/*    2148c:	aa01ffe8 */ 	swl	$at,-0x18($s0)
/*    21490:	ba01ffeb */ 	swr	$at,-0x15($s0)
/*    21494:	8a4e001c */ 	lwl	$t6,0x1c($s2)
/*    21498:	9a4e001f */ 	lwr	$t6,0x1f($s2)
/*    2149c:	aa0effec */ 	swl	$t6,-0x14($s0)
/*    214a0:	ba0effef */ 	swr	$t6,-0x11($s0)
/*    214a4:	8a410020 */ 	lwl	$at,0x20($s2)
/*    214a8:	9a410023 */ 	lwr	$at,0x23($s2)
/*    214ac:	aa01fff0 */ 	swl	$at,-0x10($s0)
/*    214b0:	ba01fff3 */ 	swr	$at,-0xd($s0)
/*    214b4:	8a410024 */ 	lwl	$at,0x24($s2)
/*    214b8:	9a410027 */ 	lwr	$at,0x27($s2)
/*    214bc:	aa01fff4 */ 	swl	$at,-0xc($s0)
/*    214c0:	ba01fff7 */ 	swr	$at,-0x9($s0)
/*    214c4:	8a580028 */ 	lwl	$t8,0x28($s2)
/*    214c8:	9a58002b */ 	lwr	$t8,0x2b($s2)
/*    214cc:	aa18fff8 */ 	swl	$t8,-0x8($s0)
/*    214d0:	ba18fffb */ 	swr	$t8,-0x5($s0)
/*    214d4:	8a41002c */ 	lwl	$at,0x2c($s2)
/*    214d8:	9a41002f */ 	lwr	$at,0x2f($s2)
/*    214dc:	a608ffd8 */ 	sh	$t0,-0x28($s0)
/*    214e0:	a607ffda */ 	sh	$a3,-0x26($s0)
/*    214e4:	aa01fffc */ 	swl	$at,-0x4($s0)
/*    214e8:	ba01ffff */ 	swr	$at,-0x1($s0)
/*    214ec:	06210004 */ 	bgez	$s1,.L00021500
/*    214f0:	322d0003 */ 	andi	$t5,$s1,0x3
/*    214f4:	11a00002 */ 	beqz	$t5,.L00021500
/*    214f8:	00000000 */ 	nop
/*    214fc:	25adfffc */ 	addiu	$t5,$t5,-4
.L00021500:
/*    21500:	01b50019 */ 	multu	$t5,$s5
/*    21504:	0000c812 */ 	mflo	$t9
/*    21508:	02592821 */ 	addu	$a1,$s2,$t9
/*    2150c:	84a90000 */ 	lh	$t1,0x0($a1)
/*    21510:	01240019 */ 	multu	$t1,$a0
/*    21514:	26290001 */ 	addiu	$t1,$s1,0x1
/*    21518:	00005012 */ 	mflo	$t2
/*    2151c:	000a5c03 */ 	sra	$t3,$t2,0x10
/*    21520:	a60bffd0 */ 	sh	$t3,-0x30($s0)
/*    21524:	84ac0002 */ 	lh	$t4,0x2($a1)
/*    21528:	01840019 */ 	multu	$t4,$a0
/*    2152c:	26840200 */ 	addiu	$a0,$s4,0x200
/*    21530:	00007012 */ 	mflo	$t6
/*    21534:	000e7c03 */ 	sra	$t7,$t6,0x10
/*    21538:	a60fffd2 */ 	sh	$t7,-0x2e($s0)
/*    2153c:	84b80004 */ 	lh	$t8,0x4($a1)
/*    21540:	a606ffe4 */ 	sh	$a2,-0x1c($s0)
/*    21544:	a608ffe6 */ 	sh	$t0,-0x1a($s0)
/*    21548:	03030019 */ 	multu	$t8,$v1
/*    2154c:	00006812 */ 	mflo	$t5
/*    21550:	000dcc03 */ 	sra	$t9,$t5,0x10
/*    21554:	a619ffd4 */ 	sh	$t9,-0x2c($s0)
/*    21558:	05210004 */ 	bgez	$t1,.L0002156c
/*    2155c:	312a0003 */ 	andi	$t2,$t1,0x3
/*    21560:	11400002 */ 	beqz	$t2,.L0002156c
/*    21564:	00000000 */ 	nop
/*    21568:	254afffc */ 	addiu	$t2,$t2,-4
.L0002156c:
/*    2156c:	01550019 */ 	multu	$t2,$s5
/*    21570:	00005812 */ 	mflo	$t3
/*    21574:	024b1021 */ 	addu	$v0,$s2,$t3
/*    21578:	844c0000 */ 	lh	$t4,0x0($v0)
/*    2157c:	01830019 */ 	multu	$t4,$v1
/*    21580:	262c0002 */ 	addiu	$t4,$s1,0x2
/*    21584:	00007012 */ 	mflo	$t6
/*    21588:	000e7c03 */ 	sra	$t7,$t6,0x10
/*    2158c:	a60fffdc */ 	sh	$t7,-0x24($s0)
/*    21590:	84580002 */ 	lh	$t8,0x2($v0)
/*    21594:	03030019 */ 	multu	$t8,$v1
/*    21598:	00006812 */ 	mflo	$t5
/*    2159c:	000dcc03 */ 	sra	$t9,$t5,0x10
/*    215a0:	a619ffde */ 	sh	$t9,-0x22($s0)
/*    215a4:	84490004 */ 	lh	$t1,0x4($v0)
/*    215a8:	a604fff0 */ 	sh	$a0,-0x10($s0)
/*    215ac:	a606fff2 */ 	sh	$a2,-0xe($s0)
/*    215b0:	01230019 */ 	multu	$t1,$v1
/*    215b4:	00005012 */ 	mflo	$t2
/*    215b8:	000a5c03 */ 	sra	$t3,$t2,0x10
/*    215bc:	a60bffe0 */ 	sh	$t3,-0x20($s0)
/*    215c0:	05810004 */ 	bgez	$t4,.L000215d4
/*    215c4:	318e0003 */ 	andi	$t6,$t4,0x3
/*    215c8:	11c00002 */ 	beqz	$t6,.L000215d4
/*    215cc:	00000000 */ 	nop
/*    215d0:	25cefffc */ 	addiu	$t6,$t6,-4
.L000215d4:
/*    215d4:	01d50019 */ 	multu	$t6,$s5
/*    215d8:	00007812 */ 	mflo	$t7
/*    215dc:	024f1021 */ 	addu	$v0,$s2,$t7
/*    215e0:	84580000 */ 	lh	$t8,0x0($v0)
/*    215e4:	03030019 */ 	multu	$t8,$v1
/*    215e8:	26380003 */ 	addiu	$t8,$s1,0x3
/*    215ec:	00006812 */ 	mflo	$t5
/*    215f0:	000dcc03 */ 	sra	$t9,$t5,0x10
/*    215f4:	a619ffe8 */ 	sh	$t9,-0x18($s0)
/*    215f8:	84490002 */ 	lh	$t1,0x2($v0)
/*    215fc:	01230019 */ 	multu	$t1,$v1
/*    21600:	00005012 */ 	mflo	$t2
/*    21604:	000a5c03 */ 	sra	$t3,$t2,0x10
/*    21608:	a60bffea */ 	sh	$t3,-0x16($s0)
/*    2160c:	844c0004 */ 	lh	$t4,0x4($v0)
/*    21610:	a607fffc */ 	sh	$a3,-0x4($s0)
/*    21614:	a604fffe */ 	sh	$a0,-0x2($s0)
/*    21618:	01830019 */ 	multu	$t4,$v1
/*    2161c:	00007012 */ 	mflo	$t6
/*    21620:	000e7c03 */ 	sra	$t7,$t6,0x10
/*    21624:	a60fffec */ 	sh	$t7,-0x14($s0)
/*    21628:	07010004 */ 	bgez	$t8,.L0002163c
/*    2162c:	330d0003 */ 	andi	$t5,$t8,0x3
/*    21630:	11a00002 */ 	beqz	$t5,.L0002163c
/*    21634:	00000000 */ 	nop
/*    21638:	25adfffc */ 	addiu	$t5,$t5,-4
.L0002163c:
/*    2163c:	01b50019 */ 	multu	$t5,$s5
/*    21640:	0000c812 */ 	mflo	$t9
/*    21644:	02591021 */ 	addu	$v0,$s2,$t9
/*    21648:	84490000 */ 	lh	$t1,0x0($v0)
/*    2164c:	26520030 */ 	addiu	$s2,$s2,0x30
/*    21650:	01230019 */ 	multu	$t1,$v1
/*    21654:	00005012 */ 	mflo	$t2
/*    21658:	000a5c03 */ 	sra	$t3,$t2,0x10
/*    2165c:	a60bfff4 */ 	sh	$t3,-0xc($s0)
/*    21660:	844c0002 */ 	lh	$t4,0x2($v0)
/*    21664:	01830019 */ 	multu	$t4,$v1
/*    21668:	00007012 */ 	mflo	$t6
/*    2166c:	000e7c03 */ 	sra	$t7,$t6,0x10
/*    21670:	a60ffff6 */ 	sh	$t7,-0xa($s0)
/*    21674:	84580004 */ 	lh	$t8,0x4($v0)
/*    21678:	03030019 */ 	multu	$t8,$v1
/*    2167c:	00006812 */ 	mflo	$t5
/*    21680:	000dcc03 */ 	sra	$t9,$t5,0x10
/*    21684:	a619fff8 */ 	sh	$t9,-0x8($s0)
/*    21688:	8fa90044 */ 	lw	$t1,0x44($sp)
/*    2168c:	8d2a0000 */ 	lw	$t2,0x0($t1)
/*    21690:	02ca082a */ 	slt	$at,$s6,$t2
/*    21694:	1420ff45 */ 	bnez	$at,.L000213ac
/*    21698:	00000000 */ 	nop
/*    2169c:	8fbf003c */ 	lw	$ra,0x3c($sp)
.L000216a0:
/*    216a0:	8fb00018 */ 	lw	$s0,0x18($sp)
/*    216a4:	8fb1001c */ 	lw	$s1,0x1c($sp)
/*    216a8:	8fb20020 */ 	lw	$s2,0x20($sp)
/*    216ac:	8fb30024 */ 	lw	$s3,0x24($sp)
/*    216b0:	8fb40028 */ 	lw	$s4,0x28($sp)
/*    216b4:	8fb5002c */ 	lw	$s5,0x2c($sp)
/*    216b8:	8fb60030 */ 	lw	$s6,0x30($sp)
/*    216bc:	8fb70034 */ 	lw	$s7,0x34($sp)
/*    216c0:	8fbe0038 */ 	lw	$s8,0x38($sp)
/*    216c4:	03e00008 */ 	jr	$ra
/*    216c8:	27bd0048 */ 	addiu	$sp,$sp,0x48
);

void model000216cc(struct modelrenderdata *renderdata, struct textureconfig *tconfig, s32 arg2)
{
	texSelect(&renderdata->gdl, tconfig, arg2, renderdata->zbufferenabled, 2, 1, NULL);
}

GLOBAL_ASM(
glabel modelRenderNodeGunfire
.late_rodata
glabel var70054454
.word 0x40c907a9
.text
/*    2170c:	27bdfef0 */ 	addiu	$sp,$sp,-272
/*    21710:	afb30020 */ 	sw	$s3,0x20($sp)
/*    21714:	00809825 */ 	or	$s3,$a0,$zero
/*    21718:	afbf0024 */ 	sw	$ra,0x24($sp)
/*    2171c:	afb2001c */ 	sw	$s2,0x1c($sp)
/*    21720:	afb10018 */ 	sw	$s1,0x18($sp)
/*    21724:	afb00014 */ 	sw	$s0,0x14($sp)
/*    21728:	afa50114 */ 	sw	$a1,0x114($sp)
/*    2172c:	00a02025 */ 	or	$a0,$a1,$zero
/*    21730:	00c08025 */ 	or	$s0,$a2,$zero
/*    21734:	8cd10004 */ 	lw	$s1,0x4($a2)
/*    21738:	0c006a87 */ 	jal	modelGetNodeRwData
/*    2173c:	00c02825 */ 	or	$a1,$a2,$zero
/*    21740:	3c198006 */ 	lui	$t9,%hi(var8005efec)
/*    21744:	2739efec */ 	addiu	$t9,$t9,%lo(var8005efec)
/*    21748:	8f210000 */ 	lw	$at,0x0($t9)
/*    2174c:	27ab0084 */ 	addiu	$t3,$sp,0x84
/*    21750:	8f380004 */ 	lw	$t8,0x4($t9)
/*    21754:	ad610000 */ 	sw	$at,0x0($t3)
/*    21758:	8f210008 */ 	lw	$at,0x8($t9)
/*    2175c:	3c0c8006 */ 	lui	$t4,%hi(var8005eff8)
/*    21760:	ad780004 */ 	sw	$t8,0x4($t3)
/*    21764:	ad610008 */ 	sw	$at,0x8($t3)
/*    21768:	8d8ceff8 */ 	lw	$t4,%lo(var8005eff8)($t4)
/*    2176c:	afac0080 */ 	sw	$t4,0x80($sp)
/*    21770:	8e6f0008 */ 	lw	$t7,0x8($s3)
/*    21774:	31ed0002 */ 	andi	$t5,$t7,0x2
/*    21778:	51a001d8 */ 	beqzl	$t5,.L00021edc
/*    2177c:	8fbf0024 */ 	lw	$ra,0x24($sp)
/*    21780:	844e0000 */ 	lh	$t6,0x0($v0)
/*    21784:	02002025 */ 	or	$a0,$s0,$zero
/*    21788:	51c001d4 */ 	beqzl	$t6,.L00021edc
/*    2178c:	8fbf0024 */ 	lw	$ra,0x24($sp)
/*    21790:	0c006949 */ 	jal	model0001a524
/*    21794:	00002825 */ 	or	$a1,$zero,$zero
/*    21798:	8fab0114 */ 	lw	$t3,0x114($sp)
/*    2179c:	0002c180 */ 	sll	$t8,$v0,0x6
/*    217a0:	c6280000 */ 	lwc1	$f8,0x0($s1)
/*    217a4:	8d79000c */ 	lw	$t9,0xc($t3)
/*    217a8:	c6240004 */ 	lwc1	$f4,0x4($s1)
/*    217ac:	03389021 */ 	addu	$s2,$t9,$t8
/*    217b0:	c64a0000 */ 	lwc1	$f10,0x0($s2)
/*    217b4:	460a4182 */ 	mul.s	$f6,$f8,$f10
/*    217b8:	c6480010 */ 	lwc1	$f8,0x10($s2)
/*    217bc:	46082282 */ 	mul.s	$f10,$f4,$f8
/*    217c0:	c6280008 */ 	lwc1	$f8,0x8($s1)
/*    217c4:	460a3100 */ 	add.s	$f4,$f6,$f10
/*    217c8:	c6460020 */ 	lwc1	$f6,0x20($s2)
/*    217cc:	46064282 */ 	mul.s	$f10,$f8,$f6
/*    217d0:	c6460030 */ 	lwc1	$f6,0x30($s2)
/*    217d4:	460a2200 */ 	add.s	$f8,$f4,$f10
/*    217d8:	46083100 */ 	add.s	$f4,$f6,$f8
/*    217dc:	46002287 */ 	neg.s	$f10,$f4
/*    217e0:	e7aa00e0 */ 	swc1	$f10,0xe0($sp)
/*    217e4:	c6260000 */ 	lwc1	$f6,0x0($s1)
/*    217e8:	c6480004 */ 	lwc1	$f8,0x4($s2)
/*    217ec:	46083102 */ 	mul.s	$f4,$f6,$f8
/*    217f0:	c6480014 */ 	lwc1	$f8,0x14($s2)
/*    217f4:	c6260004 */ 	lwc1	$f6,0x4($s1)
/*    217f8:	46083182 */ 	mul.s	$f6,$f6,$f8
/*    217fc:	46062200 */ 	add.s	$f8,$f4,$f6
/*    21800:	c6460024 */ 	lwc1	$f6,0x24($s2)
/*    21804:	c6240008 */ 	lwc1	$f4,0x8($s1)
/*    21808:	46062102 */ 	mul.s	$f4,$f4,$f6
/*    2180c:	46044180 */ 	add.s	$f6,$f8,$f4
/*    21810:	c6480034 */ 	lwc1	$f8,0x34($s2)
/*    21814:	46064100 */ 	add.s	$f4,$f8,$f6
/*    21818:	46002207 */ 	neg.s	$f8,$f4
/*    2181c:	e7a800e4 */ 	swc1	$f8,0xe4($sp)
/*    21820:	c6440008 */ 	lwc1	$f4,0x8($s2)
/*    21824:	c6260000 */ 	lwc1	$f6,0x0($s1)
/*    21828:	46043182 */ 	mul.s	$f6,$f6,$f4
/*    2182c:	c6240004 */ 	lwc1	$f4,0x4($s1)
/*    21830:	e7aa0028 */ 	swc1	$f10,0x28($sp)
/*    21834:	c64a0018 */ 	lwc1	$f10,0x18($s2)
/*    21838:	460a2102 */ 	mul.s	$f4,$f4,$f10
/*    2183c:	46043280 */ 	add.s	$f10,$f6,$f4
/*    21840:	c6440028 */ 	lwc1	$f4,0x28($s2)
/*    21844:	c6260008 */ 	lwc1	$f6,0x8($s1)
/*    21848:	46043182 */ 	mul.s	$f6,$f6,$f4
/*    2184c:	46065100 */ 	add.s	$f4,$f10,$f6
/*    21850:	c64a0038 */ 	lwc1	$f10,0x38($s2)
/*    21854:	46045180 */ 	add.s	$f6,$f10,$f4
/*    21858:	c7a40028 */ 	lwc1	$f4,0x28($sp)
/*    2185c:	46003287 */ 	neg.s	$f10,$f6
/*    21860:	46042182 */ 	mul.s	$f6,$f4,$f4
/*    21864:	e7aa00e8 */ 	swc1	$f10,0xe8($sp)
/*    21868:	46084102 */ 	mul.s	$f4,$f8,$f8
/*    2186c:	46043200 */ 	add.s	$f8,$f6,$f4
/*    21870:	460a5282 */ 	mul.s	$f10,$f10,$f10
/*    21874:	0c012974 */ 	jal	sqrtf
/*    21878:	46085300 */ 	add.s	$f12,$f10,$f8
/*    2187c:	44807000 */ 	mtc1	$zero,$f14
/*    21880:	00000000 */ 	nop
/*    21884:	4600703c */ 	c.lt.s	$f14,$f0
/*    21888:	00000000 */ 	nop
/*    2188c:	45020012 */ 	bc1fl	.L000218d8
/*    21890:	8faf0114 */ 	lw	$t7,0x114($sp)
/*    21894:	8fac0114 */ 	lw	$t4,0x114($sp)
/*    21898:	3c013f80 */ 	lui	$at,0x3f80
/*    2189c:	44813000 */ 	mtc1	$at,$f6
/*    218a0:	c5840014 */ 	lwc1	$f4,0x14($t4)
/*    218a4:	c7a800e0 */ 	lwc1	$f8,0xe0($sp)
/*    218a8:	46002282 */ 	mul.s	$f10,$f4,$f0
/*    218ac:	460a3083 */ 	div.s	$f2,$f6,$f10
/*    218b0:	c7a600e4 */ 	lwc1	$f6,0xe4($sp)
/*    218b4:	46024102 */ 	mul.s	$f4,$f8,$f2
/*    218b8:	c7a800e8 */ 	lwc1	$f8,0xe8($sp)
/*    218bc:	46023282 */ 	mul.s	$f10,$f6,$f2
/*    218c0:	e7a400e0 */ 	swc1	$f4,0xe0($sp)
/*    218c4:	46024102 */ 	mul.s	$f4,$f8,$f2
/*    218c8:	e7aa00e4 */ 	swc1	$f10,0xe4($sp)
/*    218cc:	10000009 */ 	b	.L000218f4
/*    218d0:	e7a400e8 */ 	swc1	$f4,0xe8($sp)
/*    218d4:	8faf0114 */ 	lw	$t7,0x114($sp)
.L000218d8:
/*    218d8:	e7ae00e0 */ 	swc1	$f14,0xe0($sp)
/*    218dc:	e7ae00e4 */ 	swc1	$f14,0xe4($sp)
/*    218e0:	3c013f80 */ 	lui	$at,0x3f80
/*    218e4:	44813000 */ 	mtc1	$at,$f6
/*    218e8:	c5ea0014 */ 	lwc1	$f10,0x14($t7)
/*    218ec:	460a3203 */ 	div.s	$f8,$f6,$f10
/*    218f0:	e7a800e8 */ 	swc1	$f8,0xe8($sp)
.L000218f4:
/*    218f4:	c7a400e0 */ 	lwc1	$f4,0xe0($sp)
/*    218f8:	c6460010 */ 	lwc1	$f6,0x10($s2)
/*    218fc:	c7a800e4 */ 	lwc1	$f8,0xe4($sp)
/*    21900:	46062282 */ 	mul.s	$f10,$f4,$f6
/*    21904:	c6440014 */ 	lwc1	$f4,0x14($s2)
/*    21908:	46044182 */ 	mul.s	$f6,$f8,$f4
/*    2190c:	c6440018 */ 	lwc1	$f4,0x18($s2)
/*    21910:	46065200 */ 	add.s	$f8,$f10,$f6
/*    21914:	c7aa00e8 */ 	lwc1	$f10,0xe8($sp)
/*    21918:	460a2182 */ 	mul.s	$f6,$f4,$f10
/*    2191c:	0fc25a74 */ 	jal	acosf
/*    21920:	46083300 */ 	add.s	$f12,$f6,$f8
/*    21924:	e7a000ec */ 	swc1	$f0,0xec($sp)
/*    21928:	0c0068f7 */ 	jal	sinf
/*    2192c:	46000306 */ 	mov.s	$f12,$f0
/*    21930:	c7a400e0 */ 	lwc1	$f4,0xe0($sp)
/*    21934:	c64a0020 */ 	lwc1	$f10,0x20($s2)
/*    21938:	c7a800e4 */ 	lwc1	$f8,0xe4($sp)
/*    2193c:	460a2182 */ 	mul.s	$f6,$f4,$f10
/*    21940:	c6440024 */ 	lwc1	$f4,0x24($s2)
/*    21944:	46044282 */ 	mul.s	$f10,$f8,$f4
/*    21948:	c7a400e8 */ 	lwc1	$f4,0xe8($sp)
/*    2194c:	460a3200 */ 	add.s	$f8,$f6,$f10
/*    21950:	c6460028 */ 	lwc1	$f6,0x28($s2)
/*    21954:	46062282 */ 	mul.s	$f10,$f4,$f6
/*    21958:	460a4100 */ 	add.s	$f4,$f8,$f10
/*    2195c:	46002187 */ 	neg.s	$f6,$f4
/*    21960:	0fc25a74 */ 	jal	acosf
/*    21964:	46003303 */ 	div.s	$f12,$f6,$f0
/*    21968:	c7a800e0 */ 	lwc1	$f8,0xe0($sp)
/*    2196c:	c64a0000 */ 	lwc1	$f10,0x0($s2)
/*    21970:	c7a600e4 */ 	lwc1	$f6,0xe4($sp)
/*    21974:	46000306 */ 	mov.s	$f12,$f0
/*    21978:	460a4102 */ 	mul.s	$f4,$f8,$f10
/*    2197c:	c6480004 */ 	lwc1	$f8,0x4($s2)
/*    21980:	46083282 */ 	mul.s	$f10,$f6,$f8
/*    21984:	c6480008 */ 	lwc1	$f8,0x8($s2)
/*    21988:	460a2180 */ 	add.s	$f6,$f4,$f10
/*    2198c:	c7a400e8 */ 	lwc1	$f4,0xe8($sp)
/*    21990:	46044282 */ 	mul.s	$f10,$f8,$f4
/*    21994:	44804000 */ 	mtc1	$zero,$f8
/*    21998:	46065080 */ 	add.s	$f2,$f10,$f6
/*    2199c:	46001087 */ 	neg.s	$f2,$f2
/*    219a0:	4608103c */ 	c.lt.s	$f2,$f8
/*    219a4:	00000000 */ 	nop
/*    219a8:	45000003 */ 	bc1f	.L000219b8
/*    219ac:	3c017005 */ 	lui	$at,%hi(var70054454)
/*    219b0:	c4244454 */ 	lwc1	$f4,%lo(var70054454)($at)
/*    219b4:	46002301 */ 	sub.s	$f12,$f4,$f0
.L000219b8:
/*    219b8:	0c0068f4 */ 	jal	cosf
/*    219bc:	e7ac00f0 */ 	swc1	$f12,0xf0($sp)
/*    219c0:	c7ac00f0 */ 	lwc1	$f12,0xf0($sp)
/*    219c4:	0c0068f7 */ 	jal	sinf
/*    219c8:	e7a000dc */ 	swc1	$f0,0xdc($sp)
/*    219cc:	e7a000d8 */ 	swc1	$f0,0xd8($sp)
/*    219d0:	0c0068f4 */ 	jal	cosf
/*    219d4:	c7ac00ec */ 	lwc1	$f12,0xec($sp)
/*    219d8:	e7a000d4 */ 	swc1	$f0,0xd4($sp)
/*    219dc:	0c0068f7 */ 	jal	sinf
/*    219e0:	c7ac00ec */ 	lwc1	$f12,0xec($sp)
/*    219e4:	0c004b70 */ 	jal	random
/*    219e8:	e7a000d0 */ 	swc1	$f0,0xd0($sp)
/*    219ec:	304d007f */ 	andi	$t5,$v0,0x7f
/*    219f0:	448d5000 */ 	mtc1	$t5,$f10
/*    219f4:	3c013f00 */ 	lui	$at,0x3f00
/*    219f8:	44810000 */ 	mtc1	$at,$f0
/*    219fc:	c7ae00dc */ 	lwc1	$f14,0xdc($sp)
/*    21a00:	c7b000d8 */ 	lwc1	$f16,0xd8($sp)
/*    21a04:	c7b200d4 */ 	lwc1	$f18,0xd4($sp)
/*    21a08:	05a10005 */ 	bgez	$t5,.L00021a20
/*    21a0c:	468051a0 */ 	cvt.s.w	$f6,$f10
/*    21a10:	3c014f80 */ 	lui	$at,0x4f80
/*    21a14:	44814000 */ 	mtc1	$at,$f8
/*    21a18:	00000000 */ 	nop
/*    21a1c:	46083180 */ 	add.s	$f6,$f6,$f8
.L00021a20:
/*    21a20:	3c013b80 */ 	lui	$at,0x3b80
/*    21a24:	44812000 */ 	mtc1	$at,$f4
/*    21a28:	3c013f40 */ 	lui	$at,0x3f40
/*    21a2c:	44814000 */ 	mtc1	$at,$f8
/*    21a30:	46043282 */ 	mul.s	$f10,$f6,$f4
/*    21a34:	c626000c */ 	lwc1	$f6,0xc($s1)
/*    21a38:	3c198006 */ 	lui	$t9,%hi(g_ModelVtxAllocatorFunc)
/*    21a3c:	8f39efc8 */ 	lw	$t9,%lo(g_ModelVtxAllocatorFunc)($t9)
/*    21a40:	24040004 */ 	addiu	$a0,$zero,0x4
/*    21a44:	46085080 */ 	add.s	$f2,$f10,$f8
/*    21a48:	46023102 */ 	mul.s	$f4,$f6,$f2
/*    21a4c:	e7a4009c */ 	swc1	$f4,0x9c($sp)
/*    21a50:	c62a0010 */ 	lwc1	$f10,0x10($s1)
/*    21a54:	46025202 */ 	mul.s	$f8,$f10,$f2
/*    21a58:	c7aa009c */ 	lwc1	$f10,0x9c($sp)
/*    21a5c:	e7a800a0 */ 	swc1	$f8,0xa0($sp)
/*    21a60:	c6260014 */ 	lwc1	$f6,0x14($s1)
/*    21a64:	46023102 */ 	mul.s	$f4,$f6,$f2
/*    21a68:	00000000 */ 	nop
/*    21a6c:	460e5202 */ 	mul.s	$f8,$f10,$f14
/*    21a70:	e7a400a4 */ 	swc1	$f4,0xa4($sp)
/*    21a74:	c7a400a4 */ 	lwc1	$f4,0xa4($sp)
/*    21a78:	46004182 */ 	mul.s	$f6,$f8,$f0
/*    21a7c:	00000000 */ 	nop
/*    21a80:	46102202 */ 	mul.s	$f8,$f4,$f16
/*    21a84:	e7a600cc */ 	swc1	$f6,0xcc($sp)
/*    21a88:	46004182 */ 	mul.s	$f6,$f8,$f0
/*    21a8c:	c7a800a0 */ 	lwc1	$f8,0xa0($sp)
/*    21a90:	e7a600c8 */ 	swc1	$f6,0xc8($sp)
/*    21a94:	c7a600d0 */ 	lwc1	$f6,0xd0($sp)
/*    21a98:	46064202 */ 	mul.s	$f8,$f8,$f6
/*    21a9c:	00000000 */ 	nop
/*    21aa0:	46004302 */ 	mul.s	$f12,$f8,$f0
/*    21aa4:	00000000 */ 	nop
/*    21aa8:	46125182 */ 	mul.s	$f6,$f10,$f18
/*    21aac:	00000000 */ 	nop
/*    21ab0:	46103202 */ 	mul.s	$f8,$f6,$f16
/*    21ab4:	00000000 */ 	nop
/*    21ab8:	46004182 */ 	mul.s	$f6,$f8,$f0
/*    21abc:	00000000 */ 	nop
/*    21ac0:	46122202 */ 	mul.s	$f8,$f4,$f18
/*    21ac4:	e7a600c0 */ 	swc1	$f6,0xc0($sp)
/*    21ac8:	460e4182 */ 	mul.s	$f6,$f8,$f14
/*    21acc:	00000000 */ 	nop
/*    21ad0:	46003102 */ 	mul.s	$f4,$f6,$f0
/*    21ad4:	e7a400bc */ 	swc1	$f4,0xbc($sp)
/*    21ad8:	46005182 */ 	mul.s	$f6,$f10,$f0
/*    21adc:	c6280000 */ 	lwc1	$f8,0x0($s1)
/*    21ae0:	46064101 */ 	sub.s	$f4,$f8,$f6
/*    21ae4:	e7a40090 */ 	swc1	$f4,0x90($sp)
/*    21ae8:	c62a0004 */ 	lwc1	$f10,0x4($s1)
/*    21aec:	e7aa0094 */ 	swc1	$f10,0x94($sp)
/*    21af0:	c6280008 */ 	lwc1	$f8,0x8($s1)
/*    21af4:	e7ac00c4 */ 	swc1	$f12,0xc4($sp)
/*    21af8:	0320f809 */ 	jalr	$t9
/*    21afc:	e7a80098 */ 	swc1	$f8,0x98($sp)
/*    21b00:	00408025 */ 	or	$s0,$v0,$zero
/*    21b04:	0fc59e73 */ 	jal	gfxAllocateColours
/*    21b08:	24040001 */ 	addiu	$a0,$zero,0x1
/*    21b0c:	27a30084 */ 	addiu	$v1,$sp,0x84
/*    21b10:	afa20078 */ 	sw	$v0,0x78($sp)
/*    21b14:	8c610000 */ 	lw	$at,0x0($v1)
/*    21b18:	c7ac00c4 */ 	lwc1	$f12,0xc4($sp)
/*    21b1c:	aa010000 */ 	swl	$at,0x0($s0)
/*    21b20:	ba010003 */ 	swr	$at,0x3($s0)
/*    21b24:	8c780004 */ 	lw	$t8,0x4($v1)
/*    21b28:	aa180004 */ 	swl	$t8,0x4($s0)
/*    21b2c:	ba180007 */ 	swr	$t8,0x7($s0)
/*    21b30:	8c610008 */ 	lw	$at,0x8($v1)
/*    21b34:	aa010008 */ 	swl	$at,0x8($s0)
/*    21b38:	ba01000b */ 	swr	$at,0xb($s0)
/*    21b3c:	8c610000 */ 	lw	$at,0x0($v1)
/*    21b40:	aa01000c */ 	swl	$at,0xc($s0)
/*    21b44:	ba01000f */ 	swr	$at,0xf($s0)
/*    21b48:	8c6d0004 */ 	lw	$t5,0x4($v1)
/*    21b4c:	aa0d0010 */ 	swl	$t5,0x10($s0)
/*    21b50:	ba0d0013 */ 	swr	$t5,0x13($s0)
/*    21b54:	8c610008 */ 	lw	$at,0x8($v1)
/*    21b58:	27ad0080 */ 	addiu	$t5,$sp,0x80
/*    21b5c:	aa010014 */ 	swl	$at,0x14($s0)
/*    21b60:	ba010017 */ 	swr	$at,0x17($s0)
/*    21b64:	8c610000 */ 	lw	$at,0x0($v1)
/*    21b68:	aa010018 */ 	swl	$at,0x18($s0)
/*    21b6c:	ba01001b */ 	swr	$at,0x1b($s0)
/*    21b70:	8c6e0004 */ 	lw	$t6,0x4($v1)
/*    21b74:	aa0e001c */ 	swl	$t6,0x1c($s0)
/*    21b78:	ba0e001f */ 	swr	$t6,0x1f($s0)
/*    21b7c:	8c610008 */ 	lw	$at,0x8($v1)
/*    21b80:	aa010020 */ 	swl	$at,0x20($s0)
/*    21b84:	ba010023 */ 	swr	$at,0x23($s0)
/*    21b88:	8c610000 */ 	lw	$at,0x0($v1)
/*    21b8c:	aa010024 */ 	swl	$at,0x24($s0)
/*    21b90:	ba010027 */ 	swr	$at,0x27($s0)
/*    21b94:	8c6c0004 */ 	lw	$t4,0x4($v1)
/*    21b98:	aa0c0028 */ 	swl	$t4,0x28($s0)
/*    21b9c:	ba0c002b */ 	swr	$t4,0x2b($s0)
/*    21ba0:	8c610008 */ 	lw	$at,0x8($v1)
/*    21ba4:	aa01002c */ 	swl	$at,0x2c($s0)
/*    21ba8:	ba01002f */ 	swr	$at,0x2f($s0)
/*    21bac:	8da10000 */ 	lw	$at,0x0($t5)
/*    21bb0:	ac410000 */ 	sw	$at,0x0($v0)
/*    21bb4:	c7a000cc */ 	lwc1	$f0,0xcc($sp)
/*    21bb8:	c7a60090 */ 	lwc1	$f6,0x90($sp)
/*    21bbc:	c7ae00c0 */ 	lwc1	$f14,0xc0($sp)
/*    21bc0:	46000007 */ 	neg.s	$f0,$f0
/*    21bc4:	46003100 */ 	add.s	$f4,$f6,$f0
/*    21bc8:	46007387 */ 	neg.s	$f14,$f14
/*    21bcc:	460e2280 */ 	add.s	$f10,$f4,$f14
/*    21bd0:	4600520d */ 	trunc.w.s	$f8,$f10
/*    21bd4:	44184000 */ 	mfc1	$t8,$f8
/*    21bd8:	00000000 */ 	nop
/*    21bdc:	a6180000 */ 	sh	$t8,0x0($s0)
/*    21be0:	c7a60094 */ 	lwc1	$f6,0x94($sp)
/*    21be4:	460c3101 */ 	sub.s	$f4,$f6,$f12
/*    21be8:	4600228d */ 	trunc.w.s	$f10,$f4
/*    21bec:	44195000 */ 	mfc1	$t9,$f10
/*    21bf0:	00000000 */ 	nop
/*    21bf4:	a6190002 */ 	sh	$t9,0x2($s0)
/*    21bf8:	c7a200c8 */ 	lwc1	$f2,0xc8($sp)
/*    21bfc:	c7a80098 */ 	lwc1	$f8,0x98($sp)
/*    21c00:	c7b000bc */ 	lwc1	$f16,0xbc($sp)
/*    21c04:	46001087 */ 	neg.s	$f2,$f2
/*    21c08:	46024181 */ 	sub.s	$f6,$f8,$f2
/*    21c0c:	46008407 */ 	neg.s	$f16,$f16
/*    21c10:	46103100 */ 	add.s	$f4,$f6,$f16
/*    21c14:	4600228d */ 	trunc.w.s	$f10,$f4
/*    21c18:	440d5000 */ 	mfc1	$t5,$f10
/*    21c1c:	00000000 */ 	nop
/*    21c20:	a60d0004 */ 	sh	$t5,0x4($s0)
/*    21c24:	c7a80090 */ 	lwc1	$f8,0x90($sp)
/*    21c28:	46004180 */ 	add.s	$f6,$f8,$f0
/*    21c2c:	460e3101 */ 	sub.s	$f4,$f6,$f14
/*    21c30:	4600228d */ 	trunc.w.s	$f10,$f4
/*    21c34:	440f5000 */ 	mfc1	$t7,$f10
/*    21c38:	00000000 */ 	nop
/*    21c3c:	a60f000c */ 	sh	$t7,0xc($s0)
/*    21c40:	c7a80094 */ 	lwc1	$f8,0x94($sp)
/*    21c44:	460c4180 */ 	add.s	$f6,$f8,$f12
/*    21c48:	4600310d */ 	trunc.w.s	$f4,$f6
/*    21c4c:	440c2000 */ 	mfc1	$t4,$f4
/*    21c50:	00000000 */ 	nop
/*    21c54:	a60c000e */ 	sh	$t4,0xe($s0)
/*    21c58:	c7aa0098 */ 	lwc1	$f10,0x98($sp)
/*    21c5c:	46025201 */ 	sub.s	$f8,$f10,$f2
/*    21c60:	46104181 */ 	sub.s	$f6,$f8,$f16
/*    21c64:	4600310d */ 	trunc.w.s	$f4,$f6
/*    21c68:	440b2000 */ 	mfc1	$t3,$f4
/*    21c6c:	00000000 */ 	nop
/*    21c70:	a60b0010 */ 	sh	$t3,0x10($s0)
/*    21c74:	c7aa0090 */ 	lwc1	$f10,0x90($sp)
/*    21c78:	3c0bbc00 */ 	lui	$t3,0xbc00
/*    21c7c:	356b1406 */ 	ori	$t3,$t3,0x1406
/*    21c80:	46005201 */ 	sub.s	$f8,$f10,$f0
/*    21c84:	460e4181 */ 	sub.s	$f6,$f8,$f14
/*    21c88:	4600310d */ 	trunc.w.s	$f4,$f6
/*    21c8c:	440e2000 */ 	mfc1	$t6,$f4
/*    21c90:	00000000 */ 	nop
/*    21c94:	a60e0018 */ 	sh	$t6,0x18($s0)
/*    21c98:	c7aa0094 */ 	lwc1	$f10,0x94($sp)
/*    21c9c:	460c5200 */ 	add.s	$f8,$f10,$f12
/*    21ca0:	4600418d */ 	trunc.w.s	$f6,$f8
/*    21ca4:	44183000 */ 	mfc1	$t8,$f6
/*    21ca8:	00000000 */ 	nop
/*    21cac:	a618001a */ 	sh	$t8,0x1a($s0)
/*    21cb0:	c7a40098 */ 	lwc1	$f4,0x98($sp)
/*    21cb4:	46022280 */ 	add.s	$f10,$f4,$f2
/*    21cb8:	46105201 */ 	sub.s	$f8,$f10,$f16
/*    21cbc:	4600418d */ 	trunc.w.s	$f6,$f8
/*    21cc0:	44193000 */ 	mfc1	$t9,$f6
/*    21cc4:	00000000 */ 	nop
/*    21cc8:	a619001c */ 	sh	$t9,0x1c($s0)
/*    21ccc:	c7a40090 */ 	lwc1	$f4,0x90($sp)
/*    21cd0:	46002281 */ 	sub.s	$f10,$f4,$f0
/*    21cd4:	460e5200 */ 	add.s	$f8,$f10,$f14
/*    21cd8:	4600418d */ 	trunc.w.s	$f6,$f8
/*    21cdc:	440d3000 */ 	mfc1	$t5,$f6
/*    21ce0:	00000000 */ 	nop
/*    21ce4:	a60d0024 */ 	sh	$t5,0x24($s0)
/*    21ce8:	c7a40094 */ 	lwc1	$f4,0x94($sp)
/*    21cec:	460c2281 */ 	sub.s	$f10,$f4,$f12
/*    21cf0:	4600520d */ 	trunc.w.s	$f8,$f10
/*    21cf4:	440f4000 */ 	mfc1	$t7,$f8
/*    21cf8:	00000000 */ 	nop
/*    21cfc:	a60f0026 */ 	sh	$t7,0x26($s0)
/*    21d00:	c7a60098 */ 	lwc1	$f6,0x98($sp)
/*    21d04:	46023100 */ 	add.s	$f4,$f6,$f2
/*    21d08:	46102280 */ 	add.s	$f10,$f4,$f16
/*    21d0c:	4600520d */ 	trunc.w.s	$f8,$f10
/*    21d10:	440c4000 */ 	mfc1	$t4,$f8
/*    21d14:	00000000 */ 	nop
/*    21d18:	a60c0028 */ 	sh	$t4,0x28($s0)
/*    21d1c:	8e63000c */ 	lw	$v1,0xc($s3)
/*    21d20:	24790008 */ 	addiu	$t9,$v1,0x8
/*    21d24:	ae79000c */ 	sw	$t9,0xc($s3)
/*    21d28:	ac6b0000 */ 	sw	$t3,0x0($v1)
/*    21d2c:	8e240024 */ 	lw	$a0,0x24($s1)
/*    21d30:	0c012d20 */ 	jal	osVirtualToPhysical
/*    21d34:	afa30068 */ 	sw	$v1,0x68($sp)
/*    21d38:	8fa50068 */ 	lw	$a1,0x68($sp)
/*    21d3c:	02602025 */ 	or	$a0,$s3,$zero
/*    21d40:	aca20004 */ 	sw	$v0,0x4($a1)
/*    21d44:	8e230018 */ 	lw	$v1,0x18($s1)
/*    21d48:	00002825 */ 	or	$a1,$zero,$zero
/*    21d4c:	10600036 */ 	beqz	$v1,.L00021e28
/*    21d50:	00000000 */ 	nop
/*    21d54:	0c004b70 */ 	jal	random
/*    21d58:	00608825 */ 	or	$s1,$v1,$zero
/*    21d5c:	00026a80 */ 	sll	$t5,$v0,0xa
/*    21d60:	a7ad0062 */ 	sh	$t5,0x62($sp)
/*    21d64:	0c013ef0 */ 	jal	coss
/*    21d68:	31a4ffff */ 	andi	$a0,$t5,0xffff
/*    21d6c:	922e0004 */ 	lbu	$t6,0x4($s1)
/*    21d70:	97a40062 */ 	lhu	$a0,0x62($sp)
/*    21d74:	004e0019 */ 	multu	$v0,$t6
/*    21d78:	00004012 */ 	mflo	$t0
/*    21d7c:	00087880 */ 	sll	$t7,$t0,0x2
/*    21d80:	01e87823 */ 	subu	$t7,$t7,$t0
/*    21d84:	000f7880 */ 	sll	$t7,$t7,0x2
/*    21d88:	01e87823 */ 	subu	$t7,$t7,$t0
/*    21d8c:	000f7880 */ 	sll	$t7,$t7,0x2
/*    21d90:	01e87821 */ 	addu	$t7,$t7,$t0
/*    21d94:	000f7880 */ 	sll	$t7,$t7,0x2
/*    21d98:	01e87821 */ 	addu	$t7,$t7,$t0
/*    21d9c:	000fc483 */ 	sra	$t8,$t7,0x12
/*    21da0:	0c013efc */ 	jal	sins
/*    21da4:	afb8005c */ 	sw	$t8,0x5c($sp)
/*    21da8:	92240004 */ 	lbu	$a0,0x4($s1)
/*    21dac:	8fa8005c */ 	lw	$t0,0x5c($sp)
/*    21db0:	00440019 */ 	multu	$v0,$a0
/*    21db4:	00041900 */ 	sll	$v1,$a0,0x4
/*    21db8:	00683023 */ 	subu	$a2,$v1,$t0
/*    21dbc:	a6060008 */ 	sh	$a2,0x8($s0)
/*    21dc0:	a6060016 */ 	sh	$a2,0x16($s0)
/*    21dc4:	00685021 */ 	addu	$t2,$v1,$t0
/*    21dc8:	a60a0020 */ 	sh	$t2,0x20($s0)
/*    21dcc:	a60a002e */ 	sh	$t2,0x2e($s0)
/*    21dd0:	24060004 */ 	addiu	$a2,$zero,0x4
/*    21dd4:	02602025 */ 	or	$a0,$s3,$zero
/*    21dd8:	00002812 */ 	mflo	$a1
/*    21ddc:	00056080 */ 	sll	$t4,$a1,0x2
/*    21de0:	01856023 */ 	subu	$t4,$t4,$a1
/*    21de4:	000c6080 */ 	sll	$t4,$t4,0x2
/*    21de8:	01856023 */ 	subu	$t4,$t4,$a1
/*    21dec:	000c6080 */ 	sll	$t4,$t4,0x2
/*    21df0:	01856021 */ 	addu	$t4,$t4,$a1
/*    21df4:	000c6080 */ 	sll	$t4,$t4,0x2
/*    21df8:	01856021 */ 	addu	$t4,$t4,$a1
/*    21dfc:	000ccc83 */ 	sra	$t9,$t4,0x12
/*    21e00:	00794823 */ 	subu	$t1,$v1,$t9
/*    21e04:	00793821 */ 	addu	$a3,$v1,$t9
/*    21e08:	a609000a */ 	sh	$t1,0xa($s0)
/*    21e0c:	a6070014 */ 	sh	$a3,0x14($s0)
/*    21e10:	a6070022 */ 	sh	$a3,0x22($s0)
/*    21e14:	a609002c */ 	sh	$t1,0x2c($s0)
/*    21e18:	0c0085b3 */ 	jal	model000216cc
/*    21e1c:	02202825 */ 	or	$a1,$s1,$zero
/*    21e20:	10000004 */ 	b	.L00021e34
/*    21e24:	8e63000c */ 	lw	$v1,0xc($s3)
.L00021e28:
/*    21e28:	0c0085b3 */ 	jal	model000216cc
/*    21e2c:	24060001 */ 	addiu	$a2,$zero,0x1
/*    21e30:	8e63000c */ 	lw	$v1,0xc($s3)
.L00021e34:
/*    21e34:	3c0db700 */ 	lui	$t5,0xb700
/*    21e38:	240e2000 */ 	addiu	$t6,$zero,0x2000
/*    21e3c:	246b0008 */ 	addiu	$t3,$v1,0x8
/*    21e40:	ae6b000c */ 	sw	$t3,0xc($s3)
/*    21e44:	ac6e0004 */ 	sw	$t6,0x4($v1)
/*    21e48:	ac6d0000 */ 	sw	$t5,0x0($v1)
/*    21e4c:	8e71000c */ 	lw	$s1,0xc($s3)
/*    21e50:	3c180102 */ 	lui	$t8,0x102
/*    21e54:	37180040 */ 	ori	$t8,$t8,0x40
/*    21e58:	262f0008 */ 	addiu	$t7,$s1,0x8
/*    21e5c:	ae6f000c */ 	sw	$t7,0xc($s3)
/*    21e60:	02402025 */ 	or	$a0,$s2,$zero
/*    21e64:	0c012d20 */ 	jal	osVirtualToPhysical
/*    21e68:	ae380000 */ 	sw	$t8,0x0($s1)
/*    21e6c:	ae220004 */ 	sw	$v0,0x4($s1)
/*    21e70:	8e72000c */ 	lw	$s2,0xc($s3)
/*    21e74:	3c190700 */ 	lui	$t9,0x700
/*    21e78:	37390004 */ 	ori	$t9,$t9,0x4
/*    21e7c:	264c0008 */ 	addiu	$t4,$s2,0x8
/*    21e80:	ae6c000c */ 	sw	$t4,0xc($s3)
/*    21e84:	ae590000 */ 	sw	$t9,0x0($s2)
/*    21e88:	0c012d20 */ 	jal	osVirtualToPhysical
/*    21e8c:	8fa40078 */ 	lw	$a0,0x78($sp)
/*    21e90:	ae420004 */ 	sw	$v0,0x4($s2)
/*    21e94:	8e71000c */ 	lw	$s1,0xc($s3)
/*    21e98:	3c0d0430 */ 	lui	$t5,0x430
/*    21e9c:	35ad0030 */ 	ori	$t5,$t5,0x30
/*    21ea0:	262b0008 */ 	addiu	$t3,$s1,0x8
/*    21ea4:	ae6b000c */ 	sw	$t3,0xc($s3)
/*    21ea8:	02002025 */ 	or	$a0,$s0,$zero
/*    21eac:	0c012d20 */ 	jal	osVirtualToPhysical
/*    21eb0:	ae2d0000 */ 	sw	$t5,0x0($s1)
/*    21eb4:	ae220004 */ 	sw	$v0,0x4($s1)
/*    21eb8:	8e63000c */ 	lw	$v1,0xc($s3)
/*    21ebc:	3c0fb100 */ 	lui	$t7,0xb100
/*    21ec0:	35ef0002 */ 	ori	$t7,$t7,0x2
/*    21ec4:	246e0008 */ 	addiu	$t6,$v1,0x8
/*    21ec8:	ae6e000c */ 	sw	$t6,0xc($s3)
/*    21ecc:	24183210 */ 	addiu	$t8,$zero,0x3210
/*    21ed0:	ac780004 */ 	sw	$t8,0x4($v1)
/*    21ed4:	ac6f0000 */ 	sw	$t7,0x0($v1)
/*    21ed8:	8fbf0024 */ 	lw	$ra,0x24($sp)
.L00021edc:
/*    21edc:	8fb00014 */ 	lw	$s0,0x14($sp)
/*    21ee0:	8fb10018 */ 	lw	$s1,0x18($sp)
/*    21ee4:	8fb2001c */ 	lw	$s2,0x1c($sp)
/*    21ee8:	8fb30020 */ 	lw	$s3,0x20($sp)
/*    21eec:	03e00008 */ 	jr	$ra
/*    21ef0:	27bd0110 */ 	addiu	$sp,$sp,0x110
);

//struct tmpcolour {
//	u32 value;
//};

// Mismatch: Reordered instructions near assign to spcc
//void modelRenderNodeGunfire(struct modelrenderdata *renderdata, struct model *model, struct modelnode *node)
//{
//	struct modelrodata_gunfire *rodata = &node->rodata->gunfire;
//	union modelrwdata *rwdata = modelGetNodeRwData(model, node);
//	struct gfxvtx *vertices;
//	f32 spf0;
//	f32 spec;
//	struct coord spe0;
//	f32 spdc;
//	f32 spd8;
//	f32 spd4;
//	f32 spd0;
//	f32 spcc;
//	f32 spc8;
//	f32 spc4;
//	f32 spc0;
//	f32 spbc;
//	struct coord sp9c;
//	struct coord sp90;
//	struct gfxvtx vtxtemplate = {0}; // 84, 88, 8c
//	struct tmpcolour colourtemplate = {0xffffffff}; // 80
//	f32 scale;
//	struct tmpcolour *colours;
//	struct textureconfig *texture;
//	Mtxf *mtx;
//	f32 tmp;
//	f32 distance;
//	u16 sp62;
//	s32 sp5c;
//	s32 sp58;
//	s32 centre;
//	u32 stack[6];
//
//	// 778
//	if ((renderdata->flags & 2) && rwdata->gunfire.visible) {
//		mtx = &model->matrices[model0001a524(node, 0)];
//
//		spe0.x = -(rodata->pos.f[0] * mtx->m[0][0] + rodata->pos.f[1] * mtx->m[1][0] + rodata->pos.f[2] * mtx->m[2][0] + mtx->m[3][0]);
//		spe0.y = -(rodata->pos.f[0] * mtx->m[0][1] + rodata->pos.f[1] * mtx->m[1][1] + rodata->pos.f[2] * mtx->m[2][1] + mtx->m[3][1]);
//		spe0.z = -(rodata->pos.f[0] * mtx->m[0][2] + rodata->pos.f[1] * mtx->m[1][2] + rodata->pos.f[2] * mtx->m[2][2] + mtx->m[3][2]);
//
//		distance = sqrtf(spe0.f[0] * spe0.f[0] + spe0.f[1] * spe0.f[1] + spe0.f[2] * spe0.f[2]);
//
//		// 88c
//		if (distance > 0) {
//			f32 tmp = 1 / (model->scale * distance);
//			spe0.f[0] *= tmp;
//			spe0.f[1] *= tmp;
//			spe0.f[2] *= tmp;
//		} else {
//			spe0.f[0] = 0;
//			spe0.f[1] = 0;
//			spe0.f[2] = 1 / model->scale;
//		}
//
//		// 8f4
//		spec = acosf(spe0.f[0] * mtx->m[1][0] + spe0.f[1] * mtx->m[1][1] + spe0.f[2] * mtx->m[1][2]);
//		spf0 = acosf(-(spe0.f[0] * mtx->m[2][0] + spe0.f[1] * mtx->m[2][1] + spe0.f[2] * mtx->m[2][2]) / sinf(spec));
//
//		tmp = -(spe0.f[0] * mtx->m[0][0] + spe0.f[1] * mtx->m[0][1] + spe0.f[2] * mtx->m[0][2]);
//
//		// 9a8
//		if (tmp < 0) {
//			spf0 = M_BADTAU - spf0;
//		}
//
//		// 9b8
//		spdc = cosf(spf0);
//		spd8 = sinf(spf0);
//		spd4 = cosf(spec);
//		spd0 = sinf(spec);
//
//		scale = 0.75f + (random() % 128) * (1.0f / 256.0f); // 0.75 to 1.25
//
//		sp9c.f[0] = rodata->dim.f[0] * scale;
//		sp9c.f[1] = rodata->dim.f[1] * scale;
//		sp9c.f[2] = rodata->dim.f[2] * scale;
//
//		spcc = sp9c.f[0] * spdc * 0.5f;
//		spc8 = sp9c.f[1] * spd8 * 0.5f;
//		spc4 = sp9c.f[2] * spd0 * 0.5f;
//
//		spc0 = sp9c.f[0] * spd4 * spd8 * 0.5f;
//		spbc = sp9c.f[2] * spd4 * spdc * 0.5f;
//
//		sp90.f[0] = rodata->pos.f[0] - sp9c.f[0] * 0.5f;
//		sp90.f[1] = rodata->pos.f[1];
//		sp90.f[2] = rodata->pos.f[2];
//
//		vertices = g_ModelVtxAllocatorFunc(4);
//
//		// b04
//		colours = (struct tmpcolour *) gfxAllocateColours(1);
//
//		vertices[0] = vtxtemplate;
//		vertices[1] = vtxtemplate;
//		vertices[2] = vtxtemplate;
//		vertices[3] = vtxtemplate;
//
//		colours[0] = colourtemplate;
//
//		vertices[0].x = sp90.f[0] + -spcc + -spc0;
//		vertices[0].y = sp90.f[1] - spc4;
//		vertices[0].z = sp90.f[2] - -spc8 + -spbc;
//		vertices[1].x = sp90.f[0] + -spcc - -spc0;
//		vertices[1].y = sp90.f[1] + spc4;
//		vertices[1].z = sp90.f[2] - -spc8 - -spbc;
//		vertices[2].x = sp90.f[0] - -spcc - -spc0;
//		vertices[2].y = sp90.f[1] + spc4;
//		vertices[2].z = sp90.f[2] + -spc8 - -spbc;
//		vertices[3].x = sp90.f[0] - -spcc + -spc0;
//		vertices[3].y = sp90.f[1] - spc4;
//		vertices[3].z = sp90.f[2] + -spc8 + -spbc;
//
//		gSPSegment(renderdata->gdl++, 0x05, osVirtualToPhysical(rodata->baseaddr));
//
//		// d4c
//		if (rodata->texture) {
//			tconfig = rodata->texture;
//
//			sp62 = random() * 1024;
//			sp5c = (coss(sp62) * tconfig->width * 0xb5) >> 18;
//			sp58 = (sins(sp62) * tconfig->width * 0xb5) >> 18;
//
//			centre = tconfig->width << 4;
//
//			vertices[0].unk08 = centre - sp5c;
//			vertices[0].unk0a = centre - sp58;
//			vertices[1].unk08 = centre + sp58;
//			vertices[1].unk0a = centre - sp5c;
//			vertices[2].unk08 = centre + sp5c;
//			vertices[2].unk0a = centre + sp58;
//			vertices[3].unk08 = centre - sp58;
//			vertices[3].unk0a = centre + sp5c;
//
//			model000216cc(renderdata, tconfig, 4);
//		} else {
//			model000216cc(renderdata, NULL, 1);
//		}
//
//		gSPSetGeometryMode(renderdata->gdl++, G_CULL_BACK);
//		gSPMatrix(renderdata->gdl++, osVirtualToPhysical(mtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
//		gDPSetColorArray(renderdata->gdl++, osVirtualToPhysical(colours), 1);
//		gDPSetVerticeArray(renderdata->gdl++, osVirtualToPhysical(vertices), 4);
//		gDPTri2(renderdata->gdl++, 0, 1, 2, 2, 3, 0);
//	}
//}

void modelRender(struct modelrenderdata *renderdata, struct model *model)
{
	union modelrodata *rodata;
	union modelrwdata *rwdata;
	u32 type;
	struct modelnode *node = model->filedata->rootnode;

	gSPSegment(renderdata->gdl++, 0x03, osVirtualToPhysical(model->matrices));

	while (node) {
		type = node->type & 0xff;

		switch (type) {
		case MODELNODETYPE_DISTANCE:
		case MODELNODETYPE_TOGGLE:
			rodata = node->rodata;
			rwdata = modelGetNodeRwData(model, node);

			switch (type) {
			case MODELNODETYPE_DISTANCE:
				node->child = rwdata->distance.visible ? rodata->distance.target : NULL;
				break;
			case MODELNODETYPE_TOGGLE:
				node->child = rwdata->toggle.visible ? rodata->toggle.target : NULL;
				break;
			}
			break;
		case MODELNODETYPE_HEADSPOT:
			rwdata = modelGetNodeRwData(model, node);

			if (rwdata->headspot.modelfiledata) {
				struct modelnode *loopnode = rwdata->headspot.modelfiledata->rootnode;
				node->child = loopnode;

				while (loopnode) {
					loopnode->parent = node;
					loopnode = loopnode->next;
				}
			}
			break;
		case MODELNODETYPE_REORDER:
			modelRenderNodeReorder(model, node);
			break;
		case MODELNODETYPE_GUNFIRE:
			modelRenderNodeGunfire(renderdata, model, node);
			break;
		case MODELNODETYPE_GUNDL:
			modelRenderNodeGundl(renderdata, model, node);
			break;
		case MODELNODETYPE_DL:
			modelRenderNodeDl(renderdata, model, node);
			break;
		case MODELNODETYPE_16:
			modelRenderNodeType16(renderdata, node);
			break;
		case MODELNODETYPE_CHRINFO:
		default:
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}
}

GLOBAL_ASM(
glabel model000220fc
/*    220fc:	3c018006 */ 	lui	$at,%hi(var8005efc0)
/*    22100:	c424efc0 */ 	lwc1	$f4,%lo(var8005efc0)($at)
/*    22104:	44803000 */ 	mtc1	$zero,$f6
/*    22108:	c480000c */ 	lwc1	$f0,0xc($a0)
/*    2210c:	c4820010 */ 	lwc1	$f2,0x10($a0)
/*    22110:	46043032 */ 	c.eq.s	$f6,$f4
/*    22114:	c48c0014 */ 	lwc1	$f12,0x14($a0)
/*    22118:	c4900018 */ 	lwc1	$f16,0x18($a0)
/*    2211c:	27bdff30 */ 	addiu	$sp,$sp,-208
/*    22120:	c48e0004 */ 	lwc1	$f14,0x4($a0)
/*    22124:	c4920008 */ 	lwc1	$f18,0x8($a0)
/*    22128:	e7a40014 */ 	swc1	$f4,0x14($sp)
/*    2212c:	e7a0005c */ 	swc1	$f0,0x5c($sp)
/*    22130:	e7a20058 */ 	swc1	$f2,0x58($sp)
/*    22134:	e7ac0054 */ 	swc1	$f12,0x54($sp)
/*    22138:	4501000b */ 	bc1t	.L00022168
/*    2213c:	e7b00050 */ 	swc1	$f16,0x50($sp)
/*    22140:	46040001 */ 	sub.s	$f0,$f0,$f4
/*    22144:	46041080 */ 	add.s	$f2,$f2,$f4
/*    22148:	e7a0005c */ 	swc1	$f0,0x5c($sp)
/*    2214c:	46046301 */ 	sub.s	$f12,$f12,$f4
/*    22150:	e7a20058 */ 	swc1	$f2,0x58($sp)
/*    22154:	46048400 */ 	add.s	$f16,$f16,$f4
/*    22158:	e7ac0054 */ 	swc1	$f12,0x54($sp)
/*    2215c:	46047381 */ 	sub.s	$f14,$f14,$f4
/*    22160:	e7b00050 */ 	swc1	$f16,0x50($sp)
/*    22164:	46049480 */ 	add.s	$f18,$f18,$f4
.L00022168:
/*    22168:	c4a00000 */ 	lwc1	$f0,0x0($a1)
/*    2216c:	46000202 */ 	mul.s	$f8,$f0,$f0
/*    22170:	e7a800cc */ 	swc1	$f8,0xcc($sp)
/*    22174:	c4a20004 */ 	lwc1	$f2,0x4($a1)
/*    22178:	46021282 */ 	mul.s	$f10,$f2,$f2
/*    2217c:	e7aa00c8 */ 	swc1	$f10,0xc8($sp)
/*    22180:	c4ac0008 */ 	lwc1	$f12,0x8($a1)
/*    22184:	460c6182 */ 	mul.s	$f6,$f12,$f12
/*    22188:	e7a600c4 */ 	swc1	$f6,0xc4($sp)
/*    2218c:	c4e40008 */ 	lwc1	$f4,0x8($a3)
/*    22190:	e7a40014 */ 	swc1	$f4,0x14($sp)
/*    22194:	c4e80000 */ 	lwc1	$f8,0x0($a3)
/*    22198:	e7a80010 */ 	swc1	$f8,0x10($sp)
/*    2219c:	c4ea0004 */ 	lwc1	$f10,0x4($a3)
/*    221a0:	c7a60010 */ 	lwc1	$f6,0x10($sp)
/*    221a4:	e7aa000c */ 	swc1	$f10,0xc($sp)
/*    221a8:	46060102 */ 	mul.s	$f4,$f0,$f6
/*    221ac:	c7a8000c */ 	lwc1	$f8,0xc($sp)
/*    221b0:	46081282 */ 	mul.s	$f10,$f2,$f8
/*    221b4:	c7a80014 */ 	lwc1	$f8,0x14($sp)
/*    221b8:	460a2180 */ 	add.s	$f6,$f4,$f10
/*    221bc:	460c4102 */ 	mul.s	$f4,$f8,$f12
/*    221c0:	46062280 */ 	add.s	$f10,$f4,$f6
/*    221c4:	e7aa00b4 */ 	swc1	$f10,0xb4($sp)
/*    221c8:	c4a40038 */ 	lwc1	$f4,0x38($a1)
/*    221cc:	c4c80008 */ 	lwc1	$f8,0x8($a2)
/*    221d0:	46044181 */ 	sub.s	$f6,$f8,$f4
/*    221d4:	e7a60008 */ 	swc1	$f6,0x8($sp)
/*    221d8:	c4a40030 */ 	lwc1	$f4,0x30($a1)
/*    221dc:	c4c80000 */ 	lwc1	$f8,0x0($a2)
/*    221e0:	46044181 */ 	sub.s	$f6,$f8,$f4
/*    221e4:	e7a60004 */ 	swc1	$f6,0x4($sp)
/*    221e8:	c4a40034 */ 	lwc1	$f4,0x34($a1)
/*    221ec:	c4c80004 */ 	lwc1	$f8,0x4($a2)
/*    221f0:	46044181 */ 	sub.s	$f6,$f8,$f4
/*    221f4:	c7a80004 */ 	lwc1	$f8,0x4($sp)
/*    221f8:	e7a60000 */ 	swc1	$f6,0x0($sp)
/*    221fc:	46080102 */ 	mul.s	$f4,$f0,$f8
/*    22200:	c7a60000 */ 	lwc1	$f6,0x0($sp)
/*    22204:	46061202 */ 	mul.s	$f8,$f2,$f6
/*    22208:	44801000 */ 	mtc1	$zero,$f2
/*    2220c:	00000000 */ 	nop
/*    22210:	4602503c */ 	c.lt.s	$f10,$f2
/*    22214:	46082180 */ 	add.s	$f6,$f4,$f8
/*    22218:	c7a40008 */ 	lwc1	$f4,0x8($sp)
/*    2221c:	460c2202 */ 	mul.s	$f8,$f4,$f12
/*    22220:	c7a400cc */ 	lwc1	$f4,0xcc($sp)
/*    22224:	46064400 */ 	add.s	$f16,$f8,$f6
/*    22228:	c7a800c8 */ 	lwc1	$f8,0xc8($sp)
/*    2222c:	46082180 */ 	add.s	$f6,$f4,$f8
/*    22230:	c7a400c4 */ 	lwc1	$f4,0xc4($sp)
/*    22234:	46043000 */ 	add.s	$f0,$f6,$f4
/*    22238:	46000007 */ 	neg.s	$f0,$f0
/*    2223c:	46120202 */ 	mul.s	$f8,$f0,$f18
/*    22240:	46088180 */ 	add.s	$f6,$f16,$f8
/*    22244:	460e0202 */ 	mul.s	$f8,$f0,$f14
/*    22248:	46003107 */ 	neg.s	$f4,$f6
/*    2224c:	e7a400ac */ 	swc1	$f4,0xac($sp)
/*    22250:	c7ac00ac */ 	lwc1	$f12,0xac($sp)
/*    22254:	46088400 */ 	add.s	$f16,$f16,$f8
/*    22258:	45000006 */ 	bc1f	.L00022274
/*    2225c:	46008407 */ 	neg.s	$f16,$f16
/*    22260:	46005187 */ 	neg.s	$f6,$f10
/*    22264:	46006307 */ 	neg.s	$f12,$f12
/*    22268:	e7a600b4 */ 	swc1	$f6,0xb4($sp)
/*    2226c:	46008407 */ 	neg.s	$f16,$f16
/*    22270:	e7ac00ac */ 	swc1	$f12,0xac($sp)
.L00022274:
/*    22274:	4602803c */ 	c.lt.s	$f16,$f2
/*    22278:	c7ac00ac */ 	lwc1	$f12,0xac($sp)
/*    2227c:	45020008 */ 	bc1fl	.L000222a0
/*    22280:	4610603c */ 	c.lt.s	$f12,$f16
/*    22284:	4602603c */ 	c.lt.s	$f12,$f2
/*    22288:	00000000 */ 	nop
/*    2228c:	45020004 */ 	bc1fl	.L000222a0
/*    22290:	4610603c */ 	c.lt.s	$f12,$f16
/*    22294:	100000cd */ 	b	.L000225cc
/*    22298:	00001025 */ 	or	$v0,$zero,$zero
/*    2229c:	4610603c */ 	c.lt.s	$f12,$f16
.L000222a0:
/*    222a0:	e7ac00ac */ 	swc1	$f12,0xac($sp)
/*    222a4:	45020005 */ 	bc1fl	.L000222bc
/*    222a8:	c4a00010 */ 	lwc1	$f0,0x10($a1)
/*    222ac:	46008006 */ 	mov.s	$f0,$f16
/*    222b0:	46006406 */ 	mov.s	$f16,$f12
/*    222b4:	e7a000ac */ 	swc1	$f0,0xac($sp)
/*    222b8:	c4a00010 */ 	lwc1	$f0,0x10($a1)
.L000222bc:
/*    222bc:	c7a60010 */ 	lwc1	$f6,0x10($sp)
/*    222c0:	46000102 */ 	mul.s	$f4,$f0,$f0
/*    222c4:	e7a400a0 */ 	swc1	$f4,0xa0($sp)
/*    222c8:	c4a20014 */ 	lwc1	$f2,0x14($a1)
/*    222cc:	46021202 */ 	mul.s	$f8,$f2,$f2
/*    222d0:	e7a8009c */ 	swc1	$f8,0x9c($sp)
/*    222d4:	c4ac0018 */ 	lwc1	$f12,0x18($a1)
/*    222d8:	c7a8000c */ 	lwc1	$f8,0xc($sp)
/*    222dc:	460c6282 */ 	mul.s	$f10,$f12,$f12
/*    222e0:	e7aa0098 */ 	swc1	$f10,0x98($sp)
/*    222e4:	46060102 */ 	mul.s	$f4,$f0,$f6
/*    222e8:	00000000 */ 	nop
/*    222ec:	46081282 */ 	mul.s	$f10,$f2,$f8
/*    222f0:	c7a80014 */ 	lwc1	$f8,0x14($sp)
/*    222f4:	460a2180 */ 	add.s	$f6,$f4,$f10
/*    222f8:	460c4102 */ 	mul.s	$f4,$f8,$f12
/*    222fc:	c7aa0004 */ 	lwc1	$f10,0x4($sp)
/*    22300:	460a0202 */ 	mul.s	$f8,$f0,$f10
/*    22304:	46062480 */ 	add.s	$f18,$f4,$f6
/*    22308:	c7a40000 */ 	lwc1	$f4,0x0($sp)
/*    2230c:	46041182 */ 	mul.s	$f6,$f2,$f4
/*    22310:	c7a40008 */ 	lwc1	$f4,0x8($sp)
/*    22314:	46064280 */ 	add.s	$f10,$f8,$f6
/*    22318:	460c2202 */ 	mul.s	$f8,$f4,$f12
/*    2231c:	c7a4009c */ 	lwc1	$f4,0x9c($sp)
/*    22320:	c7a600a0 */ 	lwc1	$f6,0xa0($sp)
/*    22324:	44806000 */ 	mtc1	$zero,$f12
/*    22328:	460a4380 */ 	add.s	$f14,$f8,$f10
/*    2232c:	c7aa0098 */ 	lwc1	$f10,0x98($sp)
/*    22330:	46043200 */ 	add.s	$f8,$f6,$f4
/*    22334:	c7a60058 */ 	lwc1	$f6,0x58($sp)
/*    22338:	460a4000 */ 	add.s	$f0,$f8,$f10
/*    2233c:	c7a8005c */ 	lwc1	$f8,0x5c($sp)
/*    22340:	46000007 */ 	neg.s	$f0,$f0
/*    22344:	46060102 */ 	mul.s	$f4,$f0,$f6
/*    22348:	44803000 */ 	mtc1	$zero,$f6
/*    2234c:	46080282 */ 	mul.s	$f10,$f0,$f8
/*    22350:	4606903c */ 	c.lt.s	$f18,$f6
/*    22354:	46047080 */ 	add.s	$f2,$f14,$f4
/*    22358:	c7a400b4 */ 	lwc1	$f4,0xb4($sp)
/*    2235c:	460a7380 */ 	add.s	$f14,$f14,$f10
/*    22360:	46001087 */ 	neg.s	$f2,$f2
/*    22364:	45000004 */ 	bc1f	.L00022378
/*    22368:	46007387 */ 	neg.s	$f14,$f14
/*    2236c:	46009487 */ 	neg.s	$f18,$f18
/*    22370:	46007387 */ 	neg.s	$f14,$f14
/*    22374:	46001087 */ 	neg.s	$f2,$f2
.L00022378:
/*    22378:	460c703c */ 	c.lt.s	$f14,$f12
/*    2237c:	00000000 */ 	nop
/*    22380:	45020008 */ 	bc1fl	.L000223a4
/*    22384:	460e103c */ 	c.lt.s	$f2,$f14
/*    22388:	460c103c */ 	c.lt.s	$f2,$f12
/*    2238c:	00000000 */ 	nop
/*    22390:	45020004 */ 	bc1fl	.L000223a4
/*    22394:	460e103c */ 	c.lt.s	$f2,$f14
/*    22398:	1000008c */ 	b	.L000225cc
/*    2239c:	00001025 */ 	or	$v0,$zero,$zero
/*    223a0:	460e103c */ 	c.lt.s	$f2,$f14
.L000223a4:
/*    223a4:	c7aa00ac */ 	lwc1	$f10,0xac($sp)
/*    223a8:	45000004 */ 	bc1f	.L000223bc
/*    223ac:	00000000 */ 	nop
/*    223b0:	46007006 */ 	mov.s	$f0,$f14
/*    223b4:	46001386 */ 	mov.s	$f14,$f2
/*    223b8:	46000086 */ 	mov.s	$f2,$f0
.L000223bc:
/*    223bc:	46047302 */ 	mul.s	$f12,$f14,$f4
/*    223c0:	00000000 */ 	nop
/*    223c4:	46128002 */ 	mul.s	$f0,$f16,$f18
/*    223c8:	4600603c */ 	c.lt.s	$f12,$f0
/*    223cc:	00000000 */ 	nop
/*    223d0:	4500000f */ 	bc1f	.L00022410
/*    223d4:	00000000 */ 	nop
/*    223d8:	46041302 */ 	mul.s	$f12,$f2,$f4
/*    223dc:	46002386 */ 	mov.s	$f14,$f4
/*    223e0:	4600603c */ 	c.lt.s	$f12,$f0
/*    223e4:	00000000 */ 	nop
/*    223e8:	45020004 */ 	bc1fl	.L000223fc
/*    223ec:	c7a800ac */ 	lwc1	$f8,0xac($sp)
/*    223f0:	10000076 */ 	b	.L000225cc
/*    223f4:	00001025 */ 	or	$v0,$zero,$zero
/*    223f8:	c7a800ac */ 	lwc1	$f8,0xac($sp)
.L000223fc:
/*    223fc:	e7b00074 */ 	swc1	$f16,0x74($sp)
/*    22400:	e7ae0070 */ 	swc1	$f14,0x70($sp)
/*    22404:	46124002 */ 	mul.s	$f0,$f8,$f18
/*    22408:	1000000e */ 	b	.L00022444
/*    2240c:	460c003c */ 	c.lt.s	$f0,$f12
.L00022410:
/*    22410:	46125002 */ 	mul.s	$f0,$f10,$f18
/*    22414:	460c003c */ 	c.lt.s	$f0,$f12
/*    22418:	00000000 */ 	nop
/*    2241c:	45020004 */ 	bc1fl	.L00022430
/*    22420:	e7ae0074 */ 	swc1	$f14,0x74($sp)
/*    22424:	10000069 */ 	b	.L000225cc
/*    22428:	00001025 */ 	or	$v0,$zero,$zero
/*    2242c:	e7ae0074 */ 	swc1	$f14,0x74($sp)
.L00022430:
/*    22430:	c7ae00b4 */ 	lwc1	$f14,0xb4($sp)
/*    22434:	e7b20070 */ 	swc1	$f18,0x70($sp)
/*    22438:	460e1302 */ 	mul.s	$f12,$f2,$f14
/*    2243c:	00000000 */ 	nop
/*    22440:	460c003c */ 	c.lt.s	$f0,$f12
.L00022444:
/*    22444:	c7a400ac */ 	lwc1	$f4,0xac($sp)
/*    22448:	45020005 */ 	bc1fl	.L00022460
/*    2244c:	e7a2006c */ 	swc1	$f2,0x6c($sp)
/*    22450:	e7a4006c */ 	swc1	$f4,0x6c($sp)
/*    22454:	10000003 */ 	b	.L00022464
/*    22458:	e7ae0068 */ 	swc1	$f14,0x68($sp)
/*    2245c:	e7a2006c */ 	swc1	$f2,0x6c($sp)
.L00022460:
/*    22460:	e7b20068 */ 	swc1	$f18,0x68($sp)
.L00022464:
/*    22464:	c4a00020 */ 	lwc1	$f0,0x20($a1)
/*    22468:	c4a20024 */ 	lwc1	$f2,0x24($a1)
/*    2246c:	c7a60010 */ 	lwc1	$f6,0x10($sp)
/*    22470:	46000482 */ 	mul.s	$f18,$f0,$f0
/*    22474:	00000000 */ 	nop
/*    22478:	46021202 */ 	mul.s	$f8,$f2,$f2
/*    2247c:	e7a80040 */ 	swc1	$f8,0x40($sp)
/*    22480:	c4ac0028 */ 	lwc1	$f12,0x28($a1)
/*    22484:	c7a8000c */ 	lwc1	$f8,0xc($sp)
/*    22488:	460c6282 */ 	mul.s	$f10,$f12,$f12
/*    2248c:	e7aa003c */ 	swc1	$f10,0x3c($sp)
/*    22490:	46060102 */ 	mul.s	$f4,$f0,$f6
/*    22494:	00000000 */ 	nop
/*    22498:	46081282 */ 	mul.s	$f10,$f2,$f8
/*    2249c:	c7a80014 */ 	lwc1	$f8,0x14($sp)
/*    224a0:	460a2180 */ 	add.s	$f6,$f4,$f10
/*    224a4:	460c4102 */ 	mul.s	$f4,$f8,$f12
/*    224a8:	c7aa0004 */ 	lwc1	$f10,0x4($sp)
/*    224ac:	460a0202 */ 	mul.s	$f8,$f0,$f10
/*    224b0:	46062400 */ 	add.s	$f16,$f4,$f6
/*    224b4:	c7a40000 */ 	lwc1	$f4,0x0($sp)
/*    224b8:	46041182 */ 	mul.s	$f6,$f2,$f4
/*    224bc:	c7a40008 */ 	lwc1	$f4,0x8($sp)
/*    224c0:	46064280 */ 	add.s	$f10,$f8,$f6
/*    224c4:	460c2202 */ 	mul.s	$f8,$f4,$f12
/*    224c8:	c7a60040 */ 	lwc1	$f6,0x40($sp)
/*    224cc:	46069100 */ 	add.s	$f4,$f18,$f6
/*    224d0:	460a4380 */ 	add.s	$f14,$f8,$f10
/*    224d4:	c7a8003c */ 	lwc1	$f8,0x3c($sp)
/*    224d8:	c7aa0050 */ 	lwc1	$f10,0x50($sp)
/*    224dc:	46082000 */ 	add.s	$f0,$f4,$f8
/*    224e0:	c7a40054 */ 	lwc1	$f4,0x54($sp)
/*    224e4:	46000007 */ 	neg.s	$f0,$f0
/*    224e8:	460a0182 */ 	mul.s	$f6,$f0,$f10
/*    224ec:	44805000 */ 	mtc1	$zero,$f10
/*    224f0:	46040202 */ 	mul.s	$f8,$f0,$f4
/*    224f4:	460a803c */ 	c.lt.s	$f16,$f10
/*    224f8:	46067080 */ 	add.s	$f2,$f14,$f6
/*    224fc:	44803000 */ 	mtc1	$zero,$f6
/*    22500:	46087380 */ 	add.s	$f14,$f14,$f8
/*    22504:	c7a80074 */ 	lwc1	$f8,0x74($sp)
/*    22508:	46001087 */ 	neg.s	$f2,$f2
/*    2250c:	45000004 */ 	bc1f	.L00022520
/*    22510:	46007387 */ 	neg.s	$f14,$f14
/*    22514:	46008407 */ 	neg.s	$f16,$f16
/*    22518:	46007387 */ 	neg.s	$f14,$f14
/*    2251c:	46001087 */ 	neg.s	$f2,$f2
.L00022520:
/*    22520:	4606703c */ 	c.lt.s	$f14,$f6
/*    22524:	c7ac0070 */ 	lwc1	$f12,0x70($sp)
/*    22528:	4502000a */ 	bc1fl	.L00022554
/*    2252c:	460e103c */ 	c.lt.s	$f2,$f14
/*    22530:	44802000 */ 	mtc1	$zero,$f4
/*    22534:	00000000 */ 	nop
/*    22538:	4604103c */ 	c.lt.s	$f2,$f4
/*    2253c:	00000000 */ 	nop
/*    22540:	45020004 */ 	bc1fl	.L00022554
/*    22544:	460e103c */ 	c.lt.s	$f2,$f14
/*    22548:	10000020 */ 	b	.L000225cc
/*    2254c:	00001025 */ 	or	$v0,$zero,$zero
/*    22550:	460e103c */ 	c.lt.s	$f2,$f14
.L00022554:
/*    22554:	c7a6006c */ 	lwc1	$f6,0x6c($sp)
/*    22558:	45000004 */ 	bc1f	.L0002256c
/*    2255c:	00000000 */ 	nop
/*    22560:	46007006 */ 	mov.s	$f0,$f14
/*    22564:	46001386 */ 	mov.s	$f14,$f2
/*    22568:	46000086 */ 	mov.s	$f2,$f0
.L0002256c:
/*    2256c:	46104002 */ 	mul.s	$f0,$f8,$f16
/*    22570:	c7a80068 */ 	lwc1	$f8,0x68($sp)
/*    22574:	460c7282 */ 	mul.s	$f10,$f14,$f12
/*    22578:	460a003c */ 	c.lt.s	$f0,$f10
/*    2257c:	00000000 */ 	nop
/*    22580:	4500000a */ 	bc1f	.L000225ac
/*    22584:	00000000 */ 	nop
/*    22588:	46103102 */ 	mul.s	$f4,$f6,$f16
/*    2258c:	00000000 */ 	nop
/*    22590:	46087282 */ 	mul.s	$f10,$f14,$f8
/*    22594:	460a203c */ 	c.lt.s	$f4,$f10
/*    22598:	00000000 */ 	nop
/*    2259c:	4500000a */ 	bc1f	.L000225c8
/*    225a0:	00000000 */ 	nop
/*    225a4:	10000009 */ 	b	.L000225cc
/*    225a8:	00001025 */ 	or	$v0,$zero,$zero
.L000225ac:
/*    225ac:	460c1182 */ 	mul.s	$f6,$f2,$f12
/*    225b0:	4600303c */ 	c.lt.s	$f6,$f0
/*    225b4:	00000000 */ 	nop
/*    225b8:	45020004 */ 	bc1fl	.L000225cc
/*    225bc:	24020001 */ 	addiu	$v0,$zero,0x1
/*    225c0:	10000002 */ 	b	.L000225cc
/*    225c4:	00001025 */ 	or	$v0,$zero,$zero
.L000225c8:
/*    225c8:	24020001 */ 	addiu	$v0,$zero,0x1
.L000225cc:
/*    225cc:	03e00008 */ 	jr	$ra
/*    225d0:	27bd00d0 */ 	addiu	$sp,$sp,0xd0
);

s32 model000225d4(struct model *model, struct coord *arg1, struct coord *arg2, struct modelnode **startnode)
{
	struct modelnode *node;
	bool dochildren = true;
	Mtxf *mtx;
	union modelrodata *rodata;
	union modelrwdata *rwdata;
	u32 type;

	if (model);

	if (*startnode) {
		node = *startnode;
		*startnode = NULL;
	} else {
		node = model->filedata->rootnode;
	}

	while (node) {
		if (dochildren && node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}

			if (!node) {
				break;
			}
		}

		dochildren = true;
		type = node->type & 0xff;

		switch (type) {
		case MODELNODETYPE_BBOX:
			rodata = node->rodata;
			mtx = model0001a5cc(model, node, 0);

			if (model000220fc(&rodata->bbox, mtx, arg1, arg2)) {
				*startnode = node;
				return rodata->bbox.hitpart;
			}

			dochildren = false;
			break;
		case MODELNODETYPE_DISTANCE:
			rodata = node->rodata;
			rwdata = modelGetNodeRwData(model, node);
			node->child = rwdata->distance.visible ? rodata->distance.target : NULL;
			break;
		case MODELNODETYPE_TOGGLE:
			rodata = node->rodata;
			rwdata = modelGetNodeRwData(model, node);
			node->child = rwdata->toggle.visible ? rodata->toggle.target : NULL;
			break;
		case MODELNODETYPE_HEADSPOT:
			rwdata = modelGetNodeRwData(model, node);

			if (rwdata->headspot.modelfiledata) {
				struct modelnode *loopnode = rwdata->headspot.modelfiledata->rootnode;

				node->child = loopnode;

				while (loopnode) {
					loopnode->parent = node;
					loopnode = loopnode->next;
				}
			}
			break;
		case MODELNODETYPE_CHRINFO:
		case MODELNODETYPE_DL:
		default:
			break;
		}
	}

	return 0;
}

#define PROMOTE(var) \
	if (var) \
		var = (void *)((u32)var + diff)

void modelPromoteNodeOffsetsToPointers(struct modelnode *node, u32 vma, u32 fileramaddr)
{
	union modelrodata *rodata;
	s32 diff = fileramaddr - vma;

	while (node) {
		u32 type = node->type & 0xff;

		PROMOTE(node->rodata);
		PROMOTE(node->parent);
		PROMOTE(node->next);
		PROMOTE(node->prev);
		PROMOTE(node->child);

		switch (type) {
		case MODELNODETYPE_CHRINFO:
			break;
		case MODELNODETYPE_POSITION:
			break;
		case MODELNODETYPE_GUNDL:
			rodata = node->rodata;
			PROMOTE(rodata->gundl.vertices);
			rodata->gundl.baseaddr = (void *)fileramaddr;
			break;
		case MODELNODETYPE_DL:
			rodata = node->rodata;
			PROMOTE(rodata->dl.vertices);
			rodata->dl.colourtable = (void *)fileramaddr;
			break;
		case MODELNODETYPE_DISTANCE:
			rodata = node->rodata;
			PROMOTE(rodata->distance.target);
			node->child = rodata->distance.target;
			break;
		case MODELNODETYPE_TOGGLE:
			rodata = node->rodata;
			PROMOTE(rodata->toggle.target);
			break;
		case MODELNODETYPE_REORDER:
			rodata = node->rodata;
			PROMOTE(rodata->reorder.unk18);
			PROMOTE(rodata->reorder.unk1c);
			break;
		case MODELNODETYPE_11:
			rodata = node->rodata;
			PROMOTE(rodata->type11.unk14);
			break;
		case MODELNODETYPE_0B:
			rodata = node->rodata;
			PROMOTE(rodata->type0b.unk3c);
			rodata->type0b.baseaddr = (void *)fileramaddr;
			break;
		case MODELNODETYPE_GUNFIRE:
			rodata = node->rodata;
			PROMOTE(rodata->gunfire.texture);
			rodata->gunfire.baseaddr = (void *)fileramaddr;
			break;
		case MODELNODETYPE_0D:
			rodata = node->rodata;
			PROMOTE(rodata->type0d.unk10);
			PROMOTE(rodata->type0d.unk14);
			rodata->type0d.baseaddr = (void *)fileramaddr;
			break;
		case MODELNODETYPE_16:
			rodata = node->rodata;
			PROMOTE(rodata->type16.unk04);
			rodata->type16.baseaddr = (void *)fileramaddr;
			break;
		default:
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}
}

/**
 * Convert a model file's file-relative offsets to global pointers,
 * and sort the part numbers list so they can be looked up using bisection.
 *
 * Offsets in model files are based from virtual memory address 0x0f000000.
 * This vma address is specified as an argument to the function.
 */
void modelPromoteOffsetsToPointers(struct modelfiledata *filedata, u32 vma, u32 fileramaddr)
{
	s32 diff = fileramaddr - vma;
	s32 i;
	s16 *partnums;

	PROMOTE(filedata->rootnode);
	PROMOTE(filedata->parts);
	PROMOTE(filedata->unk18);

	for (i = 0; i < filedata->numparts; i++) {
		PROMOTE(filedata->parts[i]);
	}

	modelPromoteNodeOffsetsToPointers(filedata->rootnode, vma, fileramaddr);

	// Sort parts by part number so they can be bisected during lookup
	partnums = (s16 *)&filedata->parts[filedata->numparts];

	if (filedata->numparts) {
		struct modelnode *tmpnode;
		s16 tmpnum;
		bool changed;

		do {
			changed = false;

			for (i = 0; i < filedata->numparts - 1; i++) {
				if (partnums[i] > partnums[i + 1]) {
					tmpnum = partnums[i];
					partnums[i] = partnums[i + 1];
					partnums[i + 1] = tmpnum;

					tmpnode = filedata->parts[i];
					filedata->parts[i] = filedata->parts[i + 1];
					filedata->parts[i + 1] = tmpnode;

					changed = true;
				}
			}
		} while (changed == true);
	}
}

s32 modelCalculateRwDataIndexes(struct modelnode *basenode)
{
	u16 len = 0;
	struct modelnode *node = basenode;
	union modelrodata *rodata;

	while (node) {
		u32 type = node->type & 0xff;

		switch (type) {
		case MODELNODETYPE_CHRINFO:
			rodata = node->rodata;
			rodata->chrinfo.rwdataindex = len;
			len += sizeof(struct modelrwdata_chrinfo) / 4;
			break;
		case MODELNODETYPE_DISTANCE:
			rodata = node->rodata;
			rodata->distance.rwdataindex = len;
			len += sizeof(struct modelrwdata_distance) / 4;
			node->child = rodata->distance.target;
			break;
		case MODELNODETYPE_TOGGLE:
			rodata = node->rodata;
			rodata->toggle.rwdataindex = len;
			len += sizeof(struct modelrwdata_toggle) / 4;
			node->child = rodata->toggle.target;
			break;
		case MODELNODETYPE_HEADSPOT:
			rodata = node->rodata;
			rodata->headspot.rwdataindex = len;
			len += sizeof(struct modelrwdata_headspot) / 4;
			node->child = NULL;
			break;
		case MODELNODETYPE_REORDER:
			rodata = node->rodata;
			rodata->reorder.rwdataindex = len;
			len += sizeof(struct modelrwdata_reorder) / 4;
			model0001c868(node, false);
			break;
		case MODELNODETYPE_0B:
			rodata = node->rodata;
			rodata->type0b.rwdataindex = len;
			len += sizeof(struct modelrwdata_0b) / 4;
			break;
		case MODELNODETYPE_GUNFIRE:
			rodata = node->rodata;
			rodata->gunfire.rwdataindex = len;
			len += sizeof(struct modelrwdata_gunfire) / 4;
			break;
		case MODELNODETYPE_DL:
			rodata = node->rodata;
			rodata->dl.rwdataindex = len;
			len += sizeof(struct modelrwdata_dl) / 4;
			break;
		default:
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node == basenode->parent) {
					node = NULL;
					break;
				}

				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}

	return len;
}

void modelCalculateRwDataLen(struct modelfiledata *filedata)
{
	filedata->rwdatalen = modelCalculateRwDataIndexes(filedata->rootnode);
}

void modelInitRwData(struct model *model, struct modelnode *startnode)
{
	struct modelnode *node = startnode;
	union modelrodata *rodata;
	union modelrwdata *rwdata;

	while (node) {
		u32 type = node->type & 0xff;

		switch (type) {
		case MODELNODETYPE_CHRINFO:
			rwdata = modelGetNodeRwData(model, node);

			rwdata->chrinfo.unk00 = 0;
			rwdata->chrinfo.ground = 0;
			rwdata->chrinfo.pos.x = 0;
			rwdata->chrinfo.pos.y = 0;
			rwdata->chrinfo.pos.z = 0;
			rwdata->chrinfo.unk14 = 0;
			rwdata->chrinfo.unk18 = 0;
			rwdata->chrinfo.unk1c = 0;

			rwdata->chrinfo.unk01 = 0;
			rwdata->chrinfo.unk34.x = 0;
			rwdata->chrinfo.unk34.y = 0;
			rwdata->chrinfo.unk34.z = 0;
			rwdata->chrinfo.unk30 = 0;
			rwdata->chrinfo.unk24.x = 0;
			rwdata->chrinfo.unk24.y = 0;
			rwdata->chrinfo.unk24.z = 0;
			rwdata->chrinfo.unk20 = 0;

			rwdata->chrinfo.unk02 = 0;
			rwdata->chrinfo.unk4c.x = 0;
			rwdata->chrinfo.unk4c.y = 0;
			rwdata->chrinfo.unk4c.z = 0;
			rwdata->chrinfo.unk40.x = 0;
			rwdata->chrinfo.unk40.y = 0;
			rwdata->chrinfo.unk40.z = 0;
			rwdata->chrinfo.unk5c = 0;
			break;
		case MODELNODETYPE_DISTANCE:
			rodata = node->rodata;
			rwdata = modelGetNodeRwData(model, node);
			rwdata->distance.visible = false;
			node->child = rodata->distance.target;
			break;
		case MODELNODETYPE_TOGGLE:
			rodata = node->rodata;
			rwdata = modelGetNodeRwData(model, node);
			rwdata->toggle.visible = true;
			node->child = rodata->toggle.target;
			break;
		case MODELNODETYPE_HEADSPOT:
			rwdata = modelGetNodeRwData(model, node);
			rwdata->headspot.modelfiledata = NULL;
			rwdata->headspot.rwdatas = NULL;
			break;
		case MODELNODETYPE_REORDER:
			rwdata = modelGetNodeRwData(model, node);
			rwdata->reorder.visible = false;
			modelRenderNodeReorder(model, node);
			break;
		case MODELNODETYPE_0B:
			rwdata = modelGetNodeRwData(model, node);
			rwdata->type0b.unk00 = 0;
			break;
		case MODELNODETYPE_GUNFIRE:
			rwdata = modelGetNodeRwData(model, node);
			rwdata->gunfire.visible = false;
			break;
		case MODELNODETYPE_DL:
			rodata = node->rodata;
			rwdata = modelGetNodeRwData(model, node);
			rwdata->dl.vertices = rodata->dl.vertices;
			rwdata->dl.gdl = rodata->dl.primary;
			rwdata->dl.colours = (void *) ALIGN8((u32)(rodata->dl.vertices + rodata->dl.numvertices));
			if (rodata->dl.numvertices);
			if (rodata->dl.numvertices);
			if (rodata->dl.numvertices);
			break;
		default:
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node == startnode->parent) {
					node = NULL;
					break;
				}

				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}
}

void modelInit(struct model *model, struct modelfiledata *filedata, union modelrwdata **rwdatas, bool resetanim)
{
	struct modelnode *node;

	model->unk00 = 0;
	model->filedata = filedata;
	model->rwdatas = rwdatas;
	model->unk02 = -1;
	model->scale = 1;
	model->attachedtomodel = NULL;
	model->attachedtonode = NULL;

	node = filedata->rootnode;

	while (node) {
		u32 type = node->type & 0xff;

		if (type == MODELNODETYPE_HEADSPOT) {
			model->unk00 |= 1;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node == filedata->rootnode->parent) {
					node = NULL;
					break;
				}

				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}

	if (rwdatas != NULL) {
		modelInitRwData(model, filedata->rootnode);
	}

	if (resetanim) {
		model->anim = NULL;
	}
}

void animInit(struct anim *anim)
{
	anim->animnum = 0;
	anim->animnum2 = 0;
	anim->looping = 0;
	anim->flipfunc = NULL;
	anim->unk6c = 0;
	anim->unk70 = NULL;
	anim->average = 0;
	anim->frac = 0;
	anim->timespeed = 0;
	anim->frac2 = 0;
	anim->timespeed2 = 0;
	anim->fracmerge = 0;
	anim->timemerge = 0;
	anim->timeplay = 0;
	anim->endframe = -1;
	anim->endframe2 = -1;
	anim->speed = 1;
	anim->speed2 = 1;
	anim->playspeed = 1;
	anim->animscale = 1;
}

void model00023108(struct model *pmodel, struct modelfiledata *pmodeldef, struct modelnode *pnode, struct modelfiledata *cmodeldef)
{
	struct modelrwdata_headspot *rwdata = modelGetNodeRwData(pmodel, pnode);
	struct modelnode *node;

	rwdata->modelfiledata = cmodeldef;
	rwdata->rwdatas = &pmodel->rwdatas[pmodeldef->rwdatalen];

	pnode->child = cmodeldef->rootnode;

	node = pnode->child;

	while (node) {
		node->parent = pnode;
		node = node->next;
	}

	pmodeldef->rwdatalen += modelCalculateRwDataIndexes(pnode->child);
}

/**
 * This function can be called repeatedly to iterate a model's display lists.
 *
 * On the first call, the value passed as nodeptr should point to a NULL value.
 * Each time the function is called, it will update *gdlptr to point to the next
 * display list, and will update *nodeptr to point to the current node. On
 * subsequent calls, the same values should be passed as nodeptr and gdlptr so
 * the function can continue correctly.
 *
 * Note that some node types support multiple display lists, so the function
 * may return the same node while it iterates the display lists for that node.
 */
void modelIterateDisplayLists(struct modelfiledata *filedata, struct modelnode **nodeptr, Gfx **gdlptr)
{
	struct modelnode *node = *nodeptr;
	union modelrodata *rodata;
	Gfx *gdl = NULL;

	if (node == NULL) {
		node = filedata->rootnode;
	}

	while (node) {
		u32 type = node->type & 0xff;

		switch (type) {
		case MODELNODETYPE_GUNDL:
			rodata = node->rodata;

			if (node != *nodeptr) {
				gdl = rodata->gundl.primary;
			} else if (rodata->gundl.secondary != *gdlptr) {
				gdl = rodata->gundl.secondary;
			}
			break;
		case MODELNODETYPE_DL:
			rodata = node->rodata;

			if (node != *nodeptr) {
				gdl = rodata->dl.primary;
			} else if (rodata->dl.secondary != *gdlptr) {
				gdl = rodata->dl.secondary;
			}
			break;
		case MODELNODETYPE_16:
			rodata = node->rodata;

			if (node != *nodeptr) {
				gdl = rodata->type16.unk08;
			}
			break;
		case MODELNODETYPE_DISTANCE:
			rodata = node->rodata;
			node->child = rodata->distance.target;
			break;
		case MODELNODETYPE_TOGGLE:
			rodata = node->rodata;
			node->child = rodata->toggle.target;
			break;
		case MODELNODETYPE_REORDER:
			model0001c868(node, true);
			break;
		}

		if (gdl) {
			break;
		}

		if (node->child) {
			node = node->child;
		} else {
			while (node) {
				if (node->next) {
					node = node->next;
					break;
				}

				node = node->parent;
			}
		}
	}

	*gdlptr = gdl;
	*nodeptr = node;
}

void modelNodeReplaceGdl(u32 arg0, struct modelnode *node, Gfx *find, Gfx *replacement)
{
	union modelrodata *rodata;
	u32 type = node->type & 0xff;

	switch (type) {
	case MODELNODETYPE_GUNDL:
		rodata = node->rodata;

		if (rodata->gundl.primary == find) {
			rodata->gundl.primary = replacement;
			return;
		}

		if (rodata->gundl.secondary == find) {
			rodata->gundl.secondary = replacement;
			return;
		}
		break;
	case MODELNODETYPE_DL:
		rodata = node->rodata;

		if (rodata->dl.primary == find) {
			rodata->dl.primary = replacement;
			return;
		}

		if (rodata->dl.secondary == find) {
			rodata->dl.secondary = replacement;
			return;
		}
		break;
	case MODELNODETYPE_16:
		rodata = node->rodata;

		if (rodata->type16.unk08 == find) {
			rodata->type16.unk08 = replacement;
			return;
		}
		break;
	}
}
