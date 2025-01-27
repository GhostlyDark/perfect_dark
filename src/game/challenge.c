#include <ultra64.h>
#include "constants.h"
#include "game/chrai.h"
#include "game/chraicommands.h"
#include "game/prop.h"
#include "game/atan2f.h"
#include "game/playermgr.h"
#include "game/mplayer/setup.h"
#include "game/bot.h"
#include "game/challenge.h"
#include "game/training.h"
#include "game/lang.h"
#include "game/mplayer/mplayer.h"
#include "game/mplayer/scenarios.h"
#include "game/pad.h"
#include "bss.h"
#include "lib/dma.h"
#include "lib/rng.h"
#include "data.h"
#include "types.h"

u8 g_MpFeaturesForceUnlocked[40];
u8 g_MpFeaturesUnlocked[80];

u32 g_MpChallengeIndex = 0;
struct mpconfigfull *g_MpCurrentChallengeConfig = NULL;

struct challenge g_MpChallenges[NUM_CHALLENGES] = {
	{ L_OPTIONS_406, MPCONFIG_CHALLENGE01 }, // "Challenge 1"
	{ L_OPTIONS_407, MPCONFIG_CHALLENGE02 }, // "Challenge 2"
	{ L_OPTIONS_408, MPCONFIG_CHALLENGE03 }, // "Challenge 3"
	{ L_OPTIONS_409, MPCONFIG_CHALLENGE04 }, // "Challenge 4"
	{ L_OPTIONS_410, MPCONFIG_CHALLENGE05 }, // "Challenge 5"
	{ L_OPTIONS_411, MPCONFIG_CHALLENGE06 }, // "Challenge 6"
	{ L_OPTIONS_412, MPCONFIG_CHALLENGE07 }, // "Challenge 7"
	{ L_OPTIONS_413, MPCONFIG_CHALLENGE08 }, // "Challenge 8"
	{ L_OPTIONS_414, MPCONFIG_CHALLENGE09 }, // "Challenge 9"
	{ L_OPTIONS_415, MPCONFIG_CHALLENGE10 }, // "Challenge 10"
	{ L_OPTIONS_416, MPCONFIG_CHALLENGE11 }, // "Challenge 11"
	{ L_OPTIONS_417, MPCONFIG_CHALLENGE12 }, // "Challenge 12"
	{ L_OPTIONS_418, MPCONFIG_CHALLENGE13 }, // "Challenge 13"
	{ L_OPTIONS_419, MPCONFIG_CHALLENGE14 }, // "Challenge 14"
	{ L_OPTIONS_420, MPCONFIG_CHALLENGE15 }, // "Challenge 15"
	{ L_OPTIONS_421, MPCONFIG_CHALLENGE16 }, // "Challenge 16"
	{ L_OPTIONS_422, MPCONFIG_CHALLENGE17 }, // "Challenge 17"
	{ L_OPTIONS_423, MPCONFIG_CHALLENGE18 }, // "Challenge 18"
	{ L_OPTIONS_424, MPCONFIG_CHALLENGE19 }, // "Challenge 19"
	{ L_OPTIONS_425, MPCONFIG_CHALLENGE20 }, // "Challenge 20"
	{ L_OPTIONS_426, MPCONFIG_CHALLENGE21 }, // "Challenge 21"
	{ L_OPTIONS_427, MPCONFIG_CHALLENGE22 }, // "Challenge 22"
	{ L_OPTIONS_428, MPCONFIG_CHALLENGE23 }, // "Challenge 23"
	{ L_OPTIONS_429, MPCONFIG_CHALLENGE24 }, // "Challenge 24"
	{ L_OPTIONS_430, MPCONFIG_CHALLENGE25 }, // "Challenge 25"
	{ L_OPTIONS_431, MPCONFIG_CHALLENGE26 }, // "Challenge 26"
	{ L_OPTIONS_432, MPCONFIG_CHALLENGE27 }, // "Challenge 27"
	{ L_OPTIONS_433, MPCONFIG_CHALLENGE28 }, // "Challenge 28"
	{ L_OPTIONS_434, MPCONFIG_CHALLENGE29 }, // "Challenge 29"
	{ L_OPTIONS_435, MPCONFIG_CHALLENGE30 }, // "Challenge 30"
};

bool challengeIsAvailable(s32 challengeindex)
{
	return (g_MpChallenges[challengeindex].availability & 1) != 0;
}

bool ChallengeIsAvailableToPlayer(s32 chrnum, s32 challengeindex)
{
	if ((g_MpSetup.chrslots & (1 << chrnum)) == 0) {
		return 0;
	}

	return ((g_MpChallenges[challengeindex].availability & (2 << chrnum)) != 0);
}

