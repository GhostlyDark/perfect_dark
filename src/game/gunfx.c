#include <ultra64.h>
#include "constants.h"
#include "game/acosfasinf.h"
#include "game/bondgun.h"
#include "game/gunfx.h"
#include "game/game_0b0fd0.h"
#include "game/tex.h"
#include "game/camera.h"
#include "game/game_0c33f0.h"
#include "game/gfxmemory.h"
#include "game/file.h"
#include "bss.h"
#include "lib/main.h"
#include "lib/rng.h"
#include "lib/mtx.h"
#include "data.h"
#include "types.h"

const u32 var7f1acc10[] = {0x73703100};
const u32 var7f1acc14[] = {0x73703200};
const u32 var7f1acc18[] = {0x73703300};
const u32 var7f1acc1c[] = {0x73706200};
const u32 var7f1acc20[] = {0x73706900};

struct casing g_Casings[20];
struct boltbeam g_BoltBeams[8];
struct lasersight g_LaserSights[4];

u32 var80070500 = 0x00000000;
u32 var80070504 = 0x00000000;
u32 var80070508 = 0x00000000;
u32 var8007050c = 0x00000000;
u32 var80070510 = 0x00000000;
u32 var80070514 = 0x00000000;
u32 var80070518 = 0x00000000;
u32 var8007051c = 0x00000000;
u32 var80070520 = 0x00000000;
bool g_CasingsActive = false;
u32 var80070528 = 0x00000000;
u32 var8007052c = 0x00000000;
u32 var80070530 = 0x00000000;

void beamCreate(struct beam *beam, s32 weaponnum, struct coord *from, struct coord *to)
{
	f32 distance;

	beam->from.x = from->x;
	beam->from.y = from->y;
	beam->from.z = from->z;

	beam->dir.x = to->x - from->x;
	beam->dir.y = to->y - from->y;
	beam->dir.z = to->z - from->z;

	distance = sqrtf(beam->dir.f[0] * beam->dir.f[0] + beam->dir.f[1] * beam->dir.f[1] + beam->dir.f[2] * beam->dir.f[2]);

	if (distance > 0) {
		beam->dir.x *= 1.0f / distance;
		beam->dir.y *= 1.0f / distance;
		beam->dir.z *= 1.0f / distance;
	}

	if (distance > 10000) {
		distance = 10000;
	}

	beam->age = 0;
	beam->weaponnum = weaponnum;
	beam->maxdist = distance;

	if (distance < 500) {
		distance = 500;
	}

	if (weaponnum == -1) {
		beam->speed = 0;
		beam->mindist = distance;

		if (beam->mindist > 3000) {
			beam->mindist = 3000;
		}

		beam->dist = 0;
	} else if (weaponnum == WEAPON_LASER || weaponnum == WEAPON_WATCHLASER) {
		beam->speed = 0.25f * distance;
		beam->mindist = 0.6f * distance;

		if (beam->mindist > 3000) {
			beam->mindist = 3000;
		}

		beam->dist = (-0.1f - RANDOMFRAC() * 0.3f) * distance;
	} else if (weaponnum == -2) {
		beam->speed = 0;
		beam->mindist = distance;

		if (beam->mindist > 3000) {
			beam->mindist = 3000;
		}

		beam->dist = 0;
	} else {
		f32 tmp;
		beam->speed = 0.2f * distance;
		beam->mindist = 0.2f * distance;

		if (beam->mindist > 3000) {
			beam->mindist = 3000;
		}

		tmp = RANDOMFRAC();
		beam->dist = (tmp + tmp - 1) * beam->speed;
	}

	if (beam->dist >= beam->maxdist) {
		beam->age = -1;
	}
}

void beamCreateForHand(s32 handnum)
{
	struct player *player = g_Vars.currentplayer;
	struct hand *hand = player->hands + handnum;
	Mtxf *mtx = camGetMatrix1740();
	f32 tmp;

	tmp = hand->hitpos.f[0] * mtx->m[0][2] + hand->hitpos.f[1] * mtx->m[1][2] + hand->hitpos.f[2] * mtx->m[2][2] + mtx->m[3][2];
	tmp = -tmp;

	if (tmp < hand->muzzlez) {
		// empty
	} else {
		struct beam *beam;
		s32 weaponnum = bgunGetWeaponNum(handnum);

		if (hand->gset.weaponnum == WEAPON_LASER && hand->gset.weaponfunc == FUNC_SECONDARY) {
			weaponnum = -2;
		}

		beam = &hand->beam;
		beamCreate(beam, weaponnum, &hand->muzzlepos, &hand->hitpos);

		if (beam->weaponnum == WEAPON_MAULER) {
			beam->weaponnum = -3 - (s32)player->hands[handnum].matmot1;
		}

		if (player->prop->chr && PLAYERCOUNT() >= 2) {
			struct chrdata *chr = player->prop->chr;
			struct coord disttolast;
			struct coord disttocur;
			f32 radians;

			if (chr->fireslots[handnum] == -1) {
				chr->fireslots[handnum] = bgunAllocateFireslot();
			}

			if (chr->fireslots[handnum] != -1) {
				disttolast.x = hand->hitpos.x - player->chrmuzzlelastpos[handnum].x;
				disttolast.y = hand->hitpos.y - player->chrmuzzlelastpos[handnum].y;
				disttolast.z = hand->hitpos.z - player->chrmuzzlelastpos[handnum].z;

				guNormalize(&disttolast.x, &disttolast.y, &disttolast.z);

				disttocur.x = hand->hitpos.x - hand->muzzlepos.x;
				disttocur.y = hand->hitpos.y - hand->muzzlepos.y;
				disttocur.z = hand->hitpos.z - hand->muzzlepos.z;

				guNormalize(&disttocur.x, &disttocur.y, &disttocur.z);

				radians = acosf(disttolast.f[0] * disttocur.f[0] + disttolast.f[1] * disttocur.f[1] + disttolast.f[2] * disttocur.f[2]);

				if (!(radians > 0.08725257f) || weaponnum == -2) {
					beamCreate(&g_Fireslots[chr->fireslots[handnum]].beam, weaponnum, &player->chrmuzzlelastpos[handnum], &hand->hitpos);

					if (g_Fireslots[chr->fireslots[handnum]].beam.weaponnum == WEAPON_MAULER) {
						g_Fireslots[chr->fireslots[handnum]].beam.weaponnum = -3 - (s32)player->hands[handnum].matmot1;
					}
				}
			}
		}
	}
}

GLOBAL_ASM(
glabel func0f0ac4b8
.late_rodata
glabel var7f1acc4c
.word 0x3727c5ac
glabel var7f1acc50
.word 0xb727c5ac
glabel var7f1acc54
.word 0x461c4000
glabel var7f1acc58
.word 0xc61c4000
glabel var7f1acc5c
.word 0x461c4000
glabel var7f1acc60
.word 0xc61c4000
glabel var7f1acc64
.word 0x461c4000
glabel var7f1acc68
.word 0xc61c4000
.text
/*  f0ac4b8:	27bdff10 */ 	addiu	$sp,$sp,-240
/*  f0ac4bc:	afb00028 */ 	sw	$s0,0x28($sp)
/*  f0ac4c0:	3c10800a */ 	lui	$s0,%hi(g_Vars+0x284)
/*  f0ac4c4:	8e10a244 */ 	lw	$s0,%lo(g_Vars+0x284)($s0)
/*  f0ac4c8:	afbf002c */ 	sw	$ra,0x2c($sp)
/*  f0ac4cc:	afa400f0 */ 	sw	$a0,0xf0($sp)
/*  f0ac4d0:	afa500f4 */ 	sw	$a1,0xf4($sp)
/*  f0ac4d4:	afa600f8 */ 	sw	$a2,0xf8($sp)
/*  f0ac4d8:	afa700fc */ 	sw	$a3,0xfc($sp)
/*  f0ac4dc:	24040002 */ 	addiu	$a0,$zero,0x2
/*  f0ac4e0:	0fc59e73 */ 	jal	gfxAllocateColours
/*  f0ac4e4:	26101bb0 */ 	addiu	$s0,$s0,7088
/*  f0ac4e8:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0ac4ec:	afa200c4 */ 	sw	$v0,0xc4($sp)
/*  f0ac4f0:	3c0c8007 */ 	lui	$t4,%hi(var80070500)
/*  f0ac4f4:	afa20080 */ 	sw	$v0,0x80($sp)
/*  f0ac4f8:	258c0500 */ 	addiu	$t4,$t4,%lo(var80070500)
/*  f0ac4fc:	8d810000 */ 	lw	$at,0x0($t4)
/*  f0ac500:	8fa300fc */ 	lw	$v1,0xfc($sp)
/*  f0ac504:	8fa40108 */ 	lw	$a0,0x108($sp)
/*  f0ac508:	27aa0074 */ 	addiu	$t2,$sp,0x74
/*  f0ac50c:	ad410000 */ 	sw	$at,0x0($t2)
/*  f0ac510:	8d810008 */ 	lw	$at,0x8($t4)
/*  f0ac514:	8d990004 */ 	lw	$t9,0x4($t4)
/*  f0ac518:	ad410008 */ 	sw	$at,0x8($t2)
/*  f0ac51c:	ad590004 */ 	sw	$t9,0x4($t2)
/*  f0ac520:	c4680000 */ 	lwc1	$f8,0x0($v1)
/*  f0ac524:	c4840000 */ 	lwc1	$f4,0x0($a0)
/*  f0ac528:	46082181 */ 	sub.s	$f6,$f4,$f8
/*  f0ac52c:	e7a600e4 */ 	swc1	$f6,0xe4($sp)
/*  f0ac530:	c46a0004 */ 	lwc1	$f10,0x4($v1)
/*  f0ac534:	c4920004 */ 	lwc1	$f18,0x4($a0)
/*  f0ac538:	460a9101 */ 	sub.s	$f4,$f18,$f10
/*  f0ac53c:	c7aa00e4 */ 	lwc1	$f10,0xe4($sp)
/*  f0ac540:	e7a400e8 */ 	swc1	$f4,0xe8($sp)
/*  f0ac544:	c4660008 */ 	lwc1	$f6,0x8($v1)
/*  f0ac548:	c4880008 */ 	lwc1	$f8,0x8($a0)
/*  f0ac54c:	460a5102 */ 	mul.s	$f4,$f10,$f10
/*  f0ac550:	46064481 */ 	sub.s	$f18,$f8,$f6
/*  f0ac554:	c7a800e8 */ 	lwc1	$f8,0xe8($sp)
/*  f0ac558:	46084182 */ 	mul.s	$f6,$f8,$f8
/*  f0ac55c:	e7b200ec */ 	swc1	$f18,0xec($sp)
/*  f0ac560:	c7aa00ec */ 	lwc1	$f10,0xec($sp)
/*  f0ac564:	460a5202 */ 	mul.s	$f8,$f10,$f10
/*  f0ac568:	46062480 */ 	add.s	$f18,$f4,$f6
/*  f0ac56c:	0c012974 */ 	jal	sqrtf
/*  f0ac570:	46124300 */ 	add.s	$f12,$f8,$f18
/*  f0ac574:	3c017f1b */ 	lui	$at,%hi(var7f1acc4c)
/*  f0ac578:	c424cc4c */ 	lwc1	$f4,%lo(var7f1acc4c)($at)
/*  f0ac57c:	46000086 */ 	mov.s	$f2,$f0
/*  f0ac580:	3c017f1b */ 	lui	$at,%hi(var7f1acc50)
/*  f0ac584:	4604003c */ 	c.lt.s	$f0,$f4
/*  f0ac588:	c7aa00e4 */ 	lwc1	$f10,0xe4($sp)
/*  f0ac58c:	45020009 */ 	bc1fl	.L0f0ac5b4
/*  f0ac590:	c7b200e8 */ 	lwc1	$f18,0xe8($sp)
/*  f0ac594:	c426cc50 */ 	lwc1	$f6,%lo(var7f1acc50)($at)
/*  f0ac598:	4600303c */ 	c.lt.s	$f6,$f0
/*  f0ac59c:	00000000 */ 	nop
/*  f0ac5a0:	45020004 */ 	bc1fl	.L0f0ac5b4
/*  f0ac5a4:	c7b200e8 */ 	lwc1	$f18,0xe8($sp)
/*  f0ac5a8:	10000174 */ 	b	.L0f0acb7c
/*  f0ac5ac:	8fa200f0 */ 	lw	$v0,0xf0($sp)
/*  f0ac5b0:	c7b200e8 */ 	lwc1	$f18,0xe8($sp)
.L0f0ac5b4:
/*  f0ac5b4:	c7a600ec */ 	lwc1	$f6,0xec($sp)
/*  f0ac5b8:	46025203 */ 	div.s	$f8,$f10,$f2
/*  f0ac5bc:	e7a200e0 */ 	swc1	$f2,0xe0($sp)
/*  f0ac5c0:	46029103 */ 	div.s	$f4,$f18,$f2
/*  f0ac5c4:	e7a800e4 */ 	swc1	$f8,0xe4($sp)
/*  f0ac5c8:	46023283 */ 	div.s	$f10,$f6,$f2
/*  f0ac5cc:	e7a400e8 */ 	swc1	$f4,0xe8($sp)
/*  f0ac5d0:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0ac5d4:	e7aa00ec */ 	swc1	$f10,0xec($sp)
/*  f0ac5d8:	00402025 */ 	or	$a0,$v0,$zero
/*  f0ac5dc:	8fa500fc */ 	lw	$a1,0xfc($sp)
/*  f0ac5e0:	0c0056da */ 	jal	mtx4TransformVec
/*  f0ac5e4:	27a6005c */ 	addiu	$a2,$sp,0x5c
/*  f0ac5e8:	c7ae00f8 */ 	lwc1	$f14,0xf8($sp)
/*  f0ac5ec:	c7a8005c */ 	lwc1	$f8,0x5c($sp)
/*  f0ac5f0:	3c017f1b */ 	lui	$at,%hi(var7f1acc54)
/*  f0ac5f4:	c422cc54 */ 	lwc1	$f2,%lo(var7f1acc54)($at)
/*  f0ac5f8:	460e4002 */ 	mul.s	$f0,$f8,$f14
/*  f0ac5fc:	3c017f1b */ 	lui	$at,%hi(var7f1acc58)
/*  f0ac600:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0ac604:	00000000 */ 	nop
/*  f0ac608:	45010007 */ 	bc1t	.L0f0ac628
/*  f0ac60c:	00000000 */ 	nop
/*  f0ac610:	c42ccc58 */ 	lwc1	$f12,%lo(var7f1acc58)($at)
/*  f0ac614:	c7b20060 */ 	lwc1	$f18,0x60($sp)
/*  f0ac618:	460c003c */ 	c.lt.s	$f0,$f12
/*  f0ac61c:	00000000 */ 	nop
/*  f0ac620:	45000003 */ 	bc1f	.L0f0ac630
/*  f0ac624:	00000000 */ 	nop
.L0f0ac628:
/*  f0ac628:	10000154 */ 	b	.L0f0acb7c
/*  f0ac62c:	8fa200f0 */ 	lw	$v0,0xf0($sp)
.L0f0ac630:
/*  f0ac630:	460e9002 */ 	mul.s	$f0,$f18,$f14
/*  f0ac634:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0ac638:	00000000 */ 	nop
/*  f0ac63c:	45010005 */ 	bc1t	.L0f0ac654
/*  f0ac640:	00000000 */ 	nop
/*  f0ac644:	460c003c */ 	c.lt.s	$f0,$f12
/*  f0ac648:	c7a40064 */ 	lwc1	$f4,0x64($sp)
/*  f0ac64c:	45000003 */ 	bc1f	.L0f0ac65c
/*  f0ac650:	00000000 */ 	nop
.L0f0ac654:
/*  f0ac654:	10000149 */ 	b	.L0f0acb7c
/*  f0ac658:	8fa200f0 */ 	lw	$v0,0xf0($sp)
.L0f0ac65c:
/*  f0ac65c:	460e2002 */ 	mul.s	$f0,$f4,$f14
/*  f0ac660:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0ac664:	00000000 */ 	nop
/*  f0ac668:	45010005 */ 	bc1t	.L0f0ac680
/*  f0ac66c:	00000000 */ 	nop
/*  f0ac670:	460c003c */ 	c.lt.s	$f0,$f12
/*  f0ac674:	00000000 */ 	nop
/*  f0ac678:	45000003 */ 	bc1f	.L0f0ac688
/*  f0ac67c:	00000000 */ 	nop
.L0f0ac680:
/*  f0ac680:	1000013e */ 	b	.L0f0acb7c
/*  f0ac684:	8fa200f0 */ 	lw	$v0,0xf0($sp)
.L0f0ac688:
/*  f0ac688:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0ac68c:	00000000 */ 	nop
/*  f0ac690:	00402025 */ 	or	$a0,$v0,$zero
/*  f0ac694:	8fa50108 */ 	lw	$a1,0x108($sp)
/*  f0ac698:	0c0056da */ 	jal	mtx4TransformVec
/*  f0ac69c:	27a6005c */ 	addiu	$a2,$sp,0x5c
/*  f0ac6a0:	c7b000f8 */ 	lwc1	$f16,0xf8($sp)
/*  f0ac6a4:	c7a6005c */ 	lwc1	$f6,0x5c($sp)
/*  f0ac6a8:	3c017f1b */ 	lui	$at,%hi(var7f1acc64)
/*  f0ac6ac:	c422cc64 */ 	lwc1	$f2,%lo(var7f1acc64)($at)
/*  f0ac6b0:	46103002 */ 	mul.s	$f0,$f6,$f16
/*  f0ac6b4:	3c017f1b */ 	lui	$at,%hi(var7f1acc68)
/*  f0ac6b8:	c42ccc68 */ 	lwc1	$f12,%lo(var7f1acc68)($at)
/*  f0ac6bc:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0ac6c0:	00000000 */ 	nop
/*  f0ac6c4:	45010005 */ 	bc1t	.L0f0ac6dc
/*  f0ac6c8:	00000000 */ 	nop
/*  f0ac6cc:	460c003c */ 	c.lt.s	$f0,$f12
/*  f0ac6d0:	c7aa0060 */ 	lwc1	$f10,0x60($sp)
/*  f0ac6d4:	45000003 */ 	bc1f	.L0f0ac6e4
/*  f0ac6d8:	00000000 */ 	nop
.L0f0ac6dc:
/*  f0ac6dc:	10000127 */ 	b	.L0f0acb7c
/*  f0ac6e0:	8fa200f0 */ 	lw	$v0,0xf0($sp)
.L0f0ac6e4:
/*  f0ac6e4:	46105002 */ 	mul.s	$f0,$f10,$f16
/*  f0ac6e8:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0ac6ec:	00000000 */ 	nop
/*  f0ac6f0:	45010005 */ 	bc1t	.L0f0ac708
/*  f0ac6f4:	00000000 */ 	nop
/*  f0ac6f8:	460c003c */ 	c.lt.s	$f0,$f12
/*  f0ac6fc:	c7a80064 */ 	lwc1	$f8,0x64($sp)
/*  f0ac700:	45000003 */ 	bc1f	.L0f0ac710
/*  f0ac704:	00000000 */ 	nop
.L0f0ac708:
/*  f0ac708:	1000011c */ 	b	.L0f0acb7c
/*  f0ac70c:	8fa200f0 */ 	lw	$v0,0xf0($sp)
.L0f0ac710:
/*  f0ac710:	46104002 */ 	mul.s	$f0,$f8,$f16
/*  f0ac714:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0ac718:	00000000 */ 	nop
/*  f0ac71c:	45010006 */ 	bc1t	.L0f0ac738
/*  f0ac720:	00000000 */ 	nop
/*  f0ac724:	460c003c */ 	c.lt.s	$f0,$f12
/*  f0ac728:	8fb80100 */ 	lw	$t8,0x100($sp)
/*  f0ac72c:	8fae00c4 */ 	lw	$t6,0xc4($sp)
/*  f0ac730:	45020004 */ 	bc1fl	.L0f0ac744
/*  f0ac734:	8fa200fc */ 	lw	$v0,0xfc($sp)
.L0f0ac738:
/*  f0ac738:	10000110 */ 	b	.L0f0acb7c
/*  f0ac73c:	8fa200f0 */ 	lw	$v0,0xf0($sp)
/*  f0ac740:	8fa200fc */ 	lw	$v0,0xfc($sp)
.L0f0ac744:
/*  f0ac744:	c7ae00e0 */ 	lwc1	$f14,0xe0($sp)
/*  f0ac748:	add80000 */ 	sw	$t8,0x0($t6)
/*  f0ac74c:	8faf010c */ 	lw	$t7,0x10c($sp)
/*  f0ac750:	adcf0004 */ 	sw	$t7,0x4($t6)
/*  f0ac754:	c7b200ec */ 	lwc1	$f18,0xec($sp)
/*  f0ac758:	c44a0008 */ 	lwc1	$f10,0x8($v0)
/*  f0ac75c:	c6060008 */ 	lwc1	$f6,0x8($s0)
/*  f0ac760:	46127002 */ 	mul.s	$f0,$f14,$f18
/*  f0ac764:	c7a400e8 */ 	lwc1	$f4,0xe8($sp)
/*  f0ac768:	460e2082 */ 	mul.s	$f2,$f4,$f14
/*  f0ac76c:	46005200 */ 	add.s	$f8,$f10,$f0
/*  f0ac770:	46083281 */ 	sub.s	$f10,$f6,$f8
/*  f0ac774:	c6080004 */ 	lwc1	$f8,0x4($s0)
/*  f0ac778:	460a2182 */ 	mul.s	$f6,$f4,$f10
/*  f0ac77c:	c44a0004 */ 	lwc1	$f10,0x4($v0)
/*  f0ac780:	460a1280 */ 	add.s	$f10,$f2,$f10
/*  f0ac784:	460a4201 */ 	sub.s	$f8,$f8,$f10
/*  f0ac788:	46124282 */ 	mul.s	$f10,$f8,$f18
/*  f0ac78c:	460a3201 */ 	sub.s	$f8,$f6,$f10
/*  f0ac790:	c7a600e4 */ 	lwc1	$f6,0xe4($sp)
/*  f0ac794:	46067302 */ 	mul.s	$f12,$f14,$f6
/*  f0ac798:	e7a800d0 */ 	swc1	$f8,0xd0($sp)
/*  f0ac79c:	c60a0000 */ 	lwc1	$f10,0x0($s0)
/*  f0ac7a0:	e7a40030 */ 	swc1	$f4,0x30($sp)
/*  f0ac7a4:	c4440000 */ 	lwc1	$f4,0x0($v0)
/*  f0ac7a8:	460c2100 */ 	add.s	$f4,$f4,$f12
/*  f0ac7ac:	46045281 */ 	sub.s	$f10,$f10,$f4
/*  f0ac7b0:	460a9102 */ 	mul.s	$f4,$f18,$f10
/*  f0ac7b4:	c44a0008 */ 	lwc1	$f10,0x8($v0)
/*  f0ac7b8:	c6120008 */ 	lwc1	$f18,0x8($s0)
/*  f0ac7bc:	460a0280 */ 	add.s	$f10,$f0,$f10
/*  f0ac7c0:	44800000 */ 	mtc1	$zero,$f0
/*  f0ac7c4:	460a9481 */ 	sub.s	$f18,$f18,$f10
/*  f0ac7c8:	46080032 */ 	c.eq.s	$f0,$f8
/*  f0ac7cc:	46069282 */ 	mul.s	$f10,$f18,$f6
/*  f0ac7d0:	460a2481 */ 	sub.s	$f18,$f4,$f10
/*  f0ac7d4:	e7b200d4 */ 	swc1	$f18,0xd4($sp)
/*  f0ac7d8:	c44a0004 */ 	lwc1	$f10,0x4($v0)
/*  f0ac7dc:	c6040004 */ 	lwc1	$f4,0x4($s0)
/*  f0ac7e0:	46025280 */ 	add.s	$f10,$f10,$f2
/*  f0ac7e4:	460a2101 */ 	sub.s	$f4,$f4,$f10
/*  f0ac7e8:	46043282 */ 	mul.s	$f10,$f6,$f4
/*  f0ac7ec:	c4440000 */ 	lwc1	$f4,0x0($v0)
/*  f0ac7f0:	c6060000 */ 	lwc1	$f6,0x0($s0)
/*  f0ac7f4:	46046100 */ 	add.s	$f4,$f12,$f4
/*  f0ac7f8:	46043181 */ 	sub.s	$f6,$f6,$f4
/*  f0ac7fc:	c7a40030 */ 	lwc1	$f4,0x30($sp)
/*  f0ac800:	46043182 */ 	mul.s	$f6,$f6,$f4
/*  f0ac804:	46065101 */ 	sub.s	$f4,$f10,$f6
/*  f0ac808:	45000008 */ 	bc1f	.L0f0ac82c
/*  f0ac80c:	e7a400d8 */ 	swc1	$f4,0xd8($sp)
/*  f0ac810:	46120032 */ 	c.eq.s	$f0,$f18
/*  f0ac814:	c7aa00d8 */ 	lwc1	$f10,0xd8($sp)
/*  f0ac818:	45020005 */ 	bc1fl	.L0f0ac830
/*  f0ac81c:	27a400d0 */ 	addiu	$a0,$sp,0xd0
/*  f0ac820:	460a0032 */ 	c.eq.s	$f0,$f10
/*  f0ac824:	3c013f80 */ 	lui	$at,0x3f80
/*  f0ac828:	45010006 */ 	bc1t	.L0f0ac844
.L0f0ac82c:
/*  f0ac82c:	27a400d0 */ 	addiu	$a0,$sp,0xd0
.L0f0ac830:
/*  f0ac830:	27a500d4 */ 	addiu	$a1,$sp,0xd4
/*  f0ac834:	0c0011e4 */ 	jal	guNormalize
/*  f0ac838:	27a600d8 */ 	addiu	$a2,$sp,0xd8
/*  f0ac83c:	10000005 */ 	b	.L0f0ac854
/*  f0ac840:	00000000 */ 	nop
.L0f0ac844:
/*  f0ac844:	44813000 */ 	mtc1	$at,$f6
/*  f0ac848:	e7a000d0 */ 	swc1	$f0,0xd0($sp)
/*  f0ac84c:	e7a000d8 */ 	swc1	$f0,0xd8($sp)
/*  f0ac850:	e7a600d4 */ 	swc1	$f6,0xd4($sp)
.L0f0ac854:
/*  f0ac854:	0fc59e59 */ 	jal	gfxAllocateVertices
/*  f0ac858:	24040004 */ 	addiu	$a0,$zero,0x4
/*  f0ac85c:	0fc59e66 */ 	jal	gfxAllocateMatrix
/*  f0ac860:	00408025 */ 	or	$s0,$v0,$zero
/*  f0ac864:	afa200c8 */ 	sw	$v0,0xc8($sp)
/*  f0ac868:	8fa400fc */ 	lw	$a0,0xfc($sp)
/*  f0ac86c:	0c0059b7 */ 	jal	mtx4LoadTranslation
/*  f0ac870:	27a50084 */ 	addiu	$a1,$sp,0x84
/*  f0ac874:	3c013f80 */ 	lui	$at,0x3f80
/*  f0ac878:	44812000 */ 	mtc1	$at,$f4
/*  f0ac87c:	c7a800f8 */ 	lwc1	$f8,0xf8($sp)
/*  f0ac880:	27a50084 */ 	addiu	$a1,$sp,0x84
/*  f0ac884:	0c0057c1 */ 	jal	mtx00015f04
/*  f0ac888:	46082303 */ 	div.s	$f12,$f4,$f8
/*  f0ac88c:	8fa40080 */ 	lw	$a0,0x80($sp)
/*  f0ac890:	0c0056f8 */ 	jal	mtx00015be0
/*  f0ac894:	27a50084 */ 	addiu	$a1,$sp,0x84
/*  f0ac898:	27a40084 */ 	addiu	$a0,$sp,0x84
/*  f0ac89c:	0c005815 */ 	jal	mtx00016054
/*  f0ac8a0:	8fa500c8 */ 	lw	$a1,0xc8($sp)
/*  f0ac8a4:	c7b200e0 */ 	lwc1	$f18,0xe0($sp)
/*  f0ac8a8:	c7aa00f8 */ 	lwc1	$f10,0xf8($sp)
/*  f0ac8ac:	c7a600e4 */ 	lwc1	$f6,0xe4($sp)
/*  f0ac8b0:	c7a800e8 */ 	lwc1	$f8,0xe8($sp)
/*  f0ac8b4:	460a9082 */ 	mul.s	$f2,$f18,$f10
/*  f0ac8b8:	8fa600f4 */ 	lw	$a2,0xf4($sp)
/*  f0ac8bc:	24050004 */ 	addiu	$a1,$zero,0x4
/*  f0ac8c0:	46023102 */ 	mul.s	$f4,$f6,$f2
/*  f0ac8c4:	c7a600ec */ 	lwc1	$f6,0xec($sp)
/*  f0ac8c8:	46024482 */ 	mul.s	$f18,$f8,$f2
/*  f0ac8cc:	c7a80104 */ 	lwc1	$f8,0x104($sp)
/*  f0ac8d0:	e7a40074 */ 	swc1	$f4,0x74($sp)
/*  f0ac8d4:	46023102 */ 	mul.s	$f4,$f6,$f2
/*  f0ac8d8:	e7b20078 */ 	swc1	$f18,0x78($sp)
/*  f0ac8dc:	460a4002 */ 	mul.s	$f0,$f8,$f10
/*  f0ac8e0:	c7b200d0 */ 	lwc1	$f18,0xd0($sp)
/*  f0ac8e4:	e7a4007c */ 	swc1	$f4,0x7c($sp)
/*  f0ac8e8:	46009182 */ 	mul.s	$f6,$f18,$f0
/*  f0ac8ec:	4600310d */ 	trunc.w.s	$f4,$f6
/*  f0ac8f0:	440d2000 */ 	mfc1	$t5,$f4
/*  f0ac8f4:	00000000 */ 	nop
/*  f0ac8f8:	a60d0000 */ 	sh	$t5,0x0($s0)
/*  f0ac8fc:	c7a800d4 */ 	lwc1	$f8,0xd4($sp)
/*  f0ac900:	46004282 */ 	mul.s	$f10,$f8,$f0
/*  f0ac904:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0ac908:	440a9000 */ 	mfc1	$t2,$f18
/*  f0ac90c:	00000000 */ 	nop
/*  f0ac910:	a60a0002 */ 	sh	$t2,0x2($s0)
/*  f0ac914:	c7a600d8 */ 	lwc1	$f6,0xd8($sp)
/*  f0ac918:	a6000008 */ 	sh	$zero,0x8($s0)
/*  f0ac91c:	a600000a */ 	sh	$zero,0xa($s0)
/*  f0ac920:	46003102 */ 	mul.s	$f4,$f6,$f0
/*  f0ac924:	a2000007 */ 	sb	$zero,0x7($s0)
/*  f0ac928:	4600220d */ 	trunc.w.s	$f8,$f4
/*  f0ac92c:	44194000 */ 	mfc1	$t9,$f8
/*  f0ac930:	00000000 */ 	nop
/*  f0ac934:	a6190004 */ 	sh	$t9,0x4($s0)
/*  f0ac938:	c7aa00d0 */ 	lwc1	$f10,0xd0($sp)
/*  f0ac93c:	46005487 */ 	neg.s	$f18,$f10
/*  f0ac940:	46009182 */ 	mul.s	$f6,$f18,$f0
/*  f0ac944:	4600310d */ 	trunc.w.s	$f4,$f6
/*  f0ac948:	440f2000 */ 	mfc1	$t7,$f4
/*  f0ac94c:	00000000 */ 	nop
/*  f0ac950:	a60f000c */ 	sh	$t7,0xc($s0)
/*  f0ac954:	c7a800d4 */ 	lwc1	$f8,0xd4($sp)
/*  f0ac958:	46004287 */ 	neg.s	$f10,$f8
/*  f0ac95c:	46005482 */ 	mul.s	$f18,$f10,$f0
/*  f0ac960:	4600918d */ 	trunc.w.s	$f6,$f18
/*  f0ac964:	44093000 */ 	mfc1	$t1,$f6
/*  f0ac968:	00000000 */ 	nop
/*  f0ac96c:	a609000e */ 	sh	$t1,0xe($s0)
/*  f0ac970:	c7a400d8 */ 	lwc1	$f4,0xd8($sp)
/*  f0ac974:	46002207 */ 	neg.s	$f8,$f4
/*  f0ac978:	46004282 */ 	mul.s	$f10,$f8,$f0
/*  f0ac97c:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0ac980:	440b9000 */ 	mfc1	$t3,$f18
/*  f0ac984:	00000000 */ 	nop
/*  f0ac988:	a60b0010 */ 	sh	$t3,0x10($s0)
/*  f0ac98c:	90ca0004 */ 	lbu	$t2,0x4($a2)
/*  f0ac990:	a6000016 */ 	sh	$zero,0x16($s0)
/*  f0ac994:	a2000013 */ 	sb	$zero,0x13($s0)
/*  f0ac998:	000a6140 */ 	sll	$t4,$t2,0x5
/*  f0ac99c:	a60c0014 */ 	sh	$t4,0x14($s0)
/*  f0ac9a0:	c7a400d0 */ 	lwc1	$f4,0xd0($sp)
/*  f0ac9a4:	c7a60074 */ 	lwc1	$f6,0x74($sp)
/*  f0ac9a8:	46002202 */ 	mul.s	$f8,$f4,$f0
/*  f0ac9ac:	46083281 */ 	sub.s	$f10,$f6,$f8
/*  f0ac9b0:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0ac9b4:	44189000 */ 	mfc1	$t8,$f18
/*  f0ac9b8:	00000000 */ 	nop
/*  f0ac9bc:	a6180018 */ 	sh	$t8,0x18($s0)
/*  f0ac9c0:	c7a600d4 */ 	lwc1	$f6,0xd4($sp)
/*  f0ac9c4:	c7a40078 */ 	lwc1	$f4,0x78($sp)
/*  f0ac9c8:	46003202 */ 	mul.s	$f8,$f6,$f0
/*  f0ac9cc:	46082281 */ 	sub.s	$f10,$f4,$f8
/*  f0ac9d0:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0ac9d4:	440e9000 */ 	mfc1	$t6,$f18
/*  f0ac9d8:	00000000 */ 	nop
/*  f0ac9dc:	a60e001a */ 	sh	$t6,0x1a($s0)
/*  f0ac9e0:	c7a400d8 */ 	lwc1	$f4,0xd8($sp)
/*  f0ac9e4:	c7a6007c */ 	lwc1	$f6,0x7c($sp)
/*  f0ac9e8:	240e2000 */ 	addiu	$t6,$zero,0x2000
/*  f0ac9ec:	46002202 */ 	mul.s	$f8,$f4,$f0
/*  f0ac9f0:	46083281 */ 	sub.s	$f10,$f6,$f8
/*  f0ac9f4:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0ac9f8:	440d9000 */ 	mfc1	$t5,$f18
/*  f0ac9fc:	00000000 */ 	nop
/*  f0aca00:	a60d001c */ 	sh	$t5,0x1c($s0)
/*  f0aca04:	90cb0004 */ 	lbu	$t3,0x4($a2)
/*  f0aca08:	000b5140 */ 	sll	$t2,$t3,0x5
/*  f0aca0c:	a60a0020 */ 	sh	$t2,0x20($s0)
/*  f0aca10:	90cc0005 */ 	lbu	$t4,0x5($a2)
/*  f0aca14:	a205001f */ 	sb	$a1,0x1f($s0)
/*  f0aca18:	000cc940 */ 	sll	$t9,$t4,0x5
/*  f0aca1c:	a6190022 */ 	sh	$t9,0x22($s0)
/*  f0aca20:	c7a400d0 */ 	lwc1	$f4,0xd0($sp)
/*  f0aca24:	c7a80074 */ 	lwc1	$f8,0x74($sp)
/*  f0aca28:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0aca2c:	46083280 */ 	add.s	$f10,$f6,$f8
/*  f0aca30:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0aca34:	440f9000 */ 	mfc1	$t7,$f18
/*  f0aca38:	00000000 */ 	nop
/*  f0aca3c:	a60f0024 */ 	sh	$t7,0x24($s0)
/*  f0aca40:	c7a400d4 */ 	lwc1	$f4,0xd4($sp)
/*  f0aca44:	c7a80078 */ 	lwc1	$f8,0x78($sp)
/*  f0aca48:	3c0fb600 */ 	lui	$t7,0xb600
/*  f0aca4c:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0aca50:	46083280 */ 	add.s	$f10,$f6,$f8
/*  f0aca54:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0aca58:	44099000 */ 	mfc1	$t1,$f18
/*  f0aca5c:	00000000 */ 	nop
/*  f0aca60:	a6090026 */ 	sh	$t1,0x26($s0)
/*  f0aca64:	c7a400d8 */ 	lwc1	$f4,0xd8($sp)
/*  f0aca68:	c7a8007c */ 	lwc1	$f8,0x7c($sp)
/*  f0aca6c:	a600002c */ 	sh	$zero,0x2c($s0)
/*  f0aca70:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0aca74:	46083280 */ 	add.s	$f10,$f6,$f8
/*  f0aca78:	4600548d */ 	trunc.w.s	$f18,$f10
/*  f0aca7c:	440b9000 */ 	mfc1	$t3,$f18
/*  f0aca80:	00000000 */ 	nop
/*  f0aca84:	a60b0028 */ 	sh	$t3,0x28($s0)
/*  f0aca88:	90ca0005 */ 	lbu	$t2,0x5($a2)
/*  f0aca8c:	a205002b */ 	sb	$a1,0x2b($s0)
/*  f0aca90:	3c0b0102 */ 	lui	$t3,0x102
/*  f0aca94:	000a6140 */ 	sll	$t4,$t2,0x5
/*  f0aca98:	a60c002e */ 	sh	$t4,0x2e($s0)
/*  f0aca9c:	8fb900f0 */ 	lw	$t9,0xf0($sp)
/*  f0acaa0:	356b0040 */ 	ori	$t3,$t3,0x40
/*  f0acaa4:	27380008 */ 	addiu	$t8,$t9,0x8
/*  f0acaa8:	afb800f0 */ 	sw	$t8,0xf0($sp)
/*  f0acaac:	af2e0004 */ 	sw	$t6,0x4($t9)
/*  f0acab0:	af2f0000 */ 	sw	$t7,0x0($t9)
/*  f0acab4:	8fa900f0 */ 	lw	$t1,0xf0($sp)
/*  f0acab8:	252d0008 */ 	addiu	$t5,$t1,0x8
/*  f0acabc:	afad00f0 */ 	sw	$t5,0xf0($sp)
/*  f0acac0:	ad2b0000 */ 	sw	$t3,0x0($t1)
/*  f0acac4:	8fa400c8 */ 	lw	$a0,0xc8($sp)
/*  f0acac8:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0acacc:	afa90054 */ 	sw	$t1,0x54($sp)
/*  f0acad0:	8fa30054 */ 	lw	$v1,0x54($sp)
/*  f0acad4:	3c190704 */ 	lui	$t9,0x704
/*  f0acad8:	37390008 */ 	ori	$t9,$t9,0x8
/*  f0acadc:	ac620004 */ 	sw	$v0,0x4($v1)
/*  f0acae0:	8faa00f0 */ 	lw	$t2,0xf0($sp)
/*  f0acae4:	254c0008 */ 	addiu	$t4,$t2,0x8
/*  f0acae8:	afac00f0 */ 	sw	$t4,0xf0($sp)
/*  f0acaec:	ad590000 */ 	sw	$t9,0x0($t2)
/*  f0acaf0:	8fa400c4 */ 	lw	$a0,0xc4($sp)
/*  f0acaf4:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0acaf8:	afaa0050 */ 	sw	$t2,0x50($sp)
/*  f0acafc:	8fa80050 */ 	lw	$t0,0x50($sp)
/*  f0acb00:	24180002 */ 	addiu	$t8,$zero,0x2
/*  f0acb04:	240f0001 */ 	addiu	$t7,$zero,0x1
/*  f0acb08:	ad020004 */ 	sw	$v0,0x4($t0)
/*  f0acb0c:	afa00018 */ 	sw	$zero,0x18($sp)
/*  f0acb10:	afaf0014 */ 	sw	$t7,0x14($sp)
/*  f0acb14:	afb80010 */ 	sw	$t8,0x10($sp)
/*  f0acb18:	8fa500f4 */ 	lw	$a1,0xf4($sp)
/*  f0acb1c:	27a400f0 */ 	addiu	$a0,$sp,0xf0
/*  f0acb20:	24060004 */ 	addiu	$a2,$zero,0x4
/*  f0acb24:	0fc2ce70 */ 	jal	texSelect
/*  f0acb28:	24070001 */ 	addiu	$a3,$zero,0x1
/*  f0acb2c:	8fae00f0 */ 	lw	$t6,0xf0($sp)
/*  f0acb30:	3c0d0430 */ 	lui	$t5,0x430
/*  f0acb34:	35ad0030 */ 	ori	$t5,$t5,0x30
/*  f0acb38:	25c90008 */ 	addiu	$t1,$t6,0x8
/*  f0acb3c:	afa900f0 */ 	sw	$t1,0xf0($sp)
/*  f0acb40:	adcd0000 */ 	sw	$t5,0x0($t6)
/*  f0acb44:	02002025 */ 	or	$a0,$s0,$zero
/*  f0acb48:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0acb4c:	afae004c */ 	sw	$t6,0x4c($sp)
/*  f0acb50:	8fa5004c */ 	lw	$a1,0x4c($sp)
/*  f0acb54:	3c0cb100 */ 	lui	$t4,0xb100
/*  f0acb58:	358c0002 */ 	ori	$t4,$t4,0x2
/*  f0acb5c:	aca20004 */ 	sw	$v0,0x4($a1)
/*  f0acb60:	8fab00f0 */ 	lw	$t3,0xf0($sp)
/*  f0acb64:	24193210 */ 	addiu	$t9,$zero,0x3210
/*  f0acb68:	256a0008 */ 	addiu	$t2,$t3,0x8
/*  f0acb6c:	afaa00f0 */ 	sw	$t2,0xf0($sp)
/*  f0acb70:	ad790004 */ 	sw	$t9,0x4($t3)
/*  f0acb74:	ad6c0000 */ 	sw	$t4,0x0($t3)
/*  f0acb78:	8fa200f0 */ 	lw	$v0,0xf0($sp)
.L0f0acb7c:
/*  f0acb7c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0acb80:	8fb00028 */ 	lw	$s0,0x28($sp)
/*  f0acb84:	27bd00f0 */ 	addiu	$sp,$sp,0xf0
/*  f0acb88:	03e00008 */ 	jr	$ra
/*  f0acb8c:	00000000 */ 	nop
);

GLOBAL_ASM(
glabel func0f0acb90
.late_rodata
glabel var7f1acc6c
.word 0x3fb50481
glabel var7f1acc70
.word func0f0acb90+0x130 # f0accc0
glabel var7f1acc74
.word func0f0acb90+0x130 # f0accc0
glabel var7f1acc78
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acc7c
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acc80
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acc84
.word func0f0acb90+0x110 # f0acca0
glabel var7f1acc88
.word func0f0acb90+0x130 # f0accc0
glabel var7f1acc8c
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acc90
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acc94
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acc98
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acc9c
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acca0
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acca4
.word func0f0acb90+0x13c # f0acccc
glabel var7f1acca8
.word func0f0acb90+0x130 # f0accc0
glabel var7f1accac
.word func0f0acb90+0x13c # f0acccc
glabel var7f1accb0
.word func0f0acb90+0x130 # f0accc0
glabel var7f1accb4
.word func0f0acb90+0x13c # f0acccc
glabel var7f1accb8
.word func0f0acb90+0x13c # f0acccc
glabel var7f1accbc
.word func0f0acb90+0x13c # f0acccc
glabel var7f1accc0
.word func0f0acb90+0x13c # f0acccc
glabel var7f1accc4
.word func0f0acb90+0x13c # f0acccc
glabel var7f1accc8
.word func0f0acb90+0x120 # f0accb0
glabel var7f1acccc
.word 0x3dcccccd
glabel var7f1accd0
.word 0xc6ea6000
glabel var7f1accd4
.word 0x46ea6000
glabel var7f1accd8
.word 0x3f666666
glabel var7f1accdc
.word 0x3f666666
glabel var7f1acce0
.word 0x3f666666
glabel var7f1acce4
.word 0x3f666666
glabel var7f1acce8
.word 0x3f666666
glabel var7f1accec
.word 0x3f666666
glabel var7f1accf0
.word 0x3fb50481
glabel var7f1accf4
.word 0x3f666666
.text
/*  f0acb90:	27bdfe70 */ 	addiu	$sp,$sp,-400
/*  f0acb94:	afb00024 */ 	sw	$s0,0x24($sp)
/*  f0acb98:	30f000ff */ 	andi	$s0,$a3,0xff
/*  f0acb9c:	afb10028 */ 	sw	$s1,0x28($sp)
/*  f0acba0:	2a010005 */ 	slti	$at,$s0,0x5
/*  f0acba4:	00a08825 */ 	or	$s1,$a1,$zero
/*  f0acba8:	afbf002c */ 	sw	$ra,0x2c($sp)
/*  f0acbac:	afa40190 */ 	sw	$a0,0x190($sp)
/*  f0acbb0:	afa60198 */ 	sw	$a2,0x198($sp)
/*  f0acbb4:	102003fb */ 	beqz	$at,.L0f0adba4
/*  f0acbb8:	afa7019c */ 	sw	$a3,0x19c($sp)
/*  f0acbbc:	80ac0000 */ 	lb	$t4,0x0($a1)
/*  f0acbc0:	058203f9 */ 	bltzl	$t4,.L0f0adba8
/*  f0acbc4:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0acbc8:	0fc59e73 */ 	jal	gfxAllocateColours
/*  f0acbcc:	24040001 */ 	addiu	$a0,$zero,0x1
/*  f0acbd0:	3c09800a */ 	lui	$t1,%hi(g_Vars+0x284)
/*  f0acbd4:	8d29a244 */ 	lw	$t1,%lo(g_Vars+0x284)($t1)
/*  f0acbd8:	afa20144 */ 	sw	$v0,0x144($sp)
/*  f0acbdc:	3c0d8007 */ 	lui	$t5,%hi(var8007050c)
/*  f0acbe0:	25381bb0 */ 	addiu	$t8,$t1,0x1bb0
/*  f0acbe4:	afb80134 */ 	sw	$t8,0x134($sp)
/*  f0acbe8:	c6240024 */ 	lwc1	$f4,0x24($s1)
/*  f0acbec:	25ad050c */ 	addiu	$t5,$t5,%lo(var8007050c)
/*  f0acbf0:	27af0100 */ 	addiu	$t7,$sp,0x100
/*  f0acbf4:	e7a4012c */ 	swc1	$f4,0x12c($sp)
/*  f0acbf8:	8da10000 */ 	lw	$at,0x0($t5)
/*  f0acbfc:	c6220028 */ 	lwc1	$f2,0x28($s1)
/*  f0acc00:	8daa0004 */ 	lw	$t2,0x4($t5)
/*  f0acc04:	ade10000 */ 	sw	$at,0x0($t7)
/*  f0acc08:	8da10008 */ 	lw	$at,0x8($t5)
/*  f0acc0c:	3c0c8007 */ 	lui	$t4,%hi(var80070518)
/*  f0acc10:	258c0518 */ 	addiu	$t4,$t4,%lo(var80070518)
/*  f0acc14:	adea0004 */ 	sw	$t2,0x4($t7)
/*  f0acc18:	ade10008 */ 	sw	$at,0x8($t7)
/*  f0acc1c:	8d810000 */ 	lw	$at,0x0($t4)
/*  f0acc20:	27b900f4 */ 	addiu	$t9,$sp,0xf4
/*  f0acc24:	8d8b0004 */ 	lw	$t3,0x4($t4)
/*  f0acc28:	af210000 */ 	sw	$at,0x0($t9)
/*  f0acc2c:	8d810008 */ 	lw	$at,0x8($t4)
/*  f0acc30:	3c0f800b */ 	lui	$t7,%hi(g_TexBeamConfigs)
/*  f0acc34:	af2b0004 */ 	sw	$t3,0x4($t9)
/*  f0acc38:	af210008 */ 	sw	$at,0x8($t9)
/*  f0acc3c:	8defb574 */ 	lw	$t7,%lo(g_TexBeamConfigs)($t7)
/*  f0acc40:	00107080 */ 	sll	$t6,$s0,0x2
/*  f0acc44:	3c017f1b */ 	lui	$at,%hi(var7f1acc6c)
/*  f0acc48:	01d07023 */ 	subu	$t6,$t6,$s0
/*  f0acc4c:	c42acc6c */ 	lwc1	$f10,%lo(var7f1acc6c)($at)
/*  f0acc50:	000e7080 */ 	sll	$t6,$t6,0x2
/*  f0acc54:	01cf6821 */ 	addu	$t5,$t6,$t7
/*  f0acc58:	afad00ec */ 	sw	$t5,0xec($sp)
/*  f0acc5c:	e7a20124 */ 	swc1	$f2,0x124($sp)
/*  f0acc60:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0acc64:	e7aa00f0 */ 	swc1	$f10,0xf0($sp)
/*  f0acc68:	afa200e4 */ 	sw	$v0,0xe4($sp)
/*  f0acc6c:	82230001 */ 	lb	$v1,0x1($s1)
/*  f0acc70:	3c10800b */ 	lui	$s0,%hi(g_TexBeamConfigs)
/*  f0acc74:	2610b574 */ 	addiu	$s0,$s0,%lo(g_TexBeamConfigs)
/*  f0acc78:	246afffa */ 	addiu	$t2,$v1,-6
/*  f0acc7c:	2d410017 */ 	sltiu	$at,$t2,0x17
/*  f0acc80:	10200012 */ 	beqz	$at,.L0f0acccc
/*  f0acc84:	c7a20124 */ 	lwc1	$f2,0x124($sp)
/*  f0acc88:	000a5080 */ 	sll	$t2,$t2,0x2
/*  f0acc8c:	3c017f1b */ 	lui	$at,%hi(var7f1acc70)
/*  f0acc90:	002a0821 */ 	addu	$at,$at,$t2
/*  f0acc94:	8c2acc70 */ 	lw	$t2,%lo(var7f1acc70)($at)
/*  f0acc98:	01400008 */ 	jr	$t2
/*  f0acc9c:	00000000 */ 	nop
/*  f0acca0:	8e180000 */ 	lw	$t8,0x0($s0)
/*  f0acca4:	2709000c */ 	addiu	$t1,$t8,0xc
/*  f0acca8:	10000008 */ 	b	.L0f0acccc
/*  f0accac:	afa900ec */ 	sw	$t1,0xec($sp)
/*  f0accb0:	8e190000 */ 	lw	$t9,0x0($s0)
/*  f0accb4:	272c0024 */ 	addiu	$t4,$t9,0x24
/*  f0accb8:	10000004 */ 	b	.L0f0acccc
/*  f0accbc:	afac00ec */ 	sw	$t4,0xec($sp)
/*  f0accc0:	8e0b0000 */ 	lw	$t3,0x0($s0)
/*  f0accc4:	256e0030 */ 	addiu	$t6,$t3,0x30
/*  f0accc8:	afae00ec */ 	sw	$t6,0xec($sp)
.L0f0acccc:
/*  f0acccc:	2401ffff */ 	addiu	$at,$zero,-1
/*  f0accd0:	10610004 */ 	beq	$v1,$at,.L0f0acce4
/*  f0accd4:	8fad0144 */ 	lw	$t5,0x144($sp)
/*  f0accd8:	2401000b */ 	addiu	$at,$zero,0xb
/*  f0accdc:	14610004 */ 	bne	$v1,$at,.L0f0accf0
/*  f0acce0:	8fb80144 */ 	lw	$t8,0x144($sp)
.L0f0acce4:
/*  f0acce4:	240fff7f */ 	addiu	$t7,$zero,-129
/*  f0acce8:	10000003 */ 	b	.L0f0accf8
/*  f0accec:	adaf0000 */ 	sw	$t7,0x0($t5)
.L0f0accf0:
/*  f0accf0:	240affff */ 	addiu	$t2,$zero,-1
/*  f0accf4:	af0a0000 */ 	sw	$t2,0x0($t8)
.L0f0accf8:
/*  f0accf8:	82230001 */ 	lb	$v1,0x1($s1)
/*  f0accfc:	2401001d */ 	addiu	$at,$zero,0x1d
/*  f0acd00:	3c09800b */ 	lui	$t1,%hi(g_TexLaserConfigs)
/*  f0acd04:	54610008 */ 	bnel	$v1,$at,.L0f0acd28
/*  f0acd08:	2401fffe */ 	addiu	$at,$zero,-2
/*  f0acd0c:	8d29b578 */ 	lw	$t1,%lo(g_TexLaserConfigs)($t1)
/*  f0acd10:	3c014248 */ 	lui	$at,0x4248
/*  f0acd14:	44819000 */ 	mtc1	$at,$f18
/*  f0acd18:	afa900ec */ 	sw	$t1,0xec($sp)
/*  f0acd1c:	10000029 */ 	b	.L0f0acdc4
/*  f0acd20:	e7b20130 */ 	swc1	$f18,0x130($sp)
/*  f0acd24:	2401fffe */ 	addiu	$at,$zero,-2
.L0f0acd28:
/*  f0acd28:	14610022 */ 	bne	$v1,$at,.L0f0acdb4
/*  f0acd2c:	3c19800b */ 	lui	$t9,%hi(g_TexLaserConfigs)
/*  f0acd30:	3c014120 */ 	lui	$at,0x4120
/*  f0acd34:	44813000 */ 	mtc1	$at,$f6
/*  f0acd38:	8f39b578 */ 	lw	$t9,%lo(g_TexLaserConfigs)($t9)
/*  f0acd3c:	e7a20124 */ 	swc1	$f2,0x124($sp)
/*  f0acd40:	e7a60130 */ 	swc1	$f6,0x130($sp)
/*  f0acd44:	0c004b70 */ 	jal	random
/*  f0acd48:	afb900ec */ 	sw	$t9,0xec($sp)
/*  f0acd4c:	24010032 */ 	addiu	$at,$zero,0x32
/*  f0acd50:	0041001b */ 	divu	$zero,$v0,$at
/*  f0acd54:	8fae0144 */ 	lw	$t6,0x144($sp)
/*  f0acd58:	00006010 */ 	mfhi	$t4
/*  f0acd5c:	258b0096 */ 	addiu	$t3,$t4,0x96
/*  f0acd60:	0c004b70 */ 	jal	random
/*  f0acd64:	a1cb0003 */ 	sb	$t3,0x3($t6)
/*  f0acd68:	24010005 */ 	addiu	$at,$zero,0x5
/*  f0acd6c:	0041001b */ 	divu	$zero,$v0,$at
/*  f0acd70:	00007810 */ 	mfhi	$t7
/*  f0acd74:	c7a20124 */ 	lwc1	$f2,0x124($sp)
/*  f0acd78:	15e0000c */ 	bnez	$t7,.L0f0acdac
/*  f0acd7c:	00000000 */ 	nop
/*  f0acd80:	0c004b70 */ 	jal	random
/*  f0acd84:	e7a20124 */ 	swc1	$f2,0x124($sp)
/*  f0acd88:	24010064 */ 	addiu	$at,$zero,0x64
/*  f0acd8c:	0041001b */ 	divu	$zero,$v0,$at
/*  f0acd90:	8fb80144 */ 	lw	$t8,0x144($sp)
/*  f0acd94:	00006810 */ 	mfhi	$t5
/*  f0acd98:	240a00ff */ 	addiu	$t2,$zero,0xff
/*  f0acd9c:	014d1823 */ 	subu	$v1,$t2,$t5
/*  f0acda0:	c7a20124 */ 	lwc1	$f2,0x124($sp)
/*  f0acda4:	a3030000 */ 	sb	$v1,0x0($t8)
/*  f0acda8:	a3030001 */ 	sb	$v1,0x1($t8)
.L0f0acdac:
/*  f0acdac:	10000005 */ 	b	.L0f0acdc4
/*  f0acdb0:	82230001 */ 	lb	$v1,0x1($s1)
.L0f0acdb4:
/*  f0acdb4:	3c0141f0 */ 	lui	$at,0x41f0
/*  f0acdb8:	44812000 */ 	mtc1	$at,$f4
/*  f0acdbc:	00000000 */ 	nop
/*  f0acdc0:	e7a40130 */ 	swc1	$f4,0x130($sp)
.L0f0acdc4:
/*  f0acdc4:	2861fffe */ 	slti	$at,$v1,-2
/*  f0acdc8:	1020000d */ 	beqz	$at,.L0f0ace00
/*  f0acdcc:	24690003 */ 	addiu	$t1,$v1,0x3
/*  f0acdd0:	44895000 */ 	mtc1	$t1,$f10
/*  f0acdd4:	3c013f80 */ 	lui	$at,0x3f80
/*  f0acdd8:	44813000 */ 	mtc1	$at,$f6
/*  f0acddc:	46805020 */ 	cvt.s.w	$f0,$f10
/*  f0acde0:	c7aa0130 */ 	lwc1	$f10,0x130($sp)
/*  f0acde4:	8e190000 */ 	lw	$t9,0x0($s0)
/*  f0acde8:	272c0030 */ 	addiu	$t4,$t9,0x30
/*  f0acdec:	46000200 */ 	add.s	$f8,$f0,$f0
/*  f0acdf0:	afac00ec */ 	sw	$t4,0xec($sp)
/*  f0acdf4:	46064100 */ 	add.s	$f4,$f8,$f6
/*  f0acdf8:	460a2202 */ 	mul.s	$f8,$f4,$f10
/*  f0acdfc:	e7a80130 */ 	swc1	$f8,0x130($sp)
.L0f0ace00:
/*  f0ace00:	c6260004 */ 	lwc1	$f6,0x4($s1)
/*  f0ace04:	44807000 */ 	mtc1	$zero,$f14
/*  f0ace08:	27a4010c */ 	addiu	$a0,$sp,0x10c
/*  f0ace0c:	e7a60138 */ 	swc1	$f6,0x138($sp)
/*  f0ace10:	c6240008 */ 	lwc1	$f4,0x8($s1)
/*  f0ace14:	4602703c */ 	c.lt.s	$f14,$f2
/*  f0ace18:	c7a6012c */ 	lwc1	$f6,0x12c($sp)
/*  f0ace1c:	e7a4013c */ 	swc1	$f4,0x13c($sp)
/*  f0ace20:	c62a000c */ 	lwc1	$f10,0xc($s1)
/*  f0ace24:	27a50110 */ 	addiu	$a1,$sp,0x110
/*  f0ace28:	45000011 */ 	bc1f	.L0f0ace70
/*  f0ace2c:	e7aa0140 */ 	swc1	$f10,0x140($sp)
/*  f0ace30:	c6260010 */ 	lwc1	$f6,0x10($s1)
/*  f0ace34:	c7a80138 */ 	lwc1	$f8,0x138($sp)
/*  f0ace38:	46061102 */ 	mul.s	$f4,$f2,$f6
/*  f0ace3c:	c7a6013c */ 	lwc1	$f6,0x13c($sp)
/*  f0ace40:	46044280 */ 	add.s	$f10,$f8,$f4
/*  f0ace44:	e7aa0138 */ 	swc1	$f10,0x138($sp)
/*  f0ace48:	c6280014 */ 	lwc1	$f8,0x14($s1)
/*  f0ace4c:	46081102 */ 	mul.s	$f4,$f2,$f8
/*  f0ace50:	c7a80140 */ 	lwc1	$f8,0x140($sp)
/*  f0ace54:	46043280 */ 	add.s	$f10,$f6,$f4
/*  f0ace58:	e7aa013c */ 	swc1	$f10,0x13c($sp)
/*  f0ace5c:	c6260018 */ 	lwc1	$f6,0x18($s1)
/*  f0ace60:	46061102 */ 	mul.s	$f4,$f2,$f6
/*  f0ace64:	46044280 */ 	add.s	$f10,$f8,$f4
/*  f0ace68:	10000004 */ 	b	.L0f0ace7c
/*  f0ace6c:	e7aa0140 */ 	swc1	$f10,0x140($sp)
.L0f0ace70:
/*  f0ace70:	46023200 */ 	add.s	$f8,$f6,$f2
/*  f0ace74:	46007086 */ 	mov.s	$f2,$f14
/*  f0ace78:	e7a8012c */ 	swc1	$f8,0x12c($sp)
.L0f0ace7c:
/*  f0ace7c:	c7a4012c */ 	lwc1	$f4,0x12c($sp)
/*  f0ace80:	c620001c */ 	lwc1	$f0,0x1c($s1)
/*  f0ace84:	27a60114 */ 	addiu	$a2,$sp,0x114
/*  f0ace88:	46041280 */ 	add.s	$f10,$f2,$f4
/*  f0ace8c:	460a003c */ 	c.lt.s	$f0,$f10
/*  f0ace90:	00000000 */ 	nop
/*  f0ace94:	45020004 */ 	bc1fl	.L0f0acea8
/*  f0ace98:	c7b0012c */ 	lwc1	$f16,0x12c($sp)
/*  f0ace9c:	46020401 */ 	sub.s	$f16,$f0,$f2
/*  f0acea0:	e7b0012c */ 	swc1	$f16,0x12c($sp)
/*  f0acea4:	c7b0012c */ 	lwc1	$f16,0x12c($sp)
.L0f0acea8:
/*  f0acea8:	c62c0018 */ 	lwc1	$f12,0x18($s1)
/*  f0aceac:	c7a60140 */ 	lwc1	$f6,0x140($sp)
/*  f0aceb0:	8fa20134 */ 	lw	$v0,0x134($sp)
/*  f0aceb4:	460c8202 */ 	mul.s	$f8,$f16,$f12
/*  f0aceb8:	c6220014 */ 	lwc1	$f2,0x14($s1)
/*  f0acebc:	c44a0008 */ 	lwc1	$f10,0x8($v0)
/*  f0acec0:	e7a60030 */ 	swc1	$f6,0x30($sp)
/*  f0acec4:	46083100 */ 	add.s	$f4,$f6,$f8
/*  f0acec8:	c4460004 */ 	lwc1	$f6,0x4($v0)
/*  f0acecc:	46045201 */ 	sub.s	$f8,$f10,$f4
/*  f0aced0:	46081282 */ 	mul.s	$f10,$f2,$f8
/*  f0aced4:	c7a8013c */ 	lwc1	$f8,0x13c($sp)
/*  f0aced8:	46101102 */ 	mul.s	$f4,$f2,$f16
/*  f0acedc:	46082100 */ 	add.s	$f4,$f4,$f8
/*  f0acee0:	46043181 */ 	sub.s	$f6,$f6,$f4
/*  f0acee4:	460c3102 */ 	mul.s	$f4,$f6,$f12
/*  f0acee8:	46045181 */ 	sub.s	$f6,$f10,$f4
/*  f0aceec:	c7aa0138 */ 	lwc1	$f10,0x138($sp)
/*  f0acef0:	e7a6010c */ 	swc1	$f6,0x10c($sp)
/*  f0acef4:	c6200010 */ 	lwc1	$f0,0x10($s1)
/*  f0acef8:	c62c0018 */ 	lwc1	$f12,0x18($s1)
/*  f0acefc:	e7a80034 */ 	swc1	$f8,0x34($sp)
/*  f0acf00:	46008102 */ 	mul.s	$f4,$f16,$f0
/*  f0acf04:	c4480000 */ 	lwc1	$f8,0x0($v0)
/*  f0acf08:	e7a60038 */ 	swc1	$f6,0x38($sp)
/*  f0acf0c:	c7a60030 */ 	lwc1	$f6,0x30($sp)
/*  f0acf10:	46045100 */ 	add.s	$f4,$f10,$f4
/*  f0acf14:	46044201 */ 	sub.s	$f8,$f8,$f4
/*  f0acf18:	46086102 */ 	mul.s	$f4,$f12,$f8
/*  f0acf1c:	00000000 */ 	nop
/*  f0acf20:	46106202 */ 	mul.s	$f8,$f12,$f16
/*  f0acf24:	46064200 */ 	add.s	$f8,$f8,$f6
/*  f0acf28:	c4460008 */ 	lwc1	$f6,0x8($v0)
/*  f0acf2c:	46083181 */ 	sub.s	$f6,$f6,$f8
/*  f0acf30:	46003202 */ 	mul.s	$f8,$f6,$f0
/*  f0acf34:	46082181 */ 	sub.s	$f6,$f4,$f8
/*  f0acf38:	c7a40034 */ 	lwc1	$f4,0x34($sp)
/*  f0acf3c:	e7a60110 */ 	swc1	$f6,0x110($sp)
/*  f0acf40:	c6220014 */ 	lwc1	$f2,0x14($s1)
/*  f0acf44:	c6200010 */ 	lwc1	$f0,0x10($s1)
/*  f0acf48:	46028202 */ 	mul.s	$f8,$f16,$f2
/*  f0acf4c:	46082100 */ 	add.s	$f4,$f4,$f8
/*  f0acf50:	c4480004 */ 	lwc1	$f8,0x4($v0)
/*  f0acf54:	46044201 */ 	sub.s	$f8,$f8,$f4
/*  f0acf58:	46080102 */ 	mul.s	$f4,$f0,$f8
/*  f0acf5c:	00000000 */ 	nop
/*  f0acf60:	46100202 */ 	mul.s	$f8,$f0,$f16
/*  f0acf64:	460a4200 */ 	add.s	$f8,$f8,$f10
/*  f0acf68:	c44a0000 */ 	lwc1	$f10,0x0($v0)
/*  f0acf6c:	46085281 */ 	sub.s	$f10,$f10,$f8
/*  f0acf70:	46025202 */ 	mul.s	$f8,$f10,$f2
/*  f0acf74:	46082281 */ 	sub.s	$f10,$f4,$f8
/*  f0acf78:	c7a40038 */ 	lwc1	$f4,0x38($sp)
/*  f0acf7c:	46047032 */ 	c.eq.s	$f14,$f4
/*  f0acf80:	e7aa0114 */ 	swc1	$f10,0x114($sp)
/*  f0acf84:	45000009 */ 	bc1f	.L0f0acfac
/*  f0acf88:	00000000 */ 	nop
/*  f0acf8c:	46067032 */ 	c.eq.s	$f14,$f6
/*  f0acf90:	00000000 */ 	nop
/*  f0acf94:	45000005 */ 	bc1f	.L0f0acfac
/*  f0acf98:	00000000 */ 	nop
/*  f0acf9c:	460a7032 */ 	c.eq.s	$f14,$f10
/*  f0acfa0:	00000000 */ 	nop
/*  f0acfa4:	4503000f */ 	bc1tl	.L0f0acfe4
/*  f0acfa8:	c7a60130 */ 	lwc1	$f6,0x130($sp)
.L0f0acfac:
/*  f0acfac:	0c0011e4 */ 	jal	guNormalize
/*  f0acfb0:	e7b0012c */ 	swc1	$f16,0x12c($sp)
/*  f0acfb4:	c7a00130 */ 	lwc1	$f0,0x130($sp)
/*  f0acfb8:	c7a8010c */ 	lwc1	$f8,0x10c($sp)
/*  f0acfbc:	c7a60110 */ 	lwc1	$f6,0x110($sp)
/*  f0acfc0:	46004102 */ 	mul.s	$f4,$f8,$f0
/*  f0acfc4:	c7a80114 */ 	lwc1	$f8,0x114($sp)
/*  f0acfc8:	46003282 */ 	mul.s	$f10,$f6,$f0
/*  f0acfcc:	e7a4010c */ 	swc1	$f4,0x10c($sp)
/*  f0acfd0:	46004102 */ 	mul.s	$f4,$f8,$f0
/*  f0acfd4:	e7aa0110 */ 	swc1	$f10,0x110($sp)
/*  f0acfd8:	10000006 */ 	b	.L0f0acff4
/*  f0acfdc:	e7a40114 */ 	swc1	$f4,0x114($sp)
/*  f0acfe0:	c7a60130 */ 	lwc1	$f6,0x130($sp)
.L0f0acfe4:
/*  f0acfe4:	e7ae010c */ 	swc1	$f14,0x10c($sp)
/*  f0acfe8:	e7ae0114 */ 	swc1	$f14,0x114($sp)
/*  f0acfec:	e7b0012c */ 	swc1	$f16,0x12c($sp)
/*  f0acff0:	e7a60110 */ 	swc1	$f6,0x110($sp)
.L0f0acff4:
/*  f0acff4:	c62a0014 */ 	lwc1	$f10,0x14($s1)
/*  f0acff8:	c7a80114 */ 	lwc1	$f8,0x114($sp)
/*  f0acffc:	c7a60110 */ 	lwc1	$f6,0x110($sp)
/*  f0ad000:	27a40118 */ 	addiu	$a0,$sp,0x118
/*  f0ad004:	46085102 */ 	mul.s	$f4,$f10,$f8
/*  f0ad008:	c62a0018 */ 	lwc1	$f10,0x18($s1)
/*  f0ad00c:	27a5011c */ 	addiu	$a1,$sp,0x11c
/*  f0ad010:	27a60120 */ 	addiu	$a2,$sp,0x120
/*  f0ad014:	460a3282 */ 	mul.s	$f10,$f6,$f10
/*  f0ad018:	460a2101 */ 	sub.s	$f4,$f4,$f10
/*  f0ad01c:	e7a40118 */ 	swc1	$f4,0x118($sp)
/*  f0ad020:	c62a0018 */ 	lwc1	$f10,0x18($s1)
/*  f0ad024:	c7a4010c */ 	lwc1	$f4,0x10c($sp)
/*  f0ad028:	e7a60038 */ 	swc1	$f6,0x38($sp)
/*  f0ad02c:	c6260010 */ 	lwc1	$f6,0x10($s1)
/*  f0ad030:	46045282 */ 	mul.s	$f10,$f10,$f4
/*  f0ad034:	00000000 */ 	nop
/*  f0ad038:	46064202 */ 	mul.s	$f8,$f8,$f6
/*  f0ad03c:	46085181 */ 	sub.s	$f6,$f10,$f8
/*  f0ad040:	c7a80038 */ 	lwc1	$f8,0x38($sp)
/*  f0ad044:	e7a6011c */ 	swc1	$f6,0x11c($sp)
/*  f0ad048:	c62a0010 */ 	lwc1	$f10,0x10($s1)
/*  f0ad04c:	46085182 */ 	mul.s	$f6,$f10,$f8
/*  f0ad050:	c62a0014 */ 	lwc1	$f10,0x14($s1)
/*  f0ad054:	460a2202 */ 	mul.s	$f8,$f4,$f10
/*  f0ad058:	46083101 */ 	sub.s	$f4,$f6,$f8
/*  f0ad05c:	0c0011e4 */ 	jal	guNormalize
/*  f0ad060:	e7a40120 */ 	swc1	$f4,0x120($sp)
/*  f0ad064:	c7a00130 */ 	lwc1	$f0,0x130($sp)
/*  f0ad068:	c7aa0118 */ 	lwc1	$f10,0x118($sp)
/*  f0ad06c:	c7a8011c */ 	lwc1	$f8,0x11c($sp)
/*  f0ad070:	2401001d */ 	addiu	$at,$zero,0x1d
/*  f0ad074:	46005182 */ 	mul.s	$f6,$f10,$f0
/*  f0ad078:	c7aa0120 */ 	lwc1	$f10,0x120($sp)
/*  f0ad07c:	46004102 */ 	mul.s	$f4,$f8,$f0
/*  f0ad080:	e7a60118 */ 	swc1	$f6,0x118($sp)
/*  f0ad084:	46005182 */ 	mul.s	$f6,$f10,$f0
/*  f0ad088:	e7a4011c */ 	swc1	$f4,0x11c($sp)
/*  f0ad08c:	e7a60120 */ 	swc1	$f6,0x120($sp)
/*  f0ad090:	822b0001 */ 	lb	$t3,0x1($s1)
/*  f0ad094:	15610005 */ 	bne	$t3,$at,.L0f0ad0ac
/*  f0ad098:	00000000 */ 	nop
/*  f0ad09c:	0fc59e59 */ 	jal	gfxAllocateVertices
/*  f0ad0a0:	24040008 */ 	addiu	$a0,$zero,0x8
/*  f0ad0a4:	10000004 */ 	b	.L0f0ad0b8
/*  f0ad0a8:	00408025 */ 	or	$s0,$v0,$zero
.L0f0ad0ac:
/*  f0ad0ac:	0fc59e59 */ 	jal	gfxAllocateVertices
/*  f0ad0b0:	24040004 */ 	addiu	$a0,$zero,0x4
/*  f0ad0b4:	00408025 */ 	or	$s0,$v0,$zero
.L0f0ad0b8:
/*  f0ad0b8:	0fc59e66 */ 	jal	gfxAllocateMatrix
/*  f0ad0bc:	00000000 */ 	nop
/*  f0ad0c0:	44807000 */ 	mtc1	$zero,$f14
/*  f0ad0c4:	c7a8012c */ 	lwc1	$f8,0x12c($sp)
/*  f0ad0c8:	afa20188 */ 	sw	$v0,0x188($sp)
/*  f0ad0cc:	c7ac0138 */ 	lwc1	$f12,0x138($sp)
/*  f0ad0d0:	4608703c */ 	c.lt.s	$f14,$f8
/*  f0ad0d4:	3c01c6fa */ 	lui	$at,0xc6fa
/*  f0ad0d8:	450202b3 */ 	bc1fl	.L0f0adba8
/*  f0ad0dc:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad0e0:	44817000 */ 	mtc1	$at,$f14
/*  f0ad0e4:	3c0146fa */ 	lui	$at,0x46fa
/*  f0ad0e8:	460c703c */ 	c.lt.s	$f14,$f12
/*  f0ad0ec:	00000000 */ 	nop
/*  f0ad0f0:	450202ad */ 	bc1fl	.L0f0adba8
/*  f0ad0f4:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad0f8:	44811000 */ 	mtc1	$at,$f2
/*  f0ad0fc:	c7b0013c */ 	lwc1	$f16,0x13c($sp)
/*  f0ad100:	4602603c */ 	c.lt.s	$f12,$f2
/*  f0ad104:	00000000 */ 	nop
/*  f0ad108:	450202a7 */ 	bc1fl	.L0f0adba8
/*  f0ad10c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad110:	4610703c */ 	c.lt.s	$f14,$f16
/*  f0ad114:	00000000 */ 	nop
/*  f0ad118:	450202a3 */ 	bc1fl	.L0f0adba8
/*  f0ad11c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad120:	4602803c */ 	c.lt.s	$f16,$f2
/*  f0ad124:	c7b20140 */ 	lwc1	$f18,0x140($sp)
/*  f0ad128:	4502029f */ 	bc1fl	.L0f0adba8
/*  f0ad12c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad130:	4612703c */ 	c.lt.s	$f14,$f18
/*  f0ad134:	00000000 */ 	nop
/*  f0ad138:	4502029b */ 	bc1fl	.L0f0adba8
/*  f0ad13c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad140:	4602903c */ 	c.lt.s	$f18,$f2
/*  f0ad144:	24080001 */ 	addiu	$t0,$zero,0x1
/*  f0ad148:	27a40138 */ 	addiu	$a0,$sp,0x138
/*  f0ad14c:	27a50148 */ 	addiu	$a1,$sp,0x148
/*  f0ad150:	45020295 */ 	bc1fl	.L0f0adba8
/*  f0ad154:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad158:	0c0059b7 */ 	jal	mtx4LoadTranslation
/*  f0ad15c:	afa800d8 */ 	sw	$t0,0xd8($sp)
/*  f0ad160:	3c017f1b */ 	lui	$at,%hi(var7f1acccc)
/*  f0ad164:	c42ccccc */ 	lwc1	$f12,%lo(var7f1acccc)($at)
/*  f0ad168:	0c0057c1 */ 	jal	mtx00015f04
/*  f0ad16c:	27a50148 */ 	addiu	$a1,$sp,0x148
/*  f0ad170:	8fa400e4 */ 	lw	$a0,0xe4($sp)
/*  f0ad174:	0c0056f8 */ 	jal	mtx00015be0
/*  f0ad178:	27a50148 */ 	addiu	$a1,$sp,0x148
/*  f0ad17c:	3c0146fa */ 	lui	$at,0x46fa
/*  f0ad180:	44811000 */ 	mtc1	$at,$f2
/*  f0ad184:	3c01c6fa */ 	lui	$at,0xc6fa
/*  f0ad188:	44817000 */ 	mtc1	$at,$f14
/*  f0ad18c:	8fa800d8 */ 	lw	$t0,0xd8($sp)
/*  f0ad190:	27a50148 */ 	addiu	$a1,$sp,0x148
/*  f0ad194:	27a70188 */ 	addiu	$a3,$sp,0x188
/*  f0ad198:	24040010 */ 	addiu	$a0,$zero,0x10
/*  f0ad19c:	00001025 */ 	or	$v0,$zero,$zero
.L0f0ad1a0:
/*  f0ad1a0:	00a01825 */ 	or	$v1,$a1,$zero
.L0f0ad1a4:
/*  f0ad1a4:	c4600000 */ 	lwc1	$f0,0x0($v1)
/*  f0ad1a8:	24420004 */ 	addiu	$v0,$v0,0x4
/*  f0ad1ac:	460e003c */ 	c.lt.s	$f0,$f14
/*  f0ad1b0:	00000000 */ 	nop
/*  f0ad1b4:	45010005 */ 	bc1t	.L0f0ad1cc
/*  f0ad1b8:	00000000 */ 	nop
/*  f0ad1bc:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0ad1c0:	00000000 */ 	nop
/*  f0ad1c4:	45000003 */ 	bc1f	.L0f0ad1d4
/*  f0ad1c8:	00000000 */ 	nop
.L0f0ad1cc:
/*  f0ad1cc:	10000003 */ 	b	.L0f0ad1dc
/*  f0ad1d0:	00004025 */ 	or	$t0,$zero,$zero
.L0f0ad1d4:
/*  f0ad1d4:	1444fff3 */ 	bne	$v0,$a0,.L0f0ad1a4
/*  f0ad1d8:	24630004 */ 	addiu	$v1,$v1,0x4
.L0f0ad1dc:
/*  f0ad1dc:	24a50010 */ 	addiu	$a1,$a1,0x10
/*  f0ad1e0:	54a7ffef */ 	bnel	$a1,$a3,.L0f0ad1a0
/*  f0ad1e4:	00001025 */ 	or	$v0,$zero,$zero
/*  f0ad1e8:	1100026e */ 	beqz	$t0,.L0f0adba4
/*  f0ad1ec:	27a40148 */ 	addiu	$a0,$sp,0x148
/*  f0ad1f0:	0c005815 */ 	jal	mtx00016054
/*  f0ad1f4:	8fa50188 */ 	lw	$a1,0x188($sp)
/*  f0ad1f8:	82230001 */ 	lb	$v1,0x1($s1)
/*  f0ad1fc:	2401fffe */ 	addiu	$at,$zero,-2
/*  f0ad200:	3c02800a */ 	lui	$v0,%hi(g_Vars)
/*  f0ad204:	1461006c */ 	bne	$v1,$at,.L0f0ad3b8
/*  f0ad208:	24429fc0 */ 	addiu	$v0,$v0,%lo(g_Vars)
/*  f0ad20c:	8c4e006c */ 	lw	$t6,0x6c($v0)
/*  f0ad210:	24010001 */ 	addiu	$at,$zero,0x1
/*  f0ad214:	00002825 */ 	or	$a1,$zero,$zero
/*  f0ad218:	11c00003 */ 	beqz	$t6,.L0f0ad228
/*  f0ad21c:	00001825 */ 	or	$v1,$zero,$zero
/*  f0ad220:	10000001 */ 	b	.L0f0ad228
/*  f0ad224:	24050001 */ 	addiu	$a1,$zero,0x1
.L0f0ad228:
/*  f0ad228:	8c4f0068 */ 	lw	$t7,0x68($v0)
/*  f0ad22c:	00002025 */ 	or	$a0,$zero,$zero
/*  f0ad230:	c7a0012c */ 	lwc1	$f0,0x12c($sp)
/*  f0ad234:	11e00003 */ 	beqz	$t7,.L0f0ad244
/*  f0ad238:	00000000 */ 	nop
/*  f0ad23c:	10000001 */ 	b	.L0f0ad244
/*  f0ad240:	24030001 */ 	addiu	$v1,$zero,0x1
.L0f0ad244:
/*  f0ad244:	8c4a0064 */ 	lw	$t2,0x64($v0)
/*  f0ad248:	11400003 */ 	beqz	$t2,.L0f0ad258
/*  f0ad24c:	00000000 */ 	nop
/*  f0ad250:	10000001 */ 	b	.L0f0ad258
/*  f0ad254:	24040001 */ 	addiu	$a0,$zero,0x1
.L0f0ad258:
/*  f0ad258:	8c4d0070 */ 	lw	$t5,0x70($v0)
/*  f0ad25c:	00001025 */ 	or	$v0,$zero,$zero
/*  f0ad260:	11a00003 */ 	beqz	$t5,.L0f0ad270
/*  f0ad264:	00000000 */ 	nop
/*  f0ad268:	10000001 */ 	b	.L0f0ad270
/*  f0ad26c:	24020001 */ 	addiu	$v0,$zero,0x1
.L0f0ad270:
/*  f0ad270:	0044c021 */ 	addu	$t8,$v0,$a0
/*  f0ad274:	03034821 */ 	addu	$t1,$t8,$v1
/*  f0ad278:	0125c821 */ 	addu	$t9,$t1,$a1
/*  f0ad27c:	5721004f */ 	bnel	$t9,$at,.L0f0ad3bc
/*  f0ad280:	3c014120 */ 	lui	$at,0x4120
/*  f0ad284:	c6240010 */ 	lwc1	$f4,0x10($s1)
/*  f0ad288:	c7a60138 */ 	lwc1	$f6,0x138($sp)
/*  f0ad28c:	8fa400e4 */ 	lw	$a0,0xe4($sp)
/*  f0ad290:	46002282 */ 	mul.s	$f10,$f4,$f0
/*  f0ad294:	27a500cc */ 	addiu	$a1,$sp,0xcc
/*  f0ad298:	46065200 */ 	add.s	$f8,$f10,$f6
/*  f0ad29c:	c7a6013c */ 	lwc1	$f6,0x13c($sp)
/*  f0ad2a0:	e7a800cc */ 	swc1	$f8,0xcc($sp)
/*  f0ad2a4:	c6240014 */ 	lwc1	$f4,0x14($s1)
/*  f0ad2a8:	46002282 */ 	mul.s	$f10,$f4,$f0
/*  f0ad2ac:	46065200 */ 	add.s	$f8,$f10,$f6
/*  f0ad2b0:	c7a60140 */ 	lwc1	$f6,0x140($sp)
/*  f0ad2b4:	e7a800d0 */ 	swc1	$f8,0xd0($sp)
/*  f0ad2b8:	c6240018 */ 	lwc1	$f4,0x18($s1)
/*  f0ad2bc:	46002282 */ 	mul.s	$f10,$f4,$f0
/*  f0ad2c0:	46065200 */ 	add.s	$f8,$f10,$f6
/*  f0ad2c4:	0c0056d9 */ 	jal	mtx4TransformVecInPlace
/*  f0ad2c8:	e7a800d4 */ 	swc1	$f8,0xd4($sp)
/*  f0ad2cc:	3c014120 */ 	lui	$at,0x4120
/*  f0ad2d0:	44815000 */ 	mtc1	$at,$f10
/*  f0ad2d4:	c7a40130 */ 	lwc1	$f4,0x130($sp)
/*  f0ad2d8:	c7a200d4 */ 	lwc1	$f2,0xd4($sp)
/*  f0ad2dc:	27a400b8 */ 	addiu	$a0,$sp,0xb8
/*  f0ad2e0:	460a2003 */ 	div.s	$f0,$f4,$f10
/*  f0ad2e4:	27a600c0 */ 	addiu	$a2,$sp,0xc0
/*  f0ad2e8:	46001087 */ 	neg.s	$f2,$f2
/*  f0ad2ec:	44051000 */ 	mfc1	$a1,$f2
/*  f0ad2f0:	e7a000bc */ 	swc1	$f0,0xbc($sp)
/*  f0ad2f4:	0fc2d39a */ 	jal	cam0f0b4e68
/*  f0ad2f8:	e7a000b8 */ 	swc1	$f0,0xb8($sp)
/*  f0ad2fc:	3c014000 */ 	lui	$at,0x4000
/*  f0ad300:	c7ac00c0 */ 	lwc1	$f12,0xc0($sp)
/*  f0ad304:	44813000 */ 	mtc1	$at,$f6
/*  f0ad308:	3c013f00 */ 	lui	$at,0x3f00
/*  f0ad30c:	4606603c */ 	c.lt.s	$f12,$f6
/*  f0ad310:	00000000 */ 	nop
/*  f0ad314:	4500000e */ 	bc1f	.L0f0ad350
/*  f0ad318:	00000000 */ 	nop
/*  f0ad31c:	44814000 */ 	mtc1	$at,$f8
/*  f0ad320:	c7a200cc */ 	lwc1	$f2,0xcc($sp)
/*  f0ad324:	c7ae00d0 */ 	lwc1	$f14,0xd0($sp)
/*  f0ad328:	46086002 */ 	mul.s	$f0,$f12,$f8
/*  f0ad32c:	c7a400d4 */ 	lwc1	$f4,0xd4($sp)
/*  f0ad330:	46001082 */ 	mul.s	$f2,$f2,$f0
/*  f0ad334:	00000000 */ 	nop
/*  f0ad338:	46007382 */ 	mul.s	$f14,$f14,$f0
/*  f0ad33c:	00000000 */ 	nop
/*  f0ad340:	46002282 */ 	mul.s	$f10,$f4,$f0
/*  f0ad344:	e7a200cc */ 	swc1	$f2,0xcc($sp)
/*  f0ad348:	e7ae00d0 */ 	swc1	$f14,0xd0($sp)
/*  f0ad34c:	e7aa00d4 */ 	swc1	$f10,0xd4($sp)
.L0f0ad350:
/*  f0ad350:	0fc2d5de */ 	jal	camGetUnk174c
/*  f0ad354:	00000000 */ 	nop
/*  f0ad358:	00402025 */ 	or	$a0,$v0,$zero
/*  f0ad35c:	0c0056d9 */ 	jal	mtx4TransformVecInPlace
/*  f0ad360:	27a500cc */ 	addiu	$a1,$sp,0xcc
/*  f0ad364:	c7a200cc */ 	lwc1	$f2,0xcc($sp)
/*  f0ad368:	c7ac0138 */ 	lwc1	$f12,0x138($sp)
/*  f0ad36c:	c7ae00d0 */ 	lwc1	$f14,0xd0($sp)
/*  f0ad370:	c7b0013c */ 	lwc1	$f16,0x13c($sp)
/*  f0ad374:	460c1081 */ 	sub.s	$f2,$f2,$f12
/*  f0ad378:	3c014120 */ 	lui	$at,0x4120
/*  f0ad37c:	44810000 */ 	mtc1	$at,$f0
/*  f0ad380:	46107381 */ 	sub.s	$f14,$f14,$f16
/*  f0ad384:	c7b20140 */ 	lwc1	$f18,0x140($sp)
/*  f0ad388:	c7a600d4 */ 	lwc1	$f6,0xd4($sp)
/*  f0ad38c:	46001102 */ 	mul.s	$f4,$f2,$f0
/*  f0ad390:	e7ae00d0 */ 	swc1	$f14,0xd0($sp)
/*  f0ad394:	46123201 */ 	sub.s	$f8,$f6,$f18
/*  f0ad398:	46007282 */ 	mul.s	$f10,$f14,$f0
/*  f0ad39c:	e7a200cc */ 	swc1	$f2,0xcc($sp)
/*  f0ad3a0:	46004182 */ 	mul.s	$f6,$f8,$f0
/*  f0ad3a4:	e7a800d4 */ 	swc1	$f8,0xd4($sp)
/*  f0ad3a8:	e7a40100 */ 	swc1	$f4,0x100($sp)
/*  f0ad3ac:	e7aa0104 */ 	swc1	$f10,0x104($sp)
/*  f0ad3b0:	1000000f */ 	b	.L0f0ad3f0
/*  f0ad3b4:	e7a60108 */ 	swc1	$f6,0x108($sp)
.L0f0ad3b8:
/*  f0ad3b8:	3c014120 */ 	lui	$at,0x4120
.L0f0ad3bc:
/*  f0ad3bc:	44815000 */ 	mtc1	$at,$f10
/*  f0ad3c0:	c7a4012c */ 	lwc1	$f4,0x12c($sp)
/*  f0ad3c4:	c6280010 */ 	lwc1	$f8,0x10($s1)
/*  f0ad3c8:	460a2002 */ 	mul.s	$f0,$f4,$f10
/*  f0ad3cc:	00000000 */ 	nop
/*  f0ad3d0:	46004182 */ 	mul.s	$f6,$f8,$f0
/*  f0ad3d4:	e7a60100 */ 	swc1	$f6,0x100($sp)
/*  f0ad3d8:	c6240014 */ 	lwc1	$f4,0x14($s1)
/*  f0ad3dc:	46002282 */ 	mul.s	$f10,$f4,$f0
/*  f0ad3e0:	e7aa0104 */ 	swc1	$f10,0x104($sp)
/*  f0ad3e4:	c6280018 */ 	lwc1	$f8,0x18($s1)
/*  f0ad3e8:	46004182 */ 	mul.s	$f6,$f8,$f0
/*  f0ad3ec:	e7a60108 */ 	swc1	$f6,0x108($sp)
.L0f0ad3f0:
/*  f0ad3f0:	3c017f1b */ 	lui	$at,%hi(var7f1accd0)
/*  f0ad3f4:	c420ccd0 */ 	lwc1	$f0,%lo(var7f1accd0)($at)
/*  f0ad3f8:	c7a40100 */ 	lwc1	$f4,0x100($sp)
/*  f0ad3fc:	3c017f1b */ 	lui	$at,%hi(var7f1accd4)
/*  f0ad400:	4604003c */ 	c.lt.s	$f0,$f4
/*  f0ad404:	00000000 */ 	nop
/*  f0ad408:	450201e7 */ 	bc1fl	.L0f0adba8
/*  f0ad40c:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad410:	c422ccd4 */ 	lwc1	$f2,%lo(var7f1accd4)($at)
/*  f0ad414:	c7aa0104 */ 	lwc1	$f10,0x104($sp)
/*  f0ad418:	4602203c */ 	c.lt.s	$f4,$f2
/*  f0ad41c:	00000000 */ 	nop
/*  f0ad420:	450201e1 */ 	bc1fl	.L0f0adba8
/*  f0ad424:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad428:	460a003c */ 	c.lt.s	$f0,$f10
/*  f0ad42c:	00000000 */ 	nop
/*  f0ad430:	450201dd */ 	bc1fl	.L0f0adba8
/*  f0ad434:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad438:	4602503c */ 	c.lt.s	$f10,$f2
/*  f0ad43c:	c7a80108 */ 	lwc1	$f8,0x108($sp)
/*  f0ad440:	450201d9 */ 	bc1fl	.L0f0adba8
/*  f0ad444:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad448:	4608003c */ 	c.lt.s	$f0,$f8
/*  f0ad44c:	00000000 */ 	nop
/*  f0ad450:	450201d5 */ 	bc1fl	.L0f0adba8
/*  f0ad454:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad458:	4602403c */ 	c.lt.s	$f8,$f2
/*  f0ad45c:	c7a6010c */ 	lwc1	$f6,0x10c($sp)
/*  f0ad460:	450201d1 */ 	bc1fl	.L0f0adba8
/*  f0ad464:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f0ad468:	4600310d */ 	trunc.w.s	$f4,$f6
/*  f0ad46c:	8fa500ec */ 	lw	$a1,0xec($sp)
/*  f0ad470:	3c017f1b */ 	lui	$at,%hi(var7f1accd8)
/*  f0ad474:	440b2000 */ 	mfc1	$t3,$f4
/*  f0ad478:	00000000 */ 	nop
/*  f0ad47c:	a60b0000 */ 	sh	$t3,0x0($s0)
/*  f0ad480:	c7aa0110 */ 	lwc1	$f10,0x110($sp)
/*  f0ad484:	4600520d */ 	trunc.w.s	$f8,$f10
/*  f0ad488:	440f4000 */ 	mfc1	$t7,$f8
/*  f0ad48c:	00000000 */ 	nop
/*  f0ad490:	a60f0002 */ 	sh	$t7,0x2($s0)
/*  f0ad494:	c7a60114 */ 	lwc1	$f6,0x114($sp)
/*  f0ad498:	4600310d */ 	trunc.w.s	$f4,$f6
/*  f0ad49c:	440d2000 */ 	mfc1	$t5,$f4
/*  f0ad4a0:	00000000 */ 	nop
/*  f0ad4a4:	a60d0004 */ 	sh	$t5,0x4($s0)
/*  f0ad4a8:	90b80004 */ 	lbu	$t8,0x4($a1)
/*  f0ad4ac:	a600000a */ 	sh	$zero,0xa($s0)
/*  f0ad4b0:	a2000007 */ 	sb	$zero,0x7($s0)
/*  f0ad4b4:	00184940 */ 	sll	$t1,$t8,0x5
/*  f0ad4b8:	a6090008 */ 	sh	$t1,0x8($s0)
/*  f0ad4bc:	c7aa010c */ 	lwc1	$f10,0x10c($sp)
/*  f0ad4c0:	46005207 */ 	neg.s	$f8,$f10
/*  f0ad4c4:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0ad4c8:	440c3000 */ 	mfc1	$t4,$f6
/*  f0ad4cc:	00000000 */ 	nop
/*  f0ad4d0:	a60c000c */ 	sh	$t4,0xc($s0)
/*  f0ad4d4:	c7a40110 */ 	lwc1	$f4,0x110($sp)
/*  f0ad4d8:	46002287 */ 	neg.s	$f10,$f4
/*  f0ad4dc:	4600520d */ 	trunc.w.s	$f8,$f10
/*  f0ad4e0:	440e4000 */ 	mfc1	$t6,$f8
/*  f0ad4e4:	00000000 */ 	nop
/*  f0ad4e8:	a60e000e */ 	sh	$t6,0xe($s0)
/*  f0ad4ec:	c7a60114 */ 	lwc1	$f6,0x114($sp)
/*  f0ad4f0:	a6000014 */ 	sh	$zero,0x14($s0)
/*  f0ad4f4:	a6000016 */ 	sh	$zero,0x16($s0)
/*  f0ad4f8:	46003107 */ 	neg.s	$f4,$f6
/*  f0ad4fc:	a2000013 */ 	sb	$zero,0x13($s0)
/*  f0ad500:	4600228d */ 	trunc.w.s	$f10,$f4
/*  f0ad504:	440a5000 */ 	mfc1	$t2,$f10
/*  f0ad508:	00000000 */ 	nop
/*  f0ad50c:	a60a0010 */ 	sh	$t2,0x10($s0)
/*  f0ad510:	c7a8010c */ 	lwc1	$f8,0x10c($sp)
/*  f0ad514:	c426ccd8 */ 	lwc1	$f6,%lo(var7f1accd8)($at)
/*  f0ad518:	c7aa0100 */ 	lwc1	$f10,0x100($sp)
/*  f0ad51c:	3c017f1b */ 	lui	$at,%hi(var7f1accdc)
/*  f0ad520:	46064102 */ 	mul.s	$f4,$f8,$f6
/*  f0ad524:	460a2200 */ 	add.s	$f8,$f4,$f10
/*  f0ad528:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0ad52c:	44183000 */ 	mfc1	$t8,$f6
/*  f0ad530:	00000000 */ 	nop
/*  f0ad534:	a6180018 */ 	sh	$t8,0x18($s0)
/*  f0ad538:	c7a40110 */ 	lwc1	$f4,0x110($sp)
/*  f0ad53c:	c42accdc */ 	lwc1	$f10,%lo(var7f1accdc)($at)
/*  f0ad540:	c7a60104 */ 	lwc1	$f6,0x104($sp)
/*  f0ad544:	3c017f1b */ 	lui	$at,%hi(var7f1acce0)
/*  f0ad548:	460a2202 */ 	mul.s	$f8,$f4,$f10
/*  f0ad54c:	46064100 */ 	add.s	$f4,$f8,$f6
/*  f0ad550:	4600228d */ 	trunc.w.s	$f10,$f4
/*  f0ad554:	44195000 */ 	mfc1	$t9,$f10
/*  f0ad558:	00000000 */ 	nop
/*  f0ad55c:	a619001a */ 	sh	$t9,0x1a($s0)
/*  f0ad560:	c7a80114 */ 	lwc1	$f8,0x114($sp)
/*  f0ad564:	c426cce0 */ 	lwc1	$f6,%lo(var7f1acce0)($at)
/*  f0ad568:	c7aa0108 */ 	lwc1	$f10,0x108($sp)
/*  f0ad56c:	3c017f1b */ 	lui	$at,%hi(var7f1acce4)
/*  f0ad570:	46064102 */ 	mul.s	$f4,$f8,$f6
/*  f0ad574:	460a2200 */ 	add.s	$f8,$f4,$f10
/*  f0ad578:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0ad57c:	440b3000 */ 	mfc1	$t3,$f6
/*  f0ad580:	00000000 */ 	nop
/*  f0ad584:	a60b001c */ 	sh	$t3,0x1c($s0)
/*  f0ad588:	90ae0004 */ 	lbu	$t6,0x4($a1)
/*  f0ad58c:	000e7940 */ 	sll	$t7,$t6,0x5
/*  f0ad590:	a60f0020 */ 	sh	$t7,0x20($s0)
/*  f0ad594:	90aa0005 */ 	lbu	$t2,0x5($a1)
/*  f0ad598:	a200001f */ 	sb	$zero,0x1f($s0)
/*  f0ad59c:	000a6940 */ 	sll	$t5,$t2,0x5
/*  f0ad5a0:	a60d0022 */ 	sh	$t5,0x22($s0)
/*  f0ad5a4:	c42acce4 */ 	lwc1	$f10,%lo(var7f1acce4)($at)
/*  f0ad5a8:	c7a4010c */ 	lwc1	$f4,0x10c($sp)
/*  f0ad5ac:	c7a60100 */ 	lwc1	$f6,0x100($sp)
/*  f0ad5b0:	3c017f1b */ 	lui	$at,%hi(var7f1acce8)
/*  f0ad5b4:	460a2202 */ 	mul.s	$f8,$f4,$f10
/*  f0ad5b8:	46083101 */ 	sub.s	$f4,$f6,$f8
/*  f0ad5bc:	4600228d */ 	trunc.w.s	$f10,$f4
/*  f0ad5c0:	44095000 */ 	mfc1	$t1,$f10
/*  f0ad5c4:	00000000 */ 	nop
/*  f0ad5c8:	a6090024 */ 	sh	$t1,0x24($s0)
/*  f0ad5cc:	c428cce8 */ 	lwc1	$f8,%lo(var7f1acce8)($at)
/*  f0ad5d0:	c7a60110 */ 	lwc1	$f6,0x110($sp)
/*  f0ad5d4:	c7aa0104 */ 	lwc1	$f10,0x104($sp)
/*  f0ad5d8:	3c017f1b */ 	lui	$at,%hi(var7f1accec)
/*  f0ad5dc:	46083102 */ 	mul.s	$f4,$f6,$f8
/*  f0ad5e0:	46045181 */ 	sub.s	$f6,$f10,$f4
/*  f0ad5e4:	4600320d */ 	trunc.w.s	$f8,$f6
/*  f0ad5e8:	440c4000 */ 	mfc1	$t4,$f8
/*  f0ad5ec:	00000000 */ 	nop
/*  f0ad5f0:	a60c0026 */ 	sh	$t4,0x26($s0)
/*  f0ad5f4:	c424ccec */ 	lwc1	$f4,%lo(var7f1accec)($at)
/*  f0ad5f8:	c7aa0114 */ 	lwc1	$f10,0x114($sp)
/*  f0ad5fc:	c7a80108 */ 	lwc1	$f8,0x108($sp)
/*  f0ad600:	a600002c */ 	sh	$zero,0x2c($s0)
/*  f0ad604:	46045182 */ 	mul.s	$f6,$f10,$f4
/*  f0ad608:	2401001d */ 	addiu	$at,$zero,0x1d
/*  f0ad60c:	46064281 */ 	sub.s	$f10,$f8,$f6
/*  f0ad610:	4600510d */ 	trunc.w.s	$f4,$f10
/*  f0ad614:	440e2000 */ 	mfc1	$t6,$f4
/*  f0ad618:	00000000 */ 	nop
/*  f0ad61c:	a60e0028 */ 	sh	$t6,0x28($s0)
/*  f0ad620:	90af0005 */ 	lbu	$t7,0x5($a1)
/*  f0ad624:	a200002b */ 	sb	$zero,0x2b($s0)
/*  f0ad628:	000f5140 */ 	sll	$t2,$t7,0x5
/*  f0ad62c:	a60a002e */ 	sh	$t2,0x2e($s0)
/*  f0ad630:	822d0001 */ 	lb	$t5,0x1($s1)
/*  f0ad634:	c7a80138 */ 	lwc1	$f8,0x138($sp)
/*  f0ad638:	8fb80134 */ 	lw	$t8,0x134($sp)
/*  f0ad63c:	55a100b4 */ 	bnel	$t5,$at,.L0f0ad910
/*  f0ad640:	8fa90190 */ 	lw	$t1,0x190($sp)
/*  f0ad644:	c70c0000 */ 	lwc1	$f12,0x0($t8)
/*  f0ad648:	c7020004 */ 	lwc1	$f2,0x4($t8)
/*  f0ad64c:	c7a6013c */ 	lwc1	$f6,0x13c($sp)
/*  f0ad650:	46086381 */ 	sub.s	$f14,$f12,$f8
/*  f0ad654:	c7000008 */ 	lwc1	$f0,0x8($t8)
/*  f0ad658:	e7a80038 */ 	swc1	$f8,0x38($sp)
/*  f0ad65c:	46061401 */ 	sub.s	$f16,$f2,$f6
/*  f0ad660:	460e7102 */ 	mul.s	$f4,$f14,$f14
/*  f0ad664:	c7aa0140 */ 	lwc1	$f10,0x140($sp)
/*  f0ad668:	3c017f1b */ 	lui	$at,%hi(var7f1accf0)
/*  f0ad66c:	46108202 */ 	mul.s	$f8,$f16,$f16
/*  f0ad670:	460a0481 */ 	sub.s	$f18,$f0,$f10
/*  f0ad674:	46082100 */ 	add.s	$f4,$f4,$f8
/*  f0ad678:	46129202 */ 	mul.s	$f8,$f18,$f18
/*  f0ad67c:	46082100 */ 	add.s	$f4,$f4,$f8
/*  f0ad680:	e7a400a8 */ 	swc1	$f4,0xa8($sp)
/*  f0ad684:	c6280010 */ 	lwc1	$f8,0x10($s1)
/*  f0ad688:	e7a60034 */ 	swc1	$f6,0x34($sp)
/*  f0ad68c:	c7a6012c */ 	lwc1	$f6,0x12c($sp)
/*  f0ad690:	e7aa0030 */ 	swc1	$f10,0x30($sp)
/*  f0ad694:	c7aa0038 */ 	lwc1	$f10,0x38($sp)
/*  f0ad698:	46064202 */ 	mul.s	$f8,$f8,$f6
/*  f0ad69c:	460a4200 */ 	add.s	$f8,$f8,$f10
/*  f0ad6a0:	c62a0014 */ 	lwc1	$f10,0x14($s1)
/*  f0ad6a4:	46086381 */ 	sub.s	$f14,$f12,$f8
/*  f0ad6a8:	46065202 */ 	mul.s	$f8,$f10,$f6
/*  f0ad6ac:	c7aa0034 */ 	lwc1	$f10,0x34($sp)
/*  f0ad6b0:	460a4200 */ 	add.s	$f8,$f8,$f10
/*  f0ad6b4:	c62a0018 */ 	lwc1	$f10,0x18($s1)
/*  f0ad6b8:	46081401 */ 	sub.s	$f16,$f2,$f8
/*  f0ad6bc:	46065202 */ 	mul.s	$f8,$f10,$f6
/*  f0ad6c0:	c7aa0030 */ 	lwc1	$f10,0x30($sp)
/*  f0ad6c4:	460a4180 */ 	add.s	$f6,$f8,$f10
/*  f0ad6c8:	460e7202 */ 	mul.s	$f8,$f14,$f14
/*  f0ad6cc:	00000000 */ 	nop
/*  f0ad6d0:	46108282 */ 	mul.s	$f10,$f16,$f16
/*  f0ad6d4:	46060481 */ 	sub.s	$f18,$f0,$f6
/*  f0ad6d8:	460a4180 */ 	add.s	$f6,$f8,$f10
/*  f0ad6dc:	46129202 */ 	mul.s	$f8,$f18,$f18
/*  f0ad6e0:	46083280 */ 	add.s	$f10,$f6,$f8
/*  f0ad6e4:	c7a60100 */ 	lwc1	$f6,0x100($sp)
/*  f0ad6e8:	4604503c */ 	c.lt.s	$f10,$f4
/*  f0ad6ec:	00000000 */ 	nop
/*  f0ad6f0:	4500000b */ 	bc1f	.L0f0ad720
/*  f0ad6f4:	00000000 */ 	nop
/*  f0ad6f8:	c424ccf0 */ 	lwc1	$f4,%lo(var7f1accf0)($at)
/*  f0ad6fc:	e7a600f4 */ 	swc1	$f6,0xf4($sp)
/*  f0ad700:	3c017f1b */ 	lui	$at,%hi(var7f1accf4)
/*  f0ad704:	c426ccf4 */ 	lwc1	$f6,%lo(var7f1accf4)($at)
/*  f0ad708:	c7a80104 */ 	lwc1	$f8,0x104($sp)
/*  f0ad70c:	c7aa0108 */ 	lwc1	$f10,0x108($sp)
/*  f0ad710:	46062002 */ 	mul.s	$f0,$f4,$f6
/*  f0ad714:	e7a800f8 */ 	swc1	$f8,0xf8($sp)
/*  f0ad718:	e7aa00fc */ 	swc1	$f10,0xfc($sp)
/*  f0ad71c:	e7a000f0 */ 	swc1	$f0,0xf0($sp)
.L0f0ad720:
/*  f0ad720:	c7a000f0 */ 	lwc1	$f0,0xf0($sp)
/*  f0ad724:	c7a80118 */ 	lwc1	$f8,0x118($sp)
/*  f0ad728:	c7a400f4 */ 	lwc1	$f4,0xf4($sp)
/*  f0ad72c:	3c03800b */ 	lui	$v1,%hi(g_TexGroup03Configs)
/*  f0ad730:	46004282 */ 	mul.s	$f10,$f8,$f0
/*  f0ad734:	2463b57c */ 	addiu	$v1,$v1,%lo(g_TexGroup03Configs)
/*  f0ad738:	46045180 */ 	add.s	$f6,$f10,$f4
/*  f0ad73c:	4600320d */ 	trunc.w.s	$f8,$f6
/*  f0ad740:	44194000 */ 	mfc1	$t9,$f8
/*  f0ad744:	00000000 */ 	nop
/*  f0ad748:	a6190030 */ 	sh	$t9,0x30($s0)
/*  f0ad74c:	c7aa011c */ 	lwc1	$f10,0x11c($sp)
/*  f0ad750:	c7a600f8 */ 	lwc1	$f6,0xf8($sp)
/*  f0ad754:	46005102 */ 	mul.s	$f4,$f10,$f0
/*  f0ad758:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0ad75c:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0ad760:	440b5000 */ 	mfc1	$t3,$f10
/*  f0ad764:	00000000 */ 	nop
/*  f0ad768:	a60b0032 */ 	sh	$t3,0x32($s0)
/*  f0ad76c:	c7a40120 */ 	lwc1	$f4,0x120($sp)
/*  f0ad770:	c7a800fc */ 	lwc1	$f8,0xfc($sp)
/*  f0ad774:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0ad778:	46083280 */ 	add.s	$f10,$f6,$f8
/*  f0ad77c:	4600510d */ 	trunc.w.s	$f4,$f10
/*  f0ad780:	440f2000 */ 	mfc1	$t7,$f4
/*  f0ad784:	00000000 */ 	nop
/*  f0ad788:	a60f0034 */ 	sh	$t7,0x34($s0)
/*  f0ad78c:	8c6a0000 */ 	lw	$t2,0x0($v1)
/*  f0ad790:	914d0004 */ 	lbu	$t5,0x4($t2)
/*  f0ad794:	000dc140 */ 	sll	$t8,$t5,0x5
/*  f0ad798:	a6180038 */ 	sh	$t8,0x38($s0)
/*  f0ad79c:	8c690000 */ 	lw	$t1,0x0($v1)
/*  f0ad7a0:	91390005 */ 	lbu	$t9,0x5($t1)
/*  f0ad7a4:	a2000037 */ 	sb	$zero,0x37($s0)
/*  f0ad7a8:	00196140 */ 	sll	$t4,$t9,0x5
/*  f0ad7ac:	a60c003a */ 	sh	$t4,0x3a($s0)
/*  f0ad7b0:	c7a80118 */ 	lwc1	$f8,0x118($sp)
/*  f0ad7b4:	c7a600f4 */ 	lwc1	$f6,0xf4($sp)
/*  f0ad7b8:	46004282 */ 	mul.s	$f10,$f8,$f0
/*  f0ad7bc:	460a3101 */ 	sub.s	$f4,$f6,$f10
/*  f0ad7c0:	4600220d */ 	trunc.w.s	$f8,$f4
/*  f0ad7c4:	440e4000 */ 	mfc1	$t6,$f8
/*  f0ad7c8:	00000000 */ 	nop
/*  f0ad7cc:	a60e003c */ 	sh	$t6,0x3c($s0)
/*  f0ad7d0:	c7aa011c */ 	lwc1	$f10,0x11c($sp)
/*  f0ad7d4:	c7a600f8 */ 	lwc1	$f6,0xf8($sp)
/*  f0ad7d8:	46005102 */ 	mul.s	$f4,$f10,$f0
/*  f0ad7dc:	46043201 */ 	sub.s	$f8,$f6,$f4
/*  f0ad7e0:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0ad7e4:	440a5000 */ 	mfc1	$t2,$f10
/*  f0ad7e8:	00000000 */ 	nop
/*  f0ad7ec:	a60a003e */ 	sh	$t2,0x3e($s0)
/*  f0ad7f0:	c7a40120 */ 	lwc1	$f4,0x120($sp)
/*  f0ad7f4:	c7a600fc */ 	lwc1	$f6,0xfc($sp)
/*  f0ad7f8:	a6000044 */ 	sh	$zero,0x44($s0)
/*  f0ad7fc:	46002202 */ 	mul.s	$f8,$f4,$f0
/*  f0ad800:	a6000046 */ 	sh	$zero,0x46($s0)
/*  f0ad804:	a2000043 */ 	sb	$zero,0x43($s0)
/*  f0ad808:	46083281 */ 	sub.s	$f10,$f6,$f8
/*  f0ad80c:	4600510d */ 	trunc.w.s	$f4,$f10
/*  f0ad810:	44182000 */ 	mfc1	$t8,$f4
/*  f0ad814:	00000000 */ 	nop
/*  f0ad818:	a6180040 */ 	sh	$t8,0x40($s0)
/*  f0ad81c:	c7a6010c */ 	lwc1	$f6,0x10c($sp)
/*  f0ad820:	c7aa00f4 */ 	lwc1	$f10,0xf4($sp)
/*  f0ad824:	46003202 */ 	mul.s	$f8,$f6,$f0
/*  f0ad828:	460a4100 */ 	add.s	$f4,$f8,$f10
/*  f0ad82c:	4600218d */ 	trunc.w.s	$f6,$f4
/*  f0ad830:	44193000 */ 	mfc1	$t9,$f6
/*  f0ad834:	00000000 */ 	nop
/*  f0ad838:	a6190048 */ 	sh	$t9,0x48($s0)
/*  f0ad83c:	c7a80110 */ 	lwc1	$f8,0x110($sp)
/*  f0ad840:	c7a400f8 */ 	lwc1	$f4,0xf8($sp)
/*  f0ad844:	46004282 */ 	mul.s	$f10,$f8,$f0
/*  f0ad848:	46045180 */ 	add.s	$f6,$f10,$f4
/*  f0ad84c:	4600320d */ 	trunc.w.s	$f8,$f6
/*  f0ad850:	440b4000 */ 	mfc1	$t3,$f8
/*  f0ad854:	00000000 */ 	nop
/*  f0ad858:	a60b004a */ 	sh	$t3,0x4a($s0)
/*  f0ad85c:	c7aa0114 */ 	lwc1	$f10,0x114($sp)
/*  f0ad860:	c7a600fc */ 	lwc1	$f6,0xfc($sp)
/*  f0ad864:	a6000050 */ 	sh	$zero,0x50($s0)
/*  f0ad868:	46005102 */ 	mul.s	$f4,$f10,$f0
/*  f0ad86c:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0ad870:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0ad874:	440f5000 */ 	mfc1	$t7,$f10
/*  f0ad878:	00000000 */ 	nop
/*  f0ad87c:	a60f004c */ 	sh	$t7,0x4c($s0)
/*  f0ad880:	8c6a0000 */ 	lw	$t2,0x0($v1)
/*  f0ad884:	914d0005 */ 	lbu	$t5,0x5($t2)
/*  f0ad888:	a200004f */ 	sb	$zero,0x4f($s0)
/*  f0ad88c:	000dc140 */ 	sll	$t8,$t5,0x5
/*  f0ad890:	a6180052 */ 	sh	$t8,0x52($s0)
/*  f0ad894:	c7a6010c */ 	lwc1	$f6,0x10c($sp)
/*  f0ad898:	c7a400f4 */ 	lwc1	$f4,0xf4($sp)
/*  f0ad89c:	46003202 */ 	mul.s	$f8,$f6,$f0
/*  f0ad8a0:	46082281 */ 	sub.s	$f10,$f4,$f8
/*  f0ad8a4:	4600518d */ 	trunc.w.s	$f6,$f10
/*  f0ad8a8:	44193000 */ 	mfc1	$t9,$f6
/*  f0ad8ac:	00000000 */ 	nop
/*  f0ad8b0:	a6190054 */ 	sh	$t9,0x54($s0)
/*  f0ad8b4:	c7a80110 */ 	lwc1	$f8,0x110($sp)
/*  f0ad8b8:	c7a400f8 */ 	lwc1	$f4,0xf8($sp)
/*  f0ad8bc:	46004282 */ 	mul.s	$f10,$f8,$f0
/*  f0ad8c0:	460a2181 */ 	sub.s	$f6,$f4,$f10
/*  f0ad8c4:	4600320d */ 	trunc.w.s	$f8,$f6
/*  f0ad8c8:	440b4000 */ 	mfc1	$t3,$f8
/*  f0ad8cc:	00000000 */ 	nop
/*  f0ad8d0:	a60b0056 */ 	sh	$t3,0x56($s0)
/*  f0ad8d4:	c7aa0114 */ 	lwc1	$f10,0x114($sp)
/*  f0ad8d8:	c7a400fc */ 	lwc1	$f4,0xfc($sp)
/*  f0ad8dc:	46005182 */ 	mul.s	$f6,$f10,$f0
/*  f0ad8e0:	46062201 */ 	sub.s	$f8,$f4,$f6
/*  f0ad8e4:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0ad8e8:	440f5000 */ 	mfc1	$t7,$f10
/*  f0ad8ec:	00000000 */ 	nop
/*  f0ad8f0:	a60f0058 */ 	sh	$t7,0x58($s0)
/*  f0ad8f4:	8c6a0000 */ 	lw	$t2,0x0($v1)
/*  f0ad8f8:	914d0004 */ 	lbu	$t5,0x4($t2)
/*  f0ad8fc:	a600005e */ 	sh	$zero,0x5e($s0)
/*  f0ad900:	a200005b */ 	sb	$zero,0x5b($s0)
/*  f0ad904:	000dc140 */ 	sll	$t8,$t5,0x5
/*  f0ad908:	a618005c */ 	sh	$t8,0x5c($s0)
/*  f0ad90c:	8fa90190 */ 	lw	$t1,0x190($sp)
.L0f0ad910:
/*  f0ad910:	3c0cb600 */ 	lui	$t4,0xb600
/*  f0ad914:	240b2000 */ 	addiu	$t3,$zero,0x2000
/*  f0ad918:	25390008 */ 	addiu	$t9,$t1,0x8
/*  f0ad91c:	afb90190 */ 	sw	$t9,0x190($sp)
/*  f0ad920:	ad2b0004 */ 	sw	$t3,0x4($t1)
/*  f0ad924:	ad2c0000 */ 	sw	$t4,0x0($t1)
/*  f0ad928:	8fae0190 */ 	lw	$t6,0x190($sp)
/*  f0ad92c:	3c0a0102 */ 	lui	$t2,0x102
/*  f0ad930:	354a0040 */ 	ori	$t2,$t2,0x40
/*  f0ad934:	25cf0008 */ 	addiu	$t7,$t6,0x8
/*  f0ad938:	afaf0190 */ 	sw	$t7,0x190($sp)
/*  f0ad93c:	adca0000 */ 	sw	$t2,0x0($t6)
/*  f0ad940:	8fa40188 */ 	lw	$a0,0x188($sp)
/*  f0ad944:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0ad948:	afae009c */ 	sw	$t6,0x9c($sp)
/*  f0ad94c:	8fa7009c */ 	lw	$a3,0x9c($sp)
/*  f0ad950:	3c09ba00 */ 	lui	$t1,0xba00
/*  f0ad954:	35291402 */ 	ori	$t1,$t1,0x1402
/*  f0ad958:	ace20004 */ 	sw	$v0,0x4($a3)
/*  f0ad95c:	8fad0190 */ 	lw	$t5,0x190($sp)
/*  f0ad960:	3c0bba00 */ 	lui	$t3,0xba00
/*  f0ad964:	356b0602 */ 	ori	$t3,$t3,0x602
/*  f0ad968:	25b80008 */ 	addiu	$t8,$t5,0x8
/*  f0ad96c:	afb80190 */ 	sw	$t8,0x190($sp)
/*  f0ad970:	ada00004 */ 	sw	$zero,0x4($t5)
/*  f0ad974:	ada90000 */ 	sw	$t1,0x0($t5)
/*  f0ad978:	8fb90190 */ 	lw	$t9,0x190($sp)
/*  f0ad97c:	240e00c0 */ 	addiu	$t6,$zero,0xc0
/*  f0ad980:	3c0db900 */ 	lui	$t5,0xb900
/*  f0ad984:	272c0008 */ 	addiu	$t4,$t9,0x8
/*  f0ad988:	afac0190 */ 	sw	$t4,0x190($sp)
/*  f0ad98c:	af2e0004 */ 	sw	$t6,0x4($t9)
/*  f0ad990:	af2b0000 */ 	sw	$t3,0x0($t9)
/*  f0ad994:	8faf0190 */ 	lw	$t7,0x190($sp)
/*  f0ad998:	3c180050 */ 	lui	$t8,0x50
/*  f0ad99c:	371849d8 */ 	ori	$t8,$t8,0x49d8
/*  f0ad9a0:	25ea0008 */ 	addiu	$t2,$t7,0x8
/*  f0ad9a4:	afaa0190 */ 	sw	$t2,0x190($sp)
/*  f0ad9a8:	35ad031d */ 	ori	$t5,$t5,0x31d
/*  f0ad9ac:	aded0000 */ 	sw	$t5,0x0($t7)
/*  f0ad9b0:	adf80004 */ 	sw	$t8,0x4($t7)
/*  f0ad9b4:	8fa90190 */ 	lw	$t1,0x190($sp)
/*  f0ad9b8:	3c0cb900 */ 	lui	$t4,0xb900
/*  f0ad9bc:	358c0002 */ 	ori	$t4,$t4,0x2
/*  f0ad9c0:	25390008 */ 	addiu	$t9,$t1,0x8
/*  f0ad9c4:	afb90190 */ 	sw	$t9,0x190($sp)
/*  f0ad9c8:	ad200004 */ 	sw	$zero,0x4($t1)
/*  f0ad9cc:	ad2c0000 */ 	sw	$t4,0x0($t1)
/*  f0ad9d0:	8fab0190 */ 	lw	$t3,0x190($sp)
/*  f0ad9d4:	3c0fba00 */ 	lui	$t7,0xba00
/*  f0ad9d8:	35ef1001 */ 	ori	$t7,$t7,0x1001
/*  f0ad9dc:	256e0008 */ 	addiu	$t6,$t3,0x8
/*  f0ad9e0:	afae0190 */ 	sw	$t6,0x190($sp)
/*  f0ad9e4:	ad600004 */ 	sw	$zero,0x4($t3)
/*  f0ad9e8:	ad6f0000 */ 	sw	$t7,0x0($t3)
/*  f0ad9ec:	8faa0190 */ 	lw	$t2,0x190($sp)
/*  f0ad9f0:	3c18ba00 */ 	lui	$t8,0xba00
/*  f0ad9f4:	37180903 */ 	ori	$t8,$t8,0x903
/*  f0ad9f8:	254d0008 */ 	addiu	$t5,$t2,0x8
/*  f0ad9fc:	afad0190 */ 	sw	$t5,0x190($sp)
/*  f0ada00:	24090c00 */ 	addiu	$t1,$zero,0xc00
/*  f0ada04:	ad490004 */ 	sw	$t1,0x4($t2)
/*  f0ada08:	ad580000 */ 	sw	$t8,0x0($t2)
/*  f0ada0c:	8fb90190 */ 	lw	$t9,0x190($sp)
/*  f0ada10:	3c0bfc50 */ 	lui	$t3,0xfc50
/*  f0ada14:	3c0e4432 */ 	lui	$t6,0x4432
/*  f0ada18:	272c0008 */ 	addiu	$t4,$t9,0x8
/*  f0ada1c:	afac0190 */ 	sw	$t4,0x190($sp)
/*  f0ada20:	35ce7f3f */ 	ori	$t6,$t6,0x7f3f
/*  f0ada24:	356b98a1 */ 	ori	$t3,$t3,0x98a1
/*  f0ada28:	af2b0000 */ 	sw	$t3,0x0($t9)
/*  f0ada2c:	af2e0004 */ 	sw	$t6,0x4($t9)
/*  f0ada30:	8faf0190 */ 	lw	$t7,0x190($sp)
/*  f0ada34:	3c0d0700 */ 	lui	$t5,0x700
/*  f0ada38:	35ad0004 */ 	ori	$t5,$t5,0x4
/*  f0ada3c:	25ea0008 */ 	addiu	$t2,$t7,0x8
/*  f0ada40:	afaa0190 */ 	sw	$t2,0x190($sp)
/*  f0ada44:	aded0000 */ 	sw	$t5,0x0($t7)
/*  f0ada48:	8fa40144 */ 	lw	$a0,0x144($sp)
/*  f0ada4c:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0ada50:	afaf007c */ 	sw	$t7,0x7c($sp)
/*  f0ada54:	8fa5007c */ 	lw	$a1,0x7c($sp)
/*  f0ada58:	2401001d */ 	addiu	$at,$zero,0x1d
/*  f0ada5c:	27a40190 */ 	addiu	$a0,$sp,0x190
/*  f0ada60:	aca20004 */ 	sw	$v0,0x4($a1)
/*  f0ada64:	82380001 */ 	lb	$t8,0x1($s1)
/*  f0ada68:	8fa70198 */ 	lw	$a3,0x198($sp)
/*  f0ada6c:	8fa500ec */ 	lw	$a1,0xec($sp)
/*  f0ada70:	17010033 */ 	bne	$t8,$at,.L0f0adb40
/*  f0ada74:	24060004 */ 	addiu	$a2,$zero,0x4
/*  f0ada78:	27b10190 */ 	addiu	$s1,$sp,0x190
/*  f0ada7c:	3c05800b */ 	lui	$a1,%hi(g_TexGroup03Configs)
/*  f0ada80:	24090002 */ 	addiu	$t1,$zero,0x2
/*  f0ada84:	24190001 */ 	addiu	$t9,$zero,0x1
/*  f0ada88:	afb90014 */ 	sw	$t9,0x14($sp)
/*  f0ada8c:	afa90010 */ 	sw	$t1,0x10($sp)
/*  f0ada90:	8ca5b57c */ 	lw	$a1,%lo(g_TexGroup03Configs)($a1)
/*  f0ada94:	02202025 */ 	or	$a0,$s1,$zero
/*  f0ada98:	24060004 */ 	addiu	$a2,$zero,0x4
/*  f0ada9c:	8fa70198 */ 	lw	$a3,0x198($sp)
/*  f0adaa0:	0fc2ce70 */ 	jal	texSelect
/*  f0adaa4:	afa00018 */ 	sw	$zero,0x18($sp)
/*  f0adaa8:	8fac0190 */ 	lw	$t4,0x190($sp)
/*  f0adaac:	3c0e0470 */ 	lui	$t6,0x470
/*  f0adab0:	35ce0060 */ 	ori	$t6,$t6,0x60
/*  f0adab4:	258b0008 */ 	addiu	$t3,$t4,0x8
/*  f0adab8:	afab0190 */ 	sw	$t3,0x190($sp)
/*  f0adabc:	ad8e0000 */ 	sw	$t6,0x0($t4)
/*  f0adac0:	02002025 */ 	or	$a0,$s0,$zero
/*  f0adac4:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0adac8:	afac0078 */ 	sw	$t4,0x78($sp)
/*  f0adacc:	8fa80078 */ 	lw	$t0,0x78($sp)
/*  f0adad0:	3c0db100 */ 	lui	$t5,0xb100
/*  f0adad4:	35ad0076 */ 	ori	$t5,$t5,0x76
/*  f0adad8:	ad020004 */ 	sw	$v0,0x4($t0)
/*  f0adadc:	8faf0190 */ 	lw	$t7,0x190($sp)
/*  f0adae0:	24185454 */ 	addiu	$t8,$zero,0x5454
/*  f0adae4:	24090002 */ 	addiu	$t1,$zero,0x2
/*  f0adae8:	25ea0008 */ 	addiu	$t2,$t7,0x8
/*  f0adaec:	afaa0190 */ 	sw	$t2,0x190($sp)
/*  f0adaf0:	adf80004 */ 	sw	$t8,0x4($t7)
/*  f0adaf4:	aded0000 */ 	sw	$t5,0x0($t7)
/*  f0adaf8:	24190001 */ 	addiu	$t9,$zero,0x1
/*  f0adafc:	afb90014 */ 	sw	$t9,0x14($sp)
/*  f0adb00:	afa00018 */ 	sw	$zero,0x18($sp)
/*  f0adb04:	afa90010 */ 	sw	$t1,0x10($sp)
/*  f0adb08:	8fa70198 */ 	lw	$a3,0x198($sp)
/*  f0adb0c:	8fa500ec */ 	lw	$a1,0xec($sp)
/*  f0adb10:	02202025 */ 	or	$a0,$s1,$zero
/*  f0adb14:	0fc2ce70 */ 	jal	texSelect
/*  f0adb18:	24060004 */ 	addiu	$a2,$zero,0x4
/*  f0adb1c:	8fac0190 */ 	lw	$t4,0x190($sp)
/*  f0adb20:	3c0eb100 */ 	lui	$t6,0xb100
/*  f0adb24:	35ce0013 */ 	ori	$t6,$t6,0x13
/*  f0adb28:	258b0008 */ 	addiu	$t3,$t4,0x8
/*  f0adb2c:	afab0190 */ 	sw	$t3,0x190($sp)
/*  f0adb30:	240f3020 */ 	addiu	$t7,$zero,0x3020
/*  f0adb34:	ad8f0004 */ 	sw	$t7,0x4($t4)
/*  f0adb38:	1000001a */ 	b	.L0f0adba4
/*  f0adb3c:	ad8e0000 */ 	sw	$t6,0x0($t4)
.L0f0adb40:
/*  f0adb40:	240a0002 */ 	addiu	$t2,$zero,0x2
/*  f0adb44:	240d0001 */ 	addiu	$t5,$zero,0x1
/*  f0adb48:	afad0014 */ 	sw	$t5,0x14($sp)
/*  f0adb4c:	afaa0010 */ 	sw	$t2,0x10($sp)
/*  f0adb50:	0fc2ce70 */ 	jal	texSelect
/*  f0adb54:	afa00018 */ 	sw	$zero,0x18($sp)
/*  f0adb58:	8fb80190 */ 	lw	$t8,0x190($sp)
/*  f0adb5c:	3c190430 */ 	lui	$t9,0x430
/*  f0adb60:	37390030 */ 	ori	$t9,$t9,0x30
/*  f0adb64:	27090008 */ 	addiu	$t1,$t8,0x8
/*  f0adb68:	afa90190 */ 	sw	$t1,0x190($sp)
/*  f0adb6c:	af190000 */ 	sw	$t9,0x0($t8)
/*  f0adb70:	02002025 */ 	or	$a0,$s0,$zero
/*  f0adb74:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0adb78:	afb8006c */ 	sw	$t8,0x6c($sp)
/*  f0adb7c:	8fa5006c */ 	lw	$a1,0x6c($sp)
/*  f0adb80:	3c0eb100 */ 	lui	$t6,0xb100
/*  f0adb84:	35ce0013 */ 	ori	$t6,$t6,0x13
/*  f0adb88:	aca20004 */ 	sw	$v0,0x4($a1)
/*  f0adb8c:	8fac0190 */ 	lw	$t4,0x190($sp)
/*  f0adb90:	240f3020 */ 	addiu	$t7,$zero,0x3020
/*  f0adb94:	258b0008 */ 	addiu	$t3,$t4,0x8
/*  f0adb98:	afab0190 */ 	sw	$t3,0x190($sp)
/*  f0adb9c:	ad8f0004 */ 	sw	$t7,0x4($t4)
/*  f0adba0:	ad8e0000 */ 	sw	$t6,0x0($t4)
.L0f0adba4:
/*  f0adba4:	8fbf002c */ 	lw	$ra,0x2c($sp)
.L0f0adba8:
/*  f0adba8:	8fa20190 */ 	lw	$v0,0x190($sp)
/*  f0adbac:	8fb00024 */ 	lw	$s0,0x24($sp)
/*  f0adbb0:	8fb10028 */ 	lw	$s1,0x28($sp)
/*  f0adbb4:	03e00008 */ 	jr	$ra
/*  f0adbb8:	27bd0190 */ 	addiu	$sp,$sp,0x190
);

void beamTick(struct beam *beam)
{
	if (beam->age >= 0) {
		if (beam->weaponnum == -2) {
			beam->age++;

			if (beam->age > 1) {
				beam->age = -1;
			}
		} else {
			if (g_Vars.lvupdate240 <= 8) {
				// Not lagging
				beam->dist += beam->speed * g_Vars.lvupdate240f;
			} else {
				// Lagging
				beam->dist += beam->speed * (2 + RANDOMFRAC() * 0.5f);
			}

			if (beam->dist >= beam->maxdist) {
				beam->age = -1;
			}
		}
	}
}

struct casing *casingCreate(struct modelfiledata *modeldef, Mtxf *mtx)
{
	s32 i;
	s32 j;
	f32 rot[3][3];
	struct casing *casing = g_Casings;
	struct casing *end = g_Casings + ARRAYCOUNT(g_Casings);
	u32 stack;

	while (casing < end && casing->modeldef != NULL) {
		casing++;
	}

	if (casing < end) {
		casing->modeldef = modeldef;
		casing->pos.x = mtx->m[3][0];
		casing->pos.y = mtx->m[3][1];
		casing->pos.z = mtx->m[3][2];

		mtx4ToMtx3(mtx, rot);

		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				casing->rot[i][j] = rot[i][j] * 4096.0f;
			}
		}

		g_CasingsActive = true;

		return casing;
	}

	return NULL;
}

#if PAL
GLOBAL_ASM(
glabel casingCreateForHand
.late_rodata
glabel var7f1accf8
.word 0x3f088888
glabel var7f1accfc
.word 0x40c907a9
glabel var7f1acd00
.word 0x3ec907a9
glabel var7f1acd04
.word 0x40c907a9
glabel var7f1acd08
.word 0x3ec907a9
glabel var7f1acd0c
.word 0x40c907a9
glabel var7f1acd10
.word 0x3ec907a9
glabel var7f1acd14
.word 0x4964e1c0
glabel var7f1acd18
.word 0x3e8e38e4
glabel var7f1acd1c
.word 0x3ed55555
glabel var7f1acd20
.word 0x40555555
glabel var7f1acd24
.word 0x3fb55555
glabel var7f1acd28
.word 0x3fd55555
glabel var7f1acd2c
.word 0x40c907a9
glabel var7f1acd30
.word 0x3dc907a9
glabel var7f1acd34
.word 0x40c907a9
glabel var7f1acd38
.word 0x3dc907a9
glabel var7f1acd3c
.word 0x40c907a9
glabel var7f1acd40
.word 0x3dc907a9
glabel var7f1acd44
.word 0x40c907a9
glabel var7f1acd48
.word 0x3dc907a9
glabel var7f1acd4c
.word 0x40c907a9
glabel var7f1acd50
.word 0x3dc907a9
glabel var7f1acd54
.word 0x40c907a9
glabel var7f1acd58
.word 0x3dc907a9
glabel var7f1acd5c
.word 0x4964e1c0
glabel var7f1acd60
.word 0x3e8e38e4
.text
/*  f0ade00:	27bdfec0 */ 	addiu	$sp,$sp,-320
/*  f0ade04:	3c0e800a */ 	lui	$t6,%hi(g_Vars+0x284)
/*  f0ade08:	8dcea244 */ 	lw	$t6,%lo(g_Vars+0x284)($t6)
/*  f0ade0c:	afbf001c */ 	sw	$ra,0x1c($sp)
/*  f0ade10:	afb00018 */ 	sw	$s0,0x18($sp)
/*  f0ade14:	afa40140 */ 	sw	$a0,0x140($sp)
/*  f0ade18:	afa50144 */ 	sw	$a1,0x144($sp)
/*  f0ade1c:	afa60148 */ 	sw	$a2,0x148($sp)
/*  f0ade20:	00008025 */ 	or	$s0,$zero,$zero
/*  f0ade24:	0fc2866a */ 	jal	bgunGetWeaponNum
/*  f0ade28:	afae0134 */ 	sw	$t6,0x134($sp)
/*  f0ade2c:	8fb90140 */ 	lw	$t9,0x140($sp)
/*  f0ade30:	8fb80134 */ 	lw	$t8,0x134($sp)
/*  f0ade34:	240fffff */ 	addiu	$t7,$zero,-1
/*  f0ade38:	00194900 */ 	sll	$t1,$t9,0x4
/*  f0ade3c:	01394823 */ 	subu	$t1,$t1,$t9
/*  f0ade40:	00094880 */ 	sll	$t1,$t1,0x2
/*  f0ade44:	01394821 */ 	addu	$t1,$t1,$t9
/*  f0ade48:	000948c0 */ 	sll	$t1,$t1,0x3
/*  f0ade4c:	01394821 */ 	addu	$t1,$t1,$t9
/*  f0ade50:	00094880 */ 	sll	$t1,$t1,0x2
/*  f0ade54:	03091821 */ 	addu	$v1,$t8,$t1
/*  f0ade58:	afa200c4 */ 	sw	$v0,0xc4($sp)
/*  f0ade5c:	afaf00c0 */ 	sw	$t7,0xc0($sp)
/*  f0ade60:	24640638 */ 	addiu	$a0,$v1,0x638
/*  f0ade64:	0fc2c40f */ 	jal	gsetGetWeaponFunction2
/*  f0ade68:	afa30030 */ 	sw	$v1,0x30($sp)
/*  f0ade6c:	8faa0134 */ 	lw	$t2,0x134($sp)
/*  f0ade70:	81441580 */ 	lb	$a0,0x1580($t2)
/*  f0ade74:	0fc2c3f4 */ 	jal	weaponFindById
/*  f0ade78:	afa200bc */ 	sw	$v0,0xbc($sp)
/*  f0ade7c:	8fa300bc */ 	lw	$v1,0xbc($sp)
/*  f0ade80:	24010001 */ 	addiu	$at,$zero,0x1
/*  f0ade84:	00002025 */ 	or	$a0,$zero,$zero
/*  f0ade88:	8c6b0000 */ 	lw	$t3,0x0($v1)
/*  f0ade8c:	316c00ff */ 	andi	$t4,$t3,0xff
/*  f0ade90:	55810003 */ 	bnel	$t4,$at,.L0f0adea0
/*  f0ade94:	80650007 */ 	lb	$a1,0x7($v1)
/*  f0ade98:	00602025 */ 	or	$a0,$v1,$zero
/*  f0ade9c:	80650007 */ 	lb	$a1,0x7($v1)
.L0f0adea0:
/*  f0adea0:	04a202ac */ 	bltzl	$a1,.L0f0ae954
/*  f0adea4:	8fbf001c */ 	lw	$ra,0x1c($sp)
/*  f0adea8:	10400009 */ 	beqz	$v0,.L0f0aded0
/*  f0adeac:	3c06800a */ 	lui	$a2,%hi(g_Vars)
/*  f0adeb0:	10800007 */ 	beqz	$a0,.L0f0aded0
/*  f0adeb4:	00056880 */ 	sll	$t5,$a1,0x2
/*  f0adeb8:	004d7021 */ 	addu	$t6,$v0,$t5
/*  f0adebc:	8dc3001c */ 	lw	$v1,0x1c($t6)
/*  f0adec0:	50600004 */ 	beqzl	$v1,.L0f0aded4
/*  f0adec4:	8fb900c0 */ 	lw	$t9,0xc0($sp)
/*  f0adec8:	8c6f0004 */ 	lw	$t7,0x4($v1)
/*  f0adecc:	afaf00c0 */ 	sw	$t7,0xc0($sp)
.L0f0aded0:
/*  f0aded0:	8fb900c0 */ 	lw	$t9,0xc0($sp)
.L0f0aded4:
/*  f0aded4:	24c69fc0 */ 	addiu	$a2,$a2,%lo(g_Vars)
/*  f0aded8:	0722029e */ 	bltzl	$t9,.L0f0ae954
/*  f0adedc:	8fbf001c */ 	lw	$ra,0x1c($sp)
/*  f0adee0:	8cd8006c */ 	lw	$t8,0x6c($a2)
/*  f0adee4:	00002825 */ 	or	$a1,$zero,$zero
/*  f0adee8:	00001825 */ 	or	$v1,$zero,$zero
/*  f0adeec:	13000003 */ 	beqz	$t8,.L0f0adefc
/*  f0adef0:	00002025 */ 	or	$a0,$zero,$zero
/*  f0adef4:	10000001 */ 	b	.L0f0adefc
/*  f0adef8:	24050001 */ 	addiu	$a1,$zero,0x1
.L0f0adefc:
/*  f0adefc:	8cc90068 */ 	lw	$t1,0x68($a2)
/*  f0adf00:	00001025 */ 	or	$v0,$zero,$zero
/*  f0adf04:	11200003 */ 	beqz	$t1,.L0f0adf14
/*  f0adf08:	00000000 */ 	nop
/*  f0adf0c:	10000001 */ 	b	.L0f0adf14
/*  f0adf10:	24030001 */ 	addiu	$v1,$zero,0x1
.L0f0adf14:
/*  f0adf14:	8cca0064 */ 	lw	$t2,0x64($a2)
/*  f0adf18:	11400003 */ 	beqz	$t2,.L0f0adf28
/*  f0adf1c:	00000000 */ 	nop
/*  f0adf20:	10000001 */ 	b	.L0f0adf28
/*  f0adf24:	24040001 */ 	addiu	$a0,$zero,0x1
.L0f0adf28:
/*  f0adf28:	8ccb0070 */ 	lw	$t3,0x70($a2)
/*  f0adf2c:	11600003 */ 	beqz	$t3,.L0f0adf3c
/*  f0adf30:	00000000 */ 	nop
/*  f0adf34:	10000001 */ 	b	.L0f0adf3c
/*  f0adf38:	24020001 */ 	addiu	$v0,$zero,0x1
.L0f0adf3c:
/*  f0adf3c:	00446021 */ 	addu	$t4,$v0,$a0
/*  f0adf40:	01836821 */ 	addu	$t5,$t4,$v1
/*  f0adf44:	01a57021 */ 	addu	$t6,$t5,$a1
/*  f0adf48:	29c10002 */ 	slti	$at,$t6,0x2
/*  f0adf4c:	10200280 */ 	beqz	$at,.L0f0ae950
/*  f0adf50:	8fa40148 */ 	lw	$a0,0x148($sp)
/*  f0adf54:	0c005746 */ 	jal	mtx4Copy
/*  f0adf58:	27a500ec */ 	addiu	$a1,$sp,0xec
/*  f0adf5c:	0fc27aef */ 	jal	bgun0f09ebbc
/*  f0adf60:	00000000 */ 	nop
/*  f0adf64:	10400004 */ 	beqz	$v0,.L0f0adf78
/*  f0adf68:	00402025 */ 	or	$a0,$v0,$zero
/*  f0adf6c:	0fc2b732 */ 	jal	casingCreate
/*  f0adf70:	27a500ec */ 	addiu	$a1,$sp,0xec
/*  f0adf74:	00408025 */ 	or	$s0,$v0,$zero
.L0f0adf78:
/*  f0adf78:	12000275 */ 	beqz	$s0,.L0f0ae950
/*  f0adf7c:	8fa200c4 */ 	lw	$v0,0xc4($sp)
/*  f0adf80:	3c198007 */ 	lui	$t9,%hi(var80070528)
/*  f0adf84:	27390528 */ 	addiu	$t9,$t9,%lo(var80070528)
/*  f0adf88:	8f210000 */ 	lw	$at,0x0($t9)
/*  f0adf8c:	27af00a4 */ 	addiu	$t7,$sp,0xa4
/*  f0adf90:	8f290004 */ 	lw	$t1,0x4($t9)
/*  f0adf94:	ade10000 */ 	sw	$at,0x0($t7)
/*  f0adf98:	8f210008 */ 	lw	$at,0x8($t9)
/*  f0adf9c:	ade90004 */ 	sw	$t1,0x4($t7)
/*  f0adfa0:	ade10008 */ 	sw	$at,0x8($t7)
/*  f0adfa4:	c7a40144 */ 	lwc1	$f4,0x144($sp)
/*  f0adfa8:	24010024 */ 	addiu	$at,$zero,0x24
/*  f0adfac:	10410008 */ 	beq	$v0,$at,.L0f0adfd0
/*  f0adfb0:	e6040000 */ 	swc1	$f4,0x0($s0)
/*  f0adfb4:	24010025 */ 	addiu	$at,$zero,0x25
/*  f0adfb8:	10410005 */ 	beq	$v0,$at,.L0f0adfd0
/*  f0adfbc:	24010002 */ 	addiu	$at,$zero,0x2
/*  f0adfc0:	10410003 */ 	beq	$v0,$at,.L0f0adfd0
/*  f0adfc4:	24010005 */ 	addiu	$at,$zero,0x5
/*  f0adfc8:	544100ee */ 	bnel	$v0,$at,.L0f0ae384
/*  f0adfcc:	24010014 */ 	addiu	$at,$zero,0x14
.L0f0adfd0:
/*  f0adfd0:	0c004b70 */ 	jal	random
/*  f0adfd4:	00000000 */ 	nop
/*  f0adfd8:	44823000 */ 	mtc1	$v0,$f6
/*  f0adfdc:	3c017f1b */ 	lui	$at,%hi(var7f1accf8)
/*  f0adfe0:	c420ccf8 */ 	lwc1	$f0,%lo(var7f1accf8)($at)
/*  f0adfe4:	04410005 */ 	bgez	$v0,.L0f0adffc
/*  f0adfe8:	46803220 */ 	cvt.s.w	$f8,$f6
/*  f0adfec:	3c014f80 */ 	lui	$at,0x4f80
/*  f0adff0:	44815000 */ 	mtc1	$at,$f10
/*  f0adff4:	00000000 */ 	nop
/*  f0adff8:	460a4200 */ 	add.s	$f8,$f8,$f10
.L0f0adffc:
/*  f0adffc:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae000:	44819000 */ 	mtc1	$at,$f18
/*  f0ae004:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae008:	44815000 */ 	mtc1	$at,$f10
/*  f0ae00c:	46124102 */ 	mul.s	$f4,$f8,$f18
/*  f0ae010:	00000000 */ 	nop
/*  f0ae014:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0ae018:	00000000 */ 	nop
/*  f0ae01c:	460a3202 */ 	mul.s	$f8,$f6,$f10
/*  f0ae020:	46004480 */ 	add.s	$f18,$f8,$f0
/*  f0ae024:	46009107 */ 	neg.s	$f4,$f18
/*  f0ae028:	0c004b70 */ 	jal	random
/*  f0ae02c:	e6040010 */ 	swc1	$f4,0x10($s0)
/*  f0ae030:	44823000 */ 	mtc1	$v0,$f6
/*  f0ae034:	3c014020 */ 	lui	$at,0x4020
/*  f0ae038:	44810000 */ 	mtc1	$at,$f0
/*  f0ae03c:	44801000 */ 	mtc1	$zero,$f2
/*  f0ae040:	04410005 */ 	bgez	$v0,.L0f0ae058
/*  f0ae044:	468032a0 */ 	cvt.s.w	$f10,$f6
/*  f0ae048:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae04c:	44814000 */ 	mtc1	$at,$f8
/*  f0ae050:	00000000 */ 	nop
/*  f0ae054:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae058:
/*  f0ae058:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae05c:	44819000 */ 	mtc1	$at,$f18
/*  f0ae060:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae064:	44814000 */ 	mtc1	$at,$f8
/*  f0ae068:	46125102 */ 	mul.s	$f4,$f10,$f18
/*  f0ae06c:	e6020018 */ 	swc1	$f2,0x18($s0)
/*  f0ae070:	26050010 */ 	addiu	$a1,$s0,0x10
/*  f0ae074:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0ae078:	00000000 */ 	nop
/*  f0ae07c:	46083282 */ 	mul.s	$f10,$f6,$f8
/*  f0ae080:	46005480 */ 	add.s	$f18,$f10,$f0
/*  f0ae084:	e6120014 */ 	swc1	$f18,0x14($s0)
/*  f0ae088:	0c0056c4 */ 	jal	mtx4RotateVecInPlace
/*  f0ae08c:	8fa40148 */ 	lw	$a0,0x148($sp)
/*  f0ae090:	0c004b70 */ 	jal	random
/*  f0ae094:	00000000 */ 	nop
/*  f0ae098:	44822000 */ 	mtc1	$v0,$f4
/*  f0ae09c:	04410005 */ 	bgez	$v0,.L0f0ae0b4
/*  f0ae0a0:	468021a0 */ 	cvt.s.w	$f6,$f4
/*  f0ae0a4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae0a8:	44814000 */ 	mtc1	$at,$f8
/*  f0ae0ac:	00000000 */ 	nop
/*  f0ae0b0:	46083180 */ 	add.s	$f6,$f6,$f8
.L0f0ae0b4:
/*  f0ae0b4:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae0b8:	44815000 */ 	mtc1	$at,$f10
/*  f0ae0bc:	3c017f1b */ 	lui	$at,%hi(var7f1accfc)
/*  f0ae0c0:	c424ccfc */ 	lwc1	$f4,%lo(var7f1accfc)($at)
/*  f0ae0c4:	460a3002 */ 	mul.s	$f0,$f6,$f10
/*  f0ae0c8:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae0cc:	44813000 */ 	mtc1	$at,$f6
/*  f0ae0d0:	3c017f1b */ 	lui	$at,%hi(var7f1acd00)
/*  f0ae0d4:	46000480 */ 	add.s	$f18,$f0,$f0
/*  f0ae0d8:	46049202 */ 	mul.s	$f8,$f18,$f4
/*  f0ae0dc:	c432cd00 */ 	lwc1	$f18,%lo(var7f1acd00)($at)
/*  f0ae0e0:	46064282 */ 	mul.s	$f10,$f8,$f6
/*  f0ae0e4:	46125101 */ 	sub.s	$f4,$f10,$f18
/*  f0ae0e8:	0c004b70 */ 	jal	random
/*  f0ae0ec:	e7a400a4 */ 	swc1	$f4,0xa4($sp)
/*  f0ae0f0:	44824000 */ 	mtc1	$v0,$f8
/*  f0ae0f4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae0f8:	04410004 */ 	bgez	$v0,.L0f0ae10c
/*  f0ae0fc:	468041a0 */ 	cvt.s.w	$f6,$f8
/*  f0ae100:	44815000 */ 	mtc1	$at,$f10
/*  f0ae104:	00000000 */ 	nop
/*  f0ae108:	460a3180 */ 	add.s	$f6,$f6,$f10
.L0f0ae10c:
/*  f0ae10c:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae110:	44819000 */ 	mtc1	$at,$f18
/*  f0ae114:	3c017f1b */ 	lui	$at,%hi(var7f1acd04)
/*  f0ae118:	c428cd04 */ 	lwc1	$f8,%lo(var7f1acd04)($at)
/*  f0ae11c:	46123002 */ 	mul.s	$f0,$f6,$f18
/*  f0ae120:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae124:	44813000 */ 	mtc1	$at,$f6
/*  f0ae128:	3c017f1b */ 	lui	$at,%hi(var7f1acd08)
/*  f0ae12c:	46000100 */ 	add.s	$f4,$f0,$f0
/*  f0ae130:	46082282 */ 	mul.s	$f10,$f4,$f8
/*  f0ae134:	c424cd08 */ 	lwc1	$f4,%lo(var7f1acd08)($at)
/*  f0ae138:	46065482 */ 	mul.s	$f18,$f10,$f6
/*  f0ae13c:	46049201 */ 	sub.s	$f8,$f18,$f4
/*  f0ae140:	0c004b70 */ 	jal	random
/*  f0ae144:	e7a800a8 */ 	swc1	$f8,0xa8($sp)
/*  f0ae148:	44825000 */ 	mtc1	$v0,$f10
/*  f0ae14c:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae150:	04410004 */ 	bgez	$v0,.L0f0ae164
/*  f0ae154:	468051a0 */ 	cvt.s.w	$f6,$f10
/*  f0ae158:	44819000 */ 	mtc1	$at,$f18
/*  f0ae15c:	00000000 */ 	nop
/*  f0ae160:	46123180 */ 	add.s	$f6,$f6,$f18
.L0f0ae164:
/*  f0ae164:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae168:	44812000 */ 	mtc1	$at,$f4
/*  f0ae16c:	3c017f1b */ 	lui	$at,%hi(var7f1acd0c)
/*  f0ae170:	c42acd0c */ 	lwc1	$f10,%lo(var7f1acd0c)($at)
/*  f0ae174:	46043002 */ 	mul.s	$f0,$f6,$f4
/*  f0ae178:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae17c:	44813000 */ 	mtc1	$at,$f6
/*  f0ae180:	3c017f1b */ 	lui	$at,%hi(var7f1acd10)
/*  f0ae184:	27a400a4 */ 	addiu	$a0,$sp,0xa4
/*  f0ae188:	27a50064 */ 	addiu	$a1,$sp,0x64
/*  f0ae18c:	46000200 */ 	add.s	$f8,$f0,$f0
/*  f0ae190:	460a4482 */ 	mul.s	$f18,$f8,$f10
/*  f0ae194:	c428cd10 */ 	lwc1	$f8,%lo(var7f1acd10)($at)
/*  f0ae198:	46069102 */ 	mul.s	$f4,$f18,$f6
/*  f0ae19c:	46082281 */ 	sub.s	$f10,$f4,$f8
/*  f0ae1a0:	0c005923 */ 	jal	mtx4LoadRotation
/*  f0ae1a4:	e7aa00ac */ 	swc1	$f10,0xac($sp)
/*  f0ae1a8:	27a40064 */ 	addiu	$a0,$sp,0x64
/*  f0ae1ac:	0c005768 */ 	jal	mtx4ToMtx3
/*  f0ae1b0:	27a500c8 */ 	addiu	$a1,$sp,0xc8
/*  f0ae1b4:	3c014580 */ 	lui	$at,0x4580
/*  f0ae1b8:	44810000 */ 	mtc1	$at,$f0
/*  f0ae1bc:	02003825 */ 	or	$a3,$s0,$zero
/*  f0ae1c0:	27a600c8 */ 	addiu	$a2,$sp,0xc8
/*  f0ae1c4:	27a800ec */ 	addiu	$t0,$sp,0xec
/*  f0ae1c8:	24050003 */ 	addiu	$a1,$zero,0x3
.L0f0ae1cc:
/*  f0ae1cc:	00001025 */ 	or	$v0,$zero,$zero
/*  f0ae1d0:	00e01825 */ 	or	$v1,$a3,$zero
/*  f0ae1d4:	00c02025 */ 	or	$a0,$a2,$zero
/*  f0ae1d8:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae1dc:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae1e0:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae1e4:	5045000d */ 	beql	$v0,$a1,.L0f0ae21c
/*  f0ae1e8:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae1ec:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae1f0:
/*  f0ae1f0:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae1f4:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae1f8:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae1fc:	440b8000 */ 	mfc1	$t3,$f16
/*  f0ae200:	00000000 */ 	nop
/*  f0ae204:	a46b002c */ 	sh	$t3,0x2c($v1)
/*  f0ae208:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae20c:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae210:	5445fff7 */ 	bnel	$v0,$a1,.L0f0ae1f0
/*  f0ae214:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae218:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae21c:
/*  f0ae21c:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae220:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae224:	440b8000 */ 	mfc1	$t3,$f16
/*  f0ae228:	00000000 */ 	nop
/*  f0ae22c:	a46b002c */ 	sh	$t3,0x2c($v1)
/*  f0ae230:	24c6000c */ 	addiu	$a2,$a2,0xc
/*  f0ae234:	14c8ffe5 */ 	bne	$a2,$t0,.L0f0ae1cc
/*  f0ae238:	24e70006 */ 	addiu	$a3,$a3,0x6
/*  f0ae23c:	0c004b70 */ 	jal	random
/*  f0ae240:	00000000 */ 	nop
/*  f0ae244:	3c030015 */ 	lui	$v1,0x15
/*  f0ae248:	3463aca6 */ 	ori	$v1,$v1,0xaca6
/*  f0ae24c:	00026602 */ 	srl	$t4,$v0,0x18
/*  f0ae250:	01830019 */ 	multu	$t4,$v1
/*  f0ae254:	00006812 */ 	mflo	$t5
/*  f0ae258:	000d7283 */ 	sra	$t6,$t5,0xa
/*  f0ae25c:	01c3c021 */ 	addu	$t8,$t6,$v1
/*  f0ae260:	0c004b70 */ 	jal	random
/*  f0ae264:	afb8005c */ 	sw	$t8,0x5c($sp)
/*  f0ae268:	8faf005c */ 	lw	$t7,0x5c($sp)
/*  f0ae26c:	3c03800a */ 	lui	$v1,%hi(g_Vars)
/*  f0ae270:	24639fc0 */ 	addiu	$v1,$v1,%lo(g_Vars)
/*  f0ae274:	004f001b */ 	divu	$zero,$v0,$t7
/*  f0ae278:	0000c810 */ 	mfhi	$t9
/*  f0ae27c:	44994000 */ 	mtc1	$t9,$f8
/*  f0ae280:	c60c0014 */ 	lwc1	$f12,0x14($s0)
/*  f0ae284:	15e00002 */ 	bnez	$t7,.L0f0ae290
/*  f0ae288:	00000000 */ 	nop
/*  f0ae28c:	0007000d */ 	break	0x7
.L0f0ae290:
/*  f0ae290:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae294:	07210004 */ 	bgez	$t9,.L0f0ae2a8
/*  f0ae298:	468042a0 */ 	cvt.s.w	$f10,$f8
/*  f0ae29c:	44819000 */ 	mtc1	$at,$f18
/*  f0ae2a0:	00000000 */ 	nop
/*  f0ae2a4:	46125280 */ 	add.s	$f10,$f10,$f18
.L0f0ae2a8:
/*  f0ae2a8:	3c017f1b */ 	lui	$at,%hi(var7f1acd14)
/*  f0ae2ac:	c426cd14 */ 	lwc1	$f6,%lo(var7f1acd14)($at)
/*  f0ae2b0:	3c017f1b */ 	lui	$at,%hi(var7f1acd18)
/*  f0ae2b4:	c424cd18 */ 	lwc1	$f4,%lo(var7f1acd18)($at)
/*  f0ae2b8:	46065003 */ 	div.s	$f0,$f10,$f6
/*  f0ae2bc:	3c013f00 */ 	lui	$at,0x3f00
/*  f0ae2c0:	44813000 */ 	mtc1	$at,$f6
/*  f0ae2c4:	c60e0010 */ 	lwc1	$f14,0x10($s0)
/*  f0ae2c8:	c6100018 */ 	lwc1	$f16,0x18($s0)
/*  f0ae2cc:	46040202 */ 	mul.s	$f8,$f0,$f4
/*  f0ae2d0:	46086081 */ 	sub.s	$f2,$f12,$f8
/*  f0ae2d4:	c6080008 */ 	lwc1	$f8,0x8($s0)
/*  f0ae2d8:	46026480 */ 	add.s	$f18,$f12,$f2
/*  f0ae2dc:	e6020014 */ 	swc1	$f2,0x14($s0)
/*  f0ae2e0:	46120282 */ 	mul.s	$f10,$f0,$f18
/*  f0ae2e4:	00000000 */ 	nop
/*  f0ae2e8:	46065102 */ 	mul.s	$f4,$f10,$f6
/*  f0ae2ec:	c60a0004 */ 	lwc1	$f10,0x4($s0)
/*  f0ae2f0:	460e0182 */ 	mul.s	$f6,$f0,$f14
/*  f0ae2f4:	46044480 */ 	add.s	$f18,$f8,$f4
/*  f0ae2f8:	c604000c */ 	lwc1	$f4,0xc($s0)
/*  f0ae2fc:	46065200 */ 	add.s	$f8,$f10,$f6
/*  f0ae300:	e6120008 */ 	swc1	$f18,0x8($s0)
/*  f0ae304:	46100482 */ 	mul.s	$f18,$f0,$f16
/*  f0ae308:	e6080004 */ 	swc1	$f8,0x4($s0)
/*  f0ae30c:	46122280 */ 	add.s	$f10,$f4,$f18
/*  f0ae310:	e60a000c */ 	swc1	$f10,0xc($s0)
/*  f0ae314:	8c690034 */ 	lw	$t1,0x34($v1)
/*  f0ae318:	8fa20030 */ 	lw	$v0,0x30($sp)
/*  f0ae31c:	1920018c */ 	blez	$t1,.L0f0ae950
/*  f0ae320:	00000000 */ 	nop
/*  f0ae324:	c446095c */ 	lwc1	$f6,0x95c($v0)
/*  f0ae328:	c448099c */ 	lwc1	$f8,0x99c($v0)
/*  f0ae32c:	c472004c */ 	lwc1	$f18,0x4c($v1)
/*  f0ae330:	46083101 */ 	sub.s	$f4,$f6,$f8
/*  f0ae334:	46122283 */ 	div.s	$f10,$f4,$f18
/*  f0ae338:	460a7180 */ 	add.s	$f6,$f14,$f10
/*  f0ae33c:	e6060010 */ 	swc1	$f6,0x10($s0)
/*  f0ae340:	c44409a0 */ 	lwc1	$f4,0x9a0($v0)
/*  f0ae344:	c4480960 */ 	lwc1	$f8,0x960($v0)
/*  f0ae348:	c46a004c */ 	lwc1	$f10,0x4c($v1)
/*  f0ae34c:	46044481 */ 	sub.s	$f18,$f8,$f4
/*  f0ae350:	c6080014 */ 	lwc1	$f8,0x14($s0)
/*  f0ae354:	460a9183 */ 	div.s	$f6,$f18,$f10
/*  f0ae358:	46064100 */ 	add.s	$f4,$f8,$f6
/*  f0ae35c:	e6040014 */ 	swc1	$f4,0x14($s0)
/*  f0ae360:	c44a09a4 */ 	lwc1	$f10,0x9a4($v0)
/*  f0ae364:	c4520964 */ 	lwc1	$f18,0x964($v0)
/*  f0ae368:	c466004c */ 	lwc1	$f6,0x4c($v1)
/*  f0ae36c:	460a9201 */ 	sub.s	$f8,$f18,$f10
/*  f0ae370:	46064103 */ 	div.s	$f4,$f8,$f6
/*  f0ae374:	46048480 */ 	add.s	$f18,$f16,$f4
/*  f0ae378:	10000175 */ 	b	.L0f0ae950
/*  f0ae37c:	e6120018 */ 	swc1	$f18,0x18($s0)
/*  f0ae380:	24010014 */ 	addiu	$at,$zero,0x14
.L0f0ae384:
/*  f0ae384:	1441002e */ 	bne	$v0,$at,.L0f0ae440
/*  f0ae388:	00000000 */ 	nop
/*  f0ae38c:	0c004b70 */ 	jal	random
/*  f0ae390:	00000000 */ 	nop
/*  f0ae394:	44825000 */ 	mtc1	$v0,$f10
/*  f0ae398:	3c017f1b */ 	lui	$at,%hi(var7f1acd1c)
/*  f0ae39c:	c420cd1c */ 	lwc1	$f0,%lo(var7f1acd1c)($at)
/*  f0ae3a0:	04410005 */ 	bgez	$v0,.L0f0ae3b8
/*  f0ae3a4:	46805220 */ 	cvt.s.w	$f8,$f10
/*  f0ae3a8:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae3ac:	44813000 */ 	mtc1	$at,$f6
/*  f0ae3b0:	00000000 */ 	nop
/*  f0ae3b4:	46064200 */ 	add.s	$f8,$f8,$f6
.L0f0ae3b8:
/*  f0ae3b8:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae3bc:	44812000 */ 	mtc1	$at,$f4
/*  f0ae3c0:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae3c4:	44813000 */ 	mtc1	$at,$f6
/*  f0ae3c8:	46044482 */ 	mul.s	$f18,$f8,$f4
/*  f0ae3cc:	00000000 */ 	nop
/*  f0ae3d0:	46009282 */ 	mul.s	$f10,$f18,$f0
/*  f0ae3d4:	00000000 */ 	nop
/*  f0ae3d8:	46065202 */ 	mul.s	$f8,$f10,$f6
/*  f0ae3dc:	46004100 */ 	add.s	$f4,$f8,$f0
/*  f0ae3e0:	46002487 */ 	neg.s	$f18,$f4
/*  f0ae3e4:	0c004b70 */ 	jal	random
/*  f0ae3e8:	e6120010 */ 	swc1	$f18,0x10($s0)
/*  f0ae3ec:	44825000 */ 	mtc1	$v0,$f10
/*  f0ae3f0:	3c017f1b */ 	lui	$at,%hi(var7f1acd20)
/*  f0ae3f4:	c420cd20 */ 	lwc1	$f0,%lo(var7f1acd20)($at)
/*  f0ae3f8:	04410005 */ 	bgez	$v0,.L0f0ae410
/*  f0ae3fc:	468051a0 */ 	cvt.s.w	$f6,$f10
/*  f0ae400:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae404:	44814000 */ 	mtc1	$at,$f8
/*  f0ae408:	00000000 */ 	nop
/*  f0ae40c:	46083180 */ 	add.s	$f6,$f6,$f8
.L0f0ae410:
/*  f0ae410:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae414:	44812000 */ 	mtc1	$at,$f4
/*  f0ae418:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae41c:	44814000 */ 	mtc1	$at,$f8
/*  f0ae420:	46043482 */ 	mul.s	$f18,$f6,$f4
/*  f0ae424:	00000000 */ 	nop
/*  f0ae428:	46009282 */ 	mul.s	$f10,$f18,$f0
/*  f0ae42c:	00000000 */ 	nop
/*  f0ae430:	46085182 */ 	mul.s	$f6,$f10,$f8
/*  f0ae434:	46003100 */ 	add.s	$f4,$f6,$f0
/*  f0ae438:	1000002d */ 	b	.L0f0ae4f0
/*  f0ae43c:	e6040014 */ 	swc1	$f4,0x14($s0)
.L0f0ae440:
/*  f0ae440:	0c004b70 */ 	jal	random
/*  f0ae444:	00000000 */ 	nop
/*  f0ae448:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae44c:	3c017f1b */ 	lui	$at,%hi(var7f1acd24)
/*  f0ae450:	c420cd24 */ 	lwc1	$f0,%lo(var7f1acd24)($at)
/*  f0ae454:	04410005 */ 	bgez	$v0,.L0f0ae46c
/*  f0ae458:	468092a0 */ 	cvt.s.w	$f10,$f18
/*  f0ae45c:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae460:	44814000 */ 	mtc1	$at,$f8
/*  f0ae464:	00000000 */ 	nop
/*  f0ae468:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae46c:
/*  f0ae46c:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae470:	44813000 */ 	mtc1	$at,$f6
/*  f0ae474:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae478:	44814000 */ 	mtc1	$at,$f8
/*  f0ae47c:	46065102 */ 	mul.s	$f4,$f10,$f6
/*  f0ae480:	00000000 */ 	nop
/*  f0ae484:	46002482 */ 	mul.s	$f18,$f4,$f0
/*  f0ae488:	00000000 */ 	nop
/*  f0ae48c:	46089282 */ 	mul.s	$f10,$f18,$f8
/*  f0ae490:	46005180 */ 	add.s	$f6,$f10,$f0
/*  f0ae494:	46003107 */ 	neg.s	$f4,$f6
/*  f0ae498:	0c004b70 */ 	jal	random
/*  f0ae49c:	e6040010 */ 	swc1	$f4,0x10($s0)
/*  f0ae4a0:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae4a4:	3c017f1b */ 	lui	$at,%hi(var7f1acd28)
/*  f0ae4a8:	c420cd28 */ 	lwc1	$f0,%lo(var7f1acd28)($at)
/*  f0ae4ac:	04410005 */ 	bgez	$v0,.L0f0ae4c4
/*  f0ae4b0:	46809220 */ 	cvt.s.w	$f8,$f18
/*  f0ae4b4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae4b8:	44815000 */ 	mtc1	$at,$f10
/*  f0ae4bc:	00000000 */ 	nop
/*  f0ae4c0:	460a4200 */ 	add.s	$f8,$f8,$f10
.L0f0ae4c4:
/*  f0ae4c4:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae4c8:	44813000 */ 	mtc1	$at,$f6
/*  f0ae4cc:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae4d0:	44815000 */ 	mtc1	$at,$f10
/*  f0ae4d4:	46064102 */ 	mul.s	$f4,$f8,$f6
/*  f0ae4d8:	00000000 */ 	nop
/*  f0ae4dc:	46002482 */ 	mul.s	$f18,$f4,$f0
/*  f0ae4e0:	00000000 */ 	nop
/*  f0ae4e4:	460a9202 */ 	mul.s	$f8,$f18,$f10
/*  f0ae4e8:	46004180 */ 	add.s	$f6,$f8,$f0
/*  f0ae4ec:	e6060014 */ 	swc1	$f6,0x14($s0)
.L0f0ae4f0:
/*  f0ae4f0:	44801000 */ 	mtc1	$zero,$f2
/*  f0ae4f4:	24010008 */ 	addiu	$at,$zero,0x8
/*  f0ae4f8:	26050010 */ 	addiu	$a1,$s0,0x10
/*  f0ae4fc:	e6020018 */ 	swc1	$f2,0x18($s0)
/*  f0ae500:	8faa00c4 */ 	lw	$t2,0xc4($sp)
/*  f0ae504:	11410002 */ 	beq	$t2,$at,.L0f0ae510
/*  f0ae508:	24010009 */ 	addiu	$at,$zero,0x9
/*  f0ae50c:	15410005 */ 	bne	$t2,$at,.L0f0ae524
.L0f0ae510:
/*  f0ae510:	3c01bf80 */ 	lui	$at,0xbf80
/*  f0ae514:	44812000 */ 	mtc1	$at,$f4
/*  f0ae518:	e6020010 */ 	swc1	$f2,0x10($s0)
/*  f0ae51c:	e6020014 */ 	swc1	$f2,0x14($s0)
/*  f0ae520:	e6040018 */ 	swc1	$f4,0x18($s0)
.L0f0ae524:
/*  f0ae524:	8fa40148 */ 	lw	$a0,0x148($sp)
/*  f0ae528:	0c0056c4 */ 	jal	mtx4RotateVecInPlace
/*  f0ae52c:	afa5002c */ 	sw	$a1,0x2c($sp)
/*  f0ae530:	8fab00c4 */ 	lw	$t3,0xc4($sp)
/*  f0ae534:	24010014 */ 	addiu	$at,$zero,0x14
/*  f0ae538:	1561004a */ 	bne	$t3,$at,.L0f0ae664
/*  f0ae53c:	00000000 */ 	nop
/*  f0ae540:	0c004b70 */ 	jal	random
/*  f0ae544:	00000000 */ 	nop
/*  f0ae548:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae54c:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae550:	04410004 */ 	bgez	$v0,.L0f0ae564
/*  f0ae554:	468092a0 */ 	cvt.s.w	$f10,$f18
/*  f0ae558:	44814000 */ 	mtc1	$at,$f8
/*  f0ae55c:	00000000 */ 	nop
/*  f0ae560:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae564:
/*  f0ae564:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae568:	44813000 */ 	mtc1	$at,$f6
/*  f0ae56c:	3c017f1b */ 	lui	$at,%hi(var7f1acd2c)
/*  f0ae570:	c432cd2c */ 	lwc1	$f18,%lo(var7f1acd2c)($at)
/*  f0ae574:	46065002 */ 	mul.s	$f0,$f10,$f6
/*  f0ae578:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae57c:	44815000 */ 	mtc1	$at,$f10
/*  f0ae580:	3c017f1b */ 	lui	$at,%hi(var7f1acd30)
/*  f0ae584:	46000100 */ 	add.s	$f4,$f0,$f0
/*  f0ae588:	46122202 */ 	mul.s	$f8,$f4,$f18
/*  f0ae58c:	c424cd30 */ 	lwc1	$f4,%lo(var7f1acd30)($at)
/*  f0ae590:	460a4182 */ 	mul.s	$f6,$f8,$f10
/*  f0ae594:	46043481 */ 	sub.s	$f18,$f6,$f4
/*  f0ae598:	0c004b70 */ 	jal	random
/*  f0ae59c:	e7b200a4 */ 	swc1	$f18,0xa4($sp)
/*  f0ae5a0:	44824000 */ 	mtc1	$v0,$f8
/*  f0ae5a4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae5a8:	04410004 */ 	bgez	$v0,.L0f0ae5bc
/*  f0ae5ac:	468042a0 */ 	cvt.s.w	$f10,$f8
/*  f0ae5b0:	44813000 */ 	mtc1	$at,$f6
/*  f0ae5b4:	00000000 */ 	nop
/*  f0ae5b8:	46065280 */ 	add.s	$f10,$f10,$f6
.L0f0ae5bc:
/*  f0ae5bc:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae5c0:	44812000 */ 	mtc1	$at,$f4
/*  f0ae5c4:	3c017f1b */ 	lui	$at,%hi(var7f1acd34)
/*  f0ae5c8:	c428cd34 */ 	lwc1	$f8,%lo(var7f1acd34)($at)
/*  f0ae5cc:	46045002 */ 	mul.s	$f0,$f10,$f4
/*  f0ae5d0:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae5d4:	44815000 */ 	mtc1	$at,$f10
/*  f0ae5d8:	3c017f1b */ 	lui	$at,%hi(var7f1acd38)
/*  f0ae5dc:	46000480 */ 	add.s	$f18,$f0,$f0
/*  f0ae5e0:	46089182 */ 	mul.s	$f6,$f18,$f8
/*  f0ae5e4:	c432cd38 */ 	lwc1	$f18,%lo(var7f1acd38)($at)
/*  f0ae5e8:	460a3102 */ 	mul.s	$f4,$f6,$f10
/*  f0ae5ec:	46122201 */ 	sub.s	$f8,$f4,$f18
/*  f0ae5f0:	0c004b70 */ 	jal	random
/*  f0ae5f4:	e7a800a8 */ 	swc1	$f8,0xa8($sp)
/*  f0ae5f8:	44823000 */ 	mtc1	$v0,$f6
/*  f0ae5fc:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae600:	04410004 */ 	bgez	$v0,.L0f0ae614
/*  f0ae604:	468032a0 */ 	cvt.s.w	$f10,$f6
/*  f0ae608:	44812000 */ 	mtc1	$at,$f4
/*  f0ae60c:	00000000 */ 	nop
/*  f0ae610:	46045280 */ 	add.s	$f10,$f10,$f4
.L0f0ae614:
/*  f0ae614:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae618:	44819000 */ 	mtc1	$at,$f18
/*  f0ae61c:	3c017f1b */ 	lui	$at,%hi(var7f1acd3c)
/*  f0ae620:	c426cd3c */ 	lwc1	$f6,%lo(var7f1acd3c)($at)
/*  f0ae624:	46125002 */ 	mul.s	$f0,$f10,$f18
/*  f0ae628:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae62c:	44815000 */ 	mtc1	$at,$f10
/*  f0ae630:	3c017f1b */ 	lui	$at,%hi(var7f1acd40)
/*  f0ae634:	27a400a4 */ 	addiu	$a0,$sp,0xa4
/*  f0ae638:	27a50064 */ 	addiu	$a1,$sp,0x64
/*  f0ae63c:	46000200 */ 	add.s	$f8,$f0,$f0
/*  f0ae640:	46064102 */ 	mul.s	$f4,$f8,$f6
/*  f0ae644:	c428cd40 */ 	lwc1	$f8,%lo(var7f1acd40)($at)
/*  f0ae648:	460a2482 */ 	mul.s	$f18,$f4,$f10
/*  f0ae64c:	46089181 */ 	sub.s	$f6,$f18,$f8
/*  f0ae650:	0c005923 */ 	jal	mtx4LoadRotation
/*  f0ae654:	e7a600ac */ 	swc1	$f6,0xac($sp)
/*  f0ae658:	27a40064 */ 	addiu	$a0,$sp,0x64
/*  f0ae65c:	0c0056c4 */ 	jal	mtx4RotateVecInPlace
/*  f0ae660:	8fa5002c */ 	lw	$a1,0x2c($sp)
.L0f0ae664:
/*  f0ae664:	0c004b70 */ 	jal	random
/*  f0ae668:	00000000 */ 	nop
/*  f0ae66c:	44822000 */ 	mtc1	$v0,$f4
/*  f0ae670:	04410005 */ 	bgez	$v0,.L0f0ae688
/*  f0ae674:	468022a0 */ 	cvt.s.w	$f10,$f4
/*  f0ae678:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae67c:	44819000 */ 	mtc1	$at,$f18
/*  f0ae680:	00000000 */ 	nop
/*  f0ae684:	46125280 */ 	add.s	$f10,$f10,$f18
.L0f0ae688:
/*  f0ae688:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae68c:	44814000 */ 	mtc1	$at,$f8
/*  f0ae690:	3c017f1b */ 	lui	$at,%hi(var7f1acd44)
/*  f0ae694:	c424cd44 */ 	lwc1	$f4,%lo(var7f1acd44)($at)
/*  f0ae698:	46085002 */ 	mul.s	$f0,$f10,$f8
/*  f0ae69c:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae6a0:	44815000 */ 	mtc1	$at,$f10
/*  f0ae6a4:	3c017f1b */ 	lui	$at,%hi(var7f1acd48)
/*  f0ae6a8:	46000180 */ 	add.s	$f6,$f0,$f0
/*  f0ae6ac:	46043482 */ 	mul.s	$f18,$f6,$f4
/*  f0ae6b0:	c426cd48 */ 	lwc1	$f6,%lo(var7f1acd48)($at)
/*  f0ae6b4:	460a9202 */ 	mul.s	$f8,$f18,$f10
/*  f0ae6b8:	46064101 */ 	sub.s	$f4,$f8,$f6
/*  f0ae6bc:	0c004b70 */ 	jal	random
/*  f0ae6c0:	e7a400a4 */ 	swc1	$f4,0xa4($sp)
/*  f0ae6c4:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae6c8:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae6cc:	04410004 */ 	bgez	$v0,.L0f0ae6e0
/*  f0ae6d0:	468092a0 */ 	cvt.s.w	$f10,$f18
/*  f0ae6d4:	44814000 */ 	mtc1	$at,$f8
/*  f0ae6d8:	00000000 */ 	nop
/*  f0ae6dc:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae6e0:
/*  f0ae6e0:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae6e4:	44813000 */ 	mtc1	$at,$f6
/*  f0ae6e8:	3c017f1b */ 	lui	$at,%hi(var7f1acd4c)
/*  f0ae6ec:	c432cd4c */ 	lwc1	$f18,%lo(var7f1acd4c)($at)
/*  f0ae6f0:	46065002 */ 	mul.s	$f0,$f10,$f6
/*  f0ae6f4:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae6f8:	44815000 */ 	mtc1	$at,$f10
/*  f0ae6fc:	3c017f1b */ 	lui	$at,%hi(var7f1acd50)
/*  f0ae700:	46000100 */ 	add.s	$f4,$f0,$f0
/*  f0ae704:	46122202 */ 	mul.s	$f8,$f4,$f18
/*  f0ae708:	c424cd50 */ 	lwc1	$f4,%lo(var7f1acd50)($at)
/*  f0ae70c:	460a4182 */ 	mul.s	$f6,$f8,$f10
/*  f0ae710:	46043481 */ 	sub.s	$f18,$f6,$f4
/*  f0ae714:	0c004b70 */ 	jal	random
/*  f0ae718:	e7b200a8 */ 	swc1	$f18,0xa8($sp)
/*  f0ae71c:	44824000 */ 	mtc1	$v0,$f8
/*  f0ae720:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae724:	04410004 */ 	bgez	$v0,.L0f0ae738
/*  f0ae728:	468042a0 */ 	cvt.s.w	$f10,$f8
/*  f0ae72c:	44813000 */ 	mtc1	$at,$f6
/*  f0ae730:	00000000 */ 	nop
/*  f0ae734:	46065280 */ 	add.s	$f10,$f10,$f6
.L0f0ae738:
/*  f0ae738:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae73c:	44812000 */ 	mtc1	$at,$f4
/*  f0ae740:	3c017f1b */ 	lui	$at,%hi(var7f1acd54)
/*  f0ae744:	c428cd54 */ 	lwc1	$f8,%lo(var7f1acd54)($at)
/*  f0ae748:	46045002 */ 	mul.s	$f0,$f10,$f4
/*  f0ae74c:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae750:	44815000 */ 	mtc1	$at,$f10
/*  f0ae754:	3c017f1b */ 	lui	$at,%hi(var7f1acd58)
/*  f0ae758:	27a400a4 */ 	addiu	$a0,$sp,0xa4
/*  f0ae75c:	27a50064 */ 	addiu	$a1,$sp,0x64
/*  f0ae760:	46000480 */ 	add.s	$f18,$f0,$f0
/*  f0ae764:	46089182 */ 	mul.s	$f6,$f18,$f8
/*  f0ae768:	c432cd58 */ 	lwc1	$f18,%lo(var7f1acd58)($at)
/*  f0ae76c:	460a3102 */ 	mul.s	$f4,$f6,$f10
/*  f0ae770:	46122201 */ 	sub.s	$f8,$f4,$f18
/*  f0ae774:	0c005923 */ 	jal	mtx4LoadRotation
/*  f0ae778:	e7a800ac */ 	swc1	$f8,0xac($sp)
/*  f0ae77c:	27a40064 */ 	addiu	$a0,$sp,0x64
/*  f0ae780:	0c005768 */ 	jal	mtx4ToMtx3
/*  f0ae784:	27a500c8 */ 	addiu	$a1,$sp,0xc8
/*  f0ae788:	3c014580 */ 	lui	$at,0x4580
/*  f0ae78c:	44810000 */ 	mtc1	$at,$f0
/*  f0ae790:	02003825 */ 	or	$a3,$s0,$zero
/*  f0ae794:	27a600c8 */ 	addiu	$a2,$sp,0xc8
/*  f0ae798:	27a800ec */ 	addiu	$t0,$sp,0xec
/*  f0ae79c:	24050003 */ 	addiu	$a1,$zero,0x3
.L0f0ae7a0:
/*  f0ae7a0:	00001025 */ 	or	$v0,$zero,$zero
/*  f0ae7a4:	00e01825 */ 	or	$v1,$a3,$zero
/*  f0ae7a8:	00c02025 */ 	or	$a0,$a2,$zero
/*  f0ae7ac:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae7b0:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae7b4:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae7b8:	5045000d */ 	beql	$v0,$a1,.L0f0ae7f0
/*  f0ae7bc:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae7c0:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae7c4:
/*  f0ae7c4:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae7c8:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae7cc:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae7d0:	440d8000 */ 	mfc1	$t5,$f16
/*  f0ae7d4:	00000000 */ 	nop
/*  f0ae7d8:	a46d002c */ 	sh	$t5,0x2c($v1)
/*  f0ae7dc:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae7e0:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae7e4:	5445fff7 */ 	bnel	$v0,$a1,.L0f0ae7c4
/*  f0ae7e8:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae7ec:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae7f0:
/*  f0ae7f0:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae7f4:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae7f8:	440d8000 */ 	mfc1	$t5,$f16
/*  f0ae7fc:	00000000 */ 	nop
/*  f0ae800:	a46d002c */ 	sh	$t5,0x2c($v1)
/*  f0ae804:	24c6000c */ 	addiu	$a2,$a2,0xc
/*  f0ae808:	14c8ffe5 */ 	bne	$a2,$t0,.L0f0ae7a0
/*  f0ae80c:	24e70006 */ 	addiu	$a3,$a3,0x6
/*  f0ae810:	0c004b70 */ 	jal	random
/*  f0ae814:	00000000 */ 	nop
/*  f0ae818:	3c030015 */ 	lui	$v1,0x15
/*  f0ae81c:	3463aca6 */ 	ori	$v1,$v1,0xaca6
/*  f0ae820:	00027602 */ 	srl	$t6,$v0,0x18
/*  f0ae824:	01c30019 */ 	multu	$t6,$v1
/*  f0ae828:	0000c012 */ 	mflo	$t8
/*  f0ae82c:	00187a83 */ 	sra	$t7,$t8,0xa
/*  f0ae830:	01e3c821 */ 	addu	$t9,$t7,$v1
/*  f0ae834:	0c004b70 */ 	jal	random
/*  f0ae838:	afb9004c */ 	sw	$t9,0x4c($sp)
/*  f0ae83c:	8fa9004c */ 	lw	$t1,0x4c($sp)
/*  f0ae840:	3c03800a */ 	lui	$v1,%hi(g_Vars)
/*  f0ae844:	24639fc0 */ 	addiu	$v1,$v1,%lo(g_Vars)
/*  f0ae848:	0049001b */ 	divu	$zero,$v0,$t1
/*  f0ae84c:	00005010 */ 	mfhi	$t2
/*  f0ae850:	448a9000 */ 	mtc1	$t2,$f18
/*  f0ae854:	c60c0014 */ 	lwc1	$f12,0x14($s0)
/*  f0ae858:	15200002 */ 	bnez	$t1,.L0f0ae864
/*  f0ae85c:	00000000 */ 	nop
/*  f0ae860:	0007000d */ 	break	0x7
.L0f0ae864:
/*  f0ae864:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae868:	05410004 */ 	bgez	$t2,.L0f0ae87c
/*  f0ae86c:	46809220 */ 	cvt.s.w	$f8,$f18
/*  f0ae870:	44813000 */ 	mtc1	$at,$f6
/*  f0ae874:	00000000 */ 	nop
/*  f0ae878:	46064200 */ 	add.s	$f8,$f8,$f6
.L0f0ae87c:
/*  f0ae87c:	3c017f1b */ 	lui	$at,%hi(var7f1acd5c)
/*  f0ae880:	c42acd5c */ 	lwc1	$f10,%lo(var7f1acd5c)($at)
/*  f0ae884:	3c017f1b */ 	lui	$at,%hi(var7f1acd60)
/*  f0ae888:	c424cd60 */ 	lwc1	$f4,%lo(var7f1acd60)($at)
/*  f0ae88c:	460a4003 */ 	div.s	$f0,$f8,$f10
/*  f0ae890:	3c013f00 */ 	lui	$at,0x3f00
/*  f0ae894:	44815000 */ 	mtc1	$at,$f10
/*  f0ae898:	c60e0010 */ 	lwc1	$f14,0x10($s0)
/*  f0ae89c:	c6100018 */ 	lwc1	$f16,0x18($s0)
/*  f0ae8a0:	46040482 */ 	mul.s	$f18,$f0,$f4
/*  f0ae8a4:	46126081 */ 	sub.s	$f2,$f12,$f18
/*  f0ae8a8:	c6120008 */ 	lwc1	$f18,0x8($s0)
/*  f0ae8ac:	46026180 */ 	add.s	$f6,$f12,$f2
/*  f0ae8b0:	e6020014 */ 	swc1	$f2,0x14($s0)
/*  f0ae8b4:	46060202 */ 	mul.s	$f8,$f0,$f6
/*  f0ae8b8:	00000000 */ 	nop
/*  f0ae8bc:	460a4102 */ 	mul.s	$f4,$f8,$f10
/*  f0ae8c0:	c6080004 */ 	lwc1	$f8,0x4($s0)
/*  f0ae8c4:	460e0282 */ 	mul.s	$f10,$f0,$f14
/*  f0ae8c8:	46049180 */ 	add.s	$f6,$f18,$f4
/*  f0ae8cc:	c604000c */ 	lwc1	$f4,0xc($s0)
/*  f0ae8d0:	460a4480 */ 	add.s	$f18,$f8,$f10
/*  f0ae8d4:	e6060008 */ 	swc1	$f6,0x8($s0)
/*  f0ae8d8:	46100182 */ 	mul.s	$f6,$f0,$f16
/*  f0ae8dc:	e6120004 */ 	swc1	$f18,0x4($s0)
/*  f0ae8e0:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0ae8e4:	e608000c */ 	swc1	$f8,0xc($s0)
/*  f0ae8e8:	8c6b0034 */ 	lw	$t3,0x34($v1)
/*  f0ae8ec:	8fa20030 */ 	lw	$v0,0x30($sp)
/*  f0ae8f0:	19600017 */ 	blez	$t3,.L0f0ae950
/*  f0ae8f4:	00000000 */ 	nop
/*  f0ae8f8:	c44a095c */ 	lwc1	$f10,0x95c($v0)
/*  f0ae8fc:	c452099c */ 	lwc1	$f18,0x99c($v0)
/*  f0ae900:	c466004c */ 	lwc1	$f6,0x4c($v1)
/*  f0ae904:	46125101 */ 	sub.s	$f4,$f10,$f18
/*  f0ae908:	46062203 */ 	div.s	$f8,$f4,$f6
/*  f0ae90c:	46087280 */ 	add.s	$f10,$f14,$f8
/*  f0ae910:	e60a0010 */ 	swc1	$f10,0x10($s0)
/*  f0ae914:	c44409a0 */ 	lwc1	$f4,0x9a0($v0)
/*  f0ae918:	c4520960 */ 	lwc1	$f18,0x960($v0)
/*  f0ae91c:	c468004c */ 	lwc1	$f8,0x4c($v1)
/*  f0ae920:	46049181 */ 	sub.s	$f6,$f18,$f4
/*  f0ae924:	c6120014 */ 	lwc1	$f18,0x14($s0)
/*  f0ae928:	46083283 */ 	div.s	$f10,$f6,$f8
/*  f0ae92c:	460a9100 */ 	add.s	$f4,$f18,$f10
/*  f0ae930:	e6040014 */ 	swc1	$f4,0x14($s0)
/*  f0ae934:	c44809a4 */ 	lwc1	$f8,0x9a4($v0)
/*  f0ae938:	c4460964 */ 	lwc1	$f6,0x964($v0)
/*  f0ae93c:	c46a004c */ 	lwc1	$f10,0x4c($v1)
/*  f0ae940:	46083481 */ 	sub.s	$f18,$f6,$f8
/*  f0ae944:	460a9103 */ 	div.s	$f4,$f18,$f10
/*  f0ae948:	46048180 */ 	add.s	$f6,$f16,$f4
/*  f0ae94c:	e6060018 */ 	swc1	$f6,0x18($s0)
.L0f0ae950:
/*  f0ae950:	8fbf001c */ 	lw	$ra,0x1c($sp)
.L0f0ae954:
/*  f0ae954:	8fb00018 */ 	lw	$s0,0x18($sp)
/*  f0ae958:	27bd0140 */ 	addiu	$sp,$sp,0x140
/*  f0ae95c:	03e00008 */ 	jr	$ra
/*  f0ae960:	00000000 */ 	nop
);
#else
GLOBAL_ASM(
glabel casingCreateForHand
.late_rodata
glabel var7f1accf8
.word 0x3f088888
glabel var7f1accfc
.word 0x40c907a9
glabel var7f1acd00
.word 0x3ec907a9
glabel var7f1acd04
.word 0x40c907a9
glabel var7f1acd08
.word 0x3ec907a9
glabel var7f1acd0c
.word 0x40c907a9
glabel var7f1acd10
.word 0x3ec907a9
glabel var7f1acd14
.word 0x493ebc20
glabel var7f1acd18
.word 0x3e8e38e4
glabel var7f1acd1c
.word 0x3ed55555
glabel var7f1acd20
.word 0x40555555
glabel var7f1acd24
.word 0x3fb55555
glabel var7f1acd28
.word 0x3fd55555
glabel var7f1acd2c
.word 0x40c907a9
glabel var7f1acd30
.word 0x3dc907a9
glabel var7f1acd34
.word 0x40c907a9
glabel var7f1acd38
.word 0x3dc907a9
glabel var7f1acd3c
.word 0x40c907a9
glabel var7f1acd40
.word 0x3dc907a9
glabel var7f1acd44
.word 0x40c907a9
glabel var7f1acd48
.word 0x3dc907a9
glabel var7f1acd4c
.word 0x40c907a9
glabel var7f1acd50
.word 0x3dc907a9
glabel var7f1acd54
.word 0x40c907a9
glabel var7f1acd58
.word 0x3dc907a9
glabel var7f1acd5c
.word 0x493ebc20
glabel var7f1acd60
.word 0x3e8e38e4
.text
/*  f0ade00:	27bdfec0 */ 	addiu	$sp,$sp,-320
/*  f0ade04:	3c0e800a */ 	lui	$t6,%hi(g_Vars+0x284)
/*  f0ade08:	8dcea244 */ 	lw	$t6,%lo(g_Vars+0x284)($t6)
/*  f0ade0c:	afbf001c */ 	sw	$ra,0x1c($sp)
/*  f0ade10:	afb00018 */ 	sw	$s0,0x18($sp)
/*  f0ade14:	afa40140 */ 	sw	$a0,0x140($sp)
/*  f0ade18:	afa50144 */ 	sw	$a1,0x144($sp)
/*  f0ade1c:	afa60148 */ 	sw	$a2,0x148($sp)
/*  f0ade20:	00008025 */ 	or	$s0,$zero,$zero
/*  f0ade24:	0fc2866a */ 	jal	bgunGetWeaponNum
/*  f0ade28:	afae0134 */ 	sw	$t6,0x134($sp)
/*  f0ade2c:	8fb90140 */ 	lw	$t9,0x140($sp)
/*  f0ade30:	8fb80134 */ 	lw	$t8,0x134($sp)
/*  f0ade34:	240fffff */ 	addiu	$t7,$zero,-1
/*  f0ade38:	00194900 */ 	sll	$t1,$t9,0x4
/*  f0ade3c:	01394823 */ 	subu	$t1,$t1,$t9
/*  f0ade40:	00094880 */ 	sll	$t1,$t1,0x2
/*  f0ade44:	01394821 */ 	addu	$t1,$t1,$t9
/*  f0ade48:	000948c0 */ 	sll	$t1,$t1,0x3
/*  f0ade4c:	01394821 */ 	addu	$t1,$t1,$t9
/*  f0ade50:	00094880 */ 	sll	$t1,$t1,0x2
/*  f0ade54:	03091821 */ 	addu	$v1,$t8,$t1
/*  f0ade58:	afa200c4 */ 	sw	$v0,0xc4($sp)
/*  f0ade5c:	afaf00c0 */ 	sw	$t7,0xc0($sp)
/*  f0ade60:	24640638 */ 	addiu	$a0,$v1,0x638
/*  f0ade64:	0fc2c40f */ 	jal	gsetGetWeaponFunction2
/*  f0ade68:	afa30030 */ 	sw	$v1,0x30($sp)
/*  f0ade6c:	8faa0134 */ 	lw	$t2,0x134($sp)
/*  f0ade70:	81441580 */ 	lb	$a0,0x1580($t2)
/*  f0ade74:	0fc2c3f4 */ 	jal	weaponFindById
/*  f0ade78:	afa200bc */ 	sw	$v0,0xbc($sp)
/*  f0ade7c:	8fa300bc */ 	lw	$v1,0xbc($sp)
/*  f0ade80:	24010001 */ 	addiu	$at,$zero,0x1
/*  f0ade84:	00002025 */ 	or	$a0,$zero,$zero
/*  f0ade88:	8c6b0000 */ 	lw	$t3,0x0($v1)
/*  f0ade8c:	316c00ff */ 	andi	$t4,$t3,0xff
/*  f0ade90:	55810003 */ 	bnel	$t4,$at,.L0f0adea0
/*  f0ade94:	80650007 */ 	lb	$a1,0x7($v1)
/*  f0ade98:	00602025 */ 	or	$a0,$v1,$zero
/*  f0ade9c:	80650007 */ 	lb	$a1,0x7($v1)
.L0f0adea0:
/*  f0adea0:	04a202ac */ 	bltzl	$a1,.L0f0ae954
/*  f0adea4:	8fbf001c */ 	lw	$ra,0x1c($sp)
/*  f0adea8:	10400009 */ 	beqz	$v0,.L0f0aded0
/*  f0adeac:	3c06800a */ 	lui	$a2,%hi(g_Vars)
/*  f0adeb0:	10800007 */ 	beqz	$a0,.L0f0aded0
/*  f0adeb4:	00056880 */ 	sll	$t5,$a1,0x2
/*  f0adeb8:	004d7021 */ 	addu	$t6,$v0,$t5
/*  f0adebc:	8dc3001c */ 	lw	$v1,0x1c($t6)
/*  f0adec0:	50600004 */ 	beqzl	$v1,.L0f0aded4
/*  f0adec4:	8fb900c0 */ 	lw	$t9,0xc0($sp)
/*  f0adec8:	8c6f0004 */ 	lw	$t7,0x4($v1)
/*  f0adecc:	afaf00c0 */ 	sw	$t7,0xc0($sp)
.L0f0aded0:
/*  f0aded0:	8fb900c0 */ 	lw	$t9,0xc0($sp)
.L0f0aded4:
/*  f0aded4:	24c69fc0 */ 	addiu	$a2,$a2,%lo(g_Vars)
/*  f0aded8:	0722029e */ 	bltzl	$t9,.L0f0ae954
/*  f0adedc:	8fbf001c */ 	lw	$ra,0x1c($sp)
/*  f0adee0:	8cd8006c */ 	lw	$t8,0x6c($a2)
/*  f0adee4:	00002825 */ 	or	$a1,$zero,$zero
/*  f0adee8:	00001825 */ 	or	$v1,$zero,$zero
/*  f0adeec:	13000003 */ 	beqz	$t8,.L0f0adefc
/*  f0adef0:	00002025 */ 	or	$a0,$zero,$zero
/*  f0adef4:	10000001 */ 	b	.L0f0adefc
/*  f0adef8:	24050001 */ 	addiu	$a1,$zero,0x1
.L0f0adefc:
/*  f0adefc:	8cc90068 */ 	lw	$t1,0x68($a2)
/*  f0adf00:	00001025 */ 	or	$v0,$zero,$zero
/*  f0adf04:	11200003 */ 	beqz	$t1,.L0f0adf14
/*  f0adf08:	00000000 */ 	nop
/*  f0adf0c:	10000001 */ 	b	.L0f0adf14
/*  f0adf10:	24030001 */ 	addiu	$v1,$zero,0x1
.L0f0adf14:
/*  f0adf14:	8cca0064 */ 	lw	$t2,0x64($a2)
/*  f0adf18:	11400003 */ 	beqz	$t2,.L0f0adf28
/*  f0adf1c:	00000000 */ 	nop
/*  f0adf20:	10000001 */ 	b	.L0f0adf28
/*  f0adf24:	24040001 */ 	addiu	$a0,$zero,0x1
.L0f0adf28:
/*  f0adf28:	8ccb0070 */ 	lw	$t3,0x70($a2)
/*  f0adf2c:	11600003 */ 	beqz	$t3,.L0f0adf3c
/*  f0adf30:	00000000 */ 	nop
/*  f0adf34:	10000001 */ 	b	.L0f0adf3c
/*  f0adf38:	24020001 */ 	addiu	$v0,$zero,0x1
.L0f0adf3c:
/*  f0adf3c:	00446021 */ 	addu	$t4,$v0,$a0
/*  f0adf40:	01836821 */ 	addu	$t5,$t4,$v1
/*  f0adf44:	01a57021 */ 	addu	$t6,$t5,$a1
/*  f0adf48:	29c10002 */ 	slti	$at,$t6,0x2
/*  f0adf4c:	10200280 */ 	beqz	$at,.L0f0ae950
/*  f0adf50:	8fa40148 */ 	lw	$a0,0x148($sp)
/*  f0adf54:	0c005746 */ 	jal	mtx4Copy
/*  f0adf58:	27a500ec */ 	addiu	$a1,$sp,0xec
/*  f0adf5c:	0fc27aef */ 	jal	bgun0f09ebbc
/*  f0adf60:	00000000 */ 	nop
/*  f0adf64:	10400004 */ 	beqz	$v0,.L0f0adf78
/*  f0adf68:	00402025 */ 	or	$a0,$v0,$zero
/*  f0adf6c:	0fc2b732 */ 	jal	casingCreate
/*  f0adf70:	27a500ec */ 	addiu	$a1,$sp,0xec
/*  f0adf74:	00408025 */ 	or	$s0,$v0,$zero
.L0f0adf78:
/*  f0adf78:	12000275 */ 	beqz	$s0,.L0f0ae950
/*  f0adf7c:	8fa200c4 */ 	lw	$v0,0xc4($sp)
/*  f0adf80:	3c198007 */ 	lui	$t9,%hi(var80070528)
/*  f0adf84:	27390528 */ 	addiu	$t9,$t9,%lo(var80070528)
/*  f0adf88:	8f210000 */ 	lw	$at,0x0($t9)
/*  f0adf8c:	27af00a4 */ 	addiu	$t7,$sp,0xa4
/*  f0adf90:	8f290004 */ 	lw	$t1,0x4($t9)
/*  f0adf94:	ade10000 */ 	sw	$at,0x0($t7)
/*  f0adf98:	8f210008 */ 	lw	$at,0x8($t9)
/*  f0adf9c:	ade90004 */ 	sw	$t1,0x4($t7)
/*  f0adfa0:	ade10008 */ 	sw	$at,0x8($t7)
/*  f0adfa4:	c7a40144 */ 	lwc1	$f4,0x144($sp)
/*  f0adfa8:	24010024 */ 	addiu	$at,$zero,0x24
/*  f0adfac:	10410008 */ 	beq	$v0,$at,.L0f0adfd0
/*  f0adfb0:	e6040000 */ 	swc1	$f4,0x0($s0)
/*  f0adfb4:	24010025 */ 	addiu	$at,$zero,0x25
/*  f0adfb8:	10410005 */ 	beq	$v0,$at,.L0f0adfd0
/*  f0adfbc:	24010002 */ 	addiu	$at,$zero,0x2
/*  f0adfc0:	10410003 */ 	beq	$v0,$at,.L0f0adfd0
/*  f0adfc4:	24010005 */ 	addiu	$at,$zero,0x5
/*  f0adfc8:	544100ee */ 	bnel	$v0,$at,.L0f0ae384
/*  f0adfcc:	24010014 */ 	addiu	$at,$zero,0x14
.L0f0adfd0:
/*  f0adfd0:	0c004b70 */ 	jal	random
/*  f0adfd4:	00000000 */ 	nop
/*  f0adfd8:	44823000 */ 	mtc1	$v0,$f6
/*  f0adfdc:	3c017f1b */ 	lui	$at,%hi(var7f1accf8)
/*  f0adfe0:	c420ccf8 */ 	lwc1	$f0,%lo(var7f1accf8)($at)
/*  f0adfe4:	04410005 */ 	bgez	$v0,.L0f0adffc
/*  f0adfe8:	46803220 */ 	cvt.s.w	$f8,$f6
/*  f0adfec:	3c014f80 */ 	lui	$at,0x4f80
/*  f0adff0:	44815000 */ 	mtc1	$at,$f10
/*  f0adff4:	00000000 */ 	nop
/*  f0adff8:	460a4200 */ 	add.s	$f8,$f8,$f10
.L0f0adffc:
/*  f0adffc:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae000:	44819000 */ 	mtc1	$at,$f18
/*  f0ae004:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae008:	44815000 */ 	mtc1	$at,$f10
/*  f0ae00c:	46124102 */ 	mul.s	$f4,$f8,$f18
/*  f0ae010:	00000000 */ 	nop
/*  f0ae014:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0ae018:	00000000 */ 	nop
/*  f0ae01c:	460a3202 */ 	mul.s	$f8,$f6,$f10
/*  f0ae020:	46004480 */ 	add.s	$f18,$f8,$f0
/*  f0ae024:	46009107 */ 	neg.s	$f4,$f18
/*  f0ae028:	0c004b70 */ 	jal	random
/*  f0ae02c:	e6040010 */ 	swc1	$f4,0x10($s0)
/*  f0ae030:	44823000 */ 	mtc1	$v0,$f6
/*  f0ae034:	3c014020 */ 	lui	$at,0x4020
/*  f0ae038:	44810000 */ 	mtc1	$at,$f0
/*  f0ae03c:	44801000 */ 	mtc1	$zero,$f2
/*  f0ae040:	04410005 */ 	bgez	$v0,.L0f0ae058
/*  f0ae044:	468032a0 */ 	cvt.s.w	$f10,$f6
/*  f0ae048:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae04c:	44814000 */ 	mtc1	$at,$f8
/*  f0ae050:	00000000 */ 	nop
/*  f0ae054:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae058:
/*  f0ae058:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae05c:	44819000 */ 	mtc1	$at,$f18
/*  f0ae060:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae064:	44814000 */ 	mtc1	$at,$f8
/*  f0ae068:	46125102 */ 	mul.s	$f4,$f10,$f18
/*  f0ae06c:	e6020018 */ 	swc1	$f2,0x18($s0)
/*  f0ae070:	26050010 */ 	addiu	$a1,$s0,0x10
/*  f0ae074:	46002182 */ 	mul.s	$f6,$f4,$f0
/*  f0ae078:	00000000 */ 	nop
/*  f0ae07c:	46083282 */ 	mul.s	$f10,$f6,$f8
/*  f0ae080:	46005480 */ 	add.s	$f18,$f10,$f0
/*  f0ae084:	e6120014 */ 	swc1	$f18,0x14($s0)
/*  f0ae088:	0c0056c4 */ 	jal	mtx4RotateVecInPlace
/*  f0ae08c:	8fa40148 */ 	lw	$a0,0x148($sp)
/*  f0ae090:	0c004b70 */ 	jal	random
/*  f0ae094:	00000000 */ 	nop
/*  f0ae098:	44822000 */ 	mtc1	$v0,$f4
/*  f0ae09c:	04410005 */ 	bgez	$v0,.L0f0ae0b4
/*  f0ae0a0:	468021a0 */ 	cvt.s.w	$f6,$f4
/*  f0ae0a4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae0a8:	44814000 */ 	mtc1	$at,$f8
/*  f0ae0ac:	00000000 */ 	nop
/*  f0ae0b0:	46083180 */ 	add.s	$f6,$f6,$f8
.L0f0ae0b4:
/*  f0ae0b4:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae0b8:	44815000 */ 	mtc1	$at,$f10
/*  f0ae0bc:	3c017f1b */ 	lui	$at,%hi(var7f1accfc)
/*  f0ae0c0:	c424ccfc */ 	lwc1	$f4,%lo(var7f1accfc)($at)
/*  f0ae0c4:	460a3002 */ 	mul.s	$f0,$f6,$f10
/*  f0ae0c8:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae0cc:	44813000 */ 	mtc1	$at,$f6
/*  f0ae0d0:	3c017f1b */ 	lui	$at,%hi(var7f1acd00)
/*  f0ae0d4:	46000480 */ 	add.s	$f18,$f0,$f0
/*  f0ae0d8:	46049202 */ 	mul.s	$f8,$f18,$f4
/*  f0ae0dc:	c432cd00 */ 	lwc1	$f18,%lo(var7f1acd00)($at)
/*  f0ae0e0:	46064282 */ 	mul.s	$f10,$f8,$f6
/*  f0ae0e4:	46125101 */ 	sub.s	$f4,$f10,$f18
/*  f0ae0e8:	0c004b70 */ 	jal	random
/*  f0ae0ec:	e7a400a4 */ 	swc1	$f4,0xa4($sp)
/*  f0ae0f0:	44824000 */ 	mtc1	$v0,$f8
/*  f0ae0f4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae0f8:	04410004 */ 	bgez	$v0,.L0f0ae10c
/*  f0ae0fc:	468041a0 */ 	cvt.s.w	$f6,$f8
/*  f0ae100:	44815000 */ 	mtc1	$at,$f10
/*  f0ae104:	00000000 */ 	nop
/*  f0ae108:	460a3180 */ 	add.s	$f6,$f6,$f10
.L0f0ae10c:
/*  f0ae10c:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae110:	44819000 */ 	mtc1	$at,$f18
/*  f0ae114:	3c017f1b */ 	lui	$at,%hi(var7f1acd04)
/*  f0ae118:	c428cd04 */ 	lwc1	$f8,%lo(var7f1acd04)($at)
/*  f0ae11c:	46123002 */ 	mul.s	$f0,$f6,$f18
/*  f0ae120:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae124:	44813000 */ 	mtc1	$at,$f6
/*  f0ae128:	3c017f1b */ 	lui	$at,%hi(var7f1acd08)
/*  f0ae12c:	46000100 */ 	add.s	$f4,$f0,$f0
/*  f0ae130:	46082282 */ 	mul.s	$f10,$f4,$f8
/*  f0ae134:	c424cd08 */ 	lwc1	$f4,%lo(var7f1acd08)($at)
/*  f0ae138:	46065482 */ 	mul.s	$f18,$f10,$f6
/*  f0ae13c:	46049201 */ 	sub.s	$f8,$f18,$f4
/*  f0ae140:	0c004b70 */ 	jal	random
/*  f0ae144:	e7a800a8 */ 	swc1	$f8,0xa8($sp)
/*  f0ae148:	44825000 */ 	mtc1	$v0,$f10
/*  f0ae14c:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae150:	04410004 */ 	bgez	$v0,.L0f0ae164
/*  f0ae154:	468051a0 */ 	cvt.s.w	$f6,$f10
/*  f0ae158:	44819000 */ 	mtc1	$at,$f18
/*  f0ae15c:	00000000 */ 	nop
/*  f0ae160:	46123180 */ 	add.s	$f6,$f6,$f18
.L0f0ae164:
/*  f0ae164:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae168:	44812000 */ 	mtc1	$at,$f4
/*  f0ae16c:	3c017f1b */ 	lui	$at,%hi(var7f1acd0c)
/*  f0ae170:	c42acd0c */ 	lwc1	$f10,%lo(var7f1acd0c)($at)
/*  f0ae174:	46043002 */ 	mul.s	$f0,$f6,$f4
/*  f0ae178:	3c013d80 */ 	lui	$at,0x3d80
/*  f0ae17c:	44813000 */ 	mtc1	$at,$f6
/*  f0ae180:	3c017f1b */ 	lui	$at,%hi(var7f1acd10)
/*  f0ae184:	27a400a4 */ 	addiu	$a0,$sp,0xa4
/*  f0ae188:	27a50064 */ 	addiu	$a1,$sp,0x64
/*  f0ae18c:	46000200 */ 	add.s	$f8,$f0,$f0
/*  f0ae190:	460a4482 */ 	mul.s	$f18,$f8,$f10
/*  f0ae194:	c428cd10 */ 	lwc1	$f8,%lo(var7f1acd10)($at)
/*  f0ae198:	46069102 */ 	mul.s	$f4,$f18,$f6
/*  f0ae19c:	46082281 */ 	sub.s	$f10,$f4,$f8
/*  f0ae1a0:	0c005923 */ 	jal	mtx4LoadRotation
/*  f0ae1a4:	e7aa00ac */ 	swc1	$f10,0xac($sp)
/*  f0ae1a8:	27a40064 */ 	addiu	$a0,$sp,0x64
/*  f0ae1ac:	0c005768 */ 	jal	mtx4ToMtx3
/*  f0ae1b0:	27a500c8 */ 	addiu	$a1,$sp,0xc8
/*  f0ae1b4:	3c014580 */ 	lui	$at,0x4580
/*  f0ae1b8:	44810000 */ 	mtc1	$at,$f0
/*  f0ae1bc:	02003825 */ 	or	$a3,$s0,$zero
/*  f0ae1c0:	27a600c8 */ 	addiu	$a2,$sp,0xc8
/*  f0ae1c4:	27a800ec */ 	addiu	$t0,$sp,0xec
/*  f0ae1c8:	24050003 */ 	addiu	$a1,$zero,0x3
.L0f0ae1cc:
/*  f0ae1cc:	00001025 */ 	or	$v0,$zero,$zero
/*  f0ae1d0:	00e01825 */ 	or	$v1,$a3,$zero
/*  f0ae1d4:	00c02025 */ 	or	$a0,$a2,$zero
/*  f0ae1d8:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae1dc:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae1e0:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae1e4:	5045000d */ 	beql	$v0,$a1,.L0f0ae21c
/*  f0ae1e8:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae1ec:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae1f0:
/*  f0ae1f0:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae1f4:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae1f8:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae1fc:	440b8000 */ 	mfc1	$t3,$f16
/*  f0ae200:	00000000 */ 	nop
/*  f0ae204:	a46b002c */ 	sh	$t3,0x2c($v1)
/*  f0ae208:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae20c:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae210:	5445fff7 */ 	bnel	$v0,$a1,.L0f0ae1f0
/*  f0ae214:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae218:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae21c:
/*  f0ae21c:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae220:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae224:	440b8000 */ 	mfc1	$t3,$f16
/*  f0ae228:	00000000 */ 	nop
/*  f0ae22c:	a46b002c */ 	sh	$t3,0x2c($v1)
/*  f0ae230:	24c6000c */ 	addiu	$a2,$a2,0xc
/*  f0ae234:	14c8ffe5 */ 	bne	$a2,$t0,.L0f0ae1cc
/*  f0ae238:	24e70006 */ 	addiu	$a3,$a3,0x6
/*  f0ae23c:	0c004b70 */ 	jal	random
/*  f0ae240:	00000000 */ 	nop
/*  f0ae244:	3c030015 */ 	lui	$v1,0x15
/*  f0ae248:	3463aca6 */ 	ori	$v1,$v1,0xaca6
/*  f0ae24c:	00026602 */ 	srl	$t4,$v0,0x18
/*  f0ae250:	01830019 */ 	multu	$t4,$v1
/*  f0ae254:	00006812 */ 	mflo	$t5
/*  f0ae258:	000d7283 */ 	sra	$t6,$t5,0xa
/*  f0ae25c:	01c3c021 */ 	addu	$t8,$t6,$v1
/*  f0ae260:	0c004b70 */ 	jal	random
/*  f0ae264:	afb8005c */ 	sw	$t8,0x5c($sp)
/*  f0ae268:	8faf005c */ 	lw	$t7,0x5c($sp)
/*  f0ae26c:	3c03800a */ 	lui	$v1,%hi(g_Vars)
/*  f0ae270:	24639fc0 */ 	addiu	$v1,$v1,%lo(g_Vars)
/*  f0ae274:	004f001b */ 	divu	$zero,$v0,$t7
/*  f0ae278:	0000c810 */ 	mfhi	$t9
/*  f0ae27c:	44994000 */ 	mtc1	$t9,$f8
/*  f0ae280:	c60c0014 */ 	lwc1	$f12,0x14($s0)
/*  f0ae284:	15e00002 */ 	bnez	$t7,.L0f0ae290
/*  f0ae288:	00000000 */ 	nop
/*  f0ae28c:	0007000d */ 	break	0x7
.L0f0ae290:
/*  f0ae290:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae294:	07210004 */ 	bgez	$t9,.L0f0ae2a8
/*  f0ae298:	468042a0 */ 	cvt.s.w	$f10,$f8
/*  f0ae29c:	44819000 */ 	mtc1	$at,$f18
/*  f0ae2a0:	00000000 */ 	nop
/*  f0ae2a4:	46125280 */ 	add.s	$f10,$f10,$f18
.L0f0ae2a8:
/*  f0ae2a8:	3c017f1b */ 	lui	$at,%hi(var7f1acd14)
/*  f0ae2ac:	c426cd14 */ 	lwc1	$f6,%lo(var7f1acd14)($at)
/*  f0ae2b0:	3c017f1b */ 	lui	$at,%hi(var7f1acd18)
/*  f0ae2b4:	c424cd18 */ 	lwc1	$f4,%lo(var7f1acd18)($at)
/*  f0ae2b8:	46065003 */ 	div.s	$f0,$f10,$f6
/*  f0ae2bc:	3c013f00 */ 	lui	$at,0x3f00
/*  f0ae2c0:	44813000 */ 	mtc1	$at,$f6
/*  f0ae2c4:	c60e0010 */ 	lwc1	$f14,0x10($s0)
/*  f0ae2c8:	c6100018 */ 	lwc1	$f16,0x18($s0)
/*  f0ae2cc:	46040202 */ 	mul.s	$f8,$f0,$f4
/*  f0ae2d0:	46086081 */ 	sub.s	$f2,$f12,$f8
/*  f0ae2d4:	c6080008 */ 	lwc1	$f8,0x8($s0)
/*  f0ae2d8:	46026480 */ 	add.s	$f18,$f12,$f2
/*  f0ae2dc:	e6020014 */ 	swc1	$f2,0x14($s0)
/*  f0ae2e0:	46120282 */ 	mul.s	$f10,$f0,$f18
/*  f0ae2e4:	00000000 */ 	nop
/*  f0ae2e8:	46065102 */ 	mul.s	$f4,$f10,$f6
/*  f0ae2ec:	c60a0004 */ 	lwc1	$f10,0x4($s0)
/*  f0ae2f0:	460e0182 */ 	mul.s	$f6,$f0,$f14
/*  f0ae2f4:	46044480 */ 	add.s	$f18,$f8,$f4
/*  f0ae2f8:	c604000c */ 	lwc1	$f4,0xc($s0)
/*  f0ae2fc:	46065200 */ 	add.s	$f8,$f10,$f6
/*  f0ae300:	e6120008 */ 	swc1	$f18,0x8($s0)
/*  f0ae304:	46100482 */ 	mul.s	$f18,$f0,$f16
/*  f0ae308:	e6080004 */ 	swc1	$f8,0x4($s0)
/*  f0ae30c:	46122280 */ 	add.s	$f10,$f4,$f18
/*  f0ae310:	e60a000c */ 	swc1	$f10,0xc($s0)
/*  f0ae314:	8c690034 */ 	lw	$t1,0x34($v1)
/*  f0ae318:	8fa20030 */ 	lw	$v0,0x30($sp)
/*  f0ae31c:	1920018c */ 	blez	$t1,.L0f0ae950
/*  f0ae320:	00000000 */ 	nop
/*  f0ae324:	c446095c */ 	lwc1	$f6,0x95c($v0)
/*  f0ae328:	c448099c */ 	lwc1	$f8,0x99c($v0)
/*  f0ae32c:	c472004c */ 	lwc1	$f18,0x4c($v1)
/*  f0ae330:	46083101 */ 	sub.s	$f4,$f6,$f8
/*  f0ae334:	46122283 */ 	div.s	$f10,$f4,$f18
/*  f0ae338:	460a7180 */ 	add.s	$f6,$f14,$f10
/*  f0ae33c:	e6060010 */ 	swc1	$f6,0x10($s0)
/*  f0ae340:	c44409a0 */ 	lwc1	$f4,0x9a0($v0)
/*  f0ae344:	c4480960 */ 	lwc1	$f8,0x960($v0)
/*  f0ae348:	c46a004c */ 	lwc1	$f10,0x4c($v1)
/*  f0ae34c:	46044481 */ 	sub.s	$f18,$f8,$f4
/*  f0ae350:	c6080014 */ 	lwc1	$f8,0x14($s0)
/*  f0ae354:	460a9183 */ 	div.s	$f6,$f18,$f10
/*  f0ae358:	46064100 */ 	add.s	$f4,$f8,$f6
/*  f0ae35c:	e6040014 */ 	swc1	$f4,0x14($s0)
/*  f0ae360:	c44a09a4 */ 	lwc1	$f10,0x9a4($v0)
/*  f0ae364:	c4520964 */ 	lwc1	$f18,0x964($v0)
/*  f0ae368:	c466004c */ 	lwc1	$f6,0x4c($v1)
/*  f0ae36c:	460a9201 */ 	sub.s	$f8,$f18,$f10
/*  f0ae370:	46064103 */ 	div.s	$f4,$f8,$f6
/*  f0ae374:	46048480 */ 	add.s	$f18,$f16,$f4
/*  f0ae378:	10000175 */ 	b	.L0f0ae950
/*  f0ae37c:	e6120018 */ 	swc1	$f18,0x18($s0)
/*  f0ae380:	24010014 */ 	addiu	$at,$zero,0x14
.L0f0ae384:
/*  f0ae384:	1441002e */ 	bne	$v0,$at,.L0f0ae440
/*  f0ae388:	00000000 */ 	nop
/*  f0ae38c:	0c004b70 */ 	jal	random
/*  f0ae390:	00000000 */ 	nop
/*  f0ae394:	44825000 */ 	mtc1	$v0,$f10
/*  f0ae398:	3c017f1b */ 	lui	$at,%hi(var7f1acd1c)
/*  f0ae39c:	c420cd1c */ 	lwc1	$f0,%lo(var7f1acd1c)($at)
/*  f0ae3a0:	04410005 */ 	bgez	$v0,.L0f0ae3b8
/*  f0ae3a4:	46805220 */ 	cvt.s.w	$f8,$f10
/*  f0ae3a8:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae3ac:	44813000 */ 	mtc1	$at,$f6
/*  f0ae3b0:	00000000 */ 	nop
/*  f0ae3b4:	46064200 */ 	add.s	$f8,$f8,$f6
.L0f0ae3b8:
/*  f0ae3b8:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae3bc:	44812000 */ 	mtc1	$at,$f4
/*  f0ae3c0:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae3c4:	44813000 */ 	mtc1	$at,$f6
/*  f0ae3c8:	46044482 */ 	mul.s	$f18,$f8,$f4
/*  f0ae3cc:	00000000 */ 	nop
/*  f0ae3d0:	46009282 */ 	mul.s	$f10,$f18,$f0
/*  f0ae3d4:	00000000 */ 	nop
/*  f0ae3d8:	46065202 */ 	mul.s	$f8,$f10,$f6
/*  f0ae3dc:	46004100 */ 	add.s	$f4,$f8,$f0
/*  f0ae3e0:	46002487 */ 	neg.s	$f18,$f4
/*  f0ae3e4:	0c004b70 */ 	jal	random
/*  f0ae3e8:	e6120010 */ 	swc1	$f18,0x10($s0)
/*  f0ae3ec:	44825000 */ 	mtc1	$v0,$f10
/*  f0ae3f0:	3c017f1b */ 	lui	$at,%hi(var7f1acd20)
/*  f0ae3f4:	c420cd20 */ 	lwc1	$f0,%lo(var7f1acd20)($at)
/*  f0ae3f8:	04410005 */ 	bgez	$v0,.L0f0ae410
/*  f0ae3fc:	468051a0 */ 	cvt.s.w	$f6,$f10
/*  f0ae400:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae404:	44814000 */ 	mtc1	$at,$f8
/*  f0ae408:	00000000 */ 	nop
/*  f0ae40c:	46083180 */ 	add.s	$f6,$f6,$f8
.L0f0ae410:
/*  f0ae410:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae414:	44812000 */ 	mtc1	$at,$f4
/*  f0ae418:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae41c:	44814000 */ 	mtc1	$at,$f8
/*  f0ae420:	46043482 */ 	mul.s	$f18,$f6,$f4
/*  f0ae424:	00000000 */ 	nop
/*  f0ae428:	46009282 */ 	mul.s	$f10,$f18,$f0
/*  f0ae42c:	00000000 */ 	nop
/*  f0ae430:	46085182 */ 	mul.s	$f6,$f10,$f8
/*  f0ae434:	46003100 */ 	add.s	$f4,$f6,$f0
/*  f0ae438:	1000002d */ 	b	.L0f0ae4f0
/*  f0ae43c:	e6040014 */ 	swc1	$f4,0x14($s0)
.L0f0ae440:
/*  f0ae440:	0c004b70 */ 	jal	random
/*  f0ae444:	00000000 */ 	nop
/*  f0ae448:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae44c:	3c017f1b */ 	lui	$at,%hi(var7f1acd24)
/*  f0ae450:	c420cd24 */ 	lwc1	$f0,%lo(var7f1acd24)($at)
/*  f0ae454:	04410005 */ 	bgez	$v0,.L0f0ae46c
/*  f0ae458:	468092a0 */ 	cvt.s.w	$f10,$f18
/*  f0ae45c:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae460:	44814000 */ 	mtc1	$at,$f8
/*  f0ae464:	00000000 */ 	nop
/*  f0ae468:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae46c:
/*  f0ae46c:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae470:	44813000 */ 	mtc1	$at,$f6
/*  f0ae474:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae478:	44814000 */ 	mtc1	$at,$f8
/*  f0ae47c:	46065102 */ 	mul.s	$f4,$f10,$f6
/*  f0ae480:	00000000 */ 	nop
/*  f0ae484:	46002482 */ 	mul.s	$f18,$f4,$f0
/*  f0ae488:	00000000 */ 	nop
/*  f0ae48c:	46089282 */ 	mul.s	$f10,$f18,$f8
/*  f0ae490:	46005180 */ 	add.s	$f6,$f10,$f0
/*  f0ae494:	46003107 */ 	neg.s	$f4,$f6
/*  f0ae498:	0c004b70 */ 	jal	random
/*  f0ae49c:	e6040010 */ 	swc1	$f4,0x10($s0)
/*  f0ae4a0:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae4a4:	3c017f1b */ 	lui	$at,%hi(var7f1acd28)
/*  f0ae4a8:	c420cd28 */ 	lwc1	$f0,%lo(var7f1acd28)($at)
/*  f0ae4ac:	04410005 */ 	bgez	$v0,.L0f0ae4c4
/*  f0ae4b0:	46809220 */ 	cvt.s.w	$f8,$f18
/*  f0ae4b4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae4b8:	44815000 */ 	mtc1	$at,$f10
/*  f0ae4bc:	00000000 */ 	nop
/*  f0ae4c0:	460a4200 */ 	add.s	$f8,$f8,$f10
.L0f0ae4c4:
/*  f0ae4c4:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae4c8:	44813000 */ 	mtc1	$at,$f6
/*  f0ae4cc:	3c013e00 */ 	lui	$at,0x3e00
/*  f0ae4d0:	44815000 */ 	mtc1	$at,$f10
/*  f0ae4d4:	46064102 */ 	mul.s	$f4,$f8,$f6
/*  f0ae4d8:	00000000 */ 	nop
/*  f0ae4dc:	46002482 */ 	mul.s	$f18,$f4,$f0
/*  f0ae4e0:	00000000 */ 	nop
/*  f0ae4e4:	460a9202 */ 	mul.s	$f8,$f18,$f10
/*  f0ae4e8:	46004180 */ 	add.s	$f6,$f8,$f0
/*  f0ae4ec:	e6060014 */ 	swc1	$f6,0x14($s0)
.L0f0ae4f0:
/*  f0ae4f0:	44801000 */ 	mtc1	$zero,$f2
/*  f0ae4f4:	24010008 */ 	addiu	$at,$zero,0x8
/*  f0ae4f8:	26050010 */ 	addiu	$a1,$s0,0x10
/*  f0ae4fc:	e6020018 */ 	swc1	$f2,0x18($s0)
/*  f0ae500:	8faa00c4 */ 	lw	$t2,0xc4($sp)
/*  f0ae504:	11410002 */ 	beq	$t2,$at,.L0f0ae510
/*  f0ae508:	24010009 */ 	addiu	$at,$zero,0x9
/*  f0ae50c:	15410005 */ 	bne	$t2,$at,.L0f0ae524
.L0f0ae510:
/*  f0ae510:	3c01bf80 */ 	lui	$at,0xbf80
/*  f0ae514:	44812000 */ 	mtc1	$at,$f4
/*  f0ae518:	e6020010 */ 	swc1	$f2,0x10($s0)
/*  f0ae51c:	e6020014 */ 	swc1	$f2,0x14($s0)
/*  f0ae520:	e6040018 */ 	swc1	$f4,0x18($s0)
.L0f0ae524:
/*  f0ae524:	8fa40148 */ 	lw	$a0,0x148($sp)
/*  f0ae528:	0c0056c4 */ 	jal	mtx4RotateVecInPlace
/*  f0ae52c:	afa5002c */ 	sw	$a1,0x2c($sp)
/*  f0ae530:	8fab00c4 */ 	lw	$t3,0xc4($sp)
/*  f0ae534:	24010014 */ 	addiu	$at,$zero,0x14
/*  f0ae538:	1561004a */ 	bne	$t3,$at,.L0f0ae664
/*  f0ae53c:	00000000 */ 	nop
/*  f0ae540:	0c004b70 */ 	jal	random
/*  f0ae544:	00000000 */ 	nop
/*  f0ae548:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae54c:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae550:	04410004 */ 	bgez	$v0,.L0f0ae564
/*  f0ae554:	468092a0 */ 	cvt.s.w	$f10,$f18
/*  f0ae558:	44814000 */ 	mtc1	$at,$f8
/*  f0ae55c:	00000000 */ 	nop
/*  f0ae560:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae564:
/*  f0ae564:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae568:	44813000 */ 	mtc1	$at,$f6
/*  f0ae56c:	3c017f1b */ 	lui	$at,%hi(var7f1acd2c)
/*  f0ae570:	c432cd2c */ 	lwc1	$f18,%lo(var7f1acd2c)($at)
/*  f0ae574:	46065002 */ 	mul.s	$f0,$f10,$f6
/*  f0ae578:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae57c:	44815000 */ 	mtc1	$at,$f10
/*  f0ae580:	3c017f1b */ 	lui	$at,%hi(var7f1acd30)
/*  f0ae584:	46000100 */ 	add.s	$f4,$f0,$f0
/*  f0ae588:	46122202 */ 	mul.s	$f8,$f4,$f18
/*  f0ae58c:	c424cd30 */ 	lwc1	$f4,%lo(var7f1acd30)($at)
/*  f0ae590:	460a4182 */ 	mul.s	$f6,$f8,$f10
/*  f0ae594:	46043481 */ 	sub.s	$f18,$f6,$f4
/*  f0ae598:	0c004b70 */ 	jal	random
/*  f0ae59c:	e7b200a4 */ 	swc1	$f18,0xa4($sp)
/*  f0ae5a0:	44824000 */ 	mtc1	$v0,$f8
/*  f0ae5a4:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae5a8:	04410004 */ 	bgez	$v0,.L0f0ae5bc
/*  f0ae5ac:	468042a0 */ 	cvt.s.w	$f10,$f8
/*  f0ae5b0:	44813000 */ 	mtc1	$at,$f6
/*  f0ae5b4:	00000000 */ 	nop
/*  f0ae5b8:	46065280 */ 	add.s	$f10,$f10,$f6
.L0f0ae5bc:
/*  f0ae5bc:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae5c0:	44812000 */ 	mtc1	$at,$f4
/*  f0ae5c4:	3c017f1b */ 	lui	$at,%hi(var7f1acd34)
/*  f0ae5c8:	c428cd34 */ 	lwc1	$f8,%lo(var7f1acd34)($at)
/*  f0ae5cc:	46045002 */ 	mul.s	$f0,$f10,$f4
/*  f0ae5d0:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae5d4:	44815000 */ 	mtc1	$at,$f10
/*  f0ae5d8:	3c017f1b */ 	lui	$at,%hi(var7f1acd38)
/*  f0ae5dc:	46000480 */ 	add.s	$f18,$f0,$f0
/*  f0ae5e0:	46089182 */ 	mul.s	$f6,$f18,$f8
/*  f0ae5e4:	c432cd38 */ 	lwc1	$f18,%lo(var7f1acd38)($at)
/*  f0ae5e8:	460a3102 */ 	mul.s	$f4,$f6,$f10
/*  f0ae5ec:	46122201 */ 	sub.s	$f8,$f4,$f18
/*  f0ae5f0:	0c004b70 */ 	jal	random
/*  f0ae5f4:	e7a800a8 */ 	swc1	$f8,0xa8($sp)
/*  f0ae5f8:	44823000 */ 	mtc1	$v0,$f6
/*  f0ae5fc:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae600:	04410004 */ 	bgez	$v0,.L0f0ae614
/*  f0ae604:	468032a0 */ 	cvt.s.w	$f10,$f6
/*  f0ae608:	44812000 */ 	mtc1	$at,$f4
/*  f0ae60c:	00000000 */ 	nop
/*  f0ae610:	46045280 */ 	add.s	$f10,$f10,$f4
.L0f0ae614:
/*  f0ae614:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae618:	44819000 */ 	mtc1	$at,$f18
/*  f0ae61c:	3c017f1b */ 	lui	$at,%hi(var7f1acd3c)
/*  f0ae620:	c426cd3c */ 	lwc1	$f6,%lo(var7f1acd3c)($at)
/*  f0ae624:	46125002 */ 	mul.s	$f0,$f10,$f18
/*  f0ae628:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae62c:	44815000 */ 	mtc1	$at,$f10
/*  f0ae630:	3c017f1b */ 	lui	$at,%hi(var7f1acd40)
/*  f0ae634:	27a400a4 */ 	addiu	$a0,$sp,0xa4
/*  f0ae638:	27a50064 */ 	addiu	$a1,$sp,0x64
/*  f0ae63c:	46000200 */ 	add.s	$f8,$f0,$f0
/*  f0ae640:	46064102 */ 	mul.s	$f4,$f8,$f6
/*  f0ae644:	c428cd40 */ 	lwc1	$f8,%lo(var7f1acd40)($at)
/*  f0ae648:	460a2482 */ 	mul.s	$f18,$f4,$f10
/*  f0ae64c:	46089181 */ 	sub.s	$f6,$f18,$f8
/*  f0ae650:	0c005923 */ 	jal	mtx4LoadRotation
/*  f0ae654:	e7a600ac */ 	swc1	$f6,0xac($sp)
/*  f0ae658:	27a40064 */ 	addiu	$a0,$sp,0x64
/*  f0ae65c:	0c0056c4 */ 	jal	mtx4RotateVecInPlace
/*  f0ae660:	8fa5002c */ 	lw	$a1,0x2c($sp)
.L0f0ae664:
/*  f0ae664:	0c004b70 */ 	jal	random
/*  f0ae668:	00000000 */ 	nop
/*  f0ae66c:	44822000 */ 	mtc1	$v0,$f4
/*  f0ae670:	04410005 */ 	bgez	$v0,.L0f0ae688
/*  f0ae674:	468022a0 */ 	cvt.s.w	$f10,$f4
/*  f0ae678:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae67c:	44819000 */ 	mtc1	$at,$f18
/*  f0ae680:	00000000 */ 	nop
/*  f0ae684:	46125280 */ 	add.s	$f10,$f10,$f18
.L0f0ae688:
/*  f0ae688:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae68c:	44814000 */ 	mtc1	$at,$f8
/*  f0ae690:	3c017f1b */ 	lui	$at,%hi(var7f1acd44)
/*  f0ae694:	c424cd44 */ 	lwc1	$f4,%lo(var7f1acd44)($at)
/*  f0ae698:	46085002 */ 	mul.s	$f0,$f10,$f8
/*  f0ae69c:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae6a0:	44815000 */ 	mtc1	$at,$f10
/*  f0ae6a4:	3c017f1b */ 	lui	$at,%hi(var7f1acd48)
/*  f0ae6a8:	46000180 */ 	add.s	$f6,$f0,$f0
/*  f0ae6ac:	46043482 */ 	mul.s	$f18,$f6,$f4
/*  f0ae6b0:	c426cd48 */ 	lwc1	$f6,%lo(var7f1acd48)($at)
/*  f0ae6b4:	460a9202 */ 	mul.s	$f8,$f18,$f10
/*  f0ae6b8:	46064101 */ 	sub.s	$f4,$f8,$f6
/*  f0ae6bc:	0c004b70 */ 	jal	random
/*  f0ae6c0:	e7a400a4 */ 	swc1	$f4,0xa4($sp)
/*  f0ae6c4:	44829000 */ 	mtc1	$v0,$f18
/*  f0ae6c8:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae6cc:	04410004 */ 	bgez	$v0,.L0f0ae6e0
/*  f0ae6d0:	468092a0 */ 	cvt.s.w	$f10,$f18
/*  f0ae6d4:	44814000 */ 	mtc1	$at,$f8
/*  f0ae6d8:	00000000 */ 	nop
/*  f0ae6dc:	46085280 */ 	add.s	$f10,$f10,$f8
.L0f0ae6e0:
/*  f0ae6e0:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae6e4:	44813000 */ 	mtc1	$at,$f6
/*  f0ae6e8:	3c017f1b */ 	lui	$at,%hi(var7f1acd4c)
/*  f0ae6ec:	c432cd4c */ 	lwc1	$f18,%lo(var7f1acd4c)($at)
/*  f0ae6f0:	46065002 */ 	mul.s	$f0,$f10,$f6
/*  f0ae6f4:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae6f8:	44815000 */ 	mtc1	$at,$f10
/*  f0ae6fc:	3c017f1b */ 	lui	$at,%hi(var7f1acd50)
/*  f0ae700:	46000100 */ 	add.s	$f4,$f0,$f0
/*  f0ae704:	46122202 */ 	mul.s	$f8,$f4,$f18
/*  f0ae708:	c424cd50 */ 	lwc1	$f4,%lo(var7f1acd50)($at)
/*  f0ae70c:	460a4182 */ 	mul.s	$f6,$f8,$f10
/*  f0ae710:	46043481 */ 	sub.s	$f18,$f6,$f4
/*  f0ae714:	0c004b70 */ 	jal	random
/*  f0ae718:	e7b200a8 */ 	swc1	$f18,0xa8($sp)
/*  f0ae71c:	44824000 */ 	mtc1	$v0,$f8
/*  f0ae720:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae724:	04410004 */ 	bgez	$v0,.L0f0ae738
/*  f0ae728:	468042a0 */ 	cvt.s.w	$f10,$f8
/*  f0ae72c:	44813000 */ 	mtc1	$at,$f6
/*  f0ae730:	00000000 */ 	nop
/*  f0ae734:	46065280 */ 	add.s	$f10,$f10,$f6
.L0f0ae738:
/*  f0ae738:	3c012f80 */ 	lui	$at,0x2f80
/*  f0ae73c:	44812000 */ 	mtc1	$at,$f4
/*  f0ae740:	3c017f1b */ 	lui	$at,%hi(var7f1acd54)
/*  f0ae744:	c428cd54 */ 	lwc1	$f8,%lo(var7f1acd54)($at)
/*  f0ae748:	46045002 */ 	mul.s	$f0,$f10,$f4
/*  f0ae74c:	3c013c80 */ 	lui	$at,0x3c80
/*  f0ae750:	44815000 */ 	mtc1	$at,$f10
/*  f0ae754:	3c017f1b */ 	lui	$at,%hi(var7f1acd58)
/*  f0ae758:	27a400a4 */ 	addiu	$a0,$sp,0xa4
/*  f0ae75c:	27a50064 */ 	addiu	$a1,$sp,0x64
/*  f0ae760:	46000480 */ 	add.s	$f18,$f0,$f0
/*  f0ae764:	46089182 */ 	mul.s	$f6,$f18,$f8
/*  f0ae768:	c432cd58 */ 	lwc1	$f18,%lo(var7f1acd58)($at)
/*  f0ae76c:	460a3102 */ 	mul.s	$f4,$f6,$f10
/*  f0ae770:	46122201 */ 	sub.s	$f8,$f4,$f18
/*  f0ae774:	0c005923 */ 	jal	mtx4LoadRotation
/*  f0ae778:	e7a800ac */ 	swc1	$f8,0xac($sp)
/*  f0ae77c:	27a40064 */ 	addiu	$a0,$sp,0x64
/*  f0ae780:	0c005768 */ 	jal	mtx4ToMtx3
/*  f0ae784:	27a500c8 */ 	addiu	$a1,$sp,0xc8
/*  f0ae788:	3c014580 */ 	lui	$at,0x4580
/*  f0ae78c:	44810000 */ 	mtc1	$at,$f0
/*  f0ae790:	02003825 */ 	or	$a3,$s0,$zero
/*  f0ae794:	27a600c8 */ 	addiu	$a2,$sp,0xc8
/*  f0ae798:	27a800ec */ 	addiu	$t0,$sp,0xec
/*  f0ae79c:	24050003 */ 	addiu	$a1,$zero,0x3
.L0f0ae7a0:
/*  f0ae7a0:	00001025 */ 	or	$v0,$zero,$zero
/*  f0ae7a4:	00e01825 */ 	or	$v1,$a3,$zero
/*  f0ae7a8:	00c02025 */ 	or	$a0,$a2,$zero
/*  f0ae7ac:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae7b0:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae7b4:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae7b8:	5045000d */ 	beql	$v0,$a1,.L0f0ae7f0
/*  f0ae7bc:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae7c0:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae7c4:
/*  f0ae7c4:	24420001 */ 	addiu	$v0,$v0,0x1
/*  f0ae7c8:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae7cc:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae7d0:	440d8000 */ 	mfc1	$t5,$f16
/*  f0ae7d4:	00000000 */ 	nop
/*  f0ae7d8:	a46d002c */ 	sh	$t5,0x2c($v1)
/*  f0ae7dc:	c4900000 */ 	lwc1	$f16,0x0($a0)
/*  f0ae7e0:	46008482 */ 	mul.s	$f18,$f16,$f0
/*  f0ae7e4:	5445fff7 */ 	bnel	$v0,$a1,.L0f0ae7c4
/*  f0ae7e8:	4600940d */ 	trunc.w.s	$f16,$f18
/*  f0ae7ec:	4600940d */ 	trunc.w.s	$f16,$f18
.L0f0ae7f0:
/*  f0ae7f0:	24630002 */ 	addiu	$v1,$v1,0x2
/*  f0ae7f4:	24840004 */ 	addiu	$a0,$a0,0x4
/*  f0ae7f8:	440d8000 */ 	mfc1	$t5,$f16
/*  f0ae7fc:	00000000 */ 	nop
/*  f0ae800:	a46d002c */ 	sh	$t5,0x2c($v1)
/*  f0ae804:	24c6000c */ 	addiu	$a2,$a2,0xc
/*  f0ae808:	14c8ffe5 */ 	bne	$a2,$t0,.L0f0ae7a0
/*  f0ae80c:	24e70006 */ 	addiu	$a3,$a3,0x6
/*  f0ae810:	0c004b70 */ 	jal	random
/*  f0ae814:	00000000 */ 	nop
/*  f0ae818:	3c030015 */ 	lui	$v1,0x15
/*  f0ae81c:	3463aca6 */ 	ori	$v1,$v1,0xaca6
/*  f0ae820:	00027602 */ 	srl	$t6,$v0,0x18
/*  f0ae824:	01c30019 */ 	multu	$t6,$v1
/*  f0ae828:	0000c012 */ 	mflo	$t8
/*  f0ae82c:	00187a83 */ 	sra	$t7,$t8,0xa
/*  f0ae830:	01e3c821 */ 	addu	$t9,$t7,$v1
/*  f0ae834:	0c004b70 */ 	jal	random
/*  f0ae838:	afb9004c */ 	sw	$t9,0x4c($sp)
/*  f0ae83c:	8fa9004c */ 	lw	$t1,0x4c($sp)
/*  f0ae840:	3c03800a */ 	lui	$v1,%hi(g_Vars)
/*  f0ae844:	24639fc0 */ 	addiu	$v1,$v1,%lo(g_Vars)
/*  f0ae848:	0049001b */ 	divu	$zero,$v0,$t1
/*  f0ae84c:	00005010 */ 	mfhi	$t2
/*  f0ae850:	448a9000 */ 	mtc1	$t2,$f18
/*  f0ae854:	c60c0014 */ 	lwc1	$f12,0x14($s0)
/*  f0ae858:	15200002 */ 	bnez	$t1,.L0f0ae864
/*  f0ae85c:	00000000 */ 	nop
/*  f0ae860:	0007000d */ 	break	0x7
.L0f0ae864:
/*  f0ae864:	3c014f80 */ 	lui	$at,0x4f80
/*  f0ae868:	05410004 */ 	bgez	$t2,.L0f0ae87c
/*  f0ae86c:	46809220 */ 	cvt.s.w	$f8,$f18
/*  f0ae870:	44813000 */ 	mtc1	$at,$f6
/*  f0ae874:	00000000 */ 	nop
/*  f0ae878:	46064200 */ 	add.s	$f8,$f8,$f6
.L0f0ae87c:
/*  f0ae87c:	3c017f1b */ 	lui	$at,%hi(var7f1acd5c)
/*  f0ae880:	c42acd5c */ 	lwc1	$f10,%lo(var7f1acd5c)($at)
/*  f0ae884:	3c017f1b */ 	lui	$at,%hi(var7f1acd60)
/*  f0ae888:	c424cd60 */ 	lwc1	$f4,%lo(var7f1acd60)($at)
/*  f0ae88c:	460a4003 */ 	div.s	$f0,$f8,$f10
/*  f0ae890:	3c013f00 */ 	lui	$at,0x3f00
/*  f0ae894:	44815000 */ 	mtc1	$at,$f10
/*  f0ae898:	c60e0010 */ 	lwc1	$f14,0x10($s0)
/*  f0ae89c:	c6100018 */ 	lwc1	$f16,0x18($s0)
/*  f0ae8a0:	46040482 */ 	mul.s	$f18,$f0,$f4
/*  f0ae8a4:	46126081 */ 	sub.s	$f2,$f12,$f18
/*  f0ae8a8:	c6120008 */ 	lwc1	$f18,0x8($s0)
/*  f0ae8ac:	46026180 */ 	add.s	$f6,$f12,$f2
/*  f0ae8b0:	e6020014 */ 	swc1	$f2,0x14($s0)
/*  f0ae8b4:	46060202 */ 	mul.s	$f8,$f0,$f6
/*  f0ae8b8:	00000000 */ 	nop
/*  f0ae8bc:	460a4102 */ 	mul.s	$f4,$f8,$f10
/*  f0ae8c0:	c6080004 */ 	lwc1	$f8,0x4($s0)
/*  f0ae8c4:	460e0282 */ 	mul.s	$f10,$f0,$f14
/*  f0ae8c8:	46049180 */ 	add.s	$f6,$f18,$f4
/*  f0ae8cc:	c604000c */ 	lwc1	$f4,0xc($s0)
/*  f0ae8d0:	460a4480 */ 	add.s	$f18,$f8,$f10
/*  f0ae8d4:	e6060008 */ 	swc1	$f6,0x8($s0)
/*  f0ae8d8:	46100182 */ 	mul.s	$f6,$f0,$f16
/*  f0ae8dc:	e6120004 */ 	swc1	$f18,0x4($s0)
/*  f0ae8e0:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0ae8e4:	e608000c */ 	swc1	$f8,0xc($s0)
/*  f0ae8e8:	8c6b0034 */ 	lw	$t3,0x34($v1)
/*  f0ae8ec:	8fa20030 */ 	lw	$v0,0x30($sp)
/*  f0ae8f0:	19600017 */ 	blez	$t3,.L0f0ae950
/*  f0ae8f4:	00000000 */ 	nop
/*  f0ae8f8:	c44a095c */ 	lwc1	$f10,0x95c($v0)
/*  f0ae8fc:	c452099c */ 	lwc1	$f18,0x99c($v0)
/*  f0ae900:	c466004c */ 	lwc1	$f6,0x4c($v1)
/*  f0ae904:	46125101 */ 	sub.s	$f4,$f10,$f18
/*  f0ae908:	46062203 */ 	div.s	$f8,$f4,$f6
/*  f0ae90c:	46087280 */ 	add.s	$f10,$f14,$f8
/*  f0ae910:	e60a0010 */ 	swc1	$f10,0x10($s0)
/*  f0ae914:	c44409a0 */ 	lwc1	$f4,0x9a0($v0)
/*  f0ae918:	c4520960 */ 	lwc1	$f18,0x960($v0)
/*  f0ae91c:	c468004c */ 	lwc1	$f8,0x4c($v1)
/*  f0ae920:	46049181 */ 	sub.s	$f6,$f18,$f4
/*  f0ae924:	c6120014 */ 	lwc1	$f18,0x14($s0)
/*  f0ae928:	46083283 */ 	div.s	$f10,$f6,$f8
/*  f0ae92c:	460a9100 */ 	add.s	$f4,$f18,$f10
/*  f0ae930:	e6040014 */ 	swc1	$f4,0x14($s0)
/*  f0ae934:	c44809a4 */ 	lwc1	$f8,0x9a4($v0)
/*  f0ae938:	c4460964 */ 	lwc1	$f6,0x964($v0)
/*  f0ae93c:	c46a004c */ 	lwc1	$f10,0x4c($v1)
/*  f0ae940:	46083481 */ 	sub.s	$f18,$f6,$f8
/*  f0ae944:	460a9103 */ 	div.s	$f4,$f18,$f10
/*  f0ae948:	46048180 */ 	add.s	$f6,$f16,$f4
/*  f0ae94c:	e6060018 */ 	swc1	$f6,0x18($s0)
.L0f0ae950:
/*  f0ae950:	8fbf001c */ 	lw	$ra,0x1c($sp)
.L0f0ae954:
/*  f0ae954:	8fb00018 */ 	lw	$s0,0x18($sp)
/*  f0ae958:	27bd0140 */ 	addiu	$sp,$sp,0x140
/*  f0ae95c:	03e00008 */ 	jr	$ra
/*  f0ae960:	00000000 */ 	nop
);
#endif

void casingRender(struct casing *casing, Gfx **gdlptr)
{
	Gfx *gdl = *gdlptr;
	struct modelfiledata *modeldef = casing->modeldef;
	Mtxf *matrices = gfxAllocate(modeldef->nummatrices * sizeof(Mtxf));
	struct model model;
	struct modelrenderdata renderdata = { NULL, true, 3 };
	Mtxf mtx;
	s32 i;
	s32 j;
	bool render = true;

	modelCalculateRwDataLen(modeldef);
	modelInit(&model, modeldef, NULL, true);

	model.matrices = matrices;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			mtx.m[i][j] = casing->rot[i][j] * (1.0f / 4096.0f);
		}
	}

	mtx.m[3][0] = 0.0f;
	mtx.m[3][1] = 0.0f;
	mtx.m[3][2] = 0.0f;

	mtx.m[0][3] = 0.0f;
	mtx.m[1][3] = 0.0f;
	mtx.m[2][3] = 0.0f;
	mtx.m[3][3] = 1.0f;

	mtx00015f04(0.1000000089407f, &mtx);
	mtx4SetTranslation(&casing->pos, &mtx);
	mtx00015be4(camGetMatrix1740(), &mtx, model.matrices);

	// Check if any coordinate is out of range
	for (i = 0; i < 3; i++) {
		if (model.matrices[0].m[3][i] > 30000) {
			render = false;
		} else if (model.matrices[0].m[3][i] < -30000) {
			render = false;
		}
	}

	if (render) {
		renderdata.zbufferenabled = 1;
		renderdata.gdl = gdl;
		renderdata.unk10 = matrices;
		renderdata.unk30 = 4;
		renderdata.envcolour = g_Vars.currentplayer->gunshadecol[0] << 24
			| g_Vars.currentplayer->gunshadecol[1] << 16
			| g_Vars.currentplayer->gunshadecol[2] << 8
			| g_Vars.currentplayer->gunshadecol[3];

		modelRender(&renderdata, &model);

		*gdlptr = renderdata.gdl;

		func0f0c33f0(matrices, modeldef->nummatrices);
	}
}

void casingsRender(Gfx **gdlptr)
{
	if (g_CasingsActive) {
		struct casing *end = g_Casings + ARRAYCOUNT(g_Casings);
		struct casing *casing = g_Casings;

		while (casing < end) {
			if (casing->modeldef) {
				casingRender(casing, gdlptr);
			}

			casing++;
		}
	}
}

s32 boltbeamFindByProp(struct prop *prop)
{
	s32 result = -1;
	s32 i = 0;

	for (; i < 8 && result == -1; i++) {
		if (g_BoltBeams[i].unk00_prop == prop) {
			result = i;
		}
	}

	return result;
}

s32 boltbeamCreate(struct prop *prop)
{
	s32 index = boltbeamFindByProp((struct prop *) -1);

	if (index >= 0) {
		g_BoltBeams[index].unk05 = 0;
		g_BoltBeams[index].unk00_prop = prop;
	}

	return index;
}

void boltbeamFree(struct prop *prop)
{
	s32 index = boltbeamFindByProp(prop);

	if (index != -1) {
		g_BoltBeams[index].unk00 = -1;
	}
}

void func0f0aed3c(s32 index, struct coord *coord)
{
	g_BoltBeams[index].unk14.x = coord->x;
	g_BoltBeams[index].unk14.y = coord->y;
	g_BoltBeams[index].unk14.z = coord->z;
}

void func0f0aed70(s32 index, struct coord *coord)
{
	g_BoltBeams[index].unk20.x = coord->x;
	g_BoltBeams[index].unk20.y = coord->y;
	g_BoltBeams[index].unk20.z = coord->z;
}

GLOBAL_ASM(
glabel func0f0aeda4
/*  f0aeda4:	00047080 */ 	sll	$t6,$a0,0x2
/*  f0aeda8:	01c47023 */ 	subu	$t6,$t6,$a0
/*  f0aedac:	3c0f800a */ 	lui	$t7,%hi(g_BoltBeams)
/*  f0aedb0:	25efda60 */ 	addiu	$t7,$t7,%lo(g_BoltBeams)
/*  f0aedb4:	000e7100 */ 	sll	$t6,$t6,0x4
/*  f0aedb8:	01cf1021 */ 	addu	$v0,$t6,$t7
/*  f0aedbc:	c4440028 */ 	lwc1	$f4,0x28($v0)
/*  f0aedc0:	c446001c */ 	lwc1	$f6,0x1c($v0)
/*  f0aedc4:	c4480020 */ 	lwc1	$f8,0x20($v0)
/*  f0aedc8:	c44a0014 */ 	lwc1	$f10,0x14($v0)
/*  f0aedcc:	46062001 */ 	sub.s	$f0,$f4,$f6
/*  f0aedd0:	c4460018 */ 	lwc1	$f6,0x18($v0)
/*  f0aedd4:	c4440024 */ 	lwc1	$f4,0x24($v0)
/*  f0aedd8:	460a4081 */ 	sub.s	$f2,$f8,$f10
/*  f0aeddc:	27bdffd0 */ 	addiu	$sp,$sp,-48
/*  f0aede0:	afbf0014 */ 	sw	$ra,0x14($sp)
/*  f0aede4:	46062381 */ 	sub.s	$f14,$f4,$f6
/*  f0aede8:	46021202 */ 	mul.s	$f8,$f2,$f2
/*  f0aedec:	afa50034 */ 	sw	$a1,0x34($sp)
/*  f0aedf0:	afa60038 */ 	sw	$a2,0x38($sp)
/*  f0aedf4:	460e7282 */ 	mul.s	$f10,$f14,$f14
/*  f0aedf8:	afa2001c */ 	sw	$v0,0x1c($sp)
/*  f0aedfc:	46000182 */ 	mul.s	$f6,$f0,$f0
/*  f0aee00:	460a4100 */ 	add.s	$f4,$f8,$f10
/*  f0aee04:	0c012974 */ 	jal	sqrtf
/*  f0aee08:	46043300 */ 	add.s	$f12,$f6,$f4
/*  f0aee0c:	c7a80034 */ 	lwc1	$f8,0x34($sp)
/*  f0aee10:	8fa2001c */ 	lw	$v0,0x1c($sp)
/*  f0aee14:	8fb80038 */ 	lw	$t8,0x38($sp)
/*  f0aee18:	4600403c */ 	c.lt.s	$f8,$f0
/*  f0aee1c:	00000000 */ 	nop
/*  f0aee20:	4502001e */ 	bc1fl	.L0f0aee9c
/*  f0aee24:	8fbf0014 */ 	lw	$ra,0x14($sp)
/*  f0aee28:	5700001c */ 	bnezl	$t8,.L0f0aee9c
/*  f0aee2c:	8fbf0014 */ 	lw	$ra,0x14($sp)
/*  f0aee30:	c44c0020 */ 	lwc1	$f12,0x20($v0)
/*  f0aee34:	c44a0014 */ 	lwc1	$f10,0x14($v0)
/*  f0aee38:	c4420028 */ 	lwc1	$f2,0x28($v0)
/*  f0aee3c:	c44e0024 */ 	lwc1	$f14,0x24($v0)
/*  f0aee40:	460c5181 */ 	sub.s	$f6,$f10,$f12
/*  f0aee44:	c44a001c */ 	lwc1	$f10,0x1c($v0)
/*  f0aee48:	c4440018 */ 	lwc1	$f4,0x18($v0)
/*  f0aee4c:	46003403 */ 	div.s	$f16,$f6,$f0
/*  f0aee50:	46025181 */ 	sub.s	$f6,$f10,$f2
/*  f0aee54:	460e2201 */ 	sub.s	$f8,$f4,$f14
/*  f0aee58:	46003103 */ 	div.s	$f4,$f6,$f0
/*  f0aee5c:	46004483 */ 	div.s	$f18,$f8,$f0
/*  f0aee60:	c7a80034 */ 	lwc1	$f8,0x34($sp)
/*  f0aee64:	e7a40028 */ 	swc1	$f4,0x28($sp)
/*  f0aee68:	c7a40034 */ 	lwc1	$f4,0x34($sp)
/*  f0aee6c:	46088282 */ 	mul.s	$f10,$f16,$f8
/*  f0aee70:	460c5180 */ 	add.s	$f6,$f10,$f12
/*  f0aee74:	46049202 */ 	mul.s	$f8,$f18,$f4
/*  f0aee78:	c7a40034 */ 	lwc1	$f4,0x34($sp)
/*  f0aee7c:	e4460014 */ 	swc1	$f6,0x14($v0)
/*  f0aee80:	c7a60028 */ 	lwc1	$f6,0x28($sp)
/*  f0aee84:	460e4280 */ 	add.s	$f10,$f8,$f14
/*  f0aee88:	46043202 */ 	mul.s	$f8,$f6,$f4
/*  f0aee8c:	e44a0018 */ 	swc1	$f10,0x18($v0)
/*  f0aee90:	46024280 */ 	add.s	$f10,$f8,$f2
/*  f0aee94:	e44a001c */ 	swc1	$f10,0x1c($v0)
/*  f0aee98:	8fbf0014 */ 	lw	$ra,0x14($sp)
.L0f0aee9c:
/*  f0aee9c:	27bd0030 */ 	addiu	$sp,$sp,0x30
/*  f0aeea0:	03e00008 */ 	jr	$ra
/*  f0aeea4:	00000000 */ 	nop
);

void func0f0aeea8(s32 index, f32 arg1)
{
	g_BoltBeams[index].unk05 = 1;
	g_BoltBeams[index].unk00 = 0;
	g_BoltBeams[index].unk2c = arg1;
}

Gfx *boltbeamsRender(Gfx *gdl)
{
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_BoltBeams); i++) {
		if (g_BoltBeams[i].unk00 != -1) {
			gdl = func0f0ac4b8(gdl, g_TexLaserConfigs, 1, &g_BoltBeams[i].unk14, 0xafafff00, 2, &g_BoltBeams[i].unk20, 0xafafff7f);
		}
	}

	return gdl;
}

void boltbeamsTick(void)
{
	s32 i;

	for (i = 0; i < 8; i++) {
		if (g_BoltBeams[i].unk00 != -1 && g_BoltBeams[i].unk05 == 1) {
			f32 value = sqrtf(
					(g_BoltBeams[i].unk20.f[0] - g_BoltBeams[i].unk14.f[0]) * (g_BoltBeams[i].unk20.f[0] - g_BoltBeams[i].unk14.f[0]) +
					(g_BoltBeams[i].unk20.f[1] - g_BoltBeams[i].unk14.f[1]) * (g_BoltBeams[i].unk20.f[1] - g_BoltBeams[i].unk14.f[1]) +
					(g_BoltBeams[i].unk20.f[2] - g_BoltBeams[i].unk14.f[2]) * (g_BoltBeams[i].unk20.f[2] - g_BoltBeams[i].unk14.f[2]));

			value -= g_BoltBeams[i].unk2c * LVUPDATE240FREAL() / 60.0f;

			if (value < 0) {
				g_BoltBeams[i].unk00 = -1;
			} else {
				func0f0aeda4(i, value, 0);
			}
		}
	}
}

/**
 * Return true if a lasersight with the given ID exists, or false if not.
 *
 * Additionally, populate the index pointer with the index of the lasersight
 * if it exists, or any free slot if it doesn't.
 */
bool lasersightExists(s32 id, s32 *index)
{
	s32 fallback = -1;
	s32 exact = -1;
	s32 i = 0;

	for (; i < 4 && exact == -1; i++) {
		if (g_LaserSights[i].id == id) {
			exact = i;
		}

		if (g_LaserSights[i].id == -1) {
			fallback = i;
		}
	}

	if (exact == -1) {
		*index = fallback;
		return false;
	}

	*index = exact;
	return true;
}

u32 var80070574 = 0x00000320;
u32 var80070578 = 0x00001b58;
u32 var8007057c = 0x00002328;
u32 var80070580 = 0x00000018;
u32 var80070584 = 0x00000006;
u32 var80070588 = 0x00000000;
u32 var8007058c = 0x00000000;

GLOBAL_ASM(
glabel lasersightRenderDot
.late_rodata
glabel var7f1acd70
.word 0x3e4ccccd
glabel var7f1acd74
.word 0xb58637bd
glabel var7f1acd78
.word 0x33d6bf95
glabel var7f1acd7c
.word 0x3dcccccd
glabel var7f1acd80
.word 0xb58637bd
glabel var7f1acd84
.word 0x33d6bf95
glabel var7f1acd88
.word 0xb58637bd
glabel var7f1acd8c
.word 0x33d6bf95
.text
/*  f0af158:	27bdfe00 */ 	addiu	$sp,$sp,-512
/*  f0af15c:	afb00058 */ 	sw	$s0,0x58($sp)
/*  f0af160:	afa40200 */ 	sw	$a0,0x200($sp)
/*  f0af164:	afbf0064 */ 	sw	$ra,0x64($sp)
/*  f0af168:	3c10800a */ 	lui	$s0,%hi(g_Vars+0x284)
/*  f0af16c:	3c047f1b */ 	lui	$a0,%hi(var7f1acc10)
/*  f0af170:	3c058007 */ 	lui	$a1,%hi(var80070574)
/*  f0af174:	afb20060 */ 	sw	$s2,0x60($sp)
/*  f0af178:	afb1005c */ 	sw	$s1,0x5c($sp)
/*  f0af17c:	f7be0050 */ 	sdc1	$f30,0x50($sp)
/*  f0af180:	f7bc0048 */ 	sdc1	$f28,0x48($sp)
/*  f0af184:	f7ba0040 */ 	sdc1	$f26,0x40($sp)
/*  f0af188:	f7b80038 */ 	sdc1	$f24,0x38($sp)
/*  f0af18c:	f7b60030 */ 	sdc1	$f22,0x30($sp)
/*  f0af190:	f7b40028 */ 	sdc1	$f20,0x28($sp)
/*  f0af194:	8e10a244 */ 	lw	$s0,%lo(g_Vars+0x284)($s0)
/*  f0af198:	24a50574 */ 	addiu	$a1,$a1,%lo(var80070574)
/*  f0af19c:	0c0036cc */ 	jal	mainOverrideVariable
/*  f0af1a0:	2484cc10 */ 	addiu	$a0,$a0,%lo(var7f1acc10)
/*  f0af1a4:	3c047f1b */ 	lui	$a0,%hi(var7f1acc14)
/*  f0af1a8:	3c058007 */ 	lui	$a1,%hi(var80070578)
/*  f0af1ac:	24a50578 */ 	addiu	$a1,$a1,%lo(var80070578)
/*  f0af1b0:	0c0036cc */ 	jal	mainOverrideVariable
/*  f0af1b4:	2484cc14 */ 	addiu	$a0,$a0,%lo(var7f1acc14)
/*  f0af1b8:	3c047f1b */ 	lui	$a0,%hi(var7f1acc18)
/*  f0af1bc:	3c058007 */ 	lui	$a1,%hi(var8007057c)
/*  f0af1c0:	24a5057c */ 	addiu	$a1,$a1,%lo(var8007057c)
/*  f0af1c4:	0c0036cc */ 	jal	mainOverrideVariable
/*  f0af1c8:	2484cc18 */ 	addiu	$a0,$a0,%lo(var7f1acc18)
/*  f0af1cc:	3c047f1b */ 	lui	$a0,%hi(var7f1acc1c)
/*  f0af1d0:	3c058007 */ 	lui	$a1,%hi(var80070580)
/*  f0af1d4:	24a50580 */ 	addiu	$a1,$a1,%lo(var80070580)
/*  f0af1d8:	0c0036cc */ 	jal	mainOverrideVariable
/*  f0af1dc:	2484cc1c */ 	addiu	$a0,$a0,%lo(var7f1acc1c)
/*  f0af1e0:	3c047f1b */ 	lui	$a0,%hi(var7f1acc20)
/*  f0af1e4:	3c058007 */ 	lui	$a1,%hi(var80070584)
/*  f0af1e8:	24a50584 */ 	addiu	$a1,$a1,%lo(var80070584)
/*  f0af1ec:	0c0036cc */ 	jal	mainOverrideVariable
/*  f0af1f0:	2484cc20 */ 	addiu	$a0,$a0,%lo(var7f1acc20)
/*  f0af1f4:	8fae0200 */ 	lw	$t6,0x200($sp)
/*  f0af1f8:	3c18ba00 */ 	lui	$t8,0xba00
/*  f0af1fc:	37181402 */ 	ori	$t8,$t8,0x1402
/*  f0af200:	25cf0008 */ 	addiu	$t7,$t6,0x8
/*  f0af204:	afaf0200 */ 	sw	$t7,0x200($sp)
/*  f0af208:	adc00004 */ 	sw	$zero,0x4($t6)
/*  f0af20c:	add80000 */ 	sw	$t8,0x0($t6)
/*  f0af210:	8fb90200 */ 	lw	$t9,0x200($sp)
/*  f0af214:	3c09ba00 */ 	lui	$t1,0xba00
/*  f0af218:	35290c02 */ 	ori	$t1,$t1,0xc02
/*  f0af21c:	27280008 */ 	addiu	$t0,$t9,0x8
/*  f0af220:	afa80200 */ 	sw	$t0,0x200($sp)
/*  f0af224:	240a2000 */ 	addiu	$t2,$zero,0x2000
/*  f0af228:	af2a0004 */ 	sw	$t2,0x4($t9)
/*  f0af22c:	af290000 */ 	sw	$t1,0x0($t9)
/*  f0af230:	8fab0200 */ 	lw	$t3,0x200($sp)
/*  f0af234:	3c0dba00 */ 	lui	$t5,0xba00
/*  f0af238:	35ad1301 */ 	ori	$t5,$t5,0x1301
/*  f0af23c:	256c0008 */ 	addiu	$t4,$t3,0x8
/*  f0af240:	afac0200 */ 	sw	$t4,0x200($sp)
/*  f0af244:	3c0e0008 */ 	lui	$t6,0x8
/*  f0af248:	ad6e0004 */ 	sw	$t6,0x4($t3)
/*  f0af24c:	ad6d0000 */ 	sw	$t5,0x0($t3)
/*  f0af250:	8faf0200 */ 	lw	$t7,0x200($sp)
/*  f0af254:	3c19ba00 */ 	lui	$t9,0xba00
/*  f0af258:	37390602 */ 	ori	$t9,$t9,0x602
/*  f0af25c:	25f80008 */ 	addiu	$t8,$t7,0x8
/*  f0af260:	afb80200 */ 	sw	$t8,0x200($sp)
/*  f0af264:	240800c0 */ 	addiu	$t0,$zero,0xc0
/*  f0af268:	ade80004 */ 	sw	$t0,0x4($t7)
/*  f0af26c:	adf90000 */ 	sw	$t9,0x0($t7)
/*  f0af270:	8fa90200 */ 	lw	$t1,0x200($sp)
/*  f0af274:	3c0bb900 */ 	lui	$t3,0xb900
/*  f0af278:	3c0c0050 */ 	lui	$t4,0x50
/*  f0af27c:	252a0008 */ 	addiu	$t2,$t1,0x8
/*  f0af280:	afaa0200 */ 	sw	$t2,0x200($sp)
/*  f0af284:	358c49d8 */ 	ori	$t4,$t4,0x49d8
/*  f0af288:	356b031d */ 	ori	$t3,$t3,0x31d
/*  f0af28c:	ad2b0000 */ 	sw	$t3,0x0($t1)
/*  f0af290:	ad2c0004 */ 	sw	$t4,0x4($t1)
/*  f0af294:	8fad0200 */ 	lw	$t5,0x200($sp)
/*  f0af298:	3c0fb900 */ 	lui	$t7,0xb900
/*  f0af29c:	35ef0002 */ 	ori	$t7,$t7,0x2
/*  f0af2a0:	25ae0008 */ 	addiu	$t6,$t5,0x8
/*  f0af2a4:	afae0200 */ 	sw	$t6,0x200($sp)
/*  f0af2a8:	ada00004 */ 	sw	$zero,0x4($t5)
/*  f0af2ac:	adaf0000 */ 	sw	$t7,0x0($t5)
/*  f0af2b0:	8fb80200 */ 	lw	$t8,0x200($sp)
/*  f0af2b4:	3c08ba00 */ 	lui	$t0,0xba00
/*  f0af2b8:	35081001 */ 	ori	$t0,$t0,0x1001
/*  f0af2bc:	27190008 */ 	addiu	$t9,$t8,0x8
/*  f0af2c0:	afb90200 */ 	sw	$t9,0x200($sp)
/*  f0af2c4:	af000004 */ 	sw	$zero,0x4($t8)
/*  f0af2c8:	af080000 */ 	sw	$t0,0x0($t8)
/*  f0af2cc:	8fa90200 */ 	lw	$t1,0x200($sp)
/*  f0af2d0:	3c0bba00 */ 	lui	$t3,0xba00
/*  f0af2d4:	356b0903 */ 	ori	$t3,$t3,0x903
/*  f0af2d8:	252a0008 */ 	addiu	$t2,$t1,0x8
/*  f0af2dc:	afaa0200 */ 	sw	$t2,0x200($sp)
/*  f0af2e0:	240c0c00 */ 	addiu	$t4,$zero,0xc00
/*  f0af2e4:	ad2c0004 */ 	sw	$t4,0x4($t1)
/*  f0af2e8:	ad2b0000 */ 	sw	$t3,0x0($t1)
/*  f0af2ec:	8fad0200 */ 	lw	$t5,0x200($sp)
/*  f0af2f0:	3c0fba00 */ 	lui	$t7,0xba00
/*  f0af2f4:	35ef0e02 */ 	ori	$t7,$t7,0xe02
/*  f0af2f8:	25ae0008 */ 	addiu	$t6,$t5,0x8
/*  f0af2fc:	afae0200 */ 	sw	$t6,0x200($sp)
/*  f0af300:	ada00004 */ 	sw	$zero,0x4($t5)
/*  f0af304:	adaf0000 */ 	sw	$t7,0x0($t5)
/*  f0af308:	8fb80200 */ 	lw	$t8,0x200($sp)
/*  f0af30c:	3c094432 */ 	lui	$t1,0x4432
/*  f0af310:	3c08fc50 */ 	lui	$t0,0xfc50
/*  f0af314:	27190008 */ 	addiu	$t9,$t8,0x8
/*  f0af318:	afb90200 */ 	sw	$t9,0x200($sp)
/*  f0af31c:	350898a1 */ 	ori	$t0,$t0,0x98a1
/*  f0af320:	35297f3f */ 	ori	$t1,$t1,0x7f3f
/*  f0af324:	27b10164 */ 	addiu	$s1,$sp,0x164
/*  f0af328:	02202025 */ 	or	$a0,$s1,$zero
/*  f0af32c:	af090004 */ 	sw	$t1,0x4($t8)
/*  f0af330:	0c00566c */ 	jal	mtx4LoadIdentity
/*  f0af334:	af080000 */ 	sw	$t0,0x0($t8)
/*  f0af338:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0af33c:	00000000 */ 	nop
/*  f0af340:	00402025 */ 	or	$a0,$v0,$zero
/*  f0af344:	0c0056f8 */ 	jal	mtx00015be0
/*  f0af348:	02202825 */ 	or	$a1,$s1,$zero
/*  f0af34c:	27b10124 */ 	addiu	$s1,$sp,0x124
/*  f0af350:	0c00566c */ 	jal	mtx4LoadIdentity
/*  f0af354:	02202025 */ 	or	$a0,$s1,$zero
/*  f0af358:	0fc2d5de */ 	jal	camGetUnk174c
/*  f0af35c:	00000000 */ 	nop
/*  f0af360:	00402025 */ 	or	$a0,$v0,$zero
/*  f0af364:	0c0056f8 */ 	jal	mtx00015be0
/*  f0af368:	02202825 */ 	or	$a1,$s1,$zero
/*  f0af36c:	44800000 */ 	mtc1	$zero,$f0
/*  f0af370:	27b101b0 */ 	addiu	$s1,$sp,0x1b0
/*  f0af374:	02202025 */ 	or	$a0,$s1,$zero
/*  f0af378:	e7a00158 */ 	swc1	$f0,0x158($sp)
/*  f0af37c:	e7a00154 */ 	swc1	$f0,0x154($sp)
/*  f0af380:	0c00566c */ 	jal	mtx4LoadIdentity
/*  f0af384:	e7a0015c */ 	swc1	$f0,0x15c($sp)
/*  f0af388:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0af38c:	00000000 */ 	nop
/*  f0af390:	00402025 */ 	or	$a0,$v0,$zero
/*  f0af394:	0c0056f8 */ 	jal	mtx00015be0
/*  f0af398:	02202825 */ 	or	$a1,$s1,$zero
/*  f0af39c:	c6041bb0 */ 	lwc1	$f4,0x1bb0($s0)
/*  f0af3a0:	44805000 */ 	mtc1	$zero,$f10
/*  f0af3a4:	3c017f1b */ 	lui	$at,%hi(var7f1acd70)
/*  f0af3a8:	e7a401a4 */ 	swc1	$f4,0x1a4($sp)
/*  f0af3ac:	c6061bb4 */ 	lwc1	$f6,0x1bb4($s0)
/*  f0af3b0:	44802000 */ 	mtc1	$zero,$f4
/*  f0af3b4:	c42ccd70 */ 	lwc1	$f12,%lo(var7f1acd70)($at)
/*  f0af3b8:	e7a601a8 */ 	swc1	$f6,0x1a8($sp)
/*  f0af3bc:	c6081bb8 */ 	lwc1	$f8,0x1bb8($s0)
/*  f0af3c0:	44803000 */ 	mtc1	$zero,$f6
/*  f0af3c4:	02202825 */ 	or	$a1,$s1,$zero
/*  f0af3c8:	e7aa01e0 */ 	swc1	$f10,0x1e0($sp)
/*  f0af3cc:	e7a401e4 */ 	swc1	$f4,0x1e4($sp)
/*  f0af3d0:	e7a801ac */ 	swc1	$f8,0x1ac($sp)
/*  f0af3d4:	0c0057e2 */ 	jal	mtx00015f88
/*  f0af3d8:	e7a601e8 */ 	swc1	$f6,0x1e8($sp)
/*  f0af3dc:	0fc59e66 */ 	jal	gfxAllocateMatrix
/*  f0af3e0:	00000000 */ 	nop
/*  f0af3e4:	00408025 */ 	or	$s0,$v0,$zero
/*  f0af3e8:	02202025 */ 	or	$a0,$s1,$zero
/*  f0af3ec:	0c005815 */ 	jal	mtx00016054
/*  f0af3f0:	00402825 */ 	or	$a1,$v0,$zero
/*  f0af3f4:	8fb10200 */ 	lw	$s1,0x200($sp)
/*  f0af3f8:	3c0c0102 */ 	lui	$t4,0x102
/*  f0af3fc:	358c0040 */ 	ori	$t4,$t4,0x40
/*  f0af400:	262b0008 */ 	addiu	$t3,$s1,0x8
/*  f0af404:	afab0200 */ 	sw	$t3,0x200($sp)
/*  f0af408:	02002025 */ 	or	$a0,$s0,$zero
/*  f0af40c:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0af410:	ae2c0000 */ 	sw	$t4,0x0($s1)
/*  f0af414:	ae220004 */ 	sw	$v0,0x4($s1)
/*  f0af418:	3c017f1b */ 	lui	$at,%hi(var7f1acd74)
/*  f0af41c:	c43ccd74 */ 	lwc1	$f28,%lo(var7f1acd74)($at)
/*  f0af420:	3c017f1b */ 	lui	$at,%hi(var7f1acd78)
/*  f0af424:	3c10800a */ 	lui	$s0,%hi(g_LaserSights)
/*  f0af428:	2610dbe0 */ 	addiu	$s0,$s0,%lo(g_LaserSights)
/*  f0af42c:	c43acd78 */ 	lwc1	$f26,%lo(var7f1acd78)($at)
/*  f0af430:	24120200 */ 	addiu	$s2,$zero,0x200
/*  f0af434:	8e0d0000 */ 	lw	$t5,0x0($s0)
.L0f0af438:
/*  f0af438:	2401ffff */ 	addiu	$at,$zero,-1
/*  f0af43c:	24040002 */ 	addiu	$a0,$zero,0x2
/*  f0af440:	11a10170 */ 	beq	$t5,$at,.L0f0afa04
/*  f0af444:	00000000 */ 	nop
/*  f0af448:	c600002c */ 	lwc1	$f0,0x2c($s0)
/*  f0af44c:	c6020030 */ 	lwc1	$f2,0x30($s0)
/*  f0af450:	c60e0034 */ 	lwc1	$f14,0x34($s0)
/*  f0af454:	c6080038 */ 	lwc1	$f8,0x38($s0)
/*  f0af458:	c616003c */ 	lwc1	$f22,0x3c($s0)
/*  f0af45c:	c6180040 */ 	lwc1	$f24,0x40($s0)
/*  f0af460:	e7a000e8 */ 	swc1	$f0,0xe8($sp)
/*  f0af464:	e7a200ec */ 	swc1	$f2,0xec($sp)
/*  f0af468:	e7ae00f0 */ 	swc1	$f14,0xf0($sp)
/*  f0af46c:	e7a800dc */ 	swc1	$f8,0xdc($sp)
/*  f0af470:	e7b600e0 */ 	swc1	$f22,0xe0($sp)
/*  f0af474:	0fc59e73 */ 	jal	gfxAllocateColours
/*  f0af478:	e7b800e4 */ 	swc1	$f24,0xe4($sp)
/*  f0af47c:	3c0eff00 */ 	lui	$t6,0xff00
/*  f0af480:	3c0fff00 */ 	lui	$t7,0xff00
/*  f0af484:	35ce005f */ 	ori	$t6,$t6,0x5f
/*  f0af488:	35ef000f */ 	ori	$t7,$t7,0xf
/*  f0af48c:	ac4e0000 */ 	sw	$t6,0x0($v0)
/*  f0af490:	ac4f0004 */ 	sw	$t7,0x4($v0)
/*  f0af494:	8fb10200 */ 	lw	$s1,0x200($sp)
/*  f0af498:	3c080704 */ 	lui	$t0,0x704
/*  f0af49c:	35080008 */ 	ori	$t0,$t0,0x8
/*  f0af4a0:	26390008 */ 	addiu	$t9,$s1,0x8
/*  f0af4a4:	afb90200 */ 	sw	$t9,0x200($sp)
/*  f0af4a8:	00402025 */ 	or	$a0,$v0,$zero
/*  f0af4ac:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0af4b0:	ae280000 */ 	sw	$t0,0x0($s1)
/*  f0af4b4:	ae220004 */ 	sw	$v0,0x4($s1)
/*  f0af4b8:	c6040028 */ 	lwc1	$f4,0x28($s0)
/*  f0af4bc:	44805000 */ 	mtc1	$zero,$f10
/*  f0af4c0:	00000000 */ 	nop
/*  f0af4c4:	4604503c */ 	c.lt.s	$f10,$f4
/*  f0af4c8:	00000000 */ 	nop
/*  f0af4cc:	4500014d */ 	bc1f	.L0f0afa04
/*  f0af4d0:	00000000 */ 	nop
/*  f0af4d4:	c600002c */ 	lwc1	$f0,0x2c($s0)
/*  f0af4d8:	c7a200ec */ 	lwc1	$f2,0xec($sp)
/*  f0af4dc:	c7ae00f0 */ 	lwc1	$f14,0xf0($sp)
/*  f0af4e0:	461a003c */ 	c.lt.s	$f0,$f26
/*  f0af4e4:	3c098007 */ 	lui	$t1,%hi(var80070584)
/*  f0af4e8:	c7aa01a4 */ 	lwc1	$f10,0x1a4($sp)
/*  f0af4ec:	45000017 */ 	bc1f	.L0f0af54c
/*  f0af4f0:	00000000 */ 	nop
/*  f0af4f4:	4600e03c */ 	c.lt.s	$f28,$f0
/*  f0af4f8:	00000000 */ 	nop
/*  f0af4fc:	45000013 */ 	bc1f	.L0f0af54c
/*  f0af500:	00000000 */ 	nop
/*  f0af504:	c6000030 */ 	lwc1	$f0,0x30($s0)
/*  f0af508:	461a003c */ 	c.lt.s	$f0,$f26
/*  f0af50c:	00000000 */ 	nop
/*  f0af510:	4500000e */ 	bc1f	.L0f0af54c
/*  f0af514:	00000000 */ 	nop
/*  f0af518:	4600e03c */ 	c.lt.s	$f28,$f0
/*  f0af51c:	00000000 */ 	nop
/*  f0af520:	4500000a */ 	bc1f	.L0f0af54c
/*  f0af524:	00000000 */ 	nop
/*  f0af528:	c6000034 */ 	lwc1	$f0,0x34($s0)
/*  f0af52c:	461a003c */ 	c.lt.s	$f0,$f26
/*  f0af530:	00000000 */ 	nop
/*  f0af534:	45000005 */ 	bc1f	.L0f0af54c
/*  f0af538:	00000000 */ 	nop
/*  f0af53c:	4600e03c */ 	c.lt.s	$f28,$f0
/*  f0af540:	00000000 */ 	nop
/*  f0af544:	4501012f */ 	bc1t	.L0f0afa04
/*  f0af548:	00000000 */ 	nop
.L0f0af54c:
/*  f0af54c:	8d290584 */ 	lw	$t1,%lo(var80070584)($t1)
/*  f0af550:	3c0140a0 */ 	lui	$at,0x40a0
/*  f0af554:	4481b000 */ 	mtc1	$at,$f22
/*  f0af558:	44893000 */ 	mtc1	$t1,$f6
/*  f0af55c:	c7a000e8 */ 	lwc1	$f0,0xe8($sp)
/*  f0af560:	05210005 */ 	bgez	$t1,.L0f0af578
/*  f0af564:	46803520 */ 	cvt.s.w	$f20,$f6
/*  f0af568:	3c014f80 */ 	lui	$at,0x4f80
/*  f0af56c:	44814000 */ 	mtc1	$at,$f8
/*  f0af570:	00000000 */ 	nop
/*  f0af574:	4608a500 */ 	add.s	$f20,$f20,$f8
.L0f0af578:
/*  f0af578:	460a0101 */ 	sub.s	$f4,$f0,$f10
/*  f0af57c:	c7a601a8 */ 	lwc1	$f6,0x1a8($sp)
/*  f0af580:	c7aa01ac */ 	lwc1	$f10,0x1ac($sp)
/*  f0af584:	46061201 */ 	sub.s	$f8,$f2,$f6
/*  f0af588:	46162002 */ 	mul.s	$f0,$f4,$f22
/*  f0af58c:	460a7101 */ 	sub.s	$f4,$f14,$f10
/*  f0af590:	46164082 */ 	mul.s	$f2,$f8,$f22
/*  f0af594:	00000000 */ 	nop
/*  f0af598:	46162382 */ 	mul.s	$f14,$f4,$f22
/*  f0af59c:	e7a000e8 */ 	swc1	$f0,0xe8($sp)
/*  f0af5a0:	46000182 */ 	mul.s	$f6,$f0,$f0
/*  f0af5a4:	e7a200ec */ 	swc1	$f2,0xec($sp)
/*  f0af5a8:	46021202 */ 	mul.s	$f8,$f2,$f2
/*  f0af5ac:	e7ae00f0 */ 	swc1	$f14,0xf0($sp)
/*  f0af5b0:	460e7102 */ 	mul.s	$f4,$f14,$f14
/*  f0af5b4:	46083280 */ 	add.s	$f10,$f6,$f8
/*  f0af5b8:	0c012974 */ 	jal	sqrtf
/*  f0af5bc:	460a2300 */ 	add.s	$f12,$f4,$f10
/*  f0af5c0:	3c0a8007 */ 	lui	$t2,%hi(var80070574)
/*  f0af5c4:	8d4a0574 */ 	lw	$t2,%lo(var80070574)($t2)
/*  f0af5c8:	27a40200 */ 	addiu	$a0,$sp,0x200
/*  f0af5cc:	3c05800b */ 	lui	$a1,%hi(g_TexGeneralConfigs)
/*  f0af5d0:	448a3000 */ 	mtc1	$t2,$f6
/*  f0af5d4:	24060004 */ 	addiu	$a2,$zero,0x4
/*  f0af5d8:	05410005 */ 	bgez	$t2,.L0f0af5f0
/*  f0af5dc:	468030a0 */ 	cvt.s.w	$f2,$f6
/*  f0af5e0:	3c014f80 */ 	lui	$at,0x4f80
/*  f0af5e4:	44814000 */ 	mtc1	$at,$f8
/*  f0af5e8:	00000000 */ 	nop
/*  f0af5ec:	46081080 */ 	add.s	$f2,$f2,$f8
.L0f0af5f0:
/*  f0af5f0:	4600103c */ 	c.lt.s	$f2,$f0
/*  f0af5f4:	3c0b8007 */ 	lui	$t3,%hi(var80070578)
/*  f0af5f8:	00003825 */ 	or	$a3,$zero,$zero
/*  f0af5fc:	240f0002 */ 	addiu	$t7,$zero,0x2
/*  f0af600:	4500003c */ 	bc1f	.L0f0af6f4
/*  f0af604:	24180001 */ 	addiu	$t8,$zero,0x1
/*  f0af608:	8d6b0578 */ 	lw	$t3,%lo(var80070578)($t3)
/*  f0af60c:	3c014f80 */ 	lui	$at,0x4f80
/*  f0af610:	3c0c8007 */ 	lui	$t4,%hi(var8007057c)
/*  f0af614:	448b2000 */ 	mtc1	$t3,$f4
/*  f0af618:	3c0e8007 */ 	lui	$t6,%hi(var80070580)
/*  f0af61c:	05610004 */ 	bgez	$t3,.L0f0af630
/*  f0af620:	46802320 */ 	cvt.s.w	$f12,$f4
/*  f0af624:	44815000 */ 	mtc1	$at,$f10
/*  f0af628:	00000000 */ 	nop
/*  f0af62c:	460a6300 */ 	add.s	$f12,$f12,$f10
.L0f0af630:
/*  f0af630:	4600603c */ 	c.lt.s	$f12,$f0
/*  f0af634:	00000000 */ 	nop
/*  f0af638:	45000021 */ 	bc1f	.L0f0af6c0
/*  f0af63c:	00000000 */ 	nop
/*  f0af640:	8d8c057c */ 	lw	$t4,%lo(var8007057c)($t4)
/*  f0af644:	3c014f80 */ 	lui	$at,0x4f80
/*  f0af648:	3c0d8007 */ 	lui	$t5,%hi(var80070580)
/*  f0af64c:	448c3000 */ 	mtc1	$t4,$f6
/*  f0af650:	05810004 */ 	bgez	$t4,.L0f0af664
/*  f0af654:	468033a0 */ 	cvt.s.w	$f14,$f6
/*  f0af658:	44814000 */ 	mtc1	$at,$f8
/*  f0af65c:	00000000 */ 	nop
/*  f0af660:	46087380 */ 	add.s	$f14,$f14,$f8
.L0f0af664:
/*  f0af664:	4600703c */ 	c.lt.s	$f14,$f0
/*  f0af668:	3c017f1b */ 	lui	$at,%hi(var7f1acd7c)
/*  f0af66c:	45000003 */ 	bc1f	.L0f0af67c
/*  f0af670:	00000000 */ 	nop
/*  f0af674:	10000010 */ 	b	.L0f0af6b8
/*  f0af678:	c434cd7c */ 	lwc1	$f20,%lo(var7f1acd7c)($at)
.L0f0af67c:
/*  f0af67c:	8dad0580 */ 	lw	$t5,%lo(var80070580)($t5)
/*  f0af680:	460c0201 */ 	sub.s	$f8,$f0,$f12
/*  f0af684:	3c014f80 */ 	lui	$at,0x4f80
/*  f0af688:	448d2000 */ 	mtc1	$t5,$f4
/*  f0af68c:	00000000 */ 	nop
/*  f0af690:	468022a0 */ 	cvt.s.w	$f10,$f4
/*  f0af694:	05a10004 */ 	bgez	$t5,.L0f0af6a8
/*  f0af698:	460c7101 */ 	sub.s	$f4,$f14,$f12
/*  f0af69c:	44813000 */ 	mtc1	$at,$f6
/*  f0af6a0:	00000000 */ 	nop
/*  f0af6a4:	46065280 */ 	add.s	$f10,$f10,$f6
.L0f0af6a8:
/*  f0af6a8:	46044183 */ 	div.s	$f6,$f8,$f4
/*  f0af6ac:	46145080 */ 	add.s	$f2,$f10,$f20
/*  f0af6b0:	46061282 */ 	mul.s	$f10,$f2,$f6
/*  f0af6b4:	460a1501 */ 	sub.s	$f20,$f2,$f10
.L0f0af6b8:
/*  f0af6b8:	1000000e */ 	b	.L0f0af6f4
/*  f0af6bc:	00000000 */ 	nop
.L0f0af6c0:
/*  f0af6c0:	8dce0580 */ 	lw	$t6,%lo(var80070580)($t6)
/*  f0af6c4:	46026281 */ 	sub.s	$f10,$f12,$f2
/*  f0af6c8:	3c014f80 */ 	lui	$at,0x4f80
/*  f0af6cc:	448e4000 */ 	mtc1	$t6,$f8
/*  f0af6d0:	05c10004 */ 	bgez	$t6,.L0f0af6e4
/*  f0af6d4:	46804120 */ 	cvt.s.w	$f4,$f8
/*  f0af6d8:	44813000 */ 	mtc1	$at,$f6
/*  f0af6dc:	00000000 */ 	nop
/*  f0af6e0:	46062100 */ 	add.s	$f4,$f4,$f6
.L0f0af6e4:
/*  f0af6e4:	460a2203 */ 	div.s	$f8,$f4,$f10
/*  f0af6e8:	46020181 */ 	sub.s	$f6,$f0,$f2
/*  f0af6ec:	46083102 */ 	mul.s	$f4,$f6,$f8
/*  f0af6f0:	46142500 */ 	add.s	$f20,$f4,$f20
.L0f0af6f4:
/*  f0af6f4:	8ca5b5a8 */ 	lw	$a1,%lo(g_TexGeneralConfigs)($a1)
/*  f0af6f8:	afaf0010 */ 	sw	$t7,0x10($sp)
/*  f0af6fc:	afb80014 */ 	sw	$t8,0x14($sp)
/*  f0af700:	afa00018 */ 	sw	$zero,0x18($sp)
/*  f0af704:	0fc2ce70 */ 	jal	texSelect
/*  f0af708:	24a50030 */ 	addiu	$a1,$a1,0x0030
/*  f0af70c:	44800000 */ 	mtc1	$zero,$f0
/*  f0af710:	c7aa00dc */ 	lwc1	$f10,0xdc($sp)
/*  f0af714:	c7b800e4 */ 	lwc1	$f24,0xe4($sp)
/*  f0af718:	c7a600dc */ 	lwc1	$f6,0xdc($sp)
/*  f0af71c:	460a0032 */ 	c.eq.s	$f0,$f10
/*  f0af720:	00000000 */ 	nop
/*  f0af724:	4500000b */ 	bc1f	.L0f0af754
/*  f0af728:	00000000 */ 	nop
/*  f0af72c:	46180032 */ 	c.eq.s	$f0,$f24
/*  f0af730:	3c013f80 */ 	lui	$at,0x3f80
/*  f0af734:	45000007 */ 	bc1f	.L0f0af754
/*  f0af738:	00000000 */ 	nop
/*  f0af73c:	4480c000 */ 	mtc1	$zero,$f24
/*  f0af740:	44819000 */ 	mtc1	$at,$f18
/*  f0af744:	4481b000 */ 	mtc1	$at,$f22
/*  f0af748:	4600c386 */ 	mov.s	$f14,$f24
/*  f0af74c:	1000001a */ 	b	.L0f0af7b8
/*  f0af750:	4600c406 */ 	mov.s	$f16,$f24
.L0f0af754:
/*  f0af754:	46063202 */ 	mul.s	$f8,$f6,$f6
/*  f0af758:	c7b600e0 */ 	lwc1	$f22,0xe0($sp)
/*  f0af75c:	c7b800e4 */ 	lwc1	$f24,0xe4($sp)
/*  f0af760:	4616b102 */ 	mul.s	$f4,$f22,$f22
/*  f0af764:	46044280 */ 	add.s	$f10,$f8,$f4
/*  f0af768:	4618c182 */ 	mul.s	$f6,$f24,$f24
/*  f0af76c:	0c012974 */ 	jal	sqrtf
/*  f0af770:	460a3300 */ 	add.s	$f12,$f6,$f10
/*  f0af774:	c7a800dc */ 	lwc1	$f8,0xdc($sp)
/*  f0af778:	c7b800e4 */ 	lwc1	$f24,0xe4($sp)
/*  f0af77c:	c7b600e0 */ 	lwc1	$f22,0xe0($sp)
/*  f0af780:	46004683 */ 	div.s	$f26,$f8,$f0
/*  f0af784:	4600c703 */ 	div.s	$f28,$f24,$f0
/*  f0af788:	461ad102 */ 	mul.s	$f4,$f26,$f26
/*  f0af78c:	4600b783 */ 	div.s	$f30,$f22,$f0
/*  f0af790:	461ce182 */ 	mul.s	$f6,$f28,$f28
/*  f0af794:	0c012974 */ 	jal	sqrtf
/*  f0af798:	46062300 */ 	add.s	$f12,$f4,$f6
/*  f0af79c:	4600d083 */ 	div.s	$f2,$f26,$f0
/*  f0af7a0:	46000407 */ 	neg.s	$f16,$f0
/*  f0af7a4:	4600e583 */ 	div.s	$f22,$f28,$f0
/*  f0af7a8:	4602f382 */ 	mul.s	$f14,$f30,$f2
/*  f0af7ac:	46001607 */ 	neg.s	$f24,$f2
/*  f0af7b0:	4616f482 */ 	mul.s	$f18,$f30,$f22
/*  f0af7b4:	00000000 */ 	nop
.L0f0af7b8:
/*  f0af7b8:	24040004 */ 	addiu	$a0,$zero,0x4
/*  f0af7bc:	e7ae00cc */ 	swc1	$f14,0xcc($sp)
/*  f0af7c0:	e7b000c8 */ 	swc1	$f16,0xc8($sp)
/*  f0af7c4:	0fc59e59 */ 	jal	gfxAllocateVertices
/*  f0af7c8:	e7b200c4 */ 	swc1	$f18,0xc4($sp)
/*  f0af7cc:	c7ae00cc */ 	lwc1	$f14,0xcc($sp)
/*  f0af7d0:	c7b000c8 */ 	lwc1	$f16,0xc8($sp)
/*  f0af7d4:	c7b200c4 */ 	lwc1	$f18,0xc4($sp)
/*  f0af7d8:	4600a007 */ 	neg.s	$f0,$f20
/*  f0af7dc:	a040002b */ 	sb	$zero,0x2b($v0)
/*  f0af7e0:	46160682 */ 	mul.s	$f26,$f0,$f22
/*  f0af7e4:	a040001f */ 	sb	$zero,0x1f($v0)
/*  f0af7e8:	a0400013 */ 	sb	$zero,0x13($v0)
/*  f0af7ec:	a0400007 */ 	sb	$zero,0x7($v0)
/*  f0af7f0:	a4400008 */ 	sh	$zero,0x8($v0)
/*  f0af7f4:	a440000a */ 	sh	$zero,0xa($v0)
/*  f0af7f8:	a4520014 */ 	sh	$s2,0x14($v0)
/*  f0af7fc:	a4400016 */ 	sh	$zero,0x16($v0)
/*  f0af800:	a4520020 */ 	sh	$s2,0x20($v0)
/*  f0af804:	a4520022 */ 	sh	$s2,0x22($v0)
/*  f0af808:	a440002c */ 	sh	$zero,0x2c($v0)
/*  f0af80c:	a452002e */ 	sh	$s2,0x2e($v0)
/*  f0af810:	460ea082 */ 	mul.s	$f2,$f20,$f14
/*  f0af814:	c7aa00e8 */ 	lwc1	$f10,0xe8($sp)
/*  f0af818:	3c0d0430 */ 	lui	$t5,0x430
/*  f0af81c:	4610a302 */ 	mul.s	$f12,$f20,$f16
/*  f0af820:	35ad0030 */ 	ori	$t5,$t5,0x30
/*  f0af824:	461a5200 */ 	add.s	$f8,$f10,$f26
/*  f0af828:	46180702 */ 	mul.s	$f28,$f0,$f24
/*  f0af82c:	00402025 */ 	or	$a0,$v0,$zero
/*  f0af830:	4612a782 */ 	mul.s	$f30,$f20,$f18
/*  f0af834:	46024100 */ 	add.s	$f4,$f8,$f2
/*  f0af838:	44804000 */ 	mtc1	$zero,$f8
/*  f0af83c:	4600218d */ 	trunc.w.s	$f6,$f4
/*  f0af840:	44083000 */ 	mfc1	$t0,$f6
/*  f0af844:	00000000 */ 	nop
/*  f0af848:	a4480000 */ 	sh	$t0,0x0($v0)
/*  f0af84c:	c7aa00ec */ 	lwc1	$f10,0xec($sp)
/*  f0af850:	46085100 */ 	add.s	$f4,$f10,$f8
/*  f0af854:	460c2180 */ 	add.s	$f6,$f4,$f12
/*  f0af858:	4600328d */ 	trunc.w.s	$f10,$f6
/*  f0af85c:	440a5000 */ 	mfc1	$t2,$f10
/*  f0af860:	00000000 */ 	nop
/*  f0af864:	a44a0002 */ 	sh	$t2,0x2($v0)
/*  f0af868:	c7a800f0 */ 	lwc1	$f8,0xf0($sp)
/*  f0af86c:	461c4100 */ 	add.s	$f4,$f8,$f28
/*  f0af870:	4616a202 */ 	mul.s	$f8,$f20,$f22
/*  f0af874:	461e2180 */ 	add.s	$f6,$f4,$f30
/*  f0af878:	4600328d */ 	trunc.w.s	$f10,$f6
/*  f0af87c:	440c5000 */ 	mfc1	$t4,$f10
/*  f0af880:	00000000 */ 	nop
/*  f0af884:	a44c0004 */ 	sh	$t4,0x4($v0)
/*  f0af888:	e7a80074 */ 	swc1	$f8,0x74($sp)
/*  f0af88c:	c7a60074 */ 	lwc1	$f6,0x74($sp)
/*  f0af890:	c7a400e8 */ 	lwc1	$f4,0xe8($sp)
/*  f0af894:	46062280 */ 	add.s	$f10,$f4,$f6
/*  f0af898:	46025200 */ 	add.s	$f8,$f10,$f2
/*  f0af89c:	44805000 */ 	mtc1	$zero,$f10
/*  f0af8a0:	4618a082 */ 	mul.s	$f2,$f20,$f24
/*  f0af8a4:	4600410d */ 	trunc.w.s	$f4,$f8
/*  f0af8a8:	440e2000 */ 	mfc1	$t6,$f4
/*  f0af8ac:	00000000 */ 	nop
/*  f0af8b0:	a44e000c */ 	sh	$t6,0xc($v0)
/*  f0af8b4:	c7a600ec */ 	lwc1	$f6,0xec($sp)
/*  f0af8b8:	460a3200 */ 	add.s	$f8,$f6,$f10
/*  f0af8bc:	460c4100 */ 	add.s	$f4,$f8,$f12
/*  f0af8c0:	460e0302 */ 	mul.s	$f12,$f0,$f14
/*  f0af8c4:	00000000 */ 	nop
/*  f0af8c8:	46100582 */ 	mul.s	$f22,$f0,$f16
/*  f0af8cc:	4600218d */ 	trunc.w.s	$f6,$f4
/*  f0af8d0:	44183000 */ 	mfc1	$t8,$f6
/*  f0af8d4:	00000000 */ 	nop
/*  f0af8d8:	a458000e */ 	sh	$t8,0xe($v0)
/*  f0af8dc:	c7aa00f0 */ 	lwc1	$f10,0xf0($sp)
/*  f0af8e0:	46025200 */ 	add.s	$f8,$f10,$f2
/*  f0af8e4:	461e4100 */ 	add.s	$f4,$f8,$f30
/*  f0af8e8:	4600218d */ 	trunc.w.s	$f6,$f4
/*  f0af8ec:	44083000 */ 	mfc1	$t0,$f6
/*  f0af8f0:	00000000 */ 	nop
/*  f0af8f4:	a4480010 */ 	sh	$t0,0x10($v0)
/*  f0af8f8:	c7aa00e8 */ 	lwc1	$f10,0xe8($sp)
/*  f0af8fc:	c7a80074 */ 	lwc1	$f8,0x74($sp)
/*  f0af900:	46085100 */ 	add.s	$f4,$f10,$f8
/*  f0af904:	460c2180 */ 	add.s	$f6,$f4,$f12
/*  f0af908:	44802000 */ 	mtc1	$zero,$f4
/*  f0af90c:	4600328d */ 	trunc.w.s	$f10,$f6
/*  f0af910:	440a5000 */ 	mfc1	$t2,$f10
/*  f0af914:	00000000 */ 	nop
/*  f0af918:	a44a0018 */ 	sh	$t2,0x18($v0)
/*  f0af91c:	c7a800ec */ 	lwc1	$f8,0xec($sp)
/*  f0af920:	46044180 */ 	add.s	$f6,$f8,$f4
/*  f0af924:	46120102 */ 	mul.s	$f4,$f0,$f18
/*  f0af928:	46163280 */ 	add.s	$f10,$f6,$f22
/*  f0af92c:	4600520d */ 	trunc.w.s	$f8,$f10
/*  f0af930:	440c4000 */ 	mfc1	$t4,$f8
/*  f0af934:	00000000 */ 	nop
/*  f0af938:	a44c001a */ 	sh	$t4,0x1a($v0)
/*  f0af93c:	c7a600f0 */ 	lwc1	$f6,0xf0($sp)
/*  f0af940:	e7a40068 */ 	swc1	$f4,0x68($sp)
/*  f0af944:	c7a80068 */ 	lwc1	$f8,0x68($sp)
/*  f0af948:	46023280 */ 	add.s	$f10,$f6,$f2
/*  f0af94c:	46085100 */ 	add.s	$f4,$f10,$f8
/*  f0af950:	4600218d */ 	trunc.w.s	$f6,$f4
/*  f0af954:	440e3000 */ 	mfc1	$t6,$f6
/*  f0af958:	00000000 */ 	nop
/*  f0af95c:	a44e001c */ 	sh	$t6,0x1c($v0)
/*  f0af960:	c7aa00e8 */ 	lwc1	$f10,0xe8($sp)
/*  f0af964:	461a5200 */ 	add.s	$f8,$f10,$f26
/*  f0af968:	460c4100 */ 	add.s	$f4,$f8,$f12
/*  f0af96c:	44804000 */ 	mtc1	$zero,$f8
/*  f0af970:	4600218d */ 	trunc.w.s	$f6,$f4
/*  f0af974:	44183000 */ 	mfc1	$t8,$f6
/*  f0af978:	00000000 */ 	nop
/*  f0af97c:	a4580024 */ 	sh	$t8,0x24($v0)
/*  f0af980:	c7aa00ec */ 	lwc1	$f10,0xec($sp)
/*  f0af984:	46085100 */ 	add.s	$f4,$f10,$f8
/*  f0af988:	46162180 */ 	add.s	$f6,$f4,$f22
/*  f0af98c:	4600328d */ 	trunc.w.s	$f10,$f6
/*  f0af990:	44085000 */ 	mfc1	$t0,$f10
/*  f0af994:	00000000 */ 	nop
/*  f0af998:	a4480026 */ 	sh	$t0,0x26($v0)
/*  f0af99c:	c7a800f0 */ 	lwc1	$f8,0xf0($sp)
/*  f0af9a0:	c7a60068 */ 	lwc1	$f6,0x68($sp)
/*  f0af9a4:	461c4100 */ 	add.s	$f4,$f8,$f28
/*  f0af9a8:	46062280 */ 	add.s	$f10,$f4,$f6
/*  f0af9ac:	4600520d */ 	trunc.w.s	$f8,$f10
/*  f0af9b0:	440a4000 */ 	mfc1	$t2,$f8
/*  f0af9b4:	00000000 */ 	nop
/*  f0af9b8:	a44a0028 */ 	sh	$t2,0x28($v0)
/*  f0af9bc:	8fb10200 */ 	lw	$s1,0x200($sp)
/*  f0af9c0:	262c0008 */ 	addiu	$t4,$s1,0x8
/*  f0af9c4:	afac0200 */ 	sw	$t4,0x200($sp)
/*  f0af9c8:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0af9cc:	ae2d0000 */ 	sw	$t5,0x0($s1)
/*  f0af9d0:	ae220004 */ 	sw	$v0,0x4($s1)
/*  f0af9d4:	8fae0200 */ 	lw	$t6,0x200($sp)
/*  f0af9d8:	3c18b100 */ 	lui	$t8,0xb100
/*  f0af9dc:	37180002 */ 	ori	$t8,$t8,0x2
/*  f0af9e0:	25cf0008 */ 	addiu	$t7,$t6,0x8
/*  f0af9e4:	afaf0200 */ 	sw	$t7,0x200($sp)
/*  f0af9e8:	24193210 */ 	addiu	$t9,$zero,0x3210
/*  f0af9ec:	3c017f1b */ 	lui	$at,%hi(var7f1acd88)
/*  f0af9f0:	add90004 */ 	sw	$t9,0x4($t6)
/*  f0af9f4:	add80000 */ 	sw	$t8,0x0($t6)
/*  f0af9f8:	c43ccd88 */ 	lwc1	$f28,%lo(var7f1acd88)($at)
/*  f0af9fc:	3c017f1b */ 	lui	$at,%hi(var7f1acd8c)
/*  f0afa00:	c43acd8c */ 	lwc1	$f26,%lo(var7f1acd8c)($at)
.L0f0afa04:
/*  f0afa04:	3c08800a */ 	lui	$t0,%hi(var8009dd00)
/*  f0afa08:	2508dd00 */ 	addiu	$t0,$t0,%lo(var8009dd00)
/*  f0afa0c:	26100048 */ 	addiu	$s0,$s0,72
/*  f0afa10:	5608fe89 */ 	bnel	$s0,$t0,.L0f0af438
/*  f0afa14:	8e0d0000 */ 	lw	$t5,0x0($s0)
/*  f0afa18:	8fbf0064 */ 	lw	$ra,0x64($sp)
/*  f0afa1c:	8fa20200 */ 	lw	$v0,0x200($sp)
/*  f0afa20:	d7b40028 */ 	ldc1	$f20,0x28($sp)
/*  f0afa24:	d7b60030 */ 	ldc1	$f22,0x30($sp)
/*  f0afa28:	d7b80038 */ 	ldc1	$f24,0x38($sp)
/*  f0afa2c:	d7ba0040 */ 	ldc1	$f26,0x40($sp)
/*  f0afa30:	d7bc0048 */ 	ldc1	$f28,0x48($sp)
/*  f0afa34:	d7be0050 */ 	ldc1	$f30,0x50($sp)
/*  f0afa38:	8fb00058 */ 	lw	$s0,0x58($sp)
/*  f0afa3c:	8fb1005c */ 	lw	$s1,0x5c($sp)
/*  f0afa40:	8fb20060 */ 	lw	$s2,0x60($sp)
/*  f0afa44:	03e00008 */ 	jr	$ra
/*  f0afa48:	27bd0200 */ 	addiu	$sp,$sp,0x200
);

// Mismatch: floating point callee-save register usage is different.
// eg. the registers for 0.0000001f and -0.000001f should be f26 and f28
// but the below loads them from rodata each time.
// There is also something wrong with the dot rotation.
//Gfx *lasersightRenderDot(Gfx *gdl)
//{
//	Mtxf *mtx;
//	f32 fVar10;
//	f32 f20;
//	struct player *player = g_Vars.currentplayer;
//	Mtxf sp1b0;
//	struct coord campos; // 1a4
//	Mtxf sp164;
//	Mtxf sp124;
//	s32 i;
//
//	static u32 sp1 = 800;
//	static u32 sp2 = 7000;
//	static u32 sp3 = 9000;
//	static u32 spb = 24;
//	static u32 spi = 6;
//
//	mainOverrideVariable("sp1", &sp1);
//	mainOverrideVariable("sp2", &sp2);
//	mainOverrideVariable("sp3", &sp3);
//	mainOverrideVariable("spb", &spb);
//	mainOverrideVariable("spi", &spi);
//
//	gDPSetCycleType(gdl++, G_CYC_1CYCLE);
//	gDPSetTextureFilter(gdl++, G_TF_BILERP);
//	gDPSetTexturePersp(gdl++, G_TP_PERSP);
//	gDPSetColorDither(gdl++, G_CD_DISABLE);
//	gDPSetRenderMode(gdl++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
//	gDPSetAlphaCompare(gdl++, G_AC_NONE);
//	gDPSetTextureLOD(gdl++, G_TL_TILE);
//	gDPSetTextureConvert(gdl++, G_TC_FILT);
//	gDPSetTextureLUT(gdl++, G_TT_NONE);
//	gDPSetCombineMode(gdl++, G_CC_BLENDIA, G_CC_BLENDIA);
//
//	mtx4LoadIdentity(&sp164);
//	mtx00015be0(camGetMatrix1740(), &sp164);
//	mtx4LoadIdentity(&sp124);
//	mtx00015be0(camGetUnk174c(), &sp124);
//
//	sp124.m[3][1] = 0;
//	sp124.m[3][0] = 0;
//	sp124.m[3][2] = 0;
//
//	mtx4LoadIdentity(&sp1b0);
//	mtx00015be0(camGetMatrix1740(), &sp1b0);
//
//	campos.x = player->cam_pos.x;
//	campos.y = player->cam_pos.y;
//	campos.z = player->cam_pos.z;
//
//	sp1b0.m[3][0] = 0.0f;
//	sp1b0.m[3][1] = 0.0f;
//	sp1b0.m[3][2] = 0.0f;
//
//	mtx00015f88(0.2f, &sp1b0);
//
//	mtx = gfxAllocateMatrix();
//	mtx00016054(&sp1b0, mtx);
//
//	gSPMatrix(gdl++, osVirtualToPhysical(mtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
//
//	// 438
//	for (i = 0; i < 4; i++) {
//		if (g_LaserSights[i].id != -1) {
//			struct coord pos; // e8
//			struct coord rot; // dc
//			u32 *colours;
//			struct gfxvtx *vertices;
//
//			pos.x = g_LaserSights[i].dotpos.f[0];
//			pos.y = g_LaserSights[i].dotpos.f[1];
//			pos.z = g_LaserSights[i].dotpos.f[2];
//
//			rot.x = g_LaserSights[i].dotrot.f[0];
//			rot.y = g_LaserSights[i].dotrot.f[1];
//			rot.z = g_LaserSights[i].dotrot.f[2];
//
//			colours = gfxAllocateColours(2);
//
//			colours[0] = 0xff00005f;
//			colours[1] = 0xff00000f;
//
//			gDPSetColorArray(gdl++, osVirtualToPhysical(colours), 2);
//
//			// 4c4
//			if (g_LaserSights[i].unk28 > 0.0f) {
//				if (pos.f[0] < 0.0000001f || pos.f[0] > -0.000001f
//						|| pos.f[1] < 0.0000001f || pos.f[1] > -0.000001f
//						|| pos.f[2] < 0.0000001f || pos.f[2] > -0.000001f) {
//					// 54c
//					f32 f14;
//					f32 f16;
//					f32 f18;
//					f32 f22;
//					f32 f24;
//					f32 f26;
//					f32 f28;
//					f32 f30;
//					f32 f2;
//					f32 sp1f;
//					f32 sp2f;
//					f32 sp3f;
//
//					f20 = spi; // = 6
//
//					pos.x = (pos.x - campos.x) * 5;
//					pos.y = (pos.y - campos.y) * 5;
//					pos.z = (pos.z - campos.z) * 5;
//
//					fVar10 = sqrtf(pos.f[0] * pos.f[0] + pos.f[1] * pos.f[1] + pos.f[2] * pos.f[2]);
//
//					sp1f = sp1;
//
//					// 600
//					if (fVar10 > sp1f) { // > 800
//						sp2f = sp2;
//
//						// 638
//						if (fVar10 > sp2f) { // > 7000
//							sp3f = sp3;
//
//							// 66c
//							if (fVar10 > sp3f) { // > 9000
//								f20 = 0.1f;
//							} else {
//								// 67c
//								f32 tmp = spb + f20;
//								f20 = tmp - tmp * ((fVar10 - sp2f) / (sp3f - sp2f));
//							}
//						} else {
//							// 6c0
//							f20 += (fVar10 - sp1f) * (spb / (sp2f - sp1f));
//						}
//					}
//
//					texSelect(&gdl, &g_TexGeneralConfigs[4], 4, 0, 2, 1, NULL);
//
//					// 724
//					if (rot.f[0] == 0 && rot.f[2] == 0) {
//						// 73c
//						f18 = 1;
//						f22 = 1;
//						f24 = 0;
//						f14 = 0;
//						f16 = 0;
//					} else {
//						// 754
//						f32 f0 = sqrtf(rot.f[0] * rot.f[0] + rot.f[1] * rot.f[1] + rot.f[2] * rot.f[2]);
//						f26 = rot.x / f0;
//						f30 = rot.y / f0;
//						f28 = rot.z / f0;
//
//						f0 = sqrtf(f26 * f26 + f28 * f28);
//						f2 = f26 / f0;
//
//						f22 = f28 / f0;
//						f14 = f30 * f2;
//						f16 = -f0;
//						f18 = f30 * f22;
//						f24 = -f2;
//					}
//
//					// 7b8
//					vertices = gfxAllocateVertices(4);
//
//					vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = 0;
//
//					vertices[0].unk08 = 0;
//					vertices[0].unk0a = 0;
//					vertices[1].unk08 = 512;
//					vertices[1].unk0a = 0;
//					vertices[2].unk08 = 512;
//					vertices[2].unk0a = 512;
//					vertices[3].unk08 = 0;
//					vertices[3].unk0a = 512;
//
//					vertices[0].x = pos.f[0] + ((-f20 * f22) + (f20 * f14));
//					vertices[0].y = pos.f[1] + 0.0f + (f20 * f16);
//					vertices[0].z = pos.f[2] + ((-f20 * f24) + (f20 * f18));
//
//					vertices[1].x = pos.f[0] + ((f20 * f22) + (f20 * f14));
//					vertices[1].y = pos.f[1] + 0.0f + (f20 * f16);
//					vertices[1].z = pos.f[2] + ((f20 * f24) + (f20 * f18));
//
//					vertices[2].x = pos.f[0] + ((f20 * f22) + (-f20 * f14));
//					vertices[2].y = pos.f[1] + 0.0f + (-f20 * f16);
//					vertices[2].z = pos.f[2] + ((f20 * f24) + (-f20 * f18));
//
//					vertices[3].x = pos.f[0] + ((-f20 * f22) + (-f20 * f14));
//					vertices[3].y = pos.f[1] + 0.0f + (-f20 * f16);
//					vertices[3].z = pos.f[2] + ((-f20 * f24) + (-f20 * f18));
//
//					gDPSetVerticeArray(gdl++, osVirtualToPhysical(vertices), 4);
//
//					gDPTri2(gdl++, 0, 1, 2, 2, 3, 0);
//				}
//			}
//		}
//	}
//
//	return gdl;
//}

GLOBAL_ASM(
glabel lasersightRenderBeam
.late_rodata
glabel var7f1acd90
.word 0x3e4ccccd
.text
/*  f0afa4c:	27bdfe18 */ 	addiu	$sp,$sp,-488
/*  f0afa50:	afb00058 */ 	sw	$s0,0x58($sp)
/*  f0afa54:	afa401e8 */ 	sw	$a0,0x1e8($sp)
/*  f0afa58:	3c10800a */ 	lui	$s0,%hi(g_Vars+0x284)
/*  f0afa5c:	248f0008 */ 	addiu	$t7,$a0,0x8
/*  f0afa60:	afbf007c */ 	sw	$ra,0x7c($sp)
/*  f0afa64:	afbe0078 */ 	sw	$s8,0x78($sp)
/*  f0afa68:	afb70074 */ 	sw	$s7,0x74($sp)
/*  f0afa6c:	afb60070 */ 	sw	$s6,0x70($sp)
/*  f0afa70:	afb5006c */ 	sw	$s5,0x6c($sp)
/*  f0afa74:	afb40068 */ 	sw	$s4,0x68($sp)
/*  f0afa78:	afb30064 */ 	sw	$s3,0x64($sp)
/*  f0afa7c:	afb20060 */ 	sw	$s2,0x60($sp)
/*  f0afa80:	afb1005c */ 	sw	$s1,0x5c($sp)
/*  f0afa84:	f7be0050 */ 	sdc1	$f30,0x50($sp)
/*  f0afa88:	f7bc0048 */ 	sdc1	$f28,0x48($sp)
/*  f0afa8c:	f7ba0040 */ 	sdc1	$f26,0x40($sp)
/*  f0afa90:	f7b80038 */ 	sdc1	$f24,0x38($sp)
/*  f0afa94:	f7b60030 */ 	sdc1	$f22,0x30($sp)
/*  f0afa98:	f7b40028 */ 	sdc1	$f20,0x28($sp)
/*  f0afa9c:	8e10a244 */ 	lw	$s0,%lo(g_Vars+0x284)($s0)
/*  f0afaa0:	afaf01e8 */ 	sw	$t7,0x1e8($sp)
/*  f0afaa4:	3c18ba00 */ 	lui	$t8,0xba00
/*  f0afaa8:	37181402 */ 	ori	$t8,$t8,0x1402
/*  f0afaac:	ac980000 */ 	sw	$t8,0x0($a0)
/*  f0afab0:	ac800004 */ 	sw	$zero,0x4($a0)
/*  f0afab4:	8fb901e8 */ 	lw	$t9,0x1e8($sp)
/*  f0afab8:	3c09ba00 */ 	lui	$t1,0xba00
/*  f0afabc:	35290c02 */ 	ori	$t1,$t1,0xc02
/*  f0afac0:	27280008 */ 	addiu	$t0,$t9,0x8
/*  f0afac4:	afa801e8 */ 	sw	$t0,0x1e8($sp)
/*  f0afac8:	240a2000 */ 	addiu	$t2,$zero,0x2000
/*  f0afacc:	af2a0004 */ 	sw	$t2,0x4($t9)
/*  f0afad0:	af290000 */ 	sw	$t1,0x0($t9)
/*  f0afad4:	8fab01e8 */ 	lw	$t3,0x1e8($sp)
/*  f0afad8:	3c0dba00 */ 	lui	$t5,0xba00
/*  f0afadc:	35ad1301 */ 	ori	$t5,$t5,0x1301
/*  f0afae0:	256c0008 */ 	addiu	$t4,$t3,0x8
/*  f0afae4:	afac01e8 */ 	sw	$t4,0x1e8($sp)
/*  f0afae8:	3c0e0008 */ 	lui	$t6,0x8
/*  f0afaec:	ad6e0004 */ 	sw	$t6,0x4($t3)
/*  f0afaf0:	ad6d0000 */ 	sw	$t5,0x0($t3)
/*  f0afaf4:	8faf01e8 */ 	lw	$t7,0x1e8($sp)
/*  f0afaf8:	3c19ba00 */ 	lui	$t9,0xba00
/*  f0afafc:	37390602 */ 	ori	$t9,$t9,0x602
/*  f0afb00:	25f80008 */ 	addiu	$t8,$t7,0x8
/*  f0afb04:	afb801e8 */ 	sw	$t8,0x1e8($sp)
/*  f0afb08:	240800c0 */ 	addiu	$t0,$zero,0xc0
/*  f0afb0c:	ade80004 */ 	sw	$t0,0x4($t7)
/*  f0afb10:	adf90000 */ 	sw	$t9,0x0($t7)
/*  f0afb14:	8fa901e8 */ 	lw	$t1,0x1e8($sp)
/*  f0afb18:	3c0bb900 */ 	lui	$t3,0xb900
/*  f0afb1c:	3c0c0050 */ 	lui	$t4,0x50
/*  f0afb20:	252a0008 */ 	addiu	$t2,$t1,0x8
/*  f0afb24:	afaa01e8 */ 	sw	$t2,0x1e8($sp)
/*  f0afb28:	358c41c8 */ 	ori	$t4,$t4,0x41c8
/*  f0afb2c:	356b031d */ 	ori	$t3,$t3,0x31d
/*  f0afb30:	ad2b0000 */ 	sw	$t3,0x0($t1)
/*  f0afb34:	ad2c0004 */ 	sw	$t4,0x4($t1)
/*  f0afb38:	8fad01e8 */ 	lw	$t5,0x1e8($sp)
/*  f0afb3c:	3c0fb900 */ 	lui	$t7,0xb900
/*  f0afb40:	35ef0002 */ 	ori	$t7,$t7,0x2
/*  f0afb44:	25ae0008 */ 	addiu	$t6,$t5,0x8
/*  f0afb48:	afae01e8 */ 	sw	$t6,0x1e8($sp)
/*  f0afb4c:	ada00004 */ 	sw	$zero,0x4($t5)
/*  f0afb50:	adaf0000 */ 	sw	$t7,0x0($t5)
/*  f0afb54:	8fb801e8 */ 	lw	$t8,0x1e8($sp)
/*  f0afb58:	3c08ba00 */ 	lui	$t0,0xba00
/*  f0afb5c:	35081001 */ 	ori	$t0,$t0,0x1001
/*  f0afb60:	27190008 */ 	addiu	$t9,$t8,0x8
/*  f0afb64:	afb901e8 */ 	sw	$t9,0x1e8($sp)
/*  f0afb68:	af000004 */ 	sw	$zero,0x4($t8)
/*  f0afb6c:	af080000 */ 	sw	$t0,0x0($t8)
/*  f0afb70:	8fa901e8 */ 	lw	$t1,0x1e8($sp)
/*  f0afb74:	3c0bba00 */ 	lui	$t3,0xba00
/*  f0afb78:	356b0903 */ 	ori	$t3,$t3,0x903
/*  f0afb7c:	252a0008 */ 	addiu	$t2,$t1,0x8
/*  f0afb80:	afaa01e8 */ 	sw	$t2,0x1e8($sp)
/*  f0afb84:	240c0c00 */ 	addiu	$t4,$zero,0xc00
/*  f0afb88:	ad2c0004 */ 	sw	$t4,0x4($t1)
/*  f0afb8c:	ad2b0000 */ 	sw	$t3,0x0($t1)
/*  f0afb90:	8fad01e8 */ 	lw	$t5,0x1e8($sp)
/*  f0afb94:	3c0fba00 */ 	lui	$t7,0xba00
/*  f0afb98:	35ef0e02 */ 	ori	$t7,$t7,0xe02
/*  f0afb9c:	25ae0008 */ 	addiu	$t6,$t5,0x8
/*  f0afba0:	afae01e8 */ 	sw	$t6,0x1e8($sp)
/*  f0afba4:	ada00004 */ 	sw	$zero,0x4($t5)
/*  f0afba8:	adaf0000 */ 	sw	$t7,0x0($t5)
/*  f0afbac:	8fb801e8 */ 	lw	$t8,0x1e8($sp)
/*  f0afbb0:	3c094432 */ 	lui	$t1,0x4432
/*  f0afbb4:	3c08fc50 */ 	lui	$t0,0xfc50
/*  f0afbb8:	27190008 */ 	addiu	$t9,$t8,0x8
/*  f0afbbc:	afb901e8 */ 	sw	$t9,0x1e8($sp)
/*  f0afbc0:	350898a1 */ 	ori	$t0,$t0,0x98a1
/*  f0afbc4:	35297f3f */ 	ori	$t1,$t1,0x7f3f
/*  f0afbc8:	af090004 */ 	sw	$t1,0x4($t8)
/*  f0afbcc:	af080000 */ 	sw	$t0,0x0($t8)
/*  f0afbd0:	8faa01e8 */ 	lw	$t2,0x1e8($sp)
/*  f0afbd4:	240d3000 */ 	addiu	$t5,$zero,0x3000
/*  f0afbd8:	3c0cb600 */ 	lui	$t4,0xb600
/*  f0afbdc:	254b0008 */ 	addiu	$t3,$t2,0x8
/*  f0afbe0:	afab01e8 */ 	sw	$t3,0x1e8($sp)
/*  f0afbe4:	3c05800b */ 	lui	$a1,%hi(g_TexGeneralConfigs)
/*  f0afbe8:	ad4d0004 */ 	sw	$t5,0x4($t2)
/*  f0afbec:	ad4c0000 */ 	sw	$t4,0x0($t2)
/*  f0afbf0:	8ca5b5a8 */ 	lw	$a1,%lo(g_TexGeneralConfigs)($a1)
/*  f0afbf4:	240f0001 */ 	addiu	$t7,$zero,0x1
/*  f0afbf8:	240e0002 */ 	addiu	$t6,$zero,0x2
/*  f0afbfc:	afae0010 */ 	sw	$t6,0x10($sp)
/*  f0afc00:	afaf0014 */ 	sw	$t7,0x14($sp)
/*  f0afc04:	afa00018 */ 	sw	$zero,0x18($sp)
/*  f0afc08:	27a401e8 */ 	addiu	$a0,$sp,0x1e8
/*  f0afc0c:	24060004 */ 	addiu	$a2,$zero,0x4
/*  f0afc10:	00003825 */ 	or	$a3,$zero,$zero
/*  f0afc14:	0fc2ce70 */ 	jal	texSelect
/*  f0afc18:	24a50024 */ 	addiu	$a1,$a1,0x0024
/*  f0afc1c:	0c00566c */ 	jal	mtx4LoadIdentity
/*  f0afc20:	27a4014c */ 	addiu	$a0,$sp,0x14c
/*  f0afc24:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0afc28:	00000000 */ 	nop
/*  f0afc2c:	00402025 */ 	or	$a0,$v0,$zero
/*  f0afc30:	0c0056f8 */ 	jal	mtx00015be0
/*  f0afc34:	27a5014c */ 	addiu	$a1,$sp,0x14c
/*  f0afc38:	0c00566c */ 	jal	mtx4LoadIdentity
/*  f0afc3c:	27a4010c */ 	addiu	$a0,$sp,0x10c
/*  f0afc40:	0fc2d5de */ 	jal	camGetUnk174c
/*  f0afc44:	00000000 */ 	nop
/*  f0afc48:	00402025 */ 	or	$a0,$v0,$zero
/*  f0afc4c:	0c0056f8 */ 	jal	mtx00015be0
/*  f0afc50:	27a5010c */ 	addiu	$a1,$sp,0x10c
/*  f0afc54:	4480a000 */ 	mtc1	$zero,$f20
/*  f0afc58:	27b10198 */ 	addiu	$s1,$sp,0x198
/*  f0afc5c:	02202025 */ 	or	$a0,$s1,$zero
/*  f0afc60:	e7b40140 */ 	swc1	$f20,0x140($sp)
/*  f0afc64:	e7b4013c */ 	swc1	$f20,0x13c($sp)
/*  f0afc68:	0c00566c */ 	jal	mtx4LoadIdentity
/*  f0afc6c:	e7b40144 */ 	swc1	$f20,0x144($sp)
/*  f0afc70:	0fc2d5be */ 	jal	camGetMatrix1740
/*  f0afc74:	00000000 */ 	nop
/*  f0afc78:	00402025 */ 	or	$a0,$v0,$zero
/*  f0afc7c:	0c0056f8 */ 	jal	mtx00015be0
/*  f0afc80:	02202825 */ 	or	$a1,$s1,$zero
/*  f0afc84:	c6041bb0 */ 	lwc1	$f4,0x1bb0($s0)
/*  f0afc88:	3c017f1b */ 	lui	$at,%hi(var7f1acd90)
/*  f0afc8c:	c42ccd90 */ 	lwc1	$f12,%lo(var7f1acd90)($at)
/*  f0afc90:	e7a4018c */ 	swc1	$f4,0x18c($sp)
/*  f0afc94:	c6061bb4 */ 	lwc1	$f6,0x1bb4($s0)
/*  f0afc98:	02202825 */ 	or	$a1,$s1,$zero
/*  f0afc9c:	e7a60190 */ 	swc1	$f6,0x190($sp)
/*  f0afca0:	c6081bb8 */ 	lwc1	$f8,0x1bb8($s0)
/*  f0afca4:	e7b401c8 */ 	swc1	$f20,0x1c8($sp)
/*  f0afca8:	e7b401cc */ 	swc1	$f20,0x1cc($sp)
/*  f0afcac:	e7b401d0 */ 	swc1	$f20,0x1d0($sp)
/*  f0afcb0:	0c0057e2 */ 	jal	mtx00015f88
/*  f0afcb4:	e7a80194 */ 	swc1	$f8,0x194($sp)
/*  f0afcb8:	0fc59e66 */ 	jal	gfxAllocateMatrix
/*  f0afcbc:	00000000 */ 	nop
/*  f0afcc0:	00408025 */ 	or	$s0,$v0,$zero
/*  f0afcc4:	02202025 */ 	or	$a0,$s1,$zero
/*  f0afcc8:	0c005815 */ 	jal	mtx00016054
/*  f0afccc:	00402825 */ 	or	$a1,$v0,$zero
/*  f0afcd0:	8fb101e8 */ 	lw	$s1,0x1e8($sp)
/*  f0afcd4:	3c080102 */ 	lui	$t0,0x102
/*  f0afcd8:	35080040 */ 	ori	$t0,$t0,0x40
/*  f0afcdc:	26390008 */ 	addiu	$t9,$s1,0x8
/*  f0afce0:	afb901e8 */ 	sw	$t9,0x1e8($sp)
/*  f0afce4:	02002025 */ 	or	$a0,$s0,$zero
/*  f0afce8:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0afcec:	ae280000 */ 	sw	$t0,0x0($s1)
/*  f0afcf0:	3c0143c8 */ 	lui	$at,0x43c8
/*  f0afcf4:	4481d000 */ 	mtc1	$at,$f26
/*  f0afcf8:	3c014348 */ 	lui	$at,0x4348
/*  f0afcfc:	4481c000 */ 	mtc1	$at,$f24
/*  f0afd00:	3c0140a0 */ 	lui	$at,0x40a0
/*  f0afd04:	4481b000 */ 	mtc1	$at,$f22
/*  f0afd08:	3c014170 */ 	lui	$at,0x4170
/*  f0afd0c:	3c10800a */ 	lui	$s0,%hi(g_LaserSights)
/*  f0afd10:	4481a000 */ 	mtc1	$at,$f20
/*  f0afd14:	ae220004 */ 	sw	$v0,0x4($s1)
/*  f0afd18:	2610dbe0 */ 	addiu	$s0,$s0,%lo(g_LaserSights)
/*  f0afd1c:	27be00ac */ 	addiu	$s8,$sp,0xac
/*  f0afd20:	27b70098 */ 	addiu	$s7,$sp,0x98
/*  f0afd24:	2416ffff */ 	addiu	$s6,$zero,-1
/*  f0afd28:	24150020 */ 	addiu	$s5,$zero,0x20
/*  f0afd2c:	24140004 */ 	addiu	$s4,$zero,0x4
/*  f0afd30:	27b300a8 */ 	addiu	$s3,$sp,0xa8
/*  f0afd34:	24120100 */ 	addiu	$s2,$zero,0x100
/*  f0afd38:	8e090000 */ 	lw	$t1,0x0($s0)
.L0f0afd3c:
/*  f0afd3c:	27a4014c */ 	addiu	$a0,$sp,0x14c
/*  f0afd40:	02e02825 */ 	or	$a1,$s7,$zero
/*  f0afd44:	12c90130 */ 	beq	$s6,$t1,.L0f0b0208
/*  f0afd48:	00000000 */ 	nop
/*  f0afd4c:	c60a0010 */ 	lwc1	$f10,0x10($s0)
/*  f0afd50:	c6040014 */ 	lwc1	$f4,0x14($s0)
/*  f0afd54:	c6060018 */ 	lwc1	$f6,0x18($s0)
/*  f0afd58:	e7aa0098 */ 	swc1	$f10,0x98($sp)
/*  f0afd5c:	e7a4009c */ 	swc1	$f4,0x9c($sp)
/*  f0afd60:	0c0056d9 */ 	jal	mtx4TransformVecInPlace
/*  f0afd64:	e7a600a0 */ 	swc1	$f6,0xa0($sp)
/*  f0afd68:	4480e000 */ 	mtc1	$zero,$f28
/*  f0afd6c:	c7a80098 */ 	lwc1	$f8,0x98($sp)
/*  f0afd70:	3c01bf80 */ 	lui	$at,0xbf80
/*  f0afd74:	02602025 */ 	or	$a0,$s3,$zero
/*  f0afd78:	461c403c */ 	c.lt.s	$f8,$f28
/*  f0afd7c:	03c02825 */ 	or	$a1,$s8,$zero
/*  f0afd80:	27a600b0 */ 	addiu	$a2,$sp,0xb0
/*  f0afd84:	45020006 */ 	bc1fl	.L0f0afda0
/*  f0afd88:	44812000 */ 	mtc1	$at,$f4
/*  f0afd8c:	3c013f80 */ 	lui	$at,0x3f80
/*  f0afd90:	44815000 */ 	mtc1	$at,$f10
/*  f0afd94:	10000004 */ 	b	.L0f0afda8
/*  f0afd98:	e7aa00a8 */ 	swc1	$f10,0xa8($sp)
/*  f0afd9c:	44812000 */ 	mtc1	$at,$f4
.L0f0afda0:
/*  f0afda0:	00000000 */ 	nop
/*  f0afda4:	e7a400a8 */ 	swc1	$f4,0xa8($sp)
.L0f0afda8:
/*  f0afda8:	3c014000 */ 	lui	$at,0x4000
/*  f0afdac:	44813000 */ 	mtc1	$at,$f6
/*  f0afdb0:	e7bc00b0 */ 	swc1	$f28,0xb0($sp)
/*  f0afdb4:	0c0011e4 */ 	jal	guNormalize
/*  f0afdb8:	e7a600ac */ 	swc1	$f6,0xac($sp)
/*  f0afdbc:	27a4010c */ 	addiu	$a0,$sp,0x10c
/*  f0afdc0:	0c0056c4 */ 	jal	mtx4RotateVecInPlace
/*  f0afdc4:	02602825 */ 	or	$a1,$s3,$zero
/*  f0afdc8:	c7bc018c */ 	lwc1	$f28,0x18c($sp)
/*  f0afdcc:	c6000010 */ 	lwc1	$f0,0x10($s0)
/*  f0afdd0:	c7be0190 */ 	lwc1	$f30,0x190($sp)
/*  f0afdd4:	c6020014 */ 	lwc1	$f2,0x14($s0)
/*  f0afdd8:	461c0201 */ 	sub.s	$f8,$f0,$f28
/*  f0afddc:	c60c0018 */ 	lwc1	$f12,0x18($s0)
/*  f0afde0:	c7a40194 */ 	lwc1	$f4,0x194($sp)
/*  f0afde4:	461e1281 */ 	sub.s	$f10,$f2,$f30
/*  f0afde8:	46164002 */ 	mul.s	$f0,$f8,$f22
/*  f0afdec:	c60e001c */ 	lwc1	$f14,0x1c($s0)
/*  f0afdf0:	46046181 */ 	sub.s	$f6,$f12,$f4
/*  f0afdf4:	46165082 */ 	mul.s	$f2,$f10,$f22
/*  f0afdf8:	c6100020 */ 	lwc1	$f16,0x20($s0)
/*  f0afdfc:	461c7201 */ 	sub.s	$f8,$f14,$f28
/*  f0afe00:	46163302 */ 	mul.s	$f12,$f6,$f22
/*  f0afe04:	c6120024 */ 	lwc1	$f18,0x24($s0)
/*  f0afe08:	461e8281 */ 	sub.s	$f10,$f16,$f30
/*  f0afe0c:	46164382 */ 	mul.s	$f14,$f8,$f22
/*  f0afe10:	e7a200d0 */ 	swc1	$f2,0xd0($sp)
/*  f0afe14:	46049181 */ 	sub.s	$f6,$f18,$f4
/*  f0afe18:	46165402 */ 	mul.s	$f16,$f10,$f22
/*  f0afe1c:	e7ac00d4 */ 	swc1	$f12,0xd4($sp)
/*  f0afe20:	e7a000cc */ 	swc1	$f0,0xcc($sp)
/*  f0afe24:	46163482 */ 	mul.s	$f18,$f6,$f22
/*  f0afe28:	e7ae00c0 */ 	swc1	$f14,0xc0($sp)
/*  f0afe2c:	46007201 */ 	sub.s	$f8,$f14,$f0
/*  f0afe30:	27a400b4 */ 	addiu	$a0,$sp,0xb4
/*  f0afe34:	e7b000c4 */ 	swc1	$f16,0xc4($sp)
/*  f0afe38:	46028281 */ 	sub.s	$f10,$f16,$f2
/*  f0afe3c:	e7a800b4 */ 	swc1	$f8,0xb4($sp)
/*  f0afe40:	e7b200c8 */ 	swc1	$f18,0xc8($sp)
/*  f0afe44:	460c9101 */ 	sub.s	$f4,$f18,$f12
/*  f0afe48:	e7aa00b8 */ 	swc1	$f10,0xb8($sp)
/*  f0afe4c:	27a500b8 */ 	addiu	$a1,$sp,0xb8
/*  f0afe50:	27a600bc */ 	addiu	$a2,$sp,0xbc
/*  f0afe54:	0c0011e4 */ 	jal	guNormalize
/*  f0afe58:	e7a400bc */ 	swc1	$f4,0xbc($sp)
/*  f0afe5c:	0fc59e73 */ 	jal	gfxAllocateColours
/*  f0afe60:	24040002 */ 	addiu	$a0,$zero,0x2
/*  f0afe64:	3c0aff00 */ 	lui	$t2,0xff00
/*  f0afe68:	3c0bff00 */ 	lui	$t3,0xff00
/*  f0afe6c:	354a005f */ 	ori	$t2,$t2,0x5f
/*  f0afe70:	356b000f */ 	ori	$t3,$t3,0xf
/*  f0afe74:	ac4a0000 */ 	sw	$t2,0x0($v0)
/*  f0afe78:	ac4b0004 */ 	sw	$t3,0x4($v0)
/*  f0afe7c:	8fb101e8 */ 	lw	$s1,0x1e8($sp)
/*  f0afe80:	3c0e0704 */ 	lui	$t6,0x704
/*  f0afe84:	35ce0008 */ 	ori	$t6,$t6,0x8
/*  f0afe88:	262d0008 */ 	addiu	$t5,$s1,0x8
/*  f0afe8c:	afad01e8 */ 	sw	$t5,0x1e8($sp)
/*  f0afe90:	00402025 */ 	or	$a0,$v0,$zero
/*  f0afe94:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0afe98:	ae2e0000 */ 	sw	$t6,0x0($s1)
/*  f0afe9c:	ae220004 */ 	sw	$v0,0x4($s1)
/*  f0afea0:	0fc59e59 */ 	jal	gfxAllocateVertices
/*  f0afea4:	24040006 */ 	addiu	$a0,$zero,0x6
/*  f0afea8:	a0400007 */ 	sb	$zero,0x7($v0)
/*  f0afeac:	a0400013 */ 	sb	$zero,0x13($v0)
/*  f0afeb0:	a040001f */ 	sb	$zero,0x1f($v0)
/*  f0afeb4:	a040002b */ 	sb	$zero,0x2b($v0)
/*  f0afeb8:	a0540037 */ 	sb	$s4,0x37($v0)
/*  f0afebc:	a0540043 */ 	sb	$s4,0x43($v0)
/*  f0afec0:	a4400008 */ 	sh	$zero,0x8($v0)
/*  f0afec4:	a440000a */ 	sh	$zero,0xa($v0)
/*  f0afec8:	a4400014 */ 	sh	$zero,0x14($v0)
/*  f0afecc:	a4520016 */ 	sh	$s2,0x16($v0)
/*  f0afed0:	a4550020 */ 	sh	$s5,0x20($v0)
/*  f0afed4:	a4400022 */ 	sh	$zero,0x22($v0)
/*  f0afed8:	a455002c */ 	sh	$s5,0x2c($v0)
/*  f0afedc:	a452002e */ 	sh	$s2,0x2e($v0)
/*  f0afee0:	a4400038 */ 	sh	$zero,0x38($v0)
/*  f0afee4:	a440003a */ 	sh	$zero,0x3a($v0)
/*  f0afee8:	a4400044 */ 	sh	$zero,0x44($v0)
/*  f0afeec:	a4520046 */ 	sh	$s2,0x46($v0)
/*  f0afef0:	c7a800a8 */ 	lwc1	$f8,0xa8($sp)
/*  f0afef4:	c7a600cc */ 	lwc1	$f6,0xcc($sp)
/*  f0afef8:	3c0d0450 */ 	lui	$t5,0x450
/*  f0afefc:	46144282 */ 	mul.s	$f10,$f8,$f20
/*  f0aff00:	35ad0048 */ 	ori	$t5,$t5,0x48
/*  f0aff04:	00402025 */ 	or	$a0,$v0,$zero
/*  f0aff08:	460a3101 */ 	sub.s	$f4,$f6,$f10
/*  f0aff0c:	4600220d */ 	trunc.w.s	$f8,$f4
/*  f0aff10:	44184000 */ 	mfc1	$t8,$f8
/*  f0aff14:	00000000 */ 	nop
/*  f0aff18:	a4580000 */ 	sh	$t8,0x0($v0)
/*  f0aff1c:	c7aa00ac */ 	lwc1	$f10,0xac($sp)
/*  f0aff20:	c7a600d0 */ 	lwc1	$f6,0xd0($sp)
/*  f0aff24:	46145102 */ 	mul.s	$f4,$f10,$f20
/*  f0aff28:	46043201 */ 	sub.s	$f8,$f6,$f4
/*  f0aff2c:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0aff30:	44085000 */ 	mfc1	$t0,$f10
/*  f0aff34:	00000000 */ 	nop
/*  f0aff38:	a4480002 */ 	sh	$t0,0x2($v0)
/*  f0aff3c:	c7a400b0 */ 	lwc1	$f4,0xb0($sp)
/*  f0aff40:	c7a600d4 */ 	lwc1	$f6,0xd4($sp)
/*  f0aff44:	46142202 */ 	mul.s	$f8,$f4,$f20
/*  f0aff48:	46083281 */ 	sub.s	$f10,$f6,$f8
/*  f0aff4c:	4600510d */ 	trunc.w.s	$f4,$f10
/*  f0aff50:	440a2000 */ 	mfc1	$t2,$f4
/*  f0aff54:	00000000 */ 	nop
/*  f0aff58:	a44a0004 */ 	sh	$t2,0x4($v0)
/*  f0aff5c:	c7a600a8 */ 	lwc1	$f6,0xa8($sp)
/*  f0aff60:	c7aa00cc */ 	lwc1	$f10,0xcc($sp)
/*  f0aff64:	46143202 */ 	mul.s	$f8,$f6,$f20
/*  f0aff68:	460a4100 */ 	add.s	$f4,$f8,$f10
/*  f0aff6c:	4600218d */ 	trunc.w.s	$f6,$f4
/*  f0aff70:	440c3000 */ 	mfc1	$t4,$f6
/*  f0aff74:	00000000 */ 	nop
/*  f0aff78:	a44c000c */ 	sh	$t4,0xc($v0)
/*  f0aff7c:	c7a800ac */ 	lwc1	$f8,0xac($sp)
/*  f0aff80:	c7a400d0 */ 	lwc1	$f4,0xd0($sp)
/*  f0aff84:	46144282 */ 	mul.s	$f10,$f8,$f20
/*  f0aff88:	46045180 */ 	add.s	$f6,$f10,$f4
/*  f0aff8c:	4600320d */ 	trunc.w.s	$f8,$f6
/*  f0aff90:	440e4000 */ 	mfc1	$t6,$f8
/*  f0aff94:	00000000 */ 	nop
/*  f0aff98:	a44e000e */ 	sh	$t6,0xe($v0)
/*  f0aff9c:	c7aa00b0 */ 	lwc1	$f10,0xb0($sp)
/*  f0affa0:	c7a600d4 */ 	lwc1	$f6,0xd4($sp)
/*  f0affa4:	46145102 */ 	mul.s	$f4,$f10,$f20
/*  f0affa8:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0affac:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0affb0:	44185000 */ 	mfc1	$t8,$f10
/*  f0affb4:	00000000 */ 	nop
/*  f0affb8:	a4580010 */ 	sh	$t8,0x10($v0)
/*  f0affbc:	c7a600b4 */ 	lwc1	$f6,0xb4($sp)
/*  f0affc0:	c7a400cc */ 	lwc1	$f4,0xcc($sp)
/*  f0affc4:	4606c202 */ 	mul.s	$f8,$f24,$f6
/*  f0affc8:	c7a600a8 */ 	lwc1	$f6,0xa8($sp)
/*  f0affcc:	46082280 */ 	add.s	$f10,$f4,$f8
/*  f0affd0:	46143102 */ 	mul.s	$f4,$f6,$f20
/*  f0affd4:	46045201 */ 	sub.s	$f8,$f10,$f4
/*  f0affd8:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0affdc:	44083000 */ 	mfc1	$t0,$f6
/*  f0affe0:	00000000 */ 	nop
/*  f0affe4:	a4480018 */ 	sh	$t0,0x18($v0)
/*  f0affe8:	c7a400b8 */ 	lwc1	$f4,0xb8($sp)
/*  f0affec:	c7aa00d0 */ 	lwc1	$f10,0xd0($sp)
/*  f0afff0:	4604c202 */ 	mul.s	$f8,$f24,$f4
/*  f0afff4:	c7a400ac */ 	lwc1	$f4,0xac($sp)
/*  f0afff8:	46085180 */ 	add.s	$f6,$f10,$f8
/*  f0afffc:	46142282 */ 	mul.s	$f10,$f4,$f20
/*  f0b0000:	460a3201 */ 	sub.s	$f8,$f6,$f10
/*  f0b0004:	4600410d */ 	trunc.w.s	$f4,$f8
/*  f0b0008:	440a2000 */ 	mfc1	$t2,$f4
/*  f0b000c:	00000000 */ 	nop
/*  f0b0010:	a44a001a */ 	sh	$t2,0x1a($v0)
/*  f0b0014:	c7aa00bc */ 	lwc1	$f10,0xbc($sp)
/*  f0b0018:	c7a600d4 */ 	lwc1	$f6,0xd4($sp)
/*  f0b001c:	460ac202 */ 	mul.s	$f8,$f24,$f10
/*  f0b0020:	c7aa00b0 */ 	lwc1	$f10,0xb0($sp)
/*  f0b0024:	46083100 */ 	add.s	$f4,$f6,$f8
/*  f0b0028:	46145182 */ 	mul.s	$f6,$f10,$f20
/*  f0b002c:	46062201 */ 	sub.s	$f8,$f4,$f6
/*  f0b0030:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0b0034:	440c5000 */ 	mfc1	$t4,$f10
/*  f0b0038:	00000000 */ 	nop
/*  f0b003c:	a44c001c */ 	sh	$t4,0x1c($v0)
/*  f0b0040:	c7a600b4 */ 	lwc1	$f6,0xb4($sp)
/*  f0b0044:	c7a400cc */ 	lwc1	$f4,0xcc($sp)
/*  f0b0048:	4606c202 */ 	mul.s	$f8,$f24,$f6
/*  f0b004c:	c7a600a8 */ 	lwc1	$f6,0xa8($sp)
/*  f0b0050:	46082280 */ 	add.s	$f10,$f4,$f8
/*  f0b0054:	46143102 */ 	mul.s	$f4,$f6,$f20
/*  f0b0058:	460a2200 */ 	add.s	$f8,$f4,$f10
/*  f0b005c:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0b0060:	440e3000 */ 	mfc1	$t6,$f6
/*  f0b0064:	00000000 */ 	nop
/*  f0b0068:	a44e0024 */ 	sh	$t6,0x24($v0)
/*  f0b006c:	c7aa00b8 */ 	lwc1	$f10,0xb8($sp)
/*  f0b0070:	c7a400d0 */ 	lwc1	$f4,0xd0($sp)
/*  f0b0074:	460ac202 */ 	mul.s	$f8,$f24,$f10
/*  f0b0078:	c7aa00ac */ 	lwc1	$f10,0xac($sp)
/*  f0b007c:	46082180 */ 	add.s	$f6,$f4,$f8
/*  f0b0080:	46145102 */ 	mul.s	$f4,$f10,$f20
/*  f0b0084:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0b0088:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0b008c:	44185000 */ 	mfc1	$t8,$f10
/*  f0b0090:	00000000 */ 	nop
/*  f0b0094:	a4580026 */ 	sh	$t8,0x26($v0)
/*  f0b0098:	c7a600bc */ 	lwc1	$f6,0xbc($sp)
/*  f0b009c:	c7a400d4 */ 	lwc1	$f4,0xd4($sp)
/*  f0b00a0:	4606c202 */ 	mul.s	$f8,$f24,$f6
/*  f0b00a4:	c7a600b0 */ 	lwc1	$f6,0xb0($sp)
/*  f0b00a8:	46082280 */ 	add.s	$f10,$f4,$f8
/*  f0b00ac:	46143102 */ 	mul.s	$f4,$f6,$f20
/*  f0b00b0:	460a2200 */ 	add.s	$f8,$f4,$f10
/*  f0b00b4:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0b00b8:	44083000 */ 	mfc1	$t0,$f6
/*  f0b00bc:	00000000 */ 	nop
/*  f0b00c0:	a4480028 */ 	sh	$t0,0x28($v0)
/*  f0b00c4:	c7aa00b4 */ 	lwc1	$f10,0xb4($sp)
/*  f0b00c8:	c7a400cc */ 	lwc1	$f4,0xcc($sp)
/*  f0b00cc:	460ad202 */ 	mul.s	$f8,$f26,$f10
/*  f0b00d0:	c7aa00a8 */ 	lwc1	$f10,0xa8($sp)
/*  f0b00d4:	46082180 */ 	add.s	$f6,$f4,$f8
/*  f0b00d8:	46145102 */ 	mul.s	$f4,$f10,$f20
/*  f0b00dc:	46043201 */ 	sub.s	$f8,$f6,$f4
/*  f0b00e0:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0b00e4:	440a5000 */ 	mfc1	$t2,$f10
/*  f0b00e8:	00000000 */ 	nop
/*  f0b00ec:	a44a0030 */ 	sh	$t2,0x30($v0)
/*  f0b00f0:	c7a400b8 */ 	lwc1	$f4,0xb8($sp)
/*  f0b00f4:	c7a600d0 */ 	lwc1	$f6,0xd0($sp)
/*  f0b00f8:	4604d202 */ 	mul.s	$f8,$f26,$f4
/*  f0b00fc:	c7a400ac */ 	lwc1	$f4,0xac($sp)
/*  f0b0100:	46083280 */ 	add.s	$f10,$f6,$f8
/*  f0b0104:	46142182 */ 	mul.s	$f6,$f4,$f20
/*  f0b0108:	46065201 */ 	sub.s	$f8,$f10,$f6
/*  f0b010c:	4600410d */ 	trunc.w.s	$f4,$f8
/*  f0b0110:	440c2000 */ 	mfc1	$t4,$f4
/*  f0b0114:	00000000 */ 	nop
/*  f0b0118:	a44c0032 */ 	sh	$t4,0x32($v0)
/*  f0b011c:	c7a600bc */ 	lwc1	$f6,0xbc($sp)
/*  f0b0120:	c7aa00d4 */ 	lwc1	$f10,0xd4($sp)
/*  f0b0124:	4606d202 */ 	mul.s	$f8,$f26,$f6
/*  f0b0128:	c7a600b0 */ 	lwc1	$f6,0xb0($sp)
/*  f0b012c:	46085100 */ 	add.s	$f4,$f10,$f8
/*  f0b0130:	46143282 */ 	mul.s	$f10,$f6,$f20
/*  f0b0134:	460a2201 */ 	sub.s	$f8,$f4,$f10
/*  f0b0138:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0b013c:	440e3000 */ 	mfc1	$t6,$f6
/*  f0b0140:	00000000 */ 	nop
/*  f0b0144:	a44e0034 */ 	sh	$t6,0x34($v0)
/*  f0b0148:	c7aa00b4 */ 	lwc1	$f10,0xb4($sp)
/*  f0b014c:	c7a400cc */ 	lwc1	$f4,0xcc($sp)
/*  f0b0150:	460ad202 */ 	mul.s	$f8,$f26,$f10
/*  f0b0154:	c7aa00a8 */ 	lwc1	$f10,0xa8($sp)
/*  f0b0158:	46082180 */ 	add.s	$f6,$f4,$f8
/*  f0b015c:	46145102 */ 	mul.s	$f4,$f10,$f20
/*  f0b0160:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0b0164:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0b0168:	44185000 */ 	mfc1	$t8,$f10
/*  f0b016c:	00000000 */ 	nop
/*  f0b0170:	a458003c */ 	sh	$t8,0x3c($v0)
/*  f0b0174:	c7a600b8 */ 	lwc1	$f6,0xb8($sp)
/*  f0b0178:	c7a400d0 */ 	lwc1	$f4,0xd0($sp)
/*  f0b017c:	4606d202 */ 	mul.s	$f8,$f26,$f6
/*  f0b0180:	c7a600ac */ 	lwc1	$f6,0xac($sp)
/*  f0b0184:	46082280 */ 	add.s	$f10,$f4,$f8
/*  f0b0188:	46143102 */ 	mul.s	$f4,$f6,$f20
/*  f0b018c:	460a2200 */ 	add.s	$f8,$f4,$f10
/*  f0b0190:	4600418d */ 	trunc.w.s	$f6,$f8
/*  f0b0194:	44083000 */ 	mfc1	$t0,$f6
/*  f0b0198:	00000000 */ 	nop
/*  f0b019c:	a448003e */ 	sh	$t0,0x3e($v0)
/*  f0b01a0:	c7aa00bc */ 	lwc1	$f10,0xbc($sp)
/*  f0b01a4:	c7a400d4 */ 	lwc1	$f4,0xd4($sp)
/*  f0b01a8:	460ad202 */ 	mul.s	$f8,$f26,$f10
/*  f0b01ac:	c7aa00b0 */ 	lwc1	$f10,0xb0($sp)
/*  f0b01b0:	46082180 */ 	add.s	$f6,$f4,$f8
/*  f0b01b4:	46145102 */ 	mul.s	$f4,$f10,$f20
/*  f0b01b8:	46062200 */ 	add.s	$f8,$f4,$f6
/*  f0b01bc:	4600428d */ 	trunc.w.s	$f10,$f8
/*  f0b01c0:	440a5000 */ 	mfc1	$t2,$f10
/*  f0b01c4:	00000000 */ 	nop
/*  f0b01c8:	a44a0040 */ 	sh	$t2,0x40($v0)
/*  f0b01cc:	8fb101e8 */ 	lw	$s1,0x1e8($sp)
/*  f0b01d0:	262c0008 */ 	addiu	$t4,$s1,0x8
/*  f0b01d4:	afac01e8 */ 	sw	$t4,0x1e8($sp)
/*  f0b01d8:	0c012d20 */ 	jal	osVirtualToPhysical
/*  f0b01dc:	ae2d0000 */ 	sw	$t5,0x0($s1)
/*  f0b01e0:	ae220004 */ 	sw	$v0,0x4($s1)
/*  f0b01e4:	8fae01e8 */ 	lw	$t6,0x1e8($sp)
/*  f0b01e8:	3c18b100 */ 	lui	$t8,0xb100
/*  f0b01ec:	3c195232 */ 	lui	$t9,0x5232
/*  f0b01f0:	25cf0008 */ 	addiu	$t7,$t6,0x8
/*  f0b01f4:	afaf01e8 */ 	sw	$t7,0x1e8($sp)
/*  f0b01f8:	37393210 */ 	ori	$t9,$t9,0x3210
/*  f0b01fc:	37184512 */ 	ori	$t8,$t8,0x4512
/*  f0b0200:	add80000 */ 	sw	$t8,0x0($t6)
/*  f0b0204:	add90004 */ 	sw	$t9,0x4($t6)
.L0f0b0208:
/*  f0b0208:	3c08800a */ 	lui	$t0,%hi(var8009dd00)
/*  f0b020c:	2508dd00 */ 	addiu	$t0,$t0,%lo(var8009dd00)
/*  f0b0210:	26100048 */ 	addiu	$s0,$s0,72
/*  f0b0214:	5608fec9 */ 	bnel	$s0,$t0,.L0f0afd3c
/*  f0b0218:	8e090000 */ 	lw	$t1,0x0($s0)
/*  f0b021c:	8fbf007c */ 	lw	$ra,0x7c($sp)
/*  f0b0220:	8fa201e8 */ 	lw	$v0,0x1e8($sp)
/*  f0b0224:	d7b40028 */ 	ldc1	$f20,0x28($sp)
/*  f0b0228:	d7b60030 */ 	ldc1	$f22,0x30($sp)
/*  f0b022c:	d7b80038 */ 	ldc1	$f24,0x38($sp)
/*  f0b0230:	d7ba0040 */ 	ldc1	$f26,0x40($sp)
/*  f0b0234:	d7bc0048 */ 	ldc1	$f28,0x48($sp)
/*  f0b0238:	d7be0050 */ 	ldc1	$f30,0x50($sp)
/*  f0b023c:	8fb00058 */ 	lw	$s0,0x58($sp)
/*  f0b0240:	8fb1005c */ 	lw	$s1,0x5c($sp)
/*  f0b0244:	8fb20060 */ 	lw	$s2,0x60($sp)
/*  f0b0248:	8fb30064 */ 	lw	$s3,0x64($sp)
/*  f0b024c:	8fb40068 */ 	lw	$s4,0x68($sp)
/*  f0b0250:	8fb5006c */ 	lw	$s5,0x6c($sp)
/*  f0b0254:	8fb60070 */ 	lw	$s6,0x70($sp)
/*  f0b0258:	8fb70074 */ 	lw	$s7,0x74($sp)
/*  f0b025c:	8fbe0078 */ 	lw	$s8,0x78($sp)
/*  f0b0260:	03e00008 */ 	jr	$ra
/*  f0b0264:	27bd01e8 */ 	addiu	$sp,$sp,0x1e8
);

// Mismatch: Two load instructions are swapped at afddc, and regalloc
//Gfx *lasersightRenderBeam(Gfx *gdl)
//{
//	f32 tmp;
//	struct player *player = g_Vars.currentplayer;
//	Mtxf *mtx;
//	s32 i;
//	Mtxf sp198;
//	struct coord campos; // 18c
//	Mtxf sp14c;
//	Mtxf sp10c;
//
//	gDPSetCycleType(gdl++, G_CYC_1CYCLE);
//	gDPSetTextureFilter(gdl++, G_TF_BILERP);
//	gDPSetTexturePersp(gdl++, G_TP_PERSP);
//	gDPSetColorDither(gdl++, G_CD_DISABLE);
//	gDPSetRenderMode(gdl++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
//	gDPSetAlphaCompare(gdl++, G_AC_NONE);
//	gDPSetTextureLOD(gdl++, G_TL_TILE);
//	gDPSetTextureConvert(gdl++, G_TC_FILT);
//	gDPSetTextureLUT(gdl++, G_TT_NONE);
//	gDPSetCombineMode(gdl++, G_CC_BLENDIA, G_CC_BLENDIA);
//	gSPClearGeometryMode(gdl++, G_CULL_BOTH);
//
//	texSelect(&gdl, &g_TexGeneralConfigs[3], 4, 0, 2, 1, NULL);
//	mtx4LoadIdentity(&sp14c);
//
//	mtx00015be0(camGetMatrix1740(), &sp14c);
//	mtx4LoadIdentity(&sp10c);
//	mtx00015be0(camGetUnk174c(), &sp10c);
//
//	sp10c.m[3][1] = 0;
//	sp10c.m[3][0] = 0;
//	sp10c.m[3][2] = 0;
//
//	mtx4LoadIdentity(&sp198);
//	mtx00015be0(camGetMatrix1740(), &sp198);
//
//	campos.x = player->cam_pos.x;
//	campos.y = player->cam_pos.y;
//	campos.z = player->cam_pos.z;
//
//	sp198.m[3][0] = 0;
//	sp198.m[3][1] = 0;
//	sp198.m[3][2] = 0;
//
//	mtx00015f88(0.2f, &sp198);
//	mtx = gfxAllocateMatrix();
//	mtx00016054(&sp198, mtx);
//
//	gSPMatrix(gdl++, osVirtualToPhysical(mtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
//
//	for (i = 0; i < 4; i++) {
//		if (g_LaserSights[i].id != -1) {
//			u32 *colours;
//			struct coord spcc;
//			struct coord spc0;
//			struct coord spb4;
//			struct coord spa8;
//			struct gfxvtx *vertices;
//			struct coord sp98;
//
//			sp98.f[0] = g_LaserSights[i].beamnear.f[0];
//			sp98.f[1] = g_LaserSights[i].beamnear.f[1];
//			sp98.f[2] = g_LaserSights[i].beamnear.f[2];
//
//			mtx4TransformVecInPlace(&sp14c, &sp98);
//
//			spa8.f[0] = sp98.f[0] < 0.0f ? 1 : -1;
//			spa8.f[1] = 2;
//			spa8.f[2] = 0.0f;
//
//			guNormalize(&spa8.x, &spa8.y, &spa8.z);
//
//			mtx4RotateVecInPlace(&sp10c, &spa8);
//
//			spcc.f[0] = (g_LaserSights[i].beamnear.f[0] - campos.f[0]) * 5;
//			spcc.f[1] = (g_LaserSights[i].beamnear.f[1] - campos.f[1]) * 5;
//			spcc.f[2] = (g_LaserSights[i].beamnear.f[2] - campos.f[2]) * 5;
//
//			spc0.f[0] = (g_LaserSights[i].beamfar.f[0] - campos.f[0]) * 5;
//			spc0.f[1] = (g_LaserSights[i].beamfar.f[1] - campos.f[1]) * 5;
//			spc0.f[2] = (g_LaserSights[i].beamfar.f[2] - campos.f[2]) * 5;
//
//			spb4.f[0] = spc0.f[0] - spcc.f[0];
//			spb4.f[1] = spc0.f[1] - spcc.f[1];
//			spb4.f[2] = spc0.f[2] - spcc.f[2];
//
//			guNormalize(&spb4.x, &spb4.y, &spb4.z);
//
//			colours = gfxAllocateColours(2);
//
//			colours[0] = 0xff00005f;
//			colours[1] = 0xff00000f;
//
//			gDPSetColorArray(gdl++, osVirtualToPhysical(colours), 2);
//
//			vertices = gfxAllocateVertices(6);
//
//			vertices[0].colour = 0;
//			vertices[1].colour = 0;
//			vertices[2].colour = 0;
//			vertices[3].colour = 0;
//			vertices[4].colour = 4;
//			vertices[5].colour = 4;
//
//			vertices[0].unk08 = 0;
//			vertices[0].unk0a = 0;
//			vertices[1].unk08 = 0;
//			vertices[1].unk0a = 256;
//			vertices[2].unk08 = 32;
//			vertices[2].unk0a = 0;
//			vertices[3].unk08 = 32;
//			vertices[3].unk0a = 256;
//			vertices[4].unk08 = 0;
//			vertices[4].unk0a = 0;
//			vertices[5].unk08 = 0;
//			vertices[5].unk0a = 256;
//
//			vertices[0].x = spcc.f[0] - spa8.f[0] * 15;
//			vertices[0].y = spcc.f[1] - spa8.f[1] * 15;
//			vertices[0].z = spcc.f[2] - spa8.f[2] * 15;
//
//			vertices[1].x = spcc.f[0] + spa8.f[0] * 15;
//			vertices[1].y = spcc.f[1] + spa8.f[1] * 15;
//			vertices[1].z = spcc.f[2] + spa8.f[2] * 15;
//
//			vertices[2].x = spcc.f[0] + (200 * spb4.f[0]) - (spa8.f[0] * 15);
//			vertices[2].y = spcc.f[1] + (200 * spb4.f[1]) - (spa8.f[1] * 15);
//			vertices[2].z = spcc.f[2] + (200 * spb4.f[2]) - (spa8.f[2] * 15);
//
//			vertices[3].x = spcc.f[0] + (200 * spb4.f[0]) + (spa8.f[0] * 15);
//			vertices[3].y = spcc.f[1] + (200 * spb4.f[1]) + (spa8.f[1] * 15);
//			vertices[3].z = spcc.f[2] + (200 * spb4.f[2]) + (spa8.f[2] * 15);
//
//			vertices[4].x = spcc.f[0] + (400 * spb4.f[0]) - (spa8.f[0] * 15);
//			vertices[4].y = spcc.f[1] + (400 * spb4.f[1]) - (spa8.f[1] * 15);
//			vertices[4].z = spcc.f[2] + (400 * spb4.f[2]) - (spa8.f[2] * 15);
//
//			vertices[5].x = spcc.f[0] + (400 * spb4.f[0]) + (spa8.f[0] * 15);
//			vertices[5].y = spcc.f[1] + (400 * spb4.f[1]) + (spa8.f[1] * 15);
//			vertices[5].z = spcc.f[2] + (400 * spb4.f[2]) + (spa8.f[2] * 15);
//
//			gDPSetVerticeArray(gdl++, osVirtualToPhysical(vertices), 6);
//
//			gDPTri4(gdl++, 0, 1, 2, 2, 3, 1, 2, 3, 5, 2, 5, 4);
//		}
//	}
//
//	return gdl;
//}

void lasersightSetBeam(s32 id, s32 arg1, struct coord *near, struct coord *far)
{
	s32 i;

	if (!lasersightExists(id, &i)) {
		if (i == -1) {
			return;
		}

		g_LaserSights[i].id = id;
	}

	g_LaserSights[i].unk04.x = near->x;
	g_LaserSights[i].unk04.y = near->y;
	g_LaserSights[i].unk04.z = near->z;

	g_LaserSights[i].beamnear.x = near->x;
	g_LaserSights[i].beamnear.y = near->y;
	g_LaserSights[i].beamnear.z = near->z;

	g_LaserSights[i].beamfar.x = far->x;
	g_LaserSights[i].beamfar.y = far->y;
	g_LaserSights[i].beamfar.z = far->z;

	g_LaserSights[i].unk44 = arg1;
	g_LaserSights[i].unk28 = 0;
}

void lasersightSetDot(s32 arg0, struct coord *pos, struct coord *rot)
{
	s32 i;

	if (lasersightExists(arg0, &i)) {
		g_LaserSights[i].unk28 += 1.0f;

		g_LaserSights[i].dotpos.x = pos->x;
		g_LaserSights[i].dotpos.y = pos->y;
		g_LaserSights[i].dotpos.z = pos->z;

		g_LaserSights[i].dotrot.x = rot->x;
		g_LaserSights[i].dotrot.y = rot->y;
		g_LaserSights[i].dotrot.z = rot->z;
	}
}

void lasersightFree(s32 arg0)
{
	s32 i;

	if (lasersightExists(arg0, &i)) {
		g_LaserSights[i].id = -1;
	}
}