bool challengeIsAvailableToAnyPlayer(s32 challengeindex)
{
	return (g_MpChallenges[challengeindex].availability & (((g_MpSetup.chrslots & 0xf) << 1) | 1)) != 0;
}

void challengeDetermineUnlockedFeatures(void)
{
	s32 challengeindex;
	s32 numgifted; // number of unlocked but not completed challenges
	u8 flag;
	s32 prev;
	s32 i;
	s32 j;
	s32 k;

	// Clear all challenge availability
	for (challengeindex = 0; challengeindex < 30; challengeindex++) {
		g_MpChallenges[challengeindex].availability = 0;
	}

	numgifted = 0;

	// Mark challenges completed by any player
	for (challengeindex = 0; challengeindex < 30; challengeindex++) {
		flag = 0;

		if (challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 1)
				|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 2)
				|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 3)
				|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 4)) {
			// Completed challenge
			flag = 1;
		} else if (challengeindex < 4) {
			// Not yet completed, but challenges 1-4 are always available
			flag = 1;
			numgifted++;
		} else if (challengeindex > 0
				&& (challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 1)
					|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 2)
					|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 3)
					|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 4))) {
			// Challenges are available if their previous one is complete
			flag = 1;
			numgifted++;
		}
#if VERSION == VERSION_NTSC_BETA || VERSION == VERSION_PAL_BETA
		else if (debugIsAllChallengesEnabled()) {
			flag = 1;
		}
#endif

		g_MpChallenges[challengeindex].availability |= flag;
	}

	// Gift up to 4 challenges
	for (challengeindex = 0; numgifted < 4 && challengeindex < 30; challengeindex++) {
		if ((g_MpChallenges[challengeindex].availability & 1) == 0) {
			g_MpChallenges[challengeindex].availability |= 1;
			numgifted++;
		}
	}

	// Now same as above, but per player
	for (j = 0; j < 4; j++) {
		numgifted = 0;

		for (challengeindex = 0; challengeindex < 30; challengeindex++) {
			flag = 0;

			if (challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 1)
					|| challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 2)
					|| challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 3)
					|| challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 4)) {
				// Completed challenge
				flag = 2 << j;
			} else if (challengeindex < 4) {
				// Not yet completed, but challenges 1-4 are always available
				flag = 2 << j;
				numgifted++;
			} else if (challengeindex > 0) {
				// Challenges are available if their previous one is complete
				prev = challengeindex - 1;

				if (challengeIsCompletedByPlayerWithNumPlayers(j, prev, 1)
						|| challengeIsCompletedByPlayerWithNumPlayers(j, prev, 2)
						|| challengeIsCompletedByPlayerWithNumPlayers(j, prev, 3)
						|| challengeIsCompletedByPlayerWithNumPlayers(j, prev, 4)) {
					flag = 2 << j;
					numgifted++;
				}
			}

			g_MpChallenges[challengeindex].availability |= flag;
		}

		// Gift up to 4 challenges
		for (challengeindex = 0; numgifted < 4 && challengeindex < 30; challengeindex++) {
			if ((g_MpChallenges[challengeindex].availability & (2 << j)) == 0) {
				g_MpChallenges[challengeindex].availability |= 2 << j;
				numgifted++;
			}
		}
	}

	for (j = 0; j < ARRAYCOUNT(g_MpFeaturesUnlocked); j++) {
		flag = 0;

		for (challengeindex = 0; challengeindex < 30; challengeindex++) {
			if (challengeIsAvailableToAnyPlayer(challengeindex)) {
				for (i = 0; i < 16; i++) {
					if (g_MpChallenges[challengeindex].unlockfeatures[i] == j) {
						flag |= 1;
					}
				}
			}
		}

		for (i = 0; i < ARRAYCOUNT(g_MpFeaturesForceUnlocked); i++) {
			if (g_MpFeaturesForceUnlocked[i] == j) {
				flag |= 1;
			}
		}

		for (challengeindex = 0; challengeindex < 30; challengeindex++) {
			for (prev = 0; prev < 4; prev++) {
				if (ChallengeIsAvailableToPlayer(prev, challengeindex)) {
					for (i = 0; i < 16; i++) {
						if (g_MpChallenges[challengeindex].unlockfeatures[i] == j) {
							flag |= 2 << prev;
						}
					}
				}
			}
		}

		g_MpFeaturesUnlocked[j] = flag;
	}

	for (j = 0; j < func0f188bcc(); j++) {
		struct mpweapon *weapon = &g_MpWeapons[j];

		if (weapon->unlockfeature > 0 && func0f19cbcc(weapon->weaponnum)) {
			g_MpFeaturesUnlocked[weapon->unlockfeature] |= 1;
		}
	}

	func0f1895e8();

	// If the ability to have 8 simulants hasn't been unlocked, limit them to 4
	if (!challengeIsFeatureUnlocked(MPFEATURE_8BOTS)) {
		for (k = 4; k < MAX_BOTS; k++) {
			if (g_MpSetup.chrslots & (1 << (4 + k))) {
				mpRemoveSimulant(k);
			}
		}

		if (g_Vars.mpquickteamnumsims > 4) {
			g_Vars.mpquickteamnumsims = 4;
		}
	}
}

void challengePerformSanityChecks(void)
{
	if (g_BossFile.locktype == MPLOCKTYPE_CHALLENGE) {
		s32 numplayers = 0;
		s32 i;

		// Reset player handicaps
		for (i = 0; i < 4; i++) {
			if (g_MpSetup.chrslots & (1 << i)) {
				g_PlayerConfigsArray[i].handicap = 0x80;
				numplayers++;
			}
		}

		// Turn off all simulants and turn them on if enabled
		// for this number of players
		g_MpSetup.chrslots &= 0x000f;

		for (i = 0; i != MAX_BOTS; i++) {
			g_BotConfigsArray[i].difficulty = g_MpSimulantDifficultiesPerNumPlayers[i][numplayers - 1];

			if (g_BotConfigsArray[i].difficulty != BOTDIFF_DISABLED) {
				g_MpSetup.chrslots |= 1 << (i + 4);
			}
		}

		if (g_MpSetup.scenario == MPSCENARIO_KINGOFTHEHILL) {
			g_Vars.mphilltime = 10;
		}
	} else if (!challengeIsFeatureUnlocked(MPFEATURE_8BOTS)) {
		// Limit to 4 players and 4 simulants
		g_MpSetup.chrslots &= 0x00ff;
	}
}

s32 challengeGetNumAvailable(void)
{
	s32 challengeindex;
	s32 count = 0;

	for (challengeindex = 0; challengeindex != NUM_CHALLENGES; challengeindex++) {
		if (challengeIsAvailableToAnyPlayer(challengeindex)) {
			count++;
		}
	}

	return count;
}

char *challengeGetName(s32 challengeindex)
{
	return langGet(g_MpChallenges[challengeindex].name);
}

char *challengeGetNameBySlot(s32 slot)
{
	s32 index = 0;
	s32 i;

	for (i = 0; i < 30; i++) {
		if (challengeIsAvailableToAnyPlayer(i)) {
			if (index == slot) {
				return challengeGetName(i);
			}

			index++;
		}
	}

	return "";
}

void challengeSetCurrentBySlot(s32 slotnum)
{
	s32 challengeindex;
	g_MpChallengeIndex = 0;

	for (challengeindex = 0; challengeindex != NUM_CHALLENGES; challengeindex++) {
		if (challengeIsAvailableToAnyPlayer(challengeindex)) {
			if (slotnum == 0) {
				g_MpChallengeIndex = challengeindex;
				break;
			}

			slotnum--;
		}
	}

	challengeApply();
}

s32 challengeGetCurrent(void)
{
	return g_MpChallengeIndex;
}

bool challengeIsCompletedByAnyChrWithNumPlayersBySlot(s32 slot, s32 numplayers)
{
	s32 availableindex = 0;
	s32 i;

	for (i = 0; i < 30; i++) {
		if (challengeIsAvailableToAnyPlayer(i)) {
			if (availableindex == slot) {
				return challengeIsCompletedByAnyPlayerWithNumPlayers(i, numplayers);
			}

			availableindex++;
		}
	}

	return false;
}

bool challengeIsCompletedByChrWithNumPlayersBySlot(s32 mpchrnum, s32 slot, s32 numplayers)
{
	s32 availableindex = 0;
	s32 i;

	for (i = 0; i < 30; i++) {
		if (challengeIsAvailableToAnyPlayer(i)) {
			if (availableindex == slot) {
				return challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, i, numplayers);
			}

			availableindex++;
		}
	}

	return false;
}

extern u32 _mpstringsESegmentRomStart;
extern u32 _mpstringsJSegmentRomStart;
extern u32 _mpstringsPSegmentRomStart;
extern u32 _mpstringsGSegmentRomStart;
extern u32 _mpstringsFSegmentRomStart;
extern u32 _mpstringsSSegmentRomStart;
extern u32 _mpstringsISegmentRomStart;
extern u32 _mpstringsESegmentRomEnd;
extern u32 _mpstringsJSegmentRomEnd;
extern u32 _mpstringsPSegmentRomEnd;
extern u32 _mpstringsGSegmentRomEnd;
extern u32 _mpstringsFSegmentRomEnd;
extern u32 _mpstringsSSegmentRomEnd;
extern u32 _mpstringsISegmentRomEnd;

u32 *var800887c4 = &_mpstringsESegmentRomStart;
u32 *var800887c8 = &_mpstringsESegmentRomEnd;
u32 *var800887cc = &_mpstringsJSegmentRomStart;
u32 *var800887d0 = &_mpstringsJSegmentRomEnd;
u32 *var800887d4 = &_mpstringsPSegmentRomStart;
u32 *var800887d8 = &_mpstringsPSegmentRomEnd;
u32 *var800887dc = &_mpstringsGSegmentRomStart;
u32 *var800887e0 = &_mpstringsGSegmentRomEnd;
u32 *var800887e4 = &_mpstringsFSegmentRomStart;
u32 *var800887e8 = &_mpstringsFSegmentRomEnd;
u32 *var800887ec = &_mpstringsSSegmentRomStart;
u32 *var800887f0 = &_mpstringsSSegmentRomEnd;
u32 *var800887f4 = &_mpstringsISegmentRomStart;
u32 *var800887f8 = &_mpstringsISegmentRomEnd;

GLOBAL_ASM(
glabel challengeLoadConfig
/*  f19b914:	27bdfe38 */ 	addiu	$sp,$sp,-456
/*  f19b918:	afbf0014 */ 	sw	$ra,0x14($sp)
/*  f19b91c:	afa401c8 */ 	sw	$a0,0x1c8($sp)
/*  f19b920:	afa501cc */ 	sw	$a1,0x1cc($sp)
/*  f19b924:	0fc5b9b5 */ 	jal	langGetFileNumOffset
/*  f19b928:	afa601d0 */ 	sw	$a2,0x1d0($sp)
/*  f19b92c:	3c0f8009 */ 	lui	$t7,%hi(var800887c4)
/*  f19b930:	25ef87c4 */ 	addiu	$t7,$t7,%lo(var800887c4)
/*  f19b934:	afa20050 */ 	sw	$v0,0x50($sp)
/*  f19b938:	25e80030 */ 	addiu	$t0,$t7,0x30
/*  f19b93c:	27ae0018 */ 	addiu	$t6,$sp,0x18
.L0f19b940:
/*  f19b940:	8de10000 */ 	lw	$at,0x0($t7)
/*  f19b944:	25ef000c */ 	addiu	$t7,$t7,0xc
/*  f19b948:	25ce000c */ 	addiu	$t6,$t6,0xc
/*  f19b94c:	adc1fff4 */ 	sw	$at,-0xc($t6)
/*  f19b950:	8de1fff8 */ 	lw	$at,-0x8($t7)
/*  f19b954:	adc1fff8 */ 	sw	$at,-0x8($t6)
/*  f19b958:	8de1fffc */ 	lw	$at,-0x4($t7)
/*  f19b95c:	15e8fff8 */ 	bne	$t7,$t0,.L0f19b940
/*  f19b960:	adc1fffc */ 	sw	$at,-0x4($t6)
/*  f19b964:	8de10000 */ 	lw	$at,0x0($t7)
/*  f19b968:	8de80004 */ 	lw	$t0,0x4($t7)
/*  f19b96c:	3c0b007d */ 	lui	$t3,%hi(_mpconfigsSegmentRomStart)
/*  f19b970:	adc10000 */ 	sw	$at,0x0($t6)
/*  f19b974:	adc80004 */ 	sw	$t0,0x4($t6)
/*  f19b978:	8fa901c8 */ 	lw	$t1,0x1c8($sp)
/*  f19b97c:	256b0a40 */ 	addiu	$t3,$t3,%lo(_mpconfigsSegmentRomStart)
/*  f19b980:	8fa401cc */ 	lw	$a0,0x1cc($sp)
/*  f19b984:	00095080 */ 	sll	$t2,$t1,0x2
/*  f19b988:	01495023 */ 	subu	$t2,$t2,$t1
/*  f19b98c:	000a5080 */ 	sll	$t2,$t2,0x2
/*  f19b990:	01495021 */ 	addu	$t2,$t2,$t1
/*  f19b994:	000a50c0 */ 	sll	$t2,$t2,0x3
/*  f19b998:	014b2821 */ 	addu	$a1,$t2,$t3
/*  f19b99c:	0c003522 */ 	jal	dmaExecWithAutoAlign
/*  f19b9a0:	24060068 */ 	addiu	$a2,$zero,0x68
/*  f19b9a4:	8fac0050 */ 	lw	$t4,0x50($sp)
/*  f19b9a8:	8fb901c8 */ 	lw	$t9,0x1c8($sp)
/*  f19b9ac:	27a4005c */ 	addiu	$a0,$sp,0x5c
/*  f19b9b0:	000c68c0 */ 	sll	$t5,$t4,0x3
/*  f19b9b4:	03ad1821 */ 	addu	$v1,$sp,$t5
/*  f19b9b8:	0019c080 */ 	sll	$t8,$t9,0x2
/*  f19b9bc:	8c630018 */ 	lw	$v1,0x18($v1)
/*  f19b9c0:	0319c021 */ 	addu	$t8,$t8,$t9
/*  f19b9c4:	0018c180 */ 	sll	$t8,$t8,0x6
/*  f19b9c8:	afa201c4 */ 	sw	$v0,0x1c4($sp)
/*  f19b9cc:	24060140 */ 	addiu	$a2,$zero,0x140
/*  f19b9d0:	0c003522 */ 	jal	dmaExecWithAutoAlign
/*  f19b9d4:	00782821 */ 	addu	$a1,$v1,$t8
/*  f19b9d8:	8fa701c4 */ 	lw	$a3,0x1c4($sp)
/*  f19b9dc:	00404825 */ 	or	$t1,$v0,$zero
/*  f19b9e0:	244e0138 */ 	addiu	$t6,$v0,0x138
/*  f19b9e4:	00e05025 */ 	or	$t2,$a3,$zero
.L0f19b9e8:
/*  f19b9e8:	89210000 */ 	lwl	$at,0x0($t1)
/*  f19b9ec:	99210003 */ 	lwr	$at,0x3($t1)
/*  f19b9f0:	2529000c */ 	addiu	$t1,$t1,0xc
/*  f19b9f4:	254a000c */ 	addiu	$t2,$t2,0xc
/*  f19b9f8:	a941005c */ 	swl	$at,0x5c($t2)
/*  f19b9fc:	b941005f */ 	swr	$at,0x5f($t2)
/*  f19ba00:	8921fff8 */ 	lwl	$at,-0x8($t1)
/*  f19ba04:	9921fffb */ 	lwr	$at,-0x5($t1)
/*  f19ba08:	a9410060 */ 	swl	$at,0x60($t2)
/*  f19ba0c:	b9410063 */ 	swr	$at,0x63($t2)
/*  f19ba10:	8921fffc */ 	lwl	$at,-0x4($t1)
/*  f19ba14:	9921ffff */ 	lwr	$at,-0x1($t1)
/*  f19ba18:	a9410064 */ 	swl	$at,0x64($t2)
/*  f19ba1c:	152efff2 */ 	bne	$t1,$t6,.L0f19b9e8
/*  f19ba20:	b9410067 */ 	swr	$at,0x67($t2)
/*  f19ba24:	89210000 */ 	lwl	$at,0x0($t1)
/*  f19ba28:	99210003 */ 	lwr	$at,0x3($t1)
/*  f19ba2c:	00e01025 */ 	or	$v0,$a3,$zero
/*  f19ba30:	a9410068 */ 	swl	$at,0x68($t2)
/*  f19ba34:	b941006b */ 	swr	$at,0x6b($t2)
/*  f19ba38:	892e0004 */ 	lwl	$t6,0x4($t1)
/*  f19ba3c:	992e0007 */ 	lwr	$t6,0x7($t1)
/*  f19ba40:	a94e006c */ 	swl	$t6,0x6c($t2)
/*  f19ba44:	b94e006f */ 	swr	$t6,0x6f($t2)
/*  f19ba48:	8fbf0014 */ 	lw	$ra,0x14($sp)
/*  f19ba4c:	27bd01c8 */ 	addiu	$sp,$sp,0x1c8
/*  f19ba50:	03e00008 */ 	jr	$ra
/*  f19ba54:	00000000 */ 	nop
);

// Mismatch because the arguments to an addu instruction are swapped.
// It's the addu for calculating &bank[confignum].
//struct mpconfigfull *challengeLoadConfig(s32 confignum, u8 *buffer, s32 len)
//{
//	struct mpconfigfull *mpconfig;
//	u8 buffer2[sizeof(struct mpstrings) + 40];
//	struct mpstrings *loadedstrings;
//	struct mpstrings *bank;
//	u32 language_id = langGetFileNumOffset();
//	extern struct mpconfig _mpconfigsSegmentRomStart[];
//	extern struct mpstrings _mpstringsESegmentRomStart;
//	extern struct mpstrings _mpstringsJSegmentRomStart;
//	extern struct mpstrings _mpstringsPSegmentRomStart;
//	extern struct mpstrings _mpstringsGSegmentRomStart;
//	extern struct mpstrings _mpstringsFSegmentRomStart;
//	extern struct mpstrings _mpstringsSSegmentRomStart;
//	extern struct mpstrings _mpstringsISegmentRomStart;
//	extern struct mpstrings _mpstringsESegmentRomEnd;
//	extern struct mpstrings _mpstringsJSegmentRomEnd;
//	extern struct mpstrings _mpstringsPSegmentRomEnd;
//	extern struct mpstrings _mpstringsGSegmentRomEnd;
//	extern struct mpstrings _mpstringsFSegmentRomEnd;
//	extern struct mpstrings _mpstringsSSegmentRomEnd;
//	extern struct mpstrings _mpstringsISegmentRomEnd;
//
//	struct mpstrings *banks[][2] = {
//		{ &_mpstringsESegmentRomStart, &_mpstringsESegmentRomEnd },
//		{ &_mpstringsJSegmentRomStart, &_mpstringsJSegmentRomEnd },
//		{ &_mpstringsPSegmentRomStart, &_mpstringsPSegmentRomEnd },
//		{ &_mpstringsGSegmentRomStart, &_mpstringsGSegmentRomEnd },
//		{ &_mpstringsFSegmentRomStart, &_mpstringsFSegmentRomEnd },
//		{ &_mpstringsSSegmentRomStart, &_mpstringsSSegmentRomEnd },
//		{ &_mpstringsISegmentRomStart, &_mpstringsISegmentRomEnd },
//	};
//
//	// Load mpconfigs
//	mpconfig = dmaExecWithAutoAlign(buffer, &_mpconfigsSegmentRomStart[confignum], sizeof(struct mpconfig));
//
//	// Load mpstrings
//	bank = banks[language_id][0];
//	loadedstrings = dmaExecWithAutoAlign(buffer2, &bank[confignum], sizeof(struct mpstrings));
//
//	mpconfig->strings = *loadedstrings;
//
//	return mpconfig;
//}

struct mpconfigfull *challengeLoad(s32 challengeindex, u8 *buffer, s32 len)
{
	return challengeLoadConfig(g_MpChallenges[challengeindex].confignum, buffer, len);
}

struct mpconfigfull *challengeLoadBySlot(s32 n, u8 *buffer, s32 len)
{
	s32 numavailable = 0;
	s32 challengeindex;

	for (challengeindex = 0; challengeindex != NUM_CHALLENGES; challengeindex++) {
		if (challengeIsAvailableToAnyPlayer(challengeindex)) {
			if (numavailable == n) {
				return challengeLoad(challengeindex, buffer, len);
			}

			numavailable++;
		}
	}

	return 0;
}

struct mpconfigfull *challengeLoadCurrent(u8 *buffer, s32 len)
{
	return challengeLoad(g_MpChallengeIndex, buffer, len);
}

/**
 * This is adding featurenum to the array, provided it's unique.
 */
s32 challengeForceUnlockFeature(s32 featurenum, u8 *array, s32 tail, s32 len)
{
	s32 i;

	for (i = 0; i < tail; i++) {
		if (array[i] == featurenum) {
			break;
		}
	}

	if (i >= tail && tail < len) {
		array[tail] = featurenum;
		tail++;
	}

	return tail;
}

s32 challengeForceUnlockSetupFeatures(struct mpsetup *setup, u8 *array, s32 len)
{
	s32 index = 0;
	s32 i;

	// Force unlock the weapons (if never held before)
	for (i = 0; i < 6; i++) {
		s32 featurenum = g_MpWeapons[setup->weapons[i]].unlockfeature;

		if (featurenum) {
			index = challengeForceUnlockFeature(featurenum, array, index, len);
		}
	}

	// Force unlock the stage
	for (i = 0; i < mpGetNumStages(); i++) {
		if (g_MpArenas[i].stagenum == setup->stagenum) {
			s32 featurenum = g_MpArenas[i].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, array, index, len);
			}
		}
	}

	// Force unlock the scenario
	if (setup->scenario <= MPSCENARIO_CAPTURETHECASE) {
		s32 featurenum = g_MpScenarioOverviews[setup->scenario].requirefeature;

		if (featurenum) {
			index = challengeForceUnlockFeature(featurenum, array, index, len);
		}
	}

	// Force unlock the scenario options
	if (setup->options & MPOPTION_ONEHITKILLS) {
		index = challengeForceUnlockFeature(MPFEATURE_ONEHITKILLS, array, index, len);
	}

	if (setup->options & (MPOPTION_SLOWMOTION_ON | MPOPTION_SLOWMOTION_SMART)) {
		index = challengeForceUnlockFeature(MPFEATURE_SLOWMOTION, array, index, len);
	}

	return index;
}

void challengeForceUnlockConfigFeatures(struct mpconfig *config, u8 *array, s32 len, s32 challengeindex)
{
	s32 index = challengeForceUnlockSetupFeatures(&config->setup, array, len);
	s32 featurenum;
	s32 numplayers;
	s32 i;

	for (i = 0; i < 8; i++) {
		s32 simtype = mpFindBotProfile(config->simulants[i].type, BOTDIFF_NORMAL);

		if (simtype >= 0) {
			featurenum = g_BotProfiles[simtype].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, array, index, len);
			}
		}

		for (numplayers = 0; numplayers < 4; numplayers++) {
			simtype = mpFindBotProfile(0, config->simulants[i].difficulties[numplayers]);

			if (simtype >= 0) {
				featurenum = g_BotProfiles[simtype].requirefeature;

				if (featurenum) {
					index = challengeForceUnlockFeature(featurenum, array, index, len);
				}
			}
		}

		if (config->simulants[i].mpbodynum < NUM_MPBODIES) {
			featurenum = g_MpBodies[config->simulants[i].mpbodynum].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, array, index, len);
			}
		}

		if (config->simulants[i].mpheadnum < NUM_MPHEADS) {
			featurenum = g_MpHeads[config->simulants[i].mpheadnum].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, array, index, len);
			}
		}
	}

#if VERSION >= VERSION_NTSC_1_0
	if (challengeindex >= 25) {
		index = challengeForceUnlockFeature(MPFEATURE_BOTDIFF_DARK, array, index, len);
	} else if (challengeindex >= 20) {
		index = challengeForceUnlockFeature(MPFEATURE_STAGE_CARPARK, array, index, len);
	} else if (challengeindex >= 15) {
		index = challengeForceUnlockFeature(MPFEATURE_SCENARIO_PAC, array, index, len);
	}

	if (challengeindex >= 10) {
		index = challengeForceUnlockFeature(MPFEATURE_8BOTS, array, index, len);
	}
#else
	if (challengeindex >= 10) {
		index = challengeForceUnlockFeature(MPFEATURE_8BOTS, array, index, len);
	}

	if (challengeindex >= 15) {
		index = challengeForceUnlockFeature(MPFEATURE_SCENARIO_PAC, array, index, len);
	}

	if (challengeindex >= 20) {
		index = challengeForceUnlockFeature(MPFEATURE_STAGE_CARPARK, array, index, len);
	}
#endif

	// Clear the remainder of the array
	for (i = index; i < len; i++) {
		array[i] = 0;
	}
}

void challengeForceUnlockBotFeatures(void)
{
	s32 numsims = 0;
	s32 index = challengeForceUnlockSetupFeatures(&g_MpSetup, g_MpFeaturesForceUnlocked, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
	s32 i;

	for (i = 0; i < 8; i++) {
		// Force unlock the simulant type
		s32 simtypeindex = mpFindBotProfile(g_BotConfigsArray[i].type, BOTDIFF_NORMAL);

		if (simtypeindex >= 0) {
			s32 featurenum = g_BotProfiles[simtypeindex].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}

		// Force unlock the simulant difficulty
		simtypeindex = mpFindBotProfile(BOTTYPE_GENERAL, g_BotConfigsArray[i].difficulty);

		if (simtypeindex >= 0) {
			s32 featurenum = g_BotProfiles[simtypeindex].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}

		if (simtypeindex >= 0) {
			numsims++;
		}

		// Force unlock the simulant's body
		if (g_BotConfigsArray[i].base.mpbodynum < NUM_MPBODIES) {
			s32 featurenum = g_MpBodies[g_BotConfigsArray[i].base.mpbodynum].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}

		// Force unlock the simulant's head
		if (g_BotConfigsArray[i].base.mpheadnum < NUM_MPHEADS) {
			s32 featurenum = g_MpHeads[g_BotConfigsArray[i].base.mpheadnum].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}
	}

	// Force unlock 8 simulants
	if (numsims > 4) {
		index = challengeForceUnlockFeature(MPFEATURE_8BOTS, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
	}

	// Clear the remainder of the array
	for (i = index; i < ARRAYCOUNT(g_MpFeaturesForceUnlocked); i++) {
		g_MpFeaturesForceUnlocked[i] = 0;
	}

	challengeDetermineUnlockedFeatures();
}

void challengeRemoveForceUnlocks(void)
{
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_MpFeaturesForceUnlocked); i++) {
		g_MpFeaturesForceUnlocked[i] = 0;
	}

	challengeDetermineUnlockedFeatures();
}

void challengeApply(void)
{
	s32 i;
	u8 buffer[458];

	mpApplyConfig(challengeLoadCurrent(buffer, 458));
	mpSetLock(MPLOCKTYPE_CHALLENGE, 5);

	for (i = 0; i < 4; i++) {
		g_PlayerConfigsArray[i].base.team = 0;
	}
}

s32 challengeRemovePlayerLock(void)
{
	return mpSetLock(MPLOCKTYPE_NONE, 0);
}

void challengeLoadAndStoreCurrent(u8 *buffer, s32 len)
{
	g_MpCurrentChallengeConfig = challengeLoadCurrent(buffer, len);
}

void challengeUnsetCurrent(void)
{
	g_MpCurrentChallengeConfig = NULL;
}

bool challengeIsLoaded(void)
{
	return g_MpCurrentChallengeConfig != NULL;
}

char *challengeGetCurrentDescription(void)
{
	if (g_MpCurrentChallengeConfig) {
		return g_MpCurrentChallengeConfig->strings.description;
	}

	return "";
}

char *challengeGetConfigDescription(struct mpconfigfull *mpconfig)
{
	if (mpconfig) {
		return mpconfig->strings.description;
	}

	return "";
}

/**
 * Return the index of the first incomplete challenge, but if it's in the first
 * 4 then return index 4 because the first 4 are always shown.
 */
s32 challengeGetAutoFocusedIndex(s32 mpchrnum)
{
	s32 challengeindex;
	s32 index = 0;

	for (challengeindex = 29; challengeindex >= 0; challengeindex--) {
		if (challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 1) ||
				challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 2) ||
				challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 3) ||
				challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 4)) {
			index = challengeindex + 1;
			break;
		}
	}

	if (index < 4) {
		index = 4;
	}

	return index;
}

char *xhallengeGetName(s32 arg0, s32 challengeindex)
{
	return langGet(g_MpChallenges[challengeindex].name);
}

bool challengeIsCompletedByPlayerWithNumPlayers2(s32 mpchrnum, s32 index, s32 numplayers)
{
	return challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, index, numplayers);
}

bool challengeIsCompletedByAnyPlayerWithNumPlayers(s32 index, s32 numplayers)
{
	return (g_MpChallenges[index].completions[numplayers - 1] & 1) != 0;
}

void challengeSetCompletedByAnyPlayerWithNumPlayers(s32 index, s32 numplayers, bool completed)
{
	if (completed) {
		g_MpChallenges[index].completions[numplayers - 1] |= 1;
		return;
	}

	g_MpChallenges[index].completions[numplayers - 1] &= ~1;
}

bool challengeIsCompletedByPlayerWithNumPlayers(s32 mpchrnum, s32 index, s32 numplayers)
{
	return (g_MpChallenges[index].completions[numplayers - 1] & (2 << mpchrnum)) != 0;
}

void challengeSetCompletedByPlayerWithNumPlayers(u32 mpchrnum, s32 index, s32 numplayers, bool completed)
{
	if (completed) {
		g_MpChallenges[index].completions[numplayers - 1] |= 2 << mpchrnum;
		return;
	}

	g_MpChallenges[index].completions[numplayers - 1] &= ~(2 << mpchrnum);
}

bool challengeIsCompleteForEndscreen(void)
{
	s32 prevplayernum = g_Vars.currentplayernum;
	s32 result = false;
	s32 aborted = false;
	s32 i;
	u32 stack;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		setCurrentPlayerNum(i);

		if (g_Vars.currentplayer->aborted) {
			aborted = true;
		}
	}

	setCurrentPlayerNum(prevplayernum);

	if (!aborted) {
		struct ranking rankings[12];
		mpGetTeamRankings(rankings);

		if (rankings[0].teamnum == 0) {
			result = true;
		}
	}

	return result;
}

void challengeConsiderMarkingComplete(void)
{
	bool result = challengeIsCompleteForEndscreen();

#if VERSION == VERSION_PAL_BETA
	if ((g_CheatsActiveBank0 == 0 && g_CheatsActiveBank1 == 0) && (result || debugIsSetCompleteEnabled()))
#elif VERSION >= VERSION_NTSC_1_0
	if (g_CheatsActiveBank0 == 0 && g_CheatsActiveBank1 == 0 && result)
#else
	if (result && g_CheatsActiveBank0 == 0 && g_CheatsActiveBank1 == 0)
#endif
	{
		u32 prevplayernum;
		s32 i;

		challengeSetCompletedByAnyPlayerWithNumPlayers(g_MpChallengeIndex, PLAYERCOUNT(), 1);
		prevplayernum = g_Vars.currentplayernum;

		for (i = 0; i < PLAYERCOUNT(); i++) {
			setCurrentPlayerNum(i);
			challengeSetCompletedByPlayerWithNumPlayers(g_Vars.currentplayerstats->mpindex, g_MpChallengeIndex, PLAYERCOUNT(), true);
		}

		setCurrentPlayerNum(prevplayernum);
		challengeDetermineUnlockedFeatures();
	}
}

bool challengeIsFeatureUnlocked(s32 featurenum)
{
	if (featurenum == 0) {
		return true;
	}

	return (g_MpFeaturesUnlocked[featurenum] & 1) != 0;
}

bool challengeIsFeatureUnlockedByPlayer(u32 numplayers, s32 featurenum)
{
	if (featurenum == 0) {
		return true;
	}

	return (g_MpFeaturesUnlocked[featurenum] & (2 << numplayers)) != 0;
}

bool challengeIsFeatureUnlockedByDefault(s32 featurenum)
{
	if (featurenum) {
		return false;
	}

	return true;
}
