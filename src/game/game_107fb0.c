#include <ultra64.h>
#include "constants.h"
#include "game/data/data_000000.h"
#include "game/data/data_0083d0.h"
#include "game/data/data_00e460.h"
#include "game/data/data_0160b0.h"
#include "game/data/data_01a3a0.h"
#include "game/data/data_020df0.h"
#include "game/data/data_02da90.h"
#include "game/game_0b3350.h"
#include "game/game_0d4690.h"
#include "game/game_0f09f0.h"
#include "game/game_107fb0.h"
#include "game/game_1531a0.h"
#include "game/gamefile.h"
#include "game/lang.h"
#include "game/mplayer.h"
#include "game/pak/pak.h"
#include "game/utils.h"
#include "gvars/gvars.h"
#include "lib/lib_09660.h"
#include "lib/lib_126b0.h"
#include "lib/lib_13130.h"
#include "lib/lib_13900.h"
#include "types.h"

char *filemanGetDeviceName(s32 index)
{
	u16 names[] = {
		L_OPTIONS(112), // "Controller Pak 1"
		L_OPTIONS(113), // "Controller Pak 2"
		L_OPTIONS(114), // "Controller Pak 3"
		L_OPTIONS(115), // "Controller Pak 4"
		L_OPTIONS(111), // "Game Pak"
		L_MPWEAPONS(229), // "Controller Pak Not Found"
	};

	if (index < ARRAYCOUNT(names)) {
		return langGet(names[index]);
	}

	return NULL;
}

s32 filemanDeviceNameMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_CHECKHIDDEN) {
		if ((g_Menus[g_MpPlayerNum].unke3c & 0x7f) > 4) {
			return true;
		}
	}

	return 0;
}

char *filemanMenuTextDeviceName(struct menuitem *item)
{
	return filemanGetDeviceName(g_Menus[g_MpPlayerNum].unke3c & 0x7f);
}

/**
 * Concatenates an MP player name with the amount of time played
 * for displaying in the copy/delete MP player menus.
 *
 * Suspected that arg2 is a save file type (solo, MP game, MP player)
 * and that this builds the title for whatever the file type is.
 */
void func0f1080d0(char *buffer, struct savelocation000 *arg1, u32 arg2)
{
	s32 days;
	char tmpbuffer1[28];
	char namebuffer[20];
	u32 totalinseconds;
	s32 pos;
	s32 hours;
	s32 minutes;
	s32 seconds;
	s32 totalinhours;

	switch (arg2) {
	case 0:
	case 1:
		func0f0d564c(arg1->unk06, tmpbuffer1, 0);
		break;
	case 2:
		func0f18d9a4(arg1->unk06, namebuffer, &totalinseconds);
		pos = sprintf(tmpbuffer1, "%s-", namebuffer);

		if (totalinseconds >= 0x7ffffff) { // about 4.25 years
			sprintf(tmpbuffer1 + pos, "==:==");
		} else {
			seconds = totalinseconds % 60;
			totalinseconds = totalinseconds / 60;
			totalinhours = totalinseconds / 60;
			minutes = totalinseconds % 60;
			days = totalinhours / 24;
			hours = totalinhours % 24;

			if (days == 0) {
				// seconds is passed but has no placeholder
				sprintf(tmpbuffer1 + pos, "%d:%02d", hours, minutes, seconds);
			} else {
				sprintf(tmpbuffer1 + pos, "%d:%02d:%02d", days, hours, minutes);
			}
		}
		break;
	}

	sprintf(buffer, "%s\n", tmpbuffer1);
}

const char var7f1b2f28[] = "Setup: item = %x\n";

const u32 var7f1b2f3c[] = {0xa0, 0x31, 0x4e, 0x4a0};

s32 filemanFileNameMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_CHECKHIDDEN) {
		if (g_Menus[g_MpPlayerNum].unke38 == 0) {
			return true;
		}
	}

	return 0;
}

char *filemanMenuTextDeleteFileName(struct menuitem *item)
{
	if (g_Menus[g_MpPlayerNum].unke38) {
		func0f1080d0(g_StringPointer,
				g_Menus[g_MpPlayerNum].unke38,
				g_Menus[g_MpPlayerNum].unke3d);
		return g_StringPointer;
	}

	return NULL;
}

void func0f108324(s32 arg0)
{
	s32 value = func0f11e6b0(arg0);

	if (value >= 0) {
		g_Menus[g_MpPlayerNum].unke3c = value;
	} else {
		g_Menus[g_MpPlayerNum].unke3c = 5;
	}
}

void func0f1083b0(struct savelocation000 *arg0)
{
	func0f108324(arg0->unk04);
}

void func0f1083d0(struct savelocation000 *arg0, s32 filetype)
{
	g_Menus[g_MpPlayerNum].unke3d = filetype;
	g_Menus[g_MpPlayerNum].unke38 = arg0;
	func0f1083b0(arg0);
}

u16 g_PakFailReasons[] = {
	L_OPTIONS(322), // "The Controller Pak was not found in any controller."
	L_OPTIONS(323), // "File was not saved."
	L_OPTIONS(324), // "File would not load."
	L_OPTIONS(325), // "Could not delete the file."
	L_OPTIONS(326), // "Out of memory."
	L_OPTIONS(327), // "This player is already loaded for this game."
	L_OPTIONS(328), // "has been removed."
	L_OPTIONS(329), // "Controller Pak is damaged or incorrectly inserted."
	L_OPTIONS(330), // "Game note delete failed."
};

char *filemanMenuTextFailReason(struct menuitem *item)
{
	return langGet(g_PakFailReasons[g_Menus[g_MpPlayerNum].errno]);
}

/**
 * Unused.
 */
char *func0f108484(struct menuitem *item)
{
	sprintf(g_StringPointer, "location: controller pak 1\n");
	return g_StringPointer;
}

s32 filemanDeviceNameForErrorMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_CHECKHIDDEN) {
		if ((g_Menus[g_MpPlayerNum].unke3c & 0x7f) > 4) {
			return 1;
		}

		switch (g_Menus[g_MpPlayerNum].errno) {
		case FILEERROR_OUTOFMEMORY:
		case FILEERROR_ALREADYLOADED:
		case FILEERROR_PAKDAMAGED:
		default:
			return true;
		case FILEERROR_SAVEFAILED:
		case FILEERROR_LOADFAILED:
		case FILEERROR_DELETEFAILED:
		case FILEERROR_PAKREMOVED:
		case FILEERROR_DELETENOTEFAILED:
			break;
		}
	}

	return false;
}

char *filemanMenuTextDeviceNameForError(struct menuitem *item)
{
	sprintf(g_StringPointer, "%s", filemanGetDeviceName(g_Menus[g_MpPlayerNum].unke3c & 0x7f));

	if (g_Menus[g_MpPlayerNum].errno != FILEERROR_PAKREMOVED) {
		s32 i = 0;

		while (g_StringPointer[i] != '\0') {
			i++;
		}

		g_StringPointer[i - 1] = ':';
		g_StringPointer[i] = '\n';
		g_StringPointer[i + 1] = '\0';
	}

	return g_StringPointer;
}

const char var7f1b2f6c[] = "FileMan: Failure Handler\n";
const char var7f1b2f88[] = "Copy Memory Freed\n";
const char var7f1b2f9c[] = "FileMan: Success Handler\n";
const char var7f1b2fb8[] = "Copy Memory Freed\n";
const char var7f1b2fcc[] = ">> block read going write, target file is %x-%x\n";
const char var7f1b3000[] = "SaveElsewhere\n";
const char var7f1b3010[] = "DELETING: %x-%x\n";
const char var7f1b3024[] = "MyResult: %d\n";
const char var7f1b3034[] = "PakOperationSearch>> Search for pak: %x = %d\n";

void filemanPushErrorDialog(u16 errno)
{
	g_Menus[g_MpPlayerNum].errno = errno;

	menuPushDialog(&g_FilemanErrorMenuDialog);
}

struct menuitem g_FilemanErrorMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&filemanMenuTextDeviceNameForError, 0x00000000, filemanDeviceNameForErrorMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&filemanMenuTextFailReason, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000028, L_OPTIONS(321), 0x00000000, NULL }, // "Cancel"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanErrorMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	L_OPTIONS(320), // "Error"
	g_FilemanErrorMenuItems,
	NULL,
	0x00000080,
	NULL,
};

/**
 * This function is a bit weird. It can return a pointer to a string
 * or return a regular s8 (ie. not a pointer).
 */
s32 func0f1086b8(s32 arg0, s32 arg1, s32 arg2)
{
	u16 names[] = {
		L_OPTIONS(111), // "Game Pak"
		L_OPTIONS(112), // "Controller Pak 1"
		L_OPTIONS(113), // "Controller Pak 2"
		L_OPTIONS(114), // "Controller Pak 3"
		L_OPTIONS(115), // "Controller Pak 4"
	};

	s32 i;
	s32 remaining = arg2;

	for (i = 0; i < ARRAYCOUNT(names); i++) {
		if (g_SaveLocations.locations[arg0]->unk300[i] != -1) {
			if (remaining == 0) {
				if (arg1 == 4) {
					return (s32)langGet(names[i]);
				}

				return g_SaveLocations.locations[arg0]->unk300[i];
			}

			remaining--;
		}
	}

	return 0;
}

char *filemanMenuTextErrorTitle(struct menuitem *item)
{
	u16 messages[] = {
		L_OPTIONS(331), // "Error Loading Game"
		L_OPTIONS(332), // "Error Saving Game"
		L_OPTIONS(333), // "Error Loading Player"
		L_OPTIONS(334), // "Error Saving Player"
		L_OPTIONS(335), // "Error Loading PerfectHead"
		L_OPTIONS(336), // "Error Saving PerfectHead"
		L_OPTIONS(337), // "Error Reading File"
		L_OPTIONS(338), // "Error Writing File"
		L_OPTIONS(339), // "Error"
	};

	switch (g_Menus[g_MpPlayerNum].unke42) {
	case 100:
	case 102:
		return langGet(messages[0]);
	case 0:
	case 1:
	case 2:
	case 4:
		return langGet(messages[1]);
	case 101:
		return langGet(messages[2]);
	case 3:
		return langGet(messages[3]);
	case 104:
	case 105:
	case 106:
		return langGet(messages[6]);
	case 6:
	case 7:
	case 8:
		return langGet(messages[7]);
	}

	return langGet(messages[8]);
}

char *filemanMenuTextFileType(struct menuitem *item)
{
	u16 names[] = {
		L_OPTIONS(103), // "Single Player Agent File"
		L_OPTIONS(104), // "Combat Simulator Settings File"
		L_OPTIONS(105), // "Combat Simulator Player File"
		L_OPTIONS(106), // "PerfectHead Files"
	};

	switch (g_Menus[g_MpPlayerNum].unke42) {
	case 0:
	case 1:
	case 2:
	case 6:
	case 100:
	case 104:
		return langGet(names[0]);
	case 4:
	case 7:
	case 102:
	case 105:
		return langGet(names[1]);
	case 3:
	case 8:
	case 101:
	case 106:
		return langGet(names[2]);
	}

	return langGet(names[0]);
}

void func0f10898c(void)
{
	func0f0f0ca0(-1, false);

	switch (g_Menus[g_MpPlayerNum].unke42) {
	case 6:
	case 7:
	case 8:
	case 104:
	case 105:
	case 106:
		func00012cb4(g_Menus[g_MpPlayerNum].unke44, align16(var7f1b2f3c[g_Menus[g_MpPlayerNum].data.pak.unke1c - 1]));
		break;
	case 100:
	case 101:
	case 102:
	case 103:
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		break;
	}
}

void func0f108a80(void)
{
	func0f0f0ca0(-1, false);

	switch (g_Menus[g_MpPlayerNum].unke42) {
	case 6:
	case 7:
	case 8:
		func00012cb4(g_Menus[g_MpPlayerNum].unke44,
				align16(var7f1b2f3c[g_Menus[g_MpPlayerNum].data.pak.unke1c - 1]));
		break;
	case 100:
		g_Vars.unk00047c = g_Menus[g_MpPlayerNum].unke48;
		g_Vars.unk000480 = g_Menus[g_MpPlayerNum].unke4c;
		func0f1109c0();

		if (IS4MB()) {
			func0f0f820c(&g_4MbMainMenu, MENUROOT_4MBMAINMENU);
		} else {
			func0f0f820c(&g_MainMenuMenuDialog, MENUROOT_MAINMENU);
		}
		break;
	case 104:
	case 105:
	case 106:
		func0f1094e4(&var800a21e8,
				g_Menus[g_MpPlayerNum].unke42 - 98,
				g_Menus[g_MpPlayerNum].unke44);
		break;
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 101:
	case 102:
	case 103:
		break;
	}
}

s32 filemanRetrySaveMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		func0f109038(2);
	}

	return 0;
}

s32 filemanSaveElsewhereYesMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		s32 sp1c;

		menuCloseDialog();

		switch (g_Menus[g_MpPlayerNum].unke42) {
		case 0:
		case 1:
		case 2:
		case 6:
			sp1c = 0;
			break;
		case 3:
		case 8:
			sp1c = 2;
			break;
		case 4:
		case 7:
			sp1c = 1;
			break;
		}

		func0f10a51c(g_Menus[g_MpPlayerNum].unke42 + 9, sp1c);
	}

	return 0;
}

s32 filemanCancelSave2MenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		menuCloseDialog();
		func0f10898c();
		menuUpdateCurFrame();
	}

	return 0;
}

/**
 * Unused.
 */
s32 func0f108d14(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		func0f109038(2);
	}

	return 0;
}

s32 filemanAcknowledgeFileLostMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		menuCloseDialog();
		func0f10898c();
		menuUpdateCurFrame();
	}

	return 0;
}

void filemanEraseCorruptFile(void)
{
	s32 value;
	s32 i;

	value = func0f11e6b0(g_Menus[g_MpPlayerNum].unke4c);

	if (value >= 0) {
		func0f116860(value, g_Menus[g_MpPlayerNum].unke48);
	}

	for (i = 0; i < 4; i++) {
		if (g_SaveLocations.locations[i]) {
			g_SaveLocations.locations[i]->unk30c = 1;
		}
	}

	menuPushDialog(&g_FilemanFileLostMenuDialog);
}

s32 filemanInsertOriginalPakMenuDialog(u32 operation, struct menudialog *dialog, union handlerdata *data)
{
	if (operation == MENUOP_TICK) {
		if (g_Menus[g_MpPlayerNum].curframe &&
				g_Menus[g_MpPlayerNum].curframe->dialog == dialog) {
			func0f109038(0);
		}
	}

	return false;
}

s32 filemanReinsertedOkMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		func0f11c7a0();
		func0f109038(1);
	}

	return 0;
}

s32 filemanReinsertedCancelMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		if (g_Menus[g_MpPlayerNum].unke42 < 100 && g_Menus[g_MpPlayerNum].unke42 != 1) {
			func0f0f3704(&g_FilemanSaveElsewhereMenuDialog);
		} else {
			menuPopDialog();
		}
	}

	return 0;
}

char *filemanMenuTextInsertOriginalPak(struct menuitem *item)
{
	char fullbuffer[100];
	char namebuffer[100];
	s32 i;

	sprintf(namebuffer, filemanMenuTextFileType(item));

	// Replace first line break in namebuffer with a terminator
	i = 0;

	while (namebuffer[i] != '\0') {
		if (namebuffer[i] == '\n') {
			namebuffer[i] = '\0';
		} else {
			i++;
		}
	}

	// "Please insert the Controller Pak containing your %s into any controller."
	sprintf(fullbuffer, langGet(L_OPTIONS(363)), namebuffer);

	textWrap(120, fullbuffer, g_StringPointer, var8007fb10, var8007fb0c);

	return g_StringPointer;
}

void func0f109038(s32 arg0)
{
	s32 value = func0f11e6b0(g_Menus[g_MpPlayerNum].unke4c);

	if (value == -1) {
		if (arg0 == 1) {
			filemanPushErrorDialog(FILEERROR_NOPAK);
		}

		if (arg0 == 2) {
			func0f0f3704(&g_FilemanInsertOriginalPakMenuDialog);
		}
	} else if (fileSave(value, true)) {
		if (arg0 == 2) {
			g_Menus[g_MpPlayerNum].unke3c = value;

			if (g_Menus[g_MpPlayerNum].unke42 < 100) {
				filemanPushErrorDialog(FILEERROR_SAVEFAILED);
			} else {
				filemanPushErrorDialog(FILEERROR_LOADFAILED);
			}
		} else {
			func0f108324(g_Menus[g_MpPlayerNum].unke4c);

			if (g_Menus[g_MpPlayerNum].unke42 < 100) {
				func0f0f3704(&g_FilemanSaveErrorMenuDialog);
			} else {
				filemanEraseCorruptFile();
			}
		}
	}
}

bool fileSave(s32 arg0, bool arg1)
{
	s32 errno = 0;
	u32 sp40 = (g_Menus[g_MpPlayerNum].unke50 & 1) != 0;

	const s32 sp30[] = {0x80, 0x40, 0x20, 0x08};
	s32 sp2c;

	switch (g_Menus[g_MpPlayerNum].unke42) {
	case 2:
		sp40 = 1;
		// fall through
	case 0:
	case 1:
		errno = func0f10feac(arg0,
				g_Menus[g_MpPlayerNum].unke48,
				g_Menus[g_MpPlayerNum].unke4c);
		break;
	case 3:
		errno = func0f18d9fc(
				g_Menus[g_MpPlayerNum].unke44, arg0,
				g_Menus[g_MpPlayerNum].unke48,
				g_Menus[g_MpPlayerNum].unke4c);
		break;
	case 4:
		errno = func0f18e420(arg0,
				g_Menus[g_MpPlayerNum].unke48,
				g_Menus[g_MpPlayerNum].unke4c);
		sp40 = 1;
		break;
	case 6:
	case 7:
	case 8:
		{
			sp2c = 0;
			func0f0d5690(g_Menus[g_MpPlayerNum].unke44, g_Menus[g_MpPlayerNum].unke53);
			errno = func0f116828(arg0,
					g_Menus[g_MpPlayerNum].unke48,
					sp30[g_Menus[g_MpPlayerNum].unke42 - 6],
					g_Menus[g_MpPlayerNum].unke44, &sp2c, 0);
			g_SaveLocations.unk10[g_Menus[g_MpPlayerNum].unke42 - 6] = 1;
		}
		break;
	case 100:
		errno = func0f10fac8(arg0);
		break;
	case 101:
		errno = func0f18dac0(
				g_Menus[g_MpPlayerNum].unke44,
				arg0,
				g_Menus[g_MpPlayerNum].unke48,
				g_Menus[g_MpPlayerNum].unke4c);
		break;
	case 102:
		errno = func0f18e4c8(arg0,
				g_Menus[g_MpPlayerNum].unke48,
				g_Menus[g_MpPlayerNum].unke4c);
		break;
	case 104:
	case 105:
	case 106:
		errno = func0f116800(arg0,
				g_Menus[g_MpPlayerNum].unke48,
				g_Menus[g_MpPlayerNum].unke44, 0);
		break;
	}

	if (errno == 0 && arg1) {
		menuCloseDialog();
	}

	if (g_Menus[g_MpPlayerNum].unke42 < 100) {
		if (errno == 0) {
			func0f108a80();
		}

		if (sp40 && errno == 0) {
			menuPushDialog(&g_FilemanFileSavedMenuDialog);
		}
	} else {
		if (errno == 0) {
			func0f108a80();
		}
	}

	menuUpdateCurFrame();

	return errno;
}

const char var7f1b3074[] = "FileAttemptOperation - pak %d op %d\n";
const char var7f1b309c[] = ">> blockWrite: file:%x pak:%x\n";
const char var7f1b30bc[] = ">> blockRead: file:%x pak:%x\n";
const char var7f1b30dc[] = "SUCCESS**->%d\n";
const char var7f1b30ec[] = "SUCCESS**->%d\n";
const char var7f1b30fc[] = ">>>>>>>>>>>>> FileWrite: type %d <<<<<<<<<<<<<<<\n";
const char var7f1b3130[] = "!!!!!!!!!!!! Wanted: %d\n";
const char var7f1b314c[] = "&&&&&&&&&&&&&&&&&&&&&&&&&&&PASSED**************\n";
const char var7f1b3180[] = "guid: %x gives pakno: %d\n";
const char var7f1b319c[] = "Invalidating pak %d\n";
const char var7f1b31b4[] = "YOUR TARGET: %x-%x\n";
const char var7f1b31c8[] = "MyResult: %d\n";
const char var7f1b31d8[] = "COULD NOT DELETE\n";
const char var7f1b31ec[] = "Multiplayer %d was using that file...\n";

GLOBAL_ASM(
glabel func0f1094e4
/*  f1094e4:	27bdffe8 */ 	addiu	$sp,$sp,-24
/*  f1094e8:	2403ffff */ 	addiu	$v1,$zero,-1
/*  f1094ec:	10a30012 */ 	beq	$a1,$v1,.L0f109538
/*  f1094f0:	afbf0014 */ 	sw	$ra,0x14($sp)
/*  f1094f4:	3c0e8007 */ 	lui	$t6,%hi(g_MpPlayerNum)
/*  f1094f8:	8dce1448 */ 	lw	$t6,%lo(g_MpPlayerNum)($t6)
/*  f1094fc:	3c18800a */ 	lui	$t8,%hi(g_Menus)
/*  f109500:	2718e000 */ 	addiu	$t8,$t8,%lo(g_Menus)
/*  f109504:	000e78c0 */ 	sll	$t7,$t6,0x3
/*  f109508:	01ee7823 */ 	subu	$t7,$t7,$t6
/*  f10950c:	000f7880 */ 	sll	$t7,$t7,0x2
/*  f109510:	01ee7821 */ 	addu	$t7,$t7,$t6
/*  f109514:	000f78c0 */ 	sll	$t7,$t7,0x3
/*  f109518:	01ee7823 */ 	subu	$t7,$t7,$t6
/*  f10951c:	000f7900 */ 	sll	$t7,$t7,0x4
/*  f109520:	01f81021 */ 	addu	$v0,$t7,$t8
/*  f109524:	a0450e42 */ 	sb	$a1,0xe42($v0)
/*  f109528:	ac460e44 */ 	sw	$a2,0xe44($v0)
/*  f10952c:	a4400e50 */ 	sh	$zero,0xe50($v0)
/*  f109530:	3c01800a */ 	lui	$at,%hi(var800a21f8)
/*  f109534:	ac2021f8 */ 	sw	$zero,%lo(var800a21f8)($at)
.L0f109538:
/*  f109538:	3c198007 */ 	lui	$t9,%hi(g_MpPlayerNum)
/*  f10953c:	8f391448 */ 	lw	$t9,%lo(g_MpPlayerNum)($t9)
/*  f109540:	8c8a0000 */ 	lw	$t2,0x0($a0)
/*  f109544:	3c09800a */ 	lui	$t1,%hi(g_Menus)
/*  f109548:	001940c0 */ 	sll	$t0,$t9,0x3
/*  f10954c:	01194023 */ 	subu	$t0,$t0,$t9
/*  f109550:	00084080 */ 	sll	$t0,$t0,0x2
/*  f109554:	01194021 */ 	addu	$t0,$t0,$t9
/*  f109558:	000840c0 */ 	sll	$t0,$t0,0x3
/*  f10955c:	01194023 */ 	subu	$t0,$t0,$t9
/*  f109560:	00084100 */ 	sll	$t0,$t0,0x4
/*  f109564:	2529e000 */ 	addiu	$t1,$t1,%lo(g_Menus)
/*  f109568:	01091021 */ 	addu	$v0,$t0,$t1
/*  f10956c:	ac4a0e48 */ 	sw	$t2,0xe48($v0)
/*  f109570:	948b0004 */ 	lhu	$t3,0x4($a0)
/*  f109574:	ac4b0e4c */ 	sw	$t3,0xe4c($v0)
/*  f109578:	0fc479ac */ 	jal	func0f11e6b0
/*  f10957c:	01602025 */ 	or	$a0,$t3,$zero
/*  f109580:	2403ffff */ 	addiu	$v1,$zero,-1
/*  f109584:	14430015 */ 	bne	$v0,$v1,.L0f1095dc
/*  f109588:	00402025 */ 	or	$a0,$v0,$zero
/*  f10958c:	3c0c8007 */ 	lui	$t4,%hi(g_MpPlayerNum)
/*  f109590:	8d8c1448 */ 	lw	$t4,%lo(g_MpPlayerNum)($t4)
/*  f109594:	3c0e800a */ 	lui	$t6,%hi(g_Menus)
/*  f109598:	25cee000 */ 	addiu	$t6,$t6,%lo(g_Menus)
/*  f10959c:	000c68c0 */ 	sll	$t5,$t4,0x3
/*  f1095a0:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f1095a4:	000d6880 */ 	sll	$t5,$t5,0x2
/*  f1095a8:	01ac6821 */ 	addu	$t5,$t5,$t4
/*  f1095ac:	000d68c0 */ 	sll	$t5,$t5,0x3
/*  f1095b0:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f1095b4:	000d6900 */ 	sll	$t5,$t5,0x4
/*  f1095b8:	01ae1021 */ 	addu	$v0,$t5,$t6
/*  f1095bc:	944f0e50 */ 	lhu	$t7,0xe50($v0)
/*  f1095c0:	3c048007 */ 	lui	$a0,%hi(g_FilemanInsertOriginalPakMenuDialog)
/*  f1095c4:	24844634 */ 	addiu	$a0,$a0,%lo(g_FilemanInsertOriginalPakMenuDialog)
/*  f1095c8:	35f80001 */ 	ori	$t8,$t7,0x1
/*  f1095cc:	0fc3cbd3 */ 	jal	menuPushDialog
/*  f1095d0:	a4580e50 */ 	sh	$t8,0xe50($v0)
/*  f1095d4:	1000002e */ 	b	.L0f109690
/*  f1095d8:	00001025 */ 	or	$v0,$zero,$zero
.L0f1095dc:
/*  f1095dc:	0fc42478 */ 	jal	fileSave
/*  f1095e0:	00002825 */ 	or	$a1,$zero,$zero
/*  f1095e4:	10400029 */ 	beqz	$v0,.L0f10968c
/*  f1095e8:	3c198007 */ 	lui	$t9,%hi(g_MpPlayerNum)
/*  f1095ec:	8f391448 */ 	lw	$t9,%lo(g_MpPlayerNum)($t9)
/*  f1095f0:	3c09800a */ 	lui	$t1,%hi(g_Menus)
/*  f1095f4:	2529e000 */ 	addiu	$t1,$t1,%lo(g_Menus)
/*  f1095f8:	001940c0 */ 	sll	$t0,$t9,0x3
/*  f1095fc:	01194023 */ 	subu	$t0,$t0,$t9
/*  f109600:	00084080 */ 	sll	$t0,$t0,0x2
/*  f109604:	01194021 */ 	addu	$t0,$t0,$t9
/*  f109608:	000840c0 */ 	sll	$t0,$t0,0x3
/*  f10960c:	01194023 */ 	subu	$t0,$t0,$t9
/*  f109610:	00084100 */ 	sll	$t0,$t0,0x4
/*  f109614:	01091021 */ 	addu	$v0,$t0,$t1
/*  f109618:	944a0e50 */ 	lhu	$t2,0xe50($v0)
/*  f10961c:	8c440e4c */ 	lw	$a0,0xe4c($v0)
/*  f109620:	354b0001 */ 	ori	$t3,$t2,0x1
/*  f109624:	0fc420c9 */ 	jal	func0f108324
/*  f109628:	a44b0e50 */ 	sh	$t3,0xe50($v0)
/*  f10962c:	3c0c8007 */ 	lui	$t4,%hi(g_MpPlayerNum)
/*  f109630:	8d8c1448 */ 	lw	$t4,%lo(g_MpPlayerNum)($t4)
/*  f109634:	3c0e800a */ 	lui	$t6,%hi(g_Menus+0xe42)
/*  f109638:	3c048007 */ 	lui	$a0,%hi(g_FilemanSaveErrorMenuDialog)
/*  f10963c:	000c68c0 */ 	sll	$t5,$t4,0x3
/*  f109640:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f109644:	000d6880 */ 	sll	$t5,$t5,0x2
/*  f109648:	01ac6821 */ 	addu	$t5,$t5,$t4
/*  f10964c:	000d68c0 */ 	sll	$t5,$t5,0x3
/*  f109650:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f109654:	000d6900 */ 	sll	$t5,$t5,0x4
/*  f109658:	01cd7021 */ 	addu	$t6,$t6,$t5
/*  f10965c:	91ceee42 */ 	lbu	$t6,%lo(g_Menus+0xe42)($t6)
/*  f109660:	29c10064 */ 	slti	$at,$t6,0x64
/*  f109664:	10200005 */ 	beqz	$at,.L0f10967c
/*  f109668:	00000000 */ 	nop
/*  f10966c:	0fc3cbd3 */ 	jal	menuPushDialog
/*  f109670:	248444fc */ 	addiu	$a0,$a0,%lo(g_FilemanSaveErrorMenuDialog)
/*  f109674:	10000006 */ 	b	.L0f109690
/*  f109678:	00001025 */ 	or	$v0,$zero,$zero
.L0f10967c:
/*  f10967c:	0fc42363 */ 	jal	filemanEraseCorruptFile
/*  f109680:	00000000 */ 	nop
/*  f109684:	10000002 */ 	b	.L0f109690
/*  f109688:	00001025 */ 	or	$v0,$zero,$zero
.L0f10968c:
/*  f10968c:	24020001 */ 	addiu	$v0,$zero,0x1
.L0f109690:
/*  f109690:	8fbf0014 */ 	lw	$ra,0x14($sp)
/*  f109694:	27bd0018 */ 	addiu	$sp,$sp,0x18
/*  f109698:	03e00008 */ 	jr	$ra
/*  f10969c:	00000000 */ 	nop
);

// Mismatch when preparing argument for func0f11e6b0
//bool func0f1094e4(struct savelocation_2d8 *arg0, s32 arg1, void *arg2)
//{
//	s32 value;
//
//	if (arg1 != -1) {
//		g_Menus[g_MpPlayerNum].unke42 = arg1;
//		g_Menus[g_MpPlayerNum].unke44 = arg2;
//		g_Menus[g_MpPlayerNum].unke50 = 0;
//		var800a21f8.unk00 = 0;
//	}
//
//	g_Menus[g_MpPlayerNum].unke48 = arg0->unk00;
//	g_Menus[g_MpPlayerNum].unke4c = arg0->unk04;
//
//	value = func0f11e6b0(g_Menus[g_MpPlayerNum].unke4c);
//
//	if (value == -1) {
//		g_Menus[g_MpPlayerNum].unke50 |= 1;
//		menuPushDialog(&g_FilemanInsertOriginalPakMenuDialog);
//		return false;
//	}
//
//	if (fileSave(value, false)) {
//		g_Menus[g_MpPlayerNum].unke50 |= 1;
//		func0f108324(g_Menus[g_MpPlayerNum].unke4c);
//
//		if (g_Menus[g_MpPlayerNum].unke42 < 100) {
//			menuPushDialog(&g_FilemanSaveErrorMenuDialog);
//		} else {
//			filemanEraseCorruptFile();
//		}
//
//		return false;
//	}
//
//	return true;
//}

void filemanDeleteFile(void)
{
	bool error = false;
	s8 value = func0f11e6b0(var800a21f0.unk04);
	s32 i;

	if (value >= 0) {
		if (func0f116860(value, var800a21f0.unk00)) {
			error = true;
		}
	} else {
		error = true;
	}

	g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk30c = true;

	if (error) {
		g_Menus[g_MpPlayerNum].unke3c = value;
		filemanPushErrorDialog(FILEERROR_DELETEFAILED);
	} else {
		for (i = 0; i < 4; i++) {
			if (var800a21f0.unk00 == g_MpPlayers[i].unk4c.unk00
					&& var800a21f0.unk04 == g_MpPlayers[i].unk4c.unk04) {
				func0f18800c(i, true);
			}
		}
	}
}

struct menuitem g_FilemanFileSavedMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(346), 0x00000000, NULL }, // "File Saved."
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000028, L_OPTIONS(347), 0x00000000, NULL }, // "OK"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanFileSavedMenuDialog = {
	MENUDIALOGTYPE_SUCCESS,
	L_OPTIONS(345), // "Cool!"
	g_FilemanFileSavedMenuItems,
	NULL,
	0x00000080,
	NULL,
};

struct menuitem g_FilemanSaveErrorMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&filemanMenuTextDeviceName, 0x00000000, filemanDeviceNameMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(348), 0x00000000, NULL }, // "An error occurred while trying to save"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(349), 0x00000000, filemanRetrySaveMenuHandler }, // "Try Again"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(350), 0x00000000, filemanSaveElsewhereYesMenuHandler }, // "Save Elsewhere"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(351), 0x00000000, filemanCancelSave2MenuHandler }, // "Cancel"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanSaveErrorMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	(u32)&filemanMenuTextErrorTitle,
	g_FilemanSaveErrorMenuItems,
	NULL,
	0x000000a0,
	NULL,
};

struct menuitem g_FilemanFileLostMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&filemanMenuTextDeviceName, 0x00000000, filemanDeviceNameMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_MPWEAPONS(251), 0x00000000, NULL }, // "The saved file has been erased due to corruption or damage."
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(354), 0x00000000, filemanAcknowledgeFileLostMenuHandler }, // "Cancel"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanFileLostMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	(u32)&filemanMenuTextErrorTitle,
	g_FilemanFileLostMenuItems,
	NULL,
	0x000000a0,
	NULL,
};

struct menuitem g_FilemanSaveElsewhereMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(360), 0x00000000, NULL }, // "Would you like to save your file elsewhere?"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(361), 0x00000000, filemanSaveElsewhereYesMenuHandler }, // "Yes"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(362), 0x00000000, filemanCancelSave2MenuHandler }, // "No"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanSaveElsewhereMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	L_OPTIONS(359), // "Save"
	g_FilemanSaveElsewhereMenuItems,
	NULL,
	0x000000a0,
	NULL,
};

struct menuitem g_FilemanInsertOriginalPakMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&filemanMenuTextInsertOriginalPak, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(365), 0x00000000, filemanReinsertedOkMenuHandler }, // "OK"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(366), 0x00000000, filemanReinsertedCancelMenuHandler }, // "Cancel"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanInsertOriginalPakMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	(u32)&filemanMenuTextErrorTitle,
	g_FilemanInsertOriginalPakMenuItems,
	filemanInsertOriginalPakMenuDialog,
	0x000000a0,
	NULL,
};

void func0f1097d0(s32 device)
{
	char *types[] = {"GAM", "MPG", "MPP", "CAM"};
	const u32 sizes[] = {0x80, 0x40, 0x20, 0x08};
	void *thing;

	if (g_SaveLocations.locations[0]) {
		var800a21e8.unk00 = g_SaveLocations.locations[0]->unk2d8[device].unk00;
		var800a21e8.unk04 = g_SaveLocations.locations[0]->unk2d8[device].unk04;

		thing = func00012ab0(align16(var7f1b2f3c[g_Menus[g_MpPlayerNum].data.pak.unke1c - 1]));

		if (thing) {
			func0f1094e4(&var800a21e0, g_Menus[g_MpPlayerNum].data.pak.unke1c + 0x67, thing);
		} else {
			filemanPushErrorDialog(FILEERROR_OUTOFMEMORY);
		}

		g_SaveLocations.unk10[g_Menus[g_MpPlayerNum].data.pak.unke1c - 1] = 1;
	}
}

const char var7f1b3234[] = "DestPakNo: %d (guid F:%x-%x:P)\n";
const char var7f1b3254[] = "Copy Memory Alloced\n";
const char var7f1b326c[] = "COULDNT GET THE RAM!\n";
const char var7f1b3284[] = "Saving...\n";

u32 var8007465c = 0x01020304;
u32 var80074660 = 0x00000000;

void func0f109954(s32 arg0)
{
	if (g_SaveLocations.locations[0]) {
		var800a22c0.unk00 = g_SaveLocations.locations[0]->unk2d8[arg0].unk00;
		var800a22c0.unk04 = g_SaveLocations.locations[0]->unk2d8[arg0].unk04;

		func0f1094e4(&var800a22c0, 0, NULL);
	}
}

void func0f1099a8(char *buffer, struct savelocation000 *arg1)
{
	char localbuffer[20];
	u32 sp20;

	localbuffer[0] = '\0';

	switch (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->filetype) {
	case 0:
	case 1:
		func0f0d564c(arg1->unk06, localbuffer, 0);
		break;
	case 2:
		func0f18d9a4(arg1->unk06, localbuffer, &sp20);
		break;
	}

	sprintf(buffer, "%s", localbuffer);
}

const char var7f1b3294[] = "GETFileNameForThePurposesOfTheFileRenamingChecker: Unknown type %d\n";

void func0f109a68(char *buffer)
{
	s32 i;
	s32 j;

	switch (g_Menus[g_MpPlayerNum].unke3e) {
	case 0:
	case 9:
	case 10:
	case 11:
		strcpy(buffer, g_SoloSaveFile.name);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 15:
	case 16:
	case 17:
		strcpy(buffer, g_Menus[g_MpPlayerNum].unke53);
		break;
	case 6:
	case 12:
		i = 0;
		j = 0;

		while (g_MpPlayers[g_MpPlayerNum].base.name[i] != '\0') {
			if (g_MpPlayers[g_MpPlayerNum].base.name[i] != '\n') {
				buffer[j] = g_MpPlayers[g_MpPlayerNum].base.name[i];
				j++;
			}

			i++;
		}

		buffer[j] = '\0';
		break;
	case 7:
	case 13:
		strcpy(buffer, g_MpSetup.name);
		break;
	}
}

void func0f109bb4(char *name)
{
	switch (g_Menus[g_MpPlayerNum].unke3e) {
	case 0:
	case 9:
	case 10:
	case 11:
		strcpy(g_SoloSaveFile.name, name);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 15:
	case 16:
	case 17:
		strcpy(g_Menus[g_MpPlayerNum].unke53, name);
		break;
	case 6:
	case 12:
		sprintf(g_MpPlayers[g_MpPlayerNum].base.name, "%s\n", name);
		break;
	case 7:
	case 13:
		strcpy(g_MpSetup.name, name);
		break;
	}
}

const char var7f1b32dc[] = "SetFileNameForThePurposesOfTheFileRenamingChecker: Unknown type %d\n";
const char var7f1b3320[] = "CheckFileName: Comparing range %d-%d\n";
const char var7f1b3348[] = "Compare '%s' to '%s' = %d\n";
const char var7f1b3364[] = "OI! DUPLICATE FILE NAME! NO!\n";
const char var7f1b3384[] = "()()()()()() Writing MPLAYER\n";
const char var7f1b33a4[] = "()()()()()() Writing MGAME\n";
const char var7f1b33c0[] = "Write Attempt Made...\n";
const char var7f1b33d8[] = "decided location: %d\n";

GLOBAL_ASM(
glabel func0f109c8c
/*  f109c8c:	3c0e8007 */ 	lui	$t6,%hi(g_MpPlayerNum)
/*  f109c90:	8dce1448 */ 	lw	$t6,%lo(g_MpPlayerNum)($t6)
/*  f109c94:	3c18800a */ 	lui	$t8,%hi(g_Menus+0xe3f)
/*  f109c98:	27bdff88 */ 	addiu	$sp,$sp,-120
/*  f109c9c:	000e78c0 */ 	sll	$t7,$t6,0x3
/*  f109ca0:	01ee7823 */ 	subu	$t7,$t7,$t6
/*  f109ca4:	000f7880 */ 	sll	$t7,$t7,0x2
/*  f109ca8:	01ee7821 */ 	addu	$t7,$t7,$t6
/*  f109cac:	000f78c0 */ 	sll	$t7,$t7,0x3
/*  f109cb0:	01ee7823 */ 	subu	$t7,$t7,$t6
/*  f109cb4:	000f7900 */ 	sll	$t7,$t7,0x4
/*  f109cb8:	030fc021 */ 	addu	$t8,$t8,$t7
/*  f109cbc:	9318ee3f */ 	lbu	$t8,%lo(g_Menus+0xe3f)($t8)
/*  f109cc0:	3c038007 */ 	lui	$v1,%hi(g_SaveLocations)
/*  f109cc4:	3c058007 */ 	lui	$a1,%hi(var8007465c)
/*  f109cc8:	0018c880 */ 	sll	$t9,$t8,0x2
/*  f109ccc:	00791821 */ 	addu	$v1,$v1,$t9
/*  f109cd0:	8c635bc0 */ 	lw	$v1,%lo(g_SaveLocations)($v1)
/*  f109cd4:	00a42821 */ 	addu	$a1,$a1,$a0
/*  f109cd8:	afbf002c */ 	sw	$ra,0x2c($sp)
/*  f109cdc:	afb50028 */ 	sw	$s5,0x28($sp)
/*  f109ce0:	afb40024 */ 	sw	$s4,0x24($sp)
/*  f109ce4:	afb30020 */ 	sw	$s3,0x20($sp)
/*  f109ce8:	afb2001c */ 	sw	$s2,0x1c($sp)
/*  f109cec:	afb10018 */ 	sw	$s1,0x18($sp)
/*  f109cf0:	afb00014 */ 	sw	$s0,0x14($sp)
/*  f109cf4:	14600003 */ 	bnez	$v1,.L0f109d04
/*  f109cf8:	90a5465c */ 	lbu	$a1,%lo(var8007465c)($a1)
/*  f109cfc:	10000068 */ 	b	.L0f109ea0
/*  f109d00:	24020001 */ 	addiu	$v0,$zero,0x1
.L0f109d04:
/*  f109d04:	00654021 */ 	addu	$t0,$v1,$a1
/*  f109d08:	81120300 */ 	lb	$s2,0x300($t0)
/*  f109d0c:	2404ffff */ 	addiu	$a0,$zero,-1
/*  f109d10:	24110004 */ 	addiu	$s1,$zero,0x4
/*  f109d14:	16440003 */ 	bne	$s2,$a0,.L0f109d24
/*  f109d18:	28a10004 */ 	slti	$at,$a1,0x4
/*  f109d1c:	10000060 */ 	b	.L0f109ea0
/*  f109d20:	24020001 */ 	addiu	$v0,$zero,0x1
.L0f109d24:
/*  f109d24:	10200009 */ 	beqz	$at,.L0f109d4c
/*  f109d28:	847502d0 */ 	lh	$s5,0x2d0($v1)
/*  f109d2c:	24620004 */ 	addiu	$v0,$v1,0x4
.L0f109d30:
/*  f109d30:	80430300 */ 	lb	$v1,0x300($v0)
/*  f109d34:	2631ffff */ 	addiu	$s1,$s1,-1
/*  f109d38:	10830002 */ 	beq	$a0,$v1,.L0f109d44
/*  f109d3c:	00000000 */ 	nop
/*  f109d40:	0060a825 */ 	or	$s5,$v1,$zero
.L0f109d44:
/*  f109d44:	14b1fffa */ 	bne	$a1,$s1,.L0f109d30
/*  f109d48:	2442ffff */ 	addiu	$v0,$v0,-1
.L0f109d4c:
/*  f109d4c:	27b40064 */ 	addiu	$s4,$sp,0x64
/*  f109d50:	a3a00064 */ 	sb	$zero,0x64($sp)
/*  f109d54:	0fc4269a */ 	jal	func0f109a68
/*  f109d58:	02802025 */ 	or	$a0,$s4,$zero
/*  f109d5c:	93a90064 */ 	lbu	$t1,0x64($sp)
/*  f109d60:	27a30064 */ 	addiu	$v1,$sp,0x64
/*  f109d64:	2410000a */ 	addiu	$s0,$zero,0xa
/*  f109d68:	11200011 */ 	beqz	$t1,.L0f109db0
/*  f109d6c:	02408825 */ 	or	$s1,$s2,$zero
/*  f109d70:	90620000 */ 	lbu	$v0,0x0($v1)
/*  f109d74:	28410061 */ 	slti	$at,$v0,0x61
.L0f109d78:
/*  f109d78:	14200005 */ 	bnez	$at,.L0f109d90
/*  f109d7c:	2841007b */ 	slti	$at,$v0,0x7b
/*  f109d80:	10200003 */ 	beqz	$at,.L0f109d90
/*  f109d84:	244bffe0 */ 	addiu	$t3,$v0,-32
/*  f109d88:	a06b0000 */ 	sb	$t3,0x0($v1)
/*  f109d8c:	316200ff */ 	andi	$v0,$t3,0xff
.L0f109d90:
/*  f109d90:	56020004 */ 	bnel	$s0,$v0,.L0f109da4
/*  f109d94:	24630001 */ 	addiu	$v1,$v1,0x1
/*  f109d98:	10000002 */ 	b	.L0f109da4
/*  f109d9c:	a0600000 */ 	sb	$zero,0x0($v1)
/*  f109da0:	24630001 */ 	addiu	$v1,$v1,0x1
.L0f109da4:
/*  f109da4:	90620000 */ 	lbu	$v0,0x0($v1)
/*  f109da8:	5440fff3 */ 	bnezl	$v0,.L0f109d78
/*  f109dac:	28410061 */ 	slti	$at,$v0,0x61
.L0f109db0:
/*  f109db0:	0255082a */ 	slt	$at,$s2,$s5
/*  f109db4:	10200039 */ 	beqz	$at,.L0f109e9c
/*  f109db8:	2410000a */ 	addiu	$s0,$zero,0xa
/*  f109dbc:	00129080 */ 	sll	$s2,$s2,0x2
/*  f109dc0:	02519023 */ 	subu	$s2,$s2,$s1
/*  f109dc4:	001290c0 */ 	sll	$s2,$s2,0x3
/*  f109dc8:	27b30040 */ 	addiu	$s3,$sp,0x40
.L0f109dcc:
/*  f109dcc:	3c0c8007 */ 	lui	$t4,%hi(g_MpPlayerNum)
/*  f109dd0:	8d8c1448 */ 	lw	$t4,%lo(g_MpPlayerNum)($t4)
/*  f109dd4:	3c0e800a */ 	lui	$t6,%hi(g_Menus+0xe3f)
/*  f109dd8:	3c188007 */ 	lui	$t8,%hi(g_SaveLocations)
/*  f109ddc:	000c68c0 */ 	sll	$t5,$t4,0x3
/*  f109de0:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f109de4:	000d6880 */ 	sll	$t5,$t5,0x2
/*  f109de8:	01ac6821 */ 	addu	$t5,$t5,$t4
/*  f109dec:	000d68c0 */ 	sll	$t5,$t5,0x3
/*  f109df0:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f109df4:	000d6900 */ 	sll	$t5,$t5,0x4
/*  f109df8:	01cd7021 */ 	addu	$t6,$t6,$t5
/*  f109dfc:	91ceee3f */ 	lbu	$t6,%lo(g_Menus+0xe3f)($t6)
/*  f109e00:	02602025 */ 	or	$a0,$s3,$zero
/*  f109e04:	00001025 */ 	or	$v0,$zero,$zero
/*  f109e08:	000e7880 */ 	sll	$t7,$t6,0x2
/*  f109e0c:	030fc021 */ 	addu	$t8,$t8,$t7
/*  f109e10:	8f185bc0 */ 	lw	$t8,%lo(g_SaveLocations)($t8)
/*  f109e14:	0fc4266a */ 	jal	func0f1099a8
/*  f109e18:	03122821 */ 	addu	$a1,$t8,$s2
/*  f109e1c:	93b90040 */ 	lbu	$t9,0x40($sp)
/*  f109e20:	27a30040 */ 	addiu	$v1,$sp,0x40
/*  f109e24:	02802025 */ 	or	$a0,$s4,$zero
/*  f109e28:	13200011 */ 	beqz	$t9,.L0f109e70
/*  f109e2c:	00000000 */ 	nop
/*  f109e30:	90620000 */ 	lbu	$v0,0x0($v1)
/*  f109e34:	28410061 */ 	slti	$at,$v0,0x61
.L0f109e38:
/*  f109e38:	14200005 */ 	bnez	$at,.L0f109e50
/*  f109e3c:	2841007b */ 	slti	$at,$v0,0x7b
/*  f109e40:	10200003 */ 	beqz	$at,.L0f109e50
/*  f109e44:	2449ffe0 */ 	addiu	$t1,$v0,-32
/*  f109e48:	a0690000 */ 	sb	$t1,0x0($v1)
/*  f109e4c:	312200ff */ 	andi	$v0,$t1,0xff
.L0f109e50:
/*  f109e50:	56020004 */ 	bnel	$s0,$v0,.L0f109e64
/*  f109e54:	24630001 */ 	addiu	$v1,$v1,0x1
/*  f109e58:	10000002 */ 	b	.L0f109e64
/*  f109e5c:	a0600000 */ 	sb	$zero,0x0($v1)
/*  f109e60:	24630001 */ 	addiu	$v1,$v1,0x1
.L0f109e64:
/*  f109e64:	90620000 */ 	lbu	$v0,0x0($v1)
/*  f109e68:	5440fff3 */ 	bnezl	$v0,.L0f109e38
/*  f109e6c:	28410061 */ 	slti	$at,$v0,0x61
.L0f109e70:
/*  f109e70:	0c004c9d */ 	jal	func00013274
/*  f109e74:	02602825 */ 	or	$a1,$s3,$zero
/*  f109e78:	02802025 */ 	or	$a0,$s4,$zero
/*  f109e7c:	0c004c9d */ 	jal	func00013274
/*  f109e80:	02602825 */ 	or	$a1,$s3,$zero
/*  f109e84:	14400003 */ 	bnez	$v0,.L0f109e94
/*  f109e88:	26310001 */ 	addiu	$s1,$s1,0x1
/*  f109e8c:	10000004 */ 	b	.L0f109ea0
/*  f109e90:	00001025 */ 	or	$v0,$zero,$zero
.L0f109e94:
/*  f109e94:	1635ffcd */ 	bne	$s1,$s5,.L0f109dcc
/*  f109e98:	26520018 */ 	addiu	$s2,$s2,0x18
.L0f109e9c:
/*  f109e9c:	24020001 */ 	addiu	$v0,$zero,0x1
.L0f109ea0:
/*  f109ea0:	8fbf002c */ 	lw	$ra,0x2c($sp)
/*  f109ea4:	8fb00014 */ 	lw	$s0,0x14($sp)
/*  f109ea8:	8fb10018 */ 	lw	$s1,0x18($sp)
/*  f109eac:	8fb2001c */ 	lw	$s2,0x1c($sp)
/*  f109eb0:	8fb30020 */ 	lw	$s3,0x20($sp)
/*  f109eb4:	8fb40024 */ 	lw	$s4,0x24($sp)
/*  f109eb8:	8fb50028 */ 	lw	$s5,0x28($sp)
/*  f109ebc:	03e00008 */ 	jr	$ra
/*  f109ec0:	27bd0078 */ 	addiu	$sp,$sp,0x78
);

void func0f109ec4(void)
{
	if (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->spacesfree[g_Menus[g_MpPlayerNum].unke52] > 0) {
		if (func0f109c8c(g_Menus[g_MpPlayerNum].unke52) == 0) {
			menuPushDialog(&g_FilemanDuplicateFileNameMenuDialog);
		} else {
			menuPopDialog();

			if (g_Menus[g_MpPlayerNum].unke3e == 0) {
				func0f109954(g_Menus[g_MpPlayerNum].unke52);
			} else if (g_Menus[g_MpPlayerNum].unke3e == 5) {
				// empty
			} else if (g_Menus[g_MpPlayerNum].unke3e == 6) {
				struct savelocation_2d8 thing;
				thing.unk00 = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk2d8[g_Menus[g_MpPlayerNum].unke52].unk00;
				thing.unk04 = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk2d8[g_Menus[g_MpPlayerNum].unke52].unk04;
				func0f1094e4(&thing, 3, (void *)g_MpPlayerNum);
			} else if (g_Menus[g_MpPlayerNum].unke3e == 7) {
				struct savelocation_2d8 thing;
				thing.unk00 = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk2d8[g_Menus[g_MpPlayerNum].unke52].unk00;
				thing.unk04 = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk2d8[g_Menus[g_MpPlayerNum].unke52].unk04;
				func0f1094e4(&thing, 4, NULL);
			} else if (g_Menus[g_MpPlayerNum].unke3e >= 9) {
				struct savelocation_2d8 thing;
				thing.unk00 = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk2d8[g_Menus[g_MpPlayerNum].unke52].unk00;
				thing.unk04 = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk2d8[g_Menus[g_MpPlayerNum].unke52].unk04;
				func0f1094e4(&thing, -1, NULL);
			} else {
				func0f1097d0(g_Menus[g_MpPlayerNum].unke52);
			}
		}
	}
}

s32 filemanConfirmRenameMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	char *name = data->keyboard.string;

	switch (operation) {
	case MENUOP_GETTEXT:
		func0f109a68(name);
		break;
	case MENUOP_SETTEXT:
		func0f109bb4(name);
		break;
	case MENUOP_SET:
		func0f109ec4();
		break;
	}

	return 0;
}

s32 filemanDuplicateRenameMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		menuPopDialog();
		menuPushDialog(&g_FilemanChangeFileNameMenuDialog);
	}

	return 0;
}

s32 filemanDuplicateCancelMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		menuPopDialog();
		menuPopDialog();
	}

	return 0;
}

char *filemanMenuTextDeviceNameContainingDuplicateFile(struct menuitem *item)
{
	return filemanGetDeviceName(g_Menus[g_MpPlayerNum].unke52);
}

char *filemanMenuTextDuplicateFileName(struct menuitem *item)
{
	char buffer[32];

	func0f109a68(buffer);
	sprintf(g_StringPointer, "%s\n", buffer);

	return g_StringPointer;
}

struct menuitem g_FilemanChangeFileNameMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_MPWEAPONS(239), 0x00000000, NULL }, // "Enter new file name:"
	{ MENUITEMTYPE_KEYBOARD,    0, 0x00000000, 0x00000000, 0x00000000, filemanConfirmRenameMenuHandler },
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanChangeFileNameMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_MPWEAPONS(238), // "Change File Name"
	g_FilemanChangeFileNameMenuItems,
	NULL,
	0x00000080,
	NULL,
};

struct menuitem g_FilemanDuplicateFileNameMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000030, (u32)&filemanMenuTextDeviceNameContainingDuplicateFile, 0x00000000, NULL },
	{ MENUITEMTYPE_LABEL,       0, 0x02000030, L_MPWEAPONS(233), 0x00000000, NULL }, // "already contains"
	{ MENUITEMTYPE_LABEL,       0, 0x02000030, L_MPWEAPONS(234), 0x00000000, NULL }, // "a file named"
	{ MENUITEMTYPE_LABEL,       0, 0x02000030, (u32)&filemanMenuTextDuplicateFileName, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000000, L_MPWEAPONS(235), 0x00000000, filemanDuplicateRenameMenuHandler }, // "Rename File"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000008, L_MPWEAPONS(236), 0x00000000, NULL }, // "Change Location"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000000, L_MPWEAPONS(237), 0x00000000, filemanDuplicateCancelMenuHandler }, // "Cancel"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanDuplicateFileNameMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_MPWEAPONS(232), // "Duplicate File Name"
	g_FilemanDuplicateFileNameMenuItems,
	NULL,
	0x00000080,
	NULL,
};

char *filemanMenuTextLocationName2(struct menuitem *item)
{
	u16 names[] = {
		L_OPTIONS(112), // "Controller Pak 1"
		L_OPTIONS(113), // "Controller Pak 2"
		L_OPTIONS(114), // "Controller Pak 3"
		L_OPTIONS(115), // "Controller Pak 4"
		L_OPTIONS(111), // "Game Pak"
		L_OPTIONS(4),   // ""
	};

	if (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f] == NULL) {
		return NULL;
	}

	if (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->spacesfree[item->param] < 0) {
		return langGet(names[5]);
	}

	return langGet(names[item->param]);
}

char *filemanMenuTextSaveLocationSpaces(struct menuitem *item)
{
	s32 spacesfree;

	if (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f] == NULL) {
		return NULL;
	}

	spacesfree = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->spacesfree[item->param];

	if (spacesfree < 0) {
		return "\n";
	}

	if (spacesfree == 0) {
		return langGet(L_OPTIONS(372)); // "Full"
	}

	sprintf(g_StringPointer, "%d", spacesfree);
	return g_StringPointer;
}

const char var7f1b33fc[] = "GOT OKed!, item->data = %d\n";
const char var7f1b3418[] = "GOT CANCELLED!\n";
const char var7f1b3428[] = "Picking Location, type %d wadtype %d wad %d\n";
const char var7f1b3458[] = "Torching file %d\n";
const char var7f1b346c[] = "Copying file %d\n";
const char var7f1b3480[] = "item: %x\n";
const char var7f1b348c[] = "Switched Wads Back\n";
const char var7f1b34a0[] = "MenuClosed\n";
const char var7f1b34ac[] = "Deleting files, wad %d\n";
const char var7f1b34c4[] = "%d:\n";
const char var7f1b34cc[] = "%s\n";
const char var7f1b34d0[] = "%s\n";
const char var7f1b34d4[] = "%d\n";
const char var7f1b34d8[] = "";
const char var7f1b34dc[] = "GOT OKed!\n";
const char var7f1b34e8[] = "Try to find last opened file...\n";
const char var7f1b350c[] = "%s %s";
const char var7f1b3514[] = "\n";
const char var7f1b3518[] = "%s %d:%02d:%02d";
const char var7f1b3528[] = "%s %02d:%02d";
const char var7f1b3538[] = ".%02d";

s32 filemanSelectLocationMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f] == NULL) {
		return 0;
	}

	if (operation == MENUOP_CHECKDISABLED) {
		if (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->spacesfree[item->param] < 1) {
			return true;
		}
	}

	if (operation == MENUOP_SET) {
		g_Menus[g_MpPlayerNum].unke52 = item->param;
		func0f109ec4();
	}

	return 0;
}

s32 filemanCancelSaveMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		menuPopDialog();
	}

	return 0;
}

s32 filemanDeleteFilesForSaveMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		filemanPushDeleteFileDialog(g_Menus[g_MpPlayerNum].unke3f);
		g_Menus[g_MpPlayerNum].data.filesel.unke28 = 1;
	}

	return 0;
}

void func0f10a51c(s32 arg0, u32 arg1)
{
	g_Menus[g_MpPlayerNum].unke3e = arg0;
	g_Menus[g_MpPlayerNum].unke3f = func0f110cf8(arg1);

	func0f110da8();

	menuPushDialog(&g_FilemanSelectLocationMenuDialog);
}

s32 filemanConfirmDeleteMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		menuPopDialog();
		filemanDeleteFile();
	}

	return 0;
}

char *filemanMenuTextFileInUseDescription(struct menuitem *item)
{
	if (func0f0fd570(&g_FilemanCopyListMenuDialog)) {
		return langGet(L_MPWEAPONS(240)); // "The file you are copying cannot be deleted."
	}

	return langGet(L_MPWEAPONS(160)); // "Cannot delete file as it is being used."
}

Gfx *filemanRenderPerfectHeadThumbnail(Gfx *gdl, struct menuitemrenderdata *renderdata, u32 arg2, u32 arg3)
{
	struct var800ab5a8 *texture = func0f111460(g_MpPlayerNum, arg2, arg3 & 0xffff);

	if (texture) {
		gSPDisplayList(gdl++, &var800613a0);
		gSPDisplayList(gdl++, &var80061360);

		gDPPipeSync(gdl++);
		gDPSetTexturePersp(gdl++, G_TP_NONE);
		gDPSetAlphaCompare(gdl++, G_AC_NONE);
		gDPSetTextureLOD(gdl++, G_TL_TILE);
		gDPSetTextureConvert(gdl++, G_TC_FILT);

		func0f0b39c0(&gdl, texture, 1, 0, 2, 1, 0);

		gDPSetCycleType(gdl++, G_CYC_1CYCLE);
		gDPSetTextureFilter(gdl++, G_TF_POINT);
		gDPSetEnvColor(gdl++, 0xff, 0xff, 0xff, renderdata->colour);

		gDPSetCombineLERP(gdl++,
				TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT, 0,
				TEXEL0, 0, ENVIRONMENT, 0, TEXEL0, 0, ENVIRONMENT, 0);

		gDPLoadSync(gdl++);
		gDPTileSync(gdl++);

		gSPTextureRectangle(gdl++,
				((renderdata->x + 4) << 2) * g_ScreenWidthMultiplier,
				(renderdata->y + 2) << 2,
				((renderdata->x + 20) << 2) * g_ScreenWidthMultiplier,
				(renderdata->y + 18) << 2,
				G_TX_RENDERTILE, 0, 512, 1024 / g_ScreenWidthMultiplier, -1024);

		gDPLoadSync(gdl++);
		gDPTileSync(gdl++);
		gDPPipeSync(gdl++);

		if (arg3) {
			// empty
		}
	}

	return gdl;
}

bool func0f10a97c(struct savelocation000 *arg0)
{
	s32 i;

	if (func0f0fd570(&g_FilemanCopyListMenuDialog)
			&& arg0->unk00 == var800a21e0.unk00
			&& arg0->unk04 == var800a21e0.unk04) {
		return true;
	}

	if (func0f0fd570(&menudialog_fileselect2)) {
		return false;
	}

	if (g_MenuData.root == MENUROOT_FILESELECT) {
		return false;
	}

	if (arg0->unk00 == var800a22c0.unk00 && arg0->unk04 == var800a22c0.unk04) {
		return true;
	}

	if (arg0->unk00 == g_MpSetup.unk20.unk00 && arg0->unk04 == g_MpSetup.unk20.unk04) {
		return true;
	}

	for (i = 0; i < 4; i++) {
		if ((g_MpSetup.chrslots & (1 << i))
				&& g_MpPlayers[i].unk4c.unk00 == arg0->unk00
				&& g_MpPlayers[i].unk4c.unk04 == arg0->unk04) {
			return true;
		}
	}

	return false;
}

s32 filemanFileToCopyOrDeleteListMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data, bool isdelete)
{
	s32 x;
	s32 y;
	struct savelocation *location = g_SaveLocations.locations[0];
	s32 unke3f = 0;

	if (item->param == 1) {
		unke3f = g_Menus[g_MpPlayerNum].unke3f;
		location = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f];
	}

	if (location == NULL) {
		return 0;
	}

	switch (operation) {
	case MENUOP_GETOPTIONVALUE:
		data->list.value = 0x0fffff;
		break;
	case MENUOP_GETOPTIONCOUNT:
		data->list.value = location->unk2d0;
		break;
	case MENUOP_RENDER:
		{
			Gfx *gdl = data->type19.gdl;
			struct menuitemrenderdata *renderdata = data->type19.renderdata2;
			struct savelocation000 *location000 = &location->unk000[data->list.unk04];

			if (g_Menus[g_MpPlayerNum].data.filesel.filetypeplusone == 4) {
				gdl = filemanRenderPerfectHeadThumbnail(gdl, renderdata, location000->unk00, location000->unk04);
			} else {
				u32 colour = renderdata->colour;
				char text[32];

				if (isdelete && func0f10a97c(location000)) {
					colour = 0xff333300 | (colour & 0xff);
				}

				x = renderdata->x + 2;
				y = renderdata->y + 2;

				gdl = func0f153628(gdl);

				if (location000) {
					func0f1080d0(text, location000, g_Menus[g_MpPlayerNum].data.filesel.filetypeplusone - 1);
					gdl = textRenderProjected(gdl, &x, &y, text, var8007fb10, var8007fb0c,
							colour, viGetX(), viGetY(), 0, 1);
					y = renderdata->y + 12;
					x = renderdata->x + 2;
				}

				gdl = func0f153780(gdl);
			}

			return (u32)gdl;
		}
	case MENUOP_GETOPTIONHEIGHT:
		data->list.value = 11;
		break;
	case MENUOP_GETOPTGROUPCOUNT:
		data->list.value = location->unk30a;
		break;
	case MENUOP_GETOPTGROUPTEXT:
		return func0f1086b8(unke3f, operation, data->list.value);
	case MENUOP_GETGROUPSTARTINDEX:
		data->list.groupstartindex = func0f1086b8(unke3f, operation, data->list.value);
		return 0;
	}

	return 0;
}

s32 filemanFileToDeleteListMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f] == NULL) {
		return 0;
	}

	if (operation == MENUOP_SET) {
		struct savelocation000 *thing = &g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->unk000[data->list.value];

		if (thing) {
			if (func0f10a97c(thing)) {
				func0f1083d0(thing, g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->filetype);
				menuPushDialog(&g_FilemanFileInUseMenuDialog);
			} else {
				func0f1083d0(thing, g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f]->filetype);
				var800a21f0.unk00 = thing->unk00;
				var800a21f0.unk04 = thing->unk04;
				menuPushDialog(&g_FilemanConfirmDeleteMenuDialog);
			}
		}
	}

	return filemanFileToCopyOrDeleteListMenuHandler(operation, item, data, true);
}

s32 filemanFileToCopyListMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	struct savelocation *location = g_SaveLocations.locations[0];

	if (location == NULL) {
		return 0;
	}

	if (operation == MENUOP_SET) {
		struct savelocation000 *thing = &location->unk000[data->list.value];

		if (thing) {
			var800a21e0.unk00 = thing->unk00;
			var800a21e0.unk04 = thing->unk04;

			func0f1099a8(g_Menus[g_MpPlayerNum].unke53, thing);
			func0f10a51c(g_Menus[g_MpPlayerNum].data.pak.unke1c, g_Menus[g_MpPlayerNum].data.pak.unke1c - 1);
		}
	}

	return filemanFileToCopyOrDeleteListMenuHandler(operation, item, data, false);
}

s32 filemanCopyOrDeleteListMenuDialog(u32 operation, struct menudialog *dialog, union handlerdata *data)
{
	if (operation == MENUOP_CLOSE) {
		if (g_Menus[g_MpPlayerNum].data.pak.unke28 == 1) {
			g_Menus[g_MpPlayerNum].data.pak.unke28 = 0;
		} else {
			func0f110c5c(0, FILETYPE_SOLO);
			g_Menus[g_MpPlayerNum].data.pak.unke1c = 0;
		}
	}

	return 0;
}

void filemanPushDeleteFileDialog(u32 arg0)
{
	struct savelocation *location;

	g_Menus[g_MpPlayerNum].unke3f = arg0;
	g_Menus[g_MpPlayerNum].data.pak.unke28 = 0;
	g_Menus[g_MpPlayerNum].data.pak.unke1c = 1;

	location = g_SaveLocations.locations[g_Menus[g_MpPlayerNum].unke3f];

	if (location) {
		g_Menus[g_MpPlayerNum].data.pak.unke1c = location->filetype + 1;
	}

	menuPushDialog(&g_FilemanDeleteFileMenuDialog);
}

s32 pakDeleteGameNoteMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		OSPfsState *file = &g_EditingPak->filestates[g_Menus[g_MpPlayerNum].data.pak.unke28];
		s32 result;

		g_Menus[g_MpPlayerNum].data.pak.unke24 = g_Menus[g_MpPlayerNum].data.pak.unke24 | (1 << g_Menus[g_MpPlayerNum].data.pak.device);

		menuPopDialog();

		result = pakDeleteGameNote(g_Menus[g_MpPlayerNum].data.pak.device,
				file->company_code, file->game_code, file->game_name, file->ext_name);

		g_Menus[g_MpPlayerNum].unke3c = g_Menus[g_MpPlayerNum].data.pak.device;

		if (result) {
			filemanPushErrorDialog(FILEERROR_DELETENOTEFAILED);
		}
	}

	return 0;
}

GLOBAL_ASM(
glabel pakGameNoteListMenuHandler
.late_rodata
glabel var7f1b37ac
.word pakGameNoteListMenuHandler+0x5c # f10b2c8
glabel var7f1b37b0
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37b4
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37b8
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37bc
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37c0
.word pakGameNoteListMenuHandler+0x398 # f10b604
glabel var7f1b37c4
.word pakGameNoteListMenuHandler+0x4c # f10b2b8
glabel var7f1b37c8
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37cc
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37d0
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37d4
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37d8
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37dc
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37e0
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37e4
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37e8
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37ec
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37f0
.word pakGameNoteListMenuHandler+0x3f0 # f10b65c
glabel var7f1b37f4
.word pakGameNoteListMenuHandler+0x68 # f10b2d4
glabel var7f1b37f8
.word pakGameNoteListMenuHandler+0x38c # f10b5f8
.text
/*  f10b26c:	3c038007 */ 	lui	$v1,%hi(g_EditingPak)
/*  f10b270:	8c634a4c */ 	lw	$v1,%lo(g_EditingPak)($v1)
/*  f10b274:	27bdfea8 */ 	addiu	$sp,$sp,-344
/*  f10b278:	afbf003c */ 	sw	$ra,0x3c($sp)
/*  f10b27c:	afb00038 */ 	sw	$s0,0x38($sp)
/*  f10b280:	afa5015c */ 	sw	$a1,0x15c($sp)
/*  f10b284:	14600003 */ 	bnez	$v1,.L0f10b294
/*  f10b288:	00c03825 */ 	or	$a3,$a2,$zero
/*  f10b28c:	100000f4 */ 	b	.L0f10b660
/*  f10b290:	00001025 */ 	or	$v0,$zero,$zero
.L0f10b294:
/*  f10b294:	248effff */ 	addiu	$t6,$a0,-1
/*  f10b298:	2dc10014 */ 	sltiu	$at,$t6,0x14
/*  f10b29c:	102000ef */ 	beqz	$at,.L0f10b65c
/*  f10b2a0:	000e7080 */ 	sll	$t6,$t6,0x2
/*  f10b2a4:	3c017f1b */ 	lui	$at,%hi(var7f1b37ac)
/*  f10b2a8:	002e0821 */ 	addu	$at,$at,$t6
/*  f10b2ac:	8c2e37ac */ 	lw	$t6,%lo(var7f1b37ac)($at)
/*  f10b2b0:	01c00008 */ 	jr	$t6
/*  f10b2b4:	00000000 */ 	nop
/*  f10b2b8:	3c0f000f */ 	lui	$t7,0xf
/*  f10b2bc:	35efffff */ 	ori	$t7,$t7,0xffff
/*  f10b2c0:	100000e6 */ 	b	.L0f10b65c
/*  f10b2c4:	acef0000 */ 	sw	$t7,0x0($a3)
/*  f10b2c8:	24180010 */ 	addiu	$t8,$zero,0x10
/*  f10b2cc:	100000e3 */ 	b	.L0f10b65c
/*  f10b2d0:	acf80000 */ 	sw	$t8,0x0($a3)
/*  f10b2d4:	8cf90000 */ 	lw	$t9,0x0($a3)
/*  f10b2d8:	3c057f1b */ 	lui	$a1,%hi(var7f1b34c4)
/*  f10b2dc:	24a534c4 */ 	addiu	$a1,$a1,%lo(var7f1b34c4)
/*  f10b2e0:	afb9014c */ 	sw	$t9,0x14c($sp)
/*  f10b2e4:	8ce20004 */ 	lw	$v0,0x4($a3)
/*  f10b2e8:	8cf00008 */ 	lw	$s0,0x8($a3)
/*  f10b2ec:	afa70160 */ 	sw	$a3,0x160($sp)
/*  f10b2f0:	00024140 */ 	sll	$t0,$v0,0x5
/*  f10b2f4:	00684821 */ 	addu	$t1,$v1,$t0
/*  f10b2f8:	afa90144 */ 	sw	$t1,0x144($sp)
/*  f10b2fc:	27a400d4 */ 	addiu	$a0,$sp,0xd4
/*  f10b300:	0c004dad */ 	jal	sprintf
/*  f10b304:	24460001 */ 	addiu	$a2,$v0,0x1
/*  f10b308:	8e0a0000 */ 	lw	$t2,0x0($s0)
/*  f10b30c:	254b0004 */ 	addiu	$t3,$t2,0x4
/*  f10b310:	afab0154 */ 	sw	$t3,0x154($sp)
/*  f10b314:	8e0c0004 */ 	lw	$t4,0x4($s0)
/*  f10b318:	258d0001 */ 	addiu	$t5,$t4,0x1
/*  f10b31c:	0c002f02 */ 	jal	viGetX
/*  f10b320:	afad0150 */ 	sw	$t5,0x150($sp)
/*  f10b324:	0c002f06 */ 	jal	viGetY
/*  f10b328:	a7a20050 */ 	sh	$v0,0x50($sp)
/*  f10b32c:	3c0e8008 */ 	lui	$t6,%hi(var8007fb10)
/*  f10b330:	3c0f8008 */ 	lui	$t7,%hi(var8007fb0c)
/*  f10b334:	8deffb0c */ 	lw	$t7,%lo(var8007fb0c)($t7)
/*  f10b338:	8dcefb10 */ 	lw	$t6,%lo(var8007fb10)($t6)
/*  f10b33c:	87b90050 */ 	lh	$t9,0x50($sp)
/*  f10b340:	afaf0014 */ 	sw	$t7,0x14($sp)
/*  f10b344:	afae0010 */ 	sw	$t6,0x10($sp)
/*  f10b348:	8e18000c */ 	lw	$t8,0xc($s0)
/*  f10b34c:	24080001 */ 	addiu	$t0,$zero,0x1
/*  f10b350:	afa80028 */ 	sw	$t0,0x28($sp)
/*  f10b354:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10b358:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10b35c:	8fa4014c */ 	lw	$a0,0x14c($sp)
/*  f10b360:	27a50154 */ 	addiu	$a1,$sp,0x154
/*  f10b364:	27a60150 */ 	addiu	$a2,$sp,0x150
/*  f10b368:	27a700d4 */ 	addiu	$a3,$sp,0xd4
/*  f10b36c:	afb9001c */ 	sw	$t9,0x1c($sp)
/*  f10b370:	0fc5580f */ 	jal	textRenderProjected
/*  f10b374:	afb80018 */ 	sw	$t8,0x18($sp)
/*  f10b378:	8faa0160 */ 	lw	$t2,0x160($sp)
/*  f10b37c:	afa2014c */ 	sw	$v0,0x14c($sp)
/*  f10b380:	3c098007 */ 	lui	$t1,%hi(g_EditingPak)
/*  f10b384:	8d4b0004 */ 	lw	$t3,0x4($t2)
/*  f10b388:	8d294a4c */ 	lw	$t1,%lo(g_EditingPak)($t1)
/*  f10b38c:	24010001 */ 	addiu	$at,$zero,0x1
/*  f10b390:	000b6080 */ 	sll	$t4,$t3,0x2
/*  f10b394:	012c6821 */ 	addu	$t5,$t1,$t4
/*  f10b398:	8dae0200 */ 	lw	$t6,0x200($t5)
/*  f10b39c:	15c10020 */ 	bne	$t6,$at,.L0f10b420
/*  f10b3a0:	00000000 */ 	nop
/*  f10b3a4:	8fa40144 */ 	lw	$a0,0x144($sp)
/*  f10b3a8:	27a5011c */ 	addiu	$a1,$sp,0x11c
/*  f10b3ac:	24060010 */ 	addiu	$a2,$zero,0x10
/*  f10b3b0:	0fc47986 */ 	jal	func0f11e618
/*  f10b3b4:	2484000e */ 	addiu	$a0,$a0,0xe
/*  f10b3b8:	8fa40144 */ 	lw	$a0,0x144($sp)
/*  f10b3bc:	27a50110 */ 	addiu	$a1,$sp,0x110
/*  f10b3c0:	24060004 */ 	addiu	$a2,$zero,0x4
/*  f10b3c4:	0fc47986 */ 	jal	func0f11e618
/*  f10b3c8:	2484000a */ 	addiu	$a0,$a0,0xa
/*  f10b3cc:	3c057f1b */ 	lui	$a1,%hi(var7f1b34cc)
/*  f10b3d0:	a3a00111 */ 	sb	$zero,0x111($sp)
/*  f10b3d4:	24a534cc */ 	addiu	$a1,$a1,%lo(var7f1b34cc)
/*  f10b3d8:	27a400d4 */ 	addiu	$a0,$sp,0xd4
/*  f10b3dc:	0c004dad */ 	jal	sprintf
/*  f10b3e0:	27a6011c */ 	addiu	$a2,$sp,0x11c
/*  f10b3e4:	3c057f1b */ 	lui	$a1,%hi(var7f1b34d0)
/*  f10b3e8:	24a534d0 */ 	addiu	$a1,$a1,%lo(var7f1b34d0)
/*  f10b3ec:	27a40098 */ 	addiu	$a0,$sp,0x98
/*  f10b3f0:	0c004dad */ 	jal	sprintf
/*  f10b3f4:	27a60110 */ 	addiu	$a2,$sp,0x110
/*  f10b3f8:	8faf0144 */ 	lw	$t7,0x144($sp)
/*  f10b3fc:	3c057f1b */ 	lui	$a1,%hi(var7f1b34d4)
/*  f10b400:	24a534d4 */ 	addiu	$a1,$a1,%lo(var7f1b34d4)
/*  f10b404:	8de60000 */ 	lw	$a2,0x0($t7)
/*  f10b408:	27a4005c */ 	addiu	$a0,$sp,0x5c
/*  f10b40c:	0006c202 */ 	srl	$t8,$a2,0x8
/*  f10b410:	0c004dad */ 	jal	sprintf
/*  f10b414:	03003025 */ 	or	$a2,$t8,$zero
/*  f10b418:	10000012 */ 	b	.L0f10b464
/*  f10b41c:	8e190000 */ 	lw	$t9,0x0($s0)
.L0f10b420:
/*  f10b420:	0fc5b9f1 */ 	jal	langGet
/*  f10b424:	24045788 */ 	addiu	$a0,$zero,0x5788
/*  f10b428:	27a400d4 */ 	addiu	$a0,$sp,0xd4
/*  f10b42c:	0c004dad */ 	jal	sprintf
/*  f10b430:	00402825 */ 	or	$a1,$v0,$zero
/*  f10b434:	0fc5b9f1 */ 	jal	langGet
/*  f10b438:	24045789 */ 	addiu	$a0,$zero,0x5789
/*  f10b43c:	27a4005c */ 	addiu	$a0,$sp,0x5c
/*  f10b440:	0c004dad */ 	jal	sprintf
/*  f10b444:	00402825 */ 	or	$a1,$v0,$zero
/*  f10b448:	3c057f1b */ 	lui	$a1,%hi(var7f1b34d8)
/*  f10b44c:	24a534d8 */ 	addiu	$a1,$a1,%lo(var7f1b34d8)
/*  f10b450:	27a40098 */ 	addiu	$a0,$sp,0x98
/*  f10b454:	27a6011c */ 	addiu	$a2,$sp,0x11c
/*  f10b458:	0c004dad */ 	jal	sprintf
/*  f10b45c:	27a70110 */ 	addiu	$a3,$sp,0x110
/*  f10b460:	8e190000 */ 	lw	$t9,0x0($s0)
.L0f10b464:
/*  f10b464:	27280014 */ 	addiu	$t0,$t9,0x14
/*  f10b468:	afa80154 */ 	sw	$t0,0x154($sp)
/*  f10b46c:	8e0a0004 */ 	lw	$t2,0x4($s0)
/*  f10b470:	254b0001 */ 	addiu	$t3,$t2,0x1
/*  f10b474:	0c002f02 */ 	jal	viGetX
/*  f10b478:	afab0150 */ 	sw	$t3,0x150($sp)
/*  f10b47c:	0c002f06 */ 	jal	viGetY
/*  f10b480:	a7a20050 */ 	sh	$v0,0x50($sp)
/*  f10b484:	3c098008 */ 	lui	$t1,%hi(var8007fb10)
/*  f10b488:	3c0c8008 */ 	lui	$t4,%hi(var8007fb0c)
/*  f10b48c:	8d8cfb0c */ 	lw	$t4,%lo(var8007fb0c)($t4)
/*  f10b490:	8d29fb10 */ 	lw	$t1,%lo(var8007fb10)($t1)
/*  f10b494:	87ae0050 */ 	lh	$t6,0x50($sp)
/*  f10b498:	afac0014 */ 	sw	$t4,0x14($sp)
/*  f10b49c:	afa90010 */ 	sw	$t1,0x10($sp)
/*  f10b4a0:	8e0d000c */ 	lw	$t5,0xc($s0)
/*  f10b4a4:	240f0001 */ 	addiu	$t7,$zero,0x1
/*  f10b4a8:	afaf0028 */ 	sw	$t7,0x28($sp)
/*  f10b4ac:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10b4b0:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10b4b4:	8fa4014c */ 	lw	$a0,0x14c($sp)
/*  f10b4b8:	27a50154 */ 	addiu	$a1,$sp,0x154
/*  f10b4bc:	27a60150 */ 	addiu	$a2,$sp,0x150
/*  f10b4c0:	27a700d4 */ 	addiu	$a3,$sp,0xd4
/*  f10b4c4:	afae001c */ 	sw	$t6,0x1c($sp)
/*  f10b4c8:	0fc5580f */ 	jal	textRenderProjected
/*  f10b4cc:	afad0018 */ 	sw	$t5,0x18($sp)
/*  f10b4d0:	afa2014c */ 	sw	$v0,0x14c($sp)
/*  f10b4d4:	8e180000 */ 	lw	$t8,0x0($s0)
/*  f10b4d8:	271900be */ 	addiu	$t9,$t8,0xbe
/*  f10b4dc:	afb90154 */ 	sw	$t9,0x154($sp)
/*  f10b4e0:	8e080004 */ 	lw	$t0,0x4($s0)
/*  f10b4e4:	250a0001 */ 	addiu	$t2,$t0,0x1
/*  f10b4e8:	0c002f02 */ 	jal	viGetX
/*  f10b4ec:	afaa0150 */ 	sw	$t2,0x150($sp)
/*  f10b4f0:	0c002f06 */ 	jal	viGetY
/*  f10b4f4:	a7a20050 */ 	sh	$v0,0x50($sp)
/*  f10b4f8:	3c0b8008 */ 	lui	$t3,%hi(var8007fb10)
/*  f10b4fc:	3c098008 */ 	lui	$t1,%hi(var8007fb0c)
/*  f10b500:	8d29fb0c */ 	lw	$t1,%lo(var8007fb0c)($t1)
/*  f10b504:	8d6bfb10 */ 	lw	$t3,%lo(var8007fb10)($t3)
/*  f10b508:	87ad0050 */ 	lh	$t5,0x50($sp)
/*  f10b50c:	afa90014 */ 	sw	$t1,0x14($sp)
/*  f10b510:	afab0010 */ 	sw	$t3,0x10($sp)
/*  f10b514:	8e0c000c */ 	lw	$t4,0xc($s0)
/*  f10b518:	240e0001 */ 	addiu	$t6,$zero,0x1
/*  f10b51c:	afae0028 */ 	sw	$t6,0x28($sp)
/*  f10b520:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10b524:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10b528:	8fa4014c */ 	lw	$a0,0x14c($sp)
/*  f10b52c:	27a50154 */ 	addiu	$a1,$sp,0x154
/*  f10b530:	27a60150 */ 	addiu	$a2,$sp,0x150
/*  f10b534:	27a70098 */ 	addiu	$a3,$sp,0x98
/*  f10b538:	afad001c */ 	sw	$t5,0x1c($sp)
/*  f10b53c:	0fc5580f */ 	jal	textRenderProjected
/*  f10b540:	afac0018 */ 	sw	$t4,0x18($sp)
/*  f10b544:	3c0f8008 */ 	lui	$t7,%hi(var8007fb0c)
/*  f10b548:	8deffb0c */ 	lw	$t7,%lo(var8007fb0c)($t7)
/*  f10b54c:	3c078008 */ 	lui	$a3,%hi(var8007fb10)
/*  f10b550:	afa2014c */ 	sw	$v0,0x14c($sp)
/*  f10b554:	8ce7fb10 */ 	lw	$a3,%lo(var8007fb10)($a3)
/*  f10b558:	27a40054 */ 	addiu	$a0,$sp,0x54
/*  f10b55c:	27a50058 */ 	addiu	$a1,$sp,0x58
/*  f10b560:	27a6005c */ 	addiu	$a2,$sp,0x5c
/*  f10b564:	afa00014 */ 	sw	$zero,0x14($sp)
/*  f10b568:	0fc55cbe */ 	jal	textMeasure
/*  f10b56c:	afaf0010 */ 	sw	$t7,0x10($sp)
/*  f10b570:	8e180000 */ 	lw	$t8,0x0($s0)
/*  f10b574:	8e190008 */ 	lw	$t9,0x8($s0)
/*  f10b578:	8faa0058 */ 	lw	$t2,0x58($sp)
/*  f10b57c:	03194021 */ 	addu	$t0,$t8,$t9
/*  f10b580:	010a5823 */ 	subu	$t3,$t0,$t2
/*  f10b584:	2569fffa */ 	addiu	$t1,$t3,-6
/*  f10b588:	afa90154 */ 	sw	$t1,0x154($sp)
/*  f10b58c:	8e0c0004 */ 	lw	$t4,0x4($s0)
/*  f10b590:	258d0001 */ 	addiu	$t5,$t4,0x1
/*  f10b594:	0c002f02 */ 	jal	viGetX
/*  f10b598:	afad0150 */ 	sw	$t5,0x150($sp)
/*  f10b59c:	0c002f06 */ 	jal	viGetY
/*  f10b5a0:	a7a20050 */ 	sh	$v0,0x50($sp)
/*  f10b5a4:	3c0e8008 */ 	lui	$t6,%hi(var8007fb10)
/*  f10b5a8:	3c0f8008 */ 	lui	$t7,%hi(var8007fb0c)
/*  f10b5ac:	8deffb0c */ 	lw	$t7,%lo(var8007fb0c)($t7)
/*  f10b5b0:	8dcefb10 */ 	lw	$t6,%lo(var8007fb10)($t6)
/*  f10b5b4:	87b90050 */ 	lh	$t9,0x50($sp)
/*  f10b5b8:	afaf0014 */ 	sw	$t7,0x14($sp)
/*  f10b5bc:	afae0010 */ 	sw	$t6,0x10($sp)
/*  f10b5c0:	8e18000c */ 	lw	$t8,0xc($s0)
/*  f10b5c4:	24080001 */ 	addiu	$t0,$zero,0x1
/*  f10b5c8:	afa80028 */ 	sw	$t0,0x28($sp)
/*  f10b5cc:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10b5d0:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10b5d4:	8fa4014c */ 	lw	$a0,0x14c($sp)
/*  f10b5d8:	27a50154 */ 	addiu	$a1,$sp,0x154
/*  f10b5dc:	27a60150 */ 	addiu	$a2,$sp,0x150
/*  f10b5e0:	27a7005c */ 	addiu	$a3,$sp,0x5c
/*  f10b5e4:	afb9001c */ 	sw	$t9,0x1c($sp)
/*  f10b5e8:	0fc5580f */ 	jal	textRenderProjected
/*  f10b5ec:	afb80018 */ 	sw	$t8,0x18($sp)
/*  f10b5f0:	1000001c */ 	b	.L0f10b664
/*  f10b5f4:	8fbf003c */ 	lw	$ra,0x3c($sp)
/*  f10b5f8:	240a000b */ 	addiu	$t2,$zero,0xb
/*  f10b5fc:	10000017 */ 	b	.L0f10b65c
/*  f10b600:	acea0000 */ 	sw	$t2,0x0($a3)
/*  f10b604:	8ce20000 */ 	lw	$v0,0x0($a3)
/*  f10b608:	24010001 */ 	addiu	$at,$zero,0x1
/*  f10b60c:	3c0d8007 */ 	lui	$t5,%hi(g_MpPlayerNum)
/*  f10b610:	00025880 */ 	sll	$t3,$v0,0x2
/*  f10b614:	006b4821 */ 	addu	$t1,$v1,$t3
/*  f10b618:	8d2c0200 */ 	lw	$t4,0x200($t1)
/*  f10b61c:	3c048007 */ 	lui	$a0,%hi(g_PakDeleteGameNoteMenuDialog)
/*  f10b620:	5581000f */ 	bnel	$t4,$at,.L0f10b660
/*  f10b624:	00001025 */ 	or	$v0,$zero,$zero
/*  f10b628:	8dad1448 */ 	lw	$t5,%lo(g_MpPlayerNum)($t5)
/*  f10b62c:	3c01800a */ 	lui	$at,%hi(g_Menus+0xe28)
/*  f10b630:	24844aa0 */ 	addiu	$a0,$a0,%lo(g_PakDeleteGameNoteMenuDialog)
/*  f10b634:	000d70c0 */ 	sll	$t6,$t5,0x3
/*  f10b638:	01cd7023 */ 	subu	$t6,$t6,$t5
/*  f10b63c:	000e7080 */ 	sll	$t6,$t6,0x2
/*  f10b640:	01cd7021 */ 	addu	$t6,$t6,$t5
/*  f10b644:	000e70c0 */ 	sll	$t6,$t6,0x3
/*  f10b648:	01cd7023 */ 	subu	$t6,$t6,$t5
/*  f10b64c:	000e7100 */ 	sll	$t6,$t6,0x4
/*  f10b650:	002e0821 */ 	addu	$at,$at,$t6
/*  f10b654:	0fc3cbd3 */ 	jal	menuPushDialog
/*  f10b658:	ac22ee28 */ 	sw	$v0,%lo(g_Menus+0xe28)($at)
.L0f10b65c:
/*  f10b65c:	00001025 */ 	or	$v0,$zero,$zero
.L0f10b660:
/*  f10b660:	8fbf003c */ 	lw	$ra,0x3c($sp)
.L0f10b664:
/*  f10b664:	8fb00038 */ 	lw	$s0,0x38($sp)
/*  f10b668:	27bd0158 */ 	addiu	$sp,$sp,0x158
/*  f10b66c:	03e00008 */ 	jr	$ra
/*  f10b670:	00000000 */ 	nop
);

s32 pakGameNotesMenuDialog(u32 operation, struct menudialog *dialog, union handlerdata *data)
{
	if (operation == MENUOP_TICK) {
		if (g_Menus[g_MpPlayerNum].curframe
				&& g_Menus[g_MpPlayerNum].curframe->dialog == dialog) {
			s32 value = func0f1168c4(g_Menus[g_MpPlayerNum].data.pak.device, &g_EditingPak);

			if (value) {
				menuCloseDialog();
				g_EditingPak = NULL;
				g_Menus[g_MpPlayerNum].unke3c = g_Menus[g_MpPlayerNum].data.pak.device;

				if (value == 1) {
					filemanPushErrorDialog(FILEERROR_PAKREMOVED);
				}

				menuUpdateCurFrame();
			}
		}
	}

	return 0;
}

char *pakMenuTextPagesFree(struct menuitem *item)
{
	if (g_EditingPak == NULL) {
		sprintf(g_StringPointer, langGet(L_OPTIONS(394))); // "Pages Free: "
	} else {
		sprintf(g_StringPointer, langGet(L_OPTIONS(395)), g_EditingPak->pagesfree); // "Pages Free: %d"
	}

	return g_StringPointer;
}

char *pakMenuTextPagesUsed(struct menuitem *item)
{
	if (g_EditingPak == NULL) {
		sprintf(g_StringPointer2, langGet(L_OPTIONS(396))); // "Pages Used: "
	} else {
		sprintf(g_StringPointer2, langGet(L_OPTIONS(397)), g_EditingPak->pagesused); // "Pages Used: %d"
	}

	return g_StringPointer2;
}

char *pakMenuTextStatusMessage(struct menuitem *item)
{
	s32 haspdfile = false;
	s32 hasemptyfile = false;
	s32 i;

	if (g_EditingPak == NULL) {
		return langGet(L_OPTIONS(398)); // "Perfect Dark note already exists on this Controller Pak."
	}

	for (i = 0; i < ARRAYCOUNT(g_EditingPak->filestates); i++) {
		if (g_EditingPak->filesinuse[i] == true) {
			if (g_EditingPak->filestates[i].company_code == ROM_COMPANYCODE
					&& g_EditingPak->filestates[i].game_code == ROM_GAMECODE) {
				haspdfile = true;
			}
		} else {
			hasemptyfile = true;
		}
	}

	if (haspdfile) {
		return langGet(L_OPTIONS(398)); // "Perfect Dark note already exists on this Controller Pak."
	}

	if (g_EditingPak->pagesfree < 28 || !hasemptyfile) {
		return langGet(L_OPTIONS(400)); // "Controller Pak is too full to save note- 1 note and 28 pages required to save to Controller Pak."
	}

	return langGet(L_OPTIONS(399)); // "There is enough space for Perfect Dark note."
}

char *pakMenuTextEditingPakName(struct menuitem *item)
{
	return filemanGetDeviceName(g_Menus[g_MpPlayerNum].data.pak.device);
}

s32 pakSelectionMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_CHECKDISABLED) {
		if (pakIsConnected((s8)item->param) == 0) {
			return true;
		}
	}

	if (operation == MENUOP_SET) {
		g_Menus[g_MpPlayerNum].data.filesel.slotcount = item->param;
		menuPushDialog(&g_PakGameNotesMenuDialog);
	}

	return 0;
}

s32 pakChoosePakMenuDialog(u32 operation, struct menudialog *dialog, union handlerdata *data)
{
	switch (operation) {
	case MENUOP_OPEN:
		func0001398c(3);
		g_Menus[g_MpPlayerNum].data.pak.unke24 = 0;
		break;
	case MENUOP_TICK:
		var80062944 = 1;
		break;
	case MENUOP_CLOSE:
		if (g_Vars.stagenum != STAGE_BOOTPAKMENU) {
			s32 i;

			for (i = 0; i < 4; i++) {
				if (g_Menus[g_MpPlayerNum].data.pak.unke24 & (1 << i)) {
					g_Vars.unk0004e4 &= 0xfff0;
					g_Vars.unk0004e4 |= 0x0008;
					g_Vars.unk0004e4 |= 1 << (i + 8);
				}
			}
		}
		func000139c8();
		break;
	}

	return 0;
}

s32 filemanOpenCopyFileMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		g_Menus[g_MpPlayerNum].data.filesel.filetypeplusone = item->param + 1;
		func0f110c5c(0, item->param);
		g_Menus[g_MpPlayerNum].unke3f = 0;
		g_Menus[g_MpPlayerNum].data.filesel.unke28 = 0;
		menuPushDialog(&g_FilemanCopyListMenuDialog);
	}

	return 0;
}

s32 filemanOpenDeleteFileMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	if (operation == MENUOP_SET) {
		g_Menus[g_MpPlayerNum].data.filesel.filetypeplusone = item->param + 1;
		func0f110c5c(0, item->param);
		g_Menus[g_MpPlayerNum].unke3e = -1;
		filemanPushDeleteFileDialog(0);
	}

	return 0;
}

s32 filemanAgentNameKeyboardMenuHandler(u32 operation, struct menuitem *item, union handlerdata *data)
{
	char *name = data->keyboard.string;

	if (!g_SaveLocations.locations[0]) {
		return 0;
	}

	switch (operation) {
	case MENUOP_GETTEXT:
		strcpy(name, g_SoloSaveFile.name);
		break;
	case MENUOP_SETTEXT:
		strcpy(g_SoloSaveFile.name, name);
		break;
	case MENUOP_SET:
		func0f10a51c(0, 0);
		g_Menus[g_MpPlayerNum].data.filesel.unke2c = 1;
		break;
	}

	return 0;
}

GLOBAL_ASM(
glabel fileListChooseAgentMenuHandler
.late_rodata
glabel var7f1b37fc
.word fileListChooseAgentMenuHandler+0x210 # f10bf70
glabel var7f1b3800
.word fileListChooseAgentMenuHandler+0xa08 # f10c768
glabel var7f1b3804
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3808
.word fileListChooseAgentMenuHandler+0xa18 # f10c778
glabel var7f1b380c
.word fileListChooseAgentMenuHandler+0xa50 # f10c7b0
glabel var7f1b3810
.word fileListChooseAgentMenuHandler+0x99c # f10c6fc
glabel var7f1b3814
.word fileListChooseAgentMenuHandler+0x54 # f10bdb4
glabel var7f1b3818
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b381c
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3820
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3824
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3828
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b382c
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3830
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3834
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3838
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b383c
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3840
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3844
.word fileListChooseAgentMenuHandler+0x220 # f10bf80
glabel var7f1b3848
.word fileListChooseAgentMenuHandler+0x990 # f10c6f0
glabel var7f1b384c
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3850
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3854
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b3858
.word fileListChooseAgentMenuHandler+0xa8c # f10c7ec
glabel var7f1b385c
.word fileListChooseAgentMenuHandler+0x64 # f10bdc4
.text
/*  f10bd60:	3c088007 */ 	lui	$t0,%hi(g_SaveLocations)
/*  f10bd64:	8d085bc0 */ 	lw	$t0,%lo(g_SaveLocations)($t0)
/*  f10bd68:	27bdfeb8 */ 	addiu	$sp,$sp,-328
/*  f10bd6c:	afb00038 */ 	sw	$s0,0x38($sp)
/*  f10bd70:	afa5014c */ 	sw	$a1,0x14c($sp)
/*  f10bd74:	00802825 */ 	or	$a1,$a0,$zero
/*  f10bd78:	00c08025 */ 	or	$s0,$a2,$zero
/*  f10bd7c:	afbf003c */ 	sw	$ra,0x3c($sp)
/*  f10bd80:	15000003 */ 	bnez	$t0,.L0f10bd90
/*  f10bd84:	afa40148 */ 	sw	$a0,0x148($sp)
/*  f10bd88:	10000299 */ 	b	.L0f10c7f0
/*  f10bd8c:	00001025 */ 	or	$v0,$zero,$zero
.L0f10bd90:
/*  f10bd90:	24aeffff */ 	addiu	$t6,$a1,-1
/*  f10bd94:	2dc10019 */ 	sltiu	$at,$t6,0x19
/*  f10bd98:	10200294 */ 	beqz	$at,.L0f10c7ec
/*  f10bd9c:	000e7080 */ 	sll	$t6,$t6,0x2
/*  f10bda0:	3c017f1b */ 	lui	$at,%hi(var7f1b37fc)
/*  f10bda4:	002e0821 */ 	addu	$at,$at,$t6
/*  f10bda8:	8c2e37fc */ 	lw	$t6,%lo(var7f1b37fc)($at)
/*  f10bdac:	01c00008 */ 	jr	$t6
/*  f10bdb0:	00000000 */ 	nop
/*  f10bdb4:	3c0f000f */ 	lui	$t7,0xf
/*  f10bdb8:	35efffff */ 	ori	$t7,$t7,0xffff
/*  f10bdbc:	1000028b */ 	b	.L0f10c7ec
/*  f10bdc0:	ae0f0000 */ 	sw	$t7,0x0($s0)
/*  f10bdc4:	8e180004 */ 	lw	$t8,0x4($s0)
/*  f10bdc8:	24030001 */ 	addiu	$v1,$zero,0x1
/*  f10bdcc:	00003025 */ 	or	$a2,$zero,$zero
/*  f10bdd0:	1478003c */ 	bne	$v1,$t8,.L0f10bec4
/*  f10bdd4:	00000000 */ 	nop
/*  f10bdd8:	8e190008 */ 	lw	$t9,0x8($s0)
/*  f10bddc:	3c0b8007 */ 	lui	$t3,%hi(g_MpPlayerNum)
/*  f10bde0:	54790034 */ 	bnel	$v1,$t9,.L0f10beb4
/*  f10bde4:	910f030e */ 	lbu	$t7,0x30e($t0)
/*  f10bde8:	8d6b1448 */ 	lw	$t3,%lo(g_MpPlayerNum)($t3)
/*  f10bdec:	3c0d800a */ 	lui	$t5,%hi(g_Menus)
/*  f10bdf0:	25ade000 */ 	addiu	$t5,$t5,%lo(g_Menus)
/*  f10bdf4:	000b60c0 */ 	sll	$t4,$t3,0x3
/*  f10bdf8:	018b6023 */ 	subu	$t4,$t4,$t3
/*  f10bdfc:	000c6080 */ 	sll	$t4,$t4,0x2
/*  f10be00:	018b6021 */ 	addu	$t4,$t4,$t3
/*  f10be04:	000c60c0 */ 	sll	$t4,$t4,0x3
/*  f10be08:	018b6023 */ 	subu	$t4,$t4,$t3
/*  f10be0c:	000c6100 */ 	sll	$t4,$t4,0x4
/*  f10be10:	018d1021 */ 	addu	$v0,$t4,$t5
/*  f10be14:	8c4e0e2c */ 	lw	$t6,0xe2c($v0)
/*  f10be18:	546e0026 */ 	bnel	$v1,$t6,.L0f10beb4
/*  f10be1c:	910f030e */ 	lbu	$t7,0x30e($t0)
/*  f10be20:	850502d0 */ 	lh	$a1,0x2d0($t0)
/*  f10be24:	3c07800a */ 	lui	$a3,%hi(var800a22c0)
/*  f10be28:	24e722c0 */ 	addiu	$a3,$a3,%lo(var800a22c0)
/*  f10be2c:	18a0001f */ 	blez	$a1,.L0f10beac
/*  f10be30:	00002025 */ 	or	$a0,$zero,$zero
/*  f10be34:	00001025 */ 	or	$v0,$zero,$zero
.L0f10be38:
/*  f10be38:	01021821 */ 	addu	$v1,$t0,$v0
/*  f10be3c:	8c780000 */ 	lw	$t8,0x0($v1)
/*  f10be40:	8cef0000 */ 	lw	$t7,0x0($a3)
/*  f10be44:	55f8000a */ 	bnel	$t7,$t8,.L0f10be70
/*  f10be48:	24840001 */ 	addiu	$a0,$a0,0x1
/*  f10be4c:	94f90004 */ 	lhu	$t9,0x4($a3)
/*  f10be50:	946b0004 */ 	lhu	$t3,0x4($v1)
/*  f10be54:	572b0006 */ 	bnel	$t9,$t3,.L0f10be70
/*  f10be58:	24840001 */ 	addiu	$a0,$a0,0x1
/*  f10be5c:	ae040000 */ 	sw	$a0,0x0($s0)
/*  f10be60:	3c088007 */ 	lui	$t0,%hi(g_SaveLocations)
/*  f10be64:	8d085bc0 */ 	lw	$t0,%lo(g_SaveLocations)($t0)
/*  f10be68:	850502d0 */ 	lh	$a1,0x2d0($t0)
/*  f10be6c:	24840001 */ 	addiu	$a0,$a0,0x1
.L0f10be70:
/*  f10be70:	0085082a */ 	slt	$at,$a0,$a1
/*  f10be74:	1420fff0 */ 	bnez	$at,.L0f10be38
/*  f10be78:	24420018 */ 	addiu	$v0,$v0,0x18
/*  f10be7c:	3c0c8007 */ 	lui	$t4,%hi(g_MpPlayerNum)
/*  f10be80:	8d8c1448 */ 	lw	$t4,%lo(g_MpPlayerNum)($t4)
/*  f10be84:	3c0e800a */ 	lui	$t6,%hi(g_Menus)
/*  f10be88:	25cee000 */ 	addiu	$t6,$t6,%lo(g_Menus)
/*  f10be8c:	000c68c0 */ 	sll	$t5,$t4,0x3
/*  f10be90:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f10be94:	000d6880 */ 	sll	$t5,$t5,0x2
/*  f10be98:	01ac6821 */ 	addu	$t5,$t5,$t4
/*  f10be9c:	000d68c0 */ 	sll	$t5,$t5,0x3
/*  f10bea0:	01ac6823 */ 	subu	$t5,$t5,$t4
/*  f10bea4:	000d6900 */ 	sll	$t5,$t5,0x4
/*  f10bea8:	01ae1021 */ 	addu	$v0,$t5,$t6
.L0f10beac:
/*  f10beac:	ac400e2c */ 	sw	$zero,0xe2c($v0)
/*  f10beb0:	910f030e */ 	lbu	$t7,0x30e($t0)
.L0f10beb4:
/*  f10beb4:	11e00010 */ 	beqz	$t7,.L0f10bef8
/*  f10beb8:	00000000 */ 	nop
/*  f10bebc:	1000000e */ 	b	.L0f10bef8
/*  f10bec0:	24060001 */ 	addiu	$a2,$zero,0x1
.L0f10bec4:
/*  f10bec4:	3c188007 */ 	lui	$t8,%hi(g_MpPlayerNum)
/*  f10bec8:	8f181448 */ 	lw	$t8,%lo(g_MpPlayerNum)($t8)
/*  f10becc:	3c01800a */ 	lui	$at,%hi(g_Menus+0xe2c)
/*  f10bed0:	24060001 */ 	addiu	$a2,$zero,0x1
/*  f10bed4:	0018c8c0 */ 	sll	$t9,$t8,0x3
/*  f10bed8:	0338c823 */ 	subu	$t9,$t9,$t8
/*  f10bedc:	0019c880 */ 	sll	$t9,$t9,0x2
/*  f10bee0:	0338c821 */ 	addu	$t9,$t9,$t8
/*  f10bee4:	0019c8c0 */ 	sll	$t9,$t9,0x3
/*  f10bee8:	0338c823 */ 	subu	$t9,$t9,$t8
/*  f10beec:	0019c900 */ 	sll	$t9,$t9,0x4
/*  f10bef0:	00390821 */ 	addu	$at,$at,$t9
/*  f10bef4:	ac20ee2c */ 	sw	$zero,%lo(g_Menus+0xe2c)($at)
.L0f10bef8:
/*  f10bef8:	10c0023c */ 	beqz	$a2,.L0f10c7ec
/*  f10befc:	3c06800a */ 	lui	$a2,%hi(g_Vars)
/*  f10bf00:	24c69fc0 */ 	addiu	$a2,$a2,%lo(g_Vars)
/*  f10bf04:	8ccb047c */ 	lw	$t3,0x47c($a2)
/*  f10bf08:	51600239 */ 	beqzl	$t3,.L0f10c7f0
/*  f10bf0c:	00001025 */ 	or	$v0,$zero,$zero
/*  f10bf10:	850502d0 */ 	lh	$a1,0x2d0($t0)
/*  f10bf14:	00002025 */ 	or	$a0,$zero,$zero
/*  f10bf18:	00001025 */ 	or	$v0,$zero,$zero
/*  f10bf1c:	18a00233 */ 	blez	$a1,.L0f10c7ec
.L0f10bf20:
/*  f10bf20:	01021821 */ 	addu	$v1,$t0,$v0
/*  f10bf24:	8c6d0000 */ 	lw	$t5,0x0($v1)
/*  f10bf28:	8ccc047c */ 	lw	$t4,0x47c($a2)
/*  f10bf2c:	558d000b */ 	bnel	$t4,$t5,.L0f10bf5c
/*  f10bf30:	24840001 */ 	addiu	$a0,$a0,0x1
/*  f10bf34:	94ce0480 */ 	lhu	$t6,0x480($a2)
/*  f10bf38:	946f0004 */ 	lhu	$t7,0x4($v1)
/*  f10bf3c:	55cf0007 */ 	bnel	$t6,$t7,.L0f10bf5c
/*  f10bf40:	24840001 */ 	addiu	$a0,$a0,0x1
/*  f10bf44:	ae040000 */ 	sw	$a0,0x0($s0)
/*  f10bf48:	acc0047c */ 	sw	$zero,0x47c($a2)
/*  f10bf4c:	3c088007 */ 	lui	$t0,%hi(g_SaveLocations)
/*  f10bf50:	8d085bc0 */ 	lw	$t0,%lo(g_SaveLocations)($t0)
/*  f10bf54:	850502d0 */ 	lh	$a1,0x2d0($t0)
/*  f10bf58:	24840001 */ 	addiu	$a0,$a0,0x1
.L0f10bf5c:
/*  f10bf5c:	0085082a */ 	slt	$at,$a0,$a1
/*  f10bf60:	1420ffef */ 	bnez	$at,.L0f10bf20
/*  f10bf64:	24420018 */ 	addiu	$v0,$v0,0x18
/*  f10bf68:	10000221 */ 	b	.L0f10c7f0
/*  f10bf6c:	00001025 */ 	or	$v0,$zero,$zero
/*  f10bf70:	851802d0 */ 	lh	$t8,0x2d0($t0)
/*  f10bf74:	27190001 */ 	addiu	$t9,$t8,0x1
/*  f10bf78:	1000021c */ 	b	.L0f10c7ec
/*  f10bf7c:	ae190000 */ 	sw	$t9,0x0($s0)
/*  f10bf80:	8e0b0000 */ 	lw	$t3,0x0($s0)
/*  f10bf84:	2409000c */ 	addiu	$t1,$zero,0xc
/*  f10bf88:	00005025 */ 	or	$t2,$zero,$zero
/*  f10bf8c:	afab0130 */ 	sw	$t3,0x130($sp)
/*  f10bf90:	8e0c0008 */ 	lw	$t4,0x8($s0)
/*  f10bf94:	afa00100 */ 	sw	$zero,0x100($sp)
/*  f10bf98:	afa00104 */ 	sw	$zero,0x104($sp)
/*  f10bf9c:	afa00108 */ 	sw	$zero,0x108($sp)
/*  f10bfa0:	afa0010c */ 	sw	$zero,0x10c($sp)
/*  f10bfa4:	afac012c */ 	sw	$t4,0x12c($sp)
/*  f10bfa8:	8e020004 */ 	lw	$v0,0x4($s0)
/*  f10bfac:	850d02d0 */ 	lh	$t5,0x2d0($t0)
/*  f10bfb0:	00027080 */ 	sll	$t6,$v0,0x2
/*  f10bfb4:	104d003c */ 	beq	$v0,$t5,.L0f10c0a8
/*  f10bfb8:	01c27023 */ 	subu	$t6,$t6,$v0
/*  f10bfbc:	000e70c0 */ 	sll	$t6,$t6,0x3
/*  f10bfc0:	010e5021 */ 	addu	$t2,$t0,$t6
/*  f10bfc4:	11400038 */ 	beqz	$t2,.L0f10c0a8
/*  f10bfc8:	25440006 */ 	addiu	$a0,$t2,0x6
/*  f10bfcc:	27af0110 */ 	addiu	$t7,$sp,0x110
/*  f10bfd0:	afaf0010 */ 	sw	$t7,0x10($sp)
/*  f10bfd4:	27a50118 */ 	addiu	$a1,$sp,0x118
/*  f10bfd8:	27a60117 */ 	addiu	$a2,$sp,0x117
/*  f10bfdc:	27a70116 */ 	addiu	$a3,$sp,0x116
/*  f10bfe0:	0fc4417f */ 	jal	func0f1105fc
/*  f10bfe4:	afaa0124 */ 	sw	$t2,0x124($sp)
/*  f10bfe8:	8fa60110 */ 	lw	$a2,0x110($sp)
/*  f10bfec:	2407003c */ 	addiu	$a3,$zero,0x3c
/*  f10bff0:	93a50117 */ 	lbu	$a1,0x117($sp)
/*  f10bff4:	00c7001b */ 	divu	$zero,$a2,$a3
/*  f10bff8:	0000c010 */ 	mfhi	$t8
/*  f10bffc:	00003012 */ 	mflo	$a2
/*  f10c000:	28a10012 */ 	slti	$at,$a1,0x12
/*  f10c004:	8faa0124 */ 	lw	$t2,0x124($sp)
/*  f10c008:	14e00002 */ 	bnez	$a3,.L0f10c014
/*  f10c00c:	00000000 */ 	nop
/*  f10c010:	0007000d */ 	break	0x7
.L0f10c014:
/*  f10c014:	afb80100 */ 	sw	$t8,0x100($sp)
/*  f10c018:	54200005 */ 	bnezl	$at,.L0f10c030
/*  f10c01c:	93ab0116 */ 	lbu	$t3,0x116($sp)
/*  f10c020:	24190011 */ 	addiu	$t9,$zero,0x11
/*  f10c024:	a3b90117 */ 	sb	$t9,0x117($sp)
/*  f10c028:	24050011 */ 	addiu	$a1,$zero,0x11
/*  f10c02c:	93ab0116 */ 	lbu	$t3,0x116($sp)
.L0f10c030:
/*  f10c030:	24a9000c */ 	addiu	$t1,$a1,0xc
/*  f10c034:	29610003 */ 	slti	$at,$t3,0x3
/*  f10c038:	54200004 */ 	bnezl	$at,.L0f10c04c
/*  f10c03c:	240105a0 */ 	addiu	$at,$zero,0x5a0
/*  f10c040:	240c0002 */ 	addiu	$t4,$zero,0x2
/*  f10c044:	a3ac0116 */ 	sb	$t4,0x116($sp)
/*  f10c048:	240105a0 */ 	addiu	$at,$zero,0x5a0
.L0f10c04c:
/*  f10c04c:	00c1001b */ 	divu	$zero,$a2,$at
/*  f10c050:	00001012 */ 	mflo	$v0
/*  f10c054:	00026880 */ 	sll	$t5,$v0,0x2
/*  f10c058:	01a26823 */ 	subu	$t5,$t5,$v0
/*  f10c05c:	000d6880 */ 	sll	$t5,$t5,0x2
/*  f10c060:	01a26823 */ 	subu	$t5,$t5,$v0
/*  f10c064:	000d6880 */ 	sll	$t5,$t5,0x2
/*  f10c068:	01a26821 */ 	addu	$t5,$t5,$v0
/*  f10c06c:	000d6940 */ 	sll	$t5,$t5,0x5
/*  f10c070:	00cd1823 */ 	subu	$v1,$a2,$t5
/*  f10c074:	0067001b */ 	divu	$zero,$v1,$a3
/*  f10c078:	00002012 */ 	mflo	$a0
/*  f10c07c:	00047100 */ 	sll	$t6,$a0,0x4
/*  f10c080:	01c47023 */ 	subu	$t6,$t6,$a0
/*  f10c084:	000e7080 */ 	sll	$t6,$t6,0x2
/*  f10c088:	006e7823 */ 	subu	$t7,$v1,$t6
/*  f10c08c:	afa2010c */ 	sw	$v0,0x10c($sp)
/*  f10c090:	14e00002 */ 	bnez	$a3,.L0f10c09c
/*  f10c094:	00000000 */ 	nop
/*  f10c098:	0007000d */ 	break	0x7
.L0f10c09c:
/*  f10c09c:	afa40108 */ 	sw	$a0,0x108($sp)
/*  f10c0a0:	afaf0104 */ 	sw	$t7,0x104($sp)
/*  f10c0a4:	afa60110 */ 	sw	$a2,0x110($sp)
.L0f10c0a8:
/*  f10c0a8:	8fb80130 */ 	lw	$t8,0x130($sp)
/*  f10c0ac:	3c0be700 */ 	lui	$t3,0xe700
/*  f10c0b0:	3c0eba00 */ 	lui	$t6,0xba00
/*  f10c0b4:	27190008 */ 	addiu	$t9,$t8,0x8
/*  f10c0b8:	afb90130 */ 	sw	$t9,0x130($sp)
/*  f10c0bc:	af000004 */ 	sw	$zero,0x4($t8)
/*  f10c0c0:	af0b0000 */ 	sw	$t3,0x0($t8)
/*  f10c0c4:	8fac0130 */ 	lw	$t4,0x130($sp)
/*  f10c0c8:	35ce1301 */ 	ori	$t6,$t6,0x1301
/*  f10c0cc:	3c19b900 */ 	lui	$t9,0xb900
/*  f10c0d0:	258d0008 */ 	addiu	$t5,$t4,0x8
/*  f10c0d4:	afad0130 */ 	sw	$t5,0x130($sp)
/*  f10c0d8:	ad800004 */ 	sw	$zero,0x4($t4)
/*  f10c0dc:	ad8e0000 */ 	sw	$t6,0x0($t4)
/*  f10c0e0:	8faf0130 */ 	lw	$t7,0x130($sp)
/*  f10c0e4:	37390002 */ 	ori	$t9,$t9,0x2
/*  f10c0e8:	3c0dba00 */ 	lui	$t5,0xba00
/*  f10c0ec:	25f80008 */ 	addiu	$t8,$t7,0x8
/*  f10c0f0:	afb80130 */ 	sw	$t8,0x130($sp)
/*  f10c0f4:	ade00004 */ 	sw	$zero,0x4($t7)
/*  f10c0f8:	adf90000 */ 	sw	$t9,0x0($t7)
/*  f10c0fc:	8fab0130 */ 	lw	$t3,0x130($sp)
/*  f10c100:	35ad1001 */ 	ori	$t5,$t5,0x1001
/*  f10c104:	3c18ba00 */ 	lui	$t8,0xba00
/*  f10c108:	256c0008 */ 	addiu	$t4,$t3,0x8
/*  f10c10c:	afac0130 */ 	sw	$t4,0x130($sp)
/*  f10c110:	ad600004 */ 	sw	$zero,0x4($t3)
/*  f10c114:	ad6d0000 */ 	sw	$t5,0x0($t3)
/*  f10c118:	8fae0130 */ 	lw	$t6,0x130($sp)
/*  f10c11c:	37180903 */ 	ori	$t8,$t8,0x903
/*  f10c120:	24190c00 */ 	addiu	$t9,$zero,0xc00
/*  f10c124:	25cf0008 */ 	addiu	$t7,$t6,0x8
/*  f10c128:	afaf0130 */ 	sw	$t7,0x130($sp)
/*  f10c12c:	add90004 */ 	sw	$t9,0x4($t6)
/*  f10c130:	add80000 */ 	sw	$t8,0x0($t6)
/*  f10c134:	00095880 */ 	sll	$t3,$t1,0x2
/*  f10c138:	3c0c800b */ 	lui	$t4,%hi(var800ab5a8)
/*  f10c13c:	8d8cb5a8 */ 	lw	$t4,%lo(var800ab5a8)($t4)
/*  f10c140:	01695823 */ 	subu	$t3,$t3,$t1
/*  f10c144:	000b5880 */ 	sll	$t3,$t3,0x2
/*  f10c148:	240e0001 */ 	addiu	$t6,$zero,0x1
/*  f10c14c:	240d0002 */ 	addiu	$t5,$zero,0x2
/*  f10c150:	afad0010 */ 	sw	$t5,0x10($sp)
/*  f10c154:	afae0014 */ 	sw	$t6,0x14($sp)
/*  f10c158:	afaa0124 */ 	sw	$t2,0x124($sp)
/*  f10c15c:	afa00018 */ 	sw	$zero,0x18($sp)
/*  f10c160:	27a40130 */ 	addiu	$a0,$sp,0x130
/*  f10c164:	24060002 */ 	addiu	$a2,$zero,0x2
/*  f10c168:	00003825 */ 	or	$a3,$zero,$zero
/*  f10c16c:	0fc2ce70 */ 	jal	func0f0b39c0
/*  f10c170:	016c2821 */ 	addu	$a1,$t3,$t4
/*  f10c174:	8faf0130 */ 	lw	$t7,0x130($sp)
/*  f10c178:	8fa7012c */ 	lw	$a3,0x12c($sp)
/*  f10c17c:	8faa0124 */ 	lw	$t2,0x124($sp)
/*  f10c180:	25f80008 */ 	addiu	$t8,$t7,0x8
/*  f10c184:	afb80130 */ 	sw	$t8,0x130($sp)
/*  f10c188:	3c19ba00 */ 	lui	$t9,0xba00
/*  f10c18c:	37391402 */ 	ori	$t9,$t9,0x1402
/*  f10c190:	adf90000 */ 	sw	$t9,0x0($t7)
/*  f10c194:	ade00004 */ 	sw	$zero,0x4($t7)
/*  f10c198:	8fab0130 */ 	lw	$t3,0x130($sp)
/*  f10c19c:	3c0dba00 */ 	lui	$t5,0xba00
/*  f10c1a0:	35ad0c02 */ 	ori	$t5,$t5,0xc02
/*  f10c1a4:	256c0008 */ 	addiu	$t4,$t3,0x8
/*  f10c1a8:	afac0130 */ 	sw	$t4,0x130($sp)
/*  f10c1ac:	ad600004 */ 	sw	$zero,0x4($t3)
/*  f10c1b0:	ad6d0000 */ 	sw	$t5,0x0($t3)
/*  f10c1b4:	8fae0130 */ 	lw	$t6,0x130($sp)
/*  f10c1b8:	3c18fb00 */ 	lui	$t8,0xfb00
/*  f10c1bc:	2401ff00 */ 	addiu	$at,$zero,-256
/*  f10c1c0:	25cf0008 */ 	addiu	$t7,$t6,0x8
/*  f10c1c4:	afaf0130 */ 	sw	$t7,0x130($sp)
/*  f10c1c8:	add80000 */ 	sw	$t8,0x0($t6)
/*  f10c1cc:	8cf9000c */ 	lw	$t9,0xc($a3)
/*  f10c1d0:	01c02025 */ 	or	$a0,$t6,$zero
/*  f10c1d4:	3c18ff37 */ 	lui	$t8,0xff37
/*  f10c1d8:	332b00ff */ 	andi	$t3,$t9,0xff
/*  f10c1dc:	01616025 */ 	or	$t4,$t3,$at
/*  f10c1e0:	adcc0004 */ 	sw	$t4,0x4($t6)
/*  f10c1e4:	8fad0130 */ 	lw	$t5,0x130($sp)
/*  f10c1e8:	3c0ffc12 */ 	lui	$t7,0xfc12
/*  f10c1ec:	35ef9a25 */ 	ori	$t7,$t7,0x9a25
/*  f10c1f0:	25ae0008 */ 	addiu	$t6,$t5,0x8
/*  f10c1f4:	afae0130 */ 	sw	$t6,0x130($sp)
/*  f10c1f8:	3718ffff */ 	ori	$t8,$t8,0xffff
/*  f10c1fc:	adb80004 */ 	sw	$t8,0x4($t5)
/*  f10c200:	adaf0000 */ 	sw	$t7,0x0($t5)
/*  f10c204:	8fb90130 */ 	lw	$t9,0x130($sp)
/*  f10c208:	3c088008 */ 	lui	$t0,%hi(g_ScreenWidthMultiplier)
/*  f10c20c:	2508fac0 */ 	addiu	$t0,$t0,%lo(g_ScreenWidthMultiplier)
/*  f10c210:	272b0008 */ 	addiu	$t3,$t9,0x8
/*  f10c214:	afab0130 */ 	sw	$t3,0x130($sp)
/*  f10c218:	8cec0000 */ 	lw	$t4,0x0($a3)
/*  f10c21c:	8d0f0000 */ 	lw	$t7,0x0($t0)
/*  f10c220:	01a02825 */ 	or	$a1,$t5,$zero
/*  f10c224:	258d003c */ 	addiu	$t5,$t4,0x3c
/*  f10c228:	000d7080 */ 	sll	$t6,$t5,0x2
/*  f10c22c:	01cf0019 */ 	multu	$t6,$t7
/*  f10c230:	8ced0004 */ 	lw	$t5,0x4($a3)
/*  f10c234:	03203025 */ 	or	$a2,$t9,$zero
/*  f10c238:	3c01e400 */ 	lui	$at,0xe400
/*  f10c23c:	25ae0026 */ 	addiu	$t6,$t5,0x26
/*  f10c240:	000e7880 */ 	sll	$t7,$t6,0x2
/*  f10c244:	0000c012 */ 	mflo	$t8
/*  f10c248:	33190fff */ 	andi	$t9,$t8,0xfff
/*  f10c24c:	00195b00 */ 	sll	$t3,$t9,0xc
/*  f10c250:	01616025 */ 	or	$t4,$t3,$at
/*  f10c254:	31f80fff */ 	andi	$t8,$t7,0xfff
/*  f10c258:	0198c825 */ 	or	$t9,$t4,$t8
/*  f10c25c:	acd90000 */ 	sw	$t9,0x0($a2)
/*  f10c260:	8ceb0000 */ 	lw	$t3,0x0($a3)
/*  f10c264:	8d0f0000 */ 	lw	$t7,0x0($t0)
/*  f10c268:	256d0004 */ 	addiu	$t5,$t3,0x4
/*  f10c26c:	000d7080 */ 	sll	$t6,$t5,0x2
/*  f10c270:	01cf0019 */ 	multu	$t6,$t7
/*  f10c274:	8ceb0004 */ 	lw	$t3,0x4($a3)
/*  f10c278:	256d0002 */ 	addiu	$t5,$t3,0x2
/*  f10c27c:	000d7080 */ 	sll	$t6,$t5,0x2
/*  f10c280:	31cf0fff */ 	andi	$t7,$t6,0xfff
/*  f10c284:	240e0480 */ 	addiu	$t6,$zero,0x480
/*  f10c288:	3c0db400 */ 	lui	$t5,0xb400
/*  f10c28c:	00006012 */ 	mflo	$t4
/*  f10c290:	31980fff */ 	andi	$t8,$t4,0xfff
/*  f10c294:	0018cb00 */ 	sll	$t9,$t8,0xc
/*  f10c298:	032f6025 */ 	or	$t4,$t9,$t7
/*  f10c29c:	accc0004 */ 	sw	$t4,0x4($a2)
/*  f10c2a0:	8fb80130 */ 	lw	$t8,0x130($sp)
/*  f10c2a4:	3c0cb300 */ 	lui	$t4,0xb300
/*  f10c2a8:	270b0008 */ 	addiu	$t3,$t8,0x8
/*  f10c2ac:	afab0130 */ 	sw	$t3,0x130($sp)
/*  f10c2b0:	af0e0004 */ 	sw	$t6,0x4($t8)
/*  f10c2b4:	af0d0000 */ 	sw	$t5,0x0($t8)
/*  f10c2b8:	8fb90130 */ 	lw	$t9,0x130($sp)
/*  f10c2bc:	03001025 */ 	or	$v0,$t8,$zero
/*  f10c2c0:	240b0400 */ 	addiu	$t3,$zero,0x400
/*  f10c2c4:	272f0008 */ 	addiu	$t7,$t9,0x8
/*  f10c2c8:	afaf0130 */ 	sw	$t7,0x130($sp)
/*  f10c2cc:	af2c0000 */ 	sw	$t4,0x0($t9)
/*  f10c2d0:	8d180000 */ 	lw	$t8,0x0($t0)
/*  f10c2d4:	03201825 */ 	or	$v1,$t9,$zero
/*  f10c2d8:	0178001a */ 	div	$zero,$t3,$t8
/*  f10c2dc:	00007012 */ 	mflo	$t6
/*  f10c2e0:	000ecc00 */ 	sll	$t9,$t6,0x10
/*  f10c2e4:	372ffc00 */ 	ori	$t7,$t9,0xfc00
/*  f10c2e8:	ac6f0004 */ 	sw	$t7,0x4($v1)
/*  f10c2ec:	8cec0000 */ 	lw	$t4,0x0($a3)
/*  f10c2f0:	17000002 */ 	bnez	$t8,.L0f10c2fc
/*  f10c2f4:	00000000 */ 	nop
/*  f10c2f8:	0007000d */ 	break	0x7
.L0f10c2fc:
/*  f10c2fc:	2401ffff */ 	addiu	$at,$zero,-1
/*  f10c300:	17010004 */ 	bne	$t8,$at,.L0f10c314
/*  f10c304:	3c018000 */ 	lui	$at,0x8000
/*  f10c308:	15610002 */ 	bne	$t3,$at,.L0f10c314
/*  f10c30c:	00000000 */ 	nop
/*  f10c310:	0006000d */ 	break	0x6
.L0f10c314:
/*  f10c314:	258b003e */ 	addiu	$t3,$t4,0x3e
/*  f10c318:	afab0144 */ 	sw	$t3,0x144($sp)
/*  f10c31c:	8cf80004 */ 	lw	$t8,0x4($a3)
/*  f10c320:	8fa40130 */ 	lw	$a0,0x130($sp)
/*  f10c324:	afaa0124 */ 	sw	$t2,0x124($sp)
/*  f10c328:	270d0004 */ 	addiu	$t5,$t8,0x4
/*  f10c32c:	0fc54d8a */ 	jal	func0f153628
/*  f10c330:	afad0140 */ 	sw	$t5,0x140($sp)
/*  f10c334:	3c198007 */ 	lui	$t9,%hi(g_SaveLocations)
/*  f10c338:	8f395bc0 */ 	lw	$t9,%lo(g_SaveLocations)($t9)
/*  f10c33c:	afa20130 */ 	sw	$v0,0x130($sp)
/*  f10c340:	8e0e0004 */ 	lw	$t6,0x4($s0)
/*  f10c344:	872f02d0 */ 	lh	$t7,0x2d0($t9)
/*  f10c348:	8faa0124 */ 	lw	$t2,0x124($sp)
/*  f10c34c:	15cf001d */ 	bne	$t6,$t7,.L0f10c3c4
/*  f10c350:	00000000 */ 	nop
/*  f10c354:	0fc5b9f1 */ 	jal	langGet
/*  f10c358:	24045793 */ 	addiu	$a0,$zero,0x5793
/*  f10c35c:	0c002f02 */ 	jal	viGetX
/*  f10c360:	afa20058 */ 	sw	$v0,0x58($sp)
/*  f10c364:	00028400 */ 	sll	$s0,$v0,0x10
/*  f10c368:	00106403 */ 	sra	$t4,$s0,0x10
/*  f10c36c:	0c002f06 */ 	jal	viGetY
/*  f10c370:	01808025 */ 	or	$s0,$t4,$zero
/*  f10c374:	3c0b8008 */ 	lui	$t3,%hi(var8007fb18)
/*  f10c378:	3c188008 */ 	lui	$t8,%hi(var8007fb14)
/*  f10c37c:	8f18fb14 */ 	lw	$t8,%lo(var8007fb14)($t8)
/*  f10c380:	8d6bfb18 */ 	lw	$t3,%lo(var8007fb18)($t3)
/*  f10c384:	8fad012c */ 	lw	$t5,0x12c($sp)
/*  f10c388:	afb80014 */ 	sw	$t8,0x14($sp)
/*  f10c38c:	afab0010 */ 	sw	$t3,0x10($sp)
/*  f10c390:	8db9000c */ 	lw	$t9,0xc($t5)
/*  f10c394:	afa00028 */ 	sw	$zero,0x28($sp)
/*  f10c398:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10c39c:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10c3a0:	afb0001c */ 	sw	$s0,0x1c($sp)
/*  f10c3a4:	8fa40130 */ 	lw	$a0,0x130($sp)
/*  f10c3a8:	27a50144 */ 	addiu	$a1,$sp,0x144
/*  f10c3ac:	27a60140 */ 	addiu	$a2,$sp,0x140
/*  f10c3b0:	8fa70058 */ 	lw	$a3,0x58($sp)
/*  f10c3b4:	0fc5580f */ 	jal	textRenderProjected
/*  f10c3b8:	afb90018 */ 	sw	$t9,0x18($sp)
/*  f10c3bc:	100000c8 */ 	b	.L0f10c6e0
/*  f10c3c0:	afa20130 */ 	sw	$v0,0x130($sp)
.L0f10c3c4:
/*  f10c3c4:	114000c6 */ 	beqz	$t2,.L0f10c6e0
/*  f10c3c8:	00000000 */ 	nop
/*  f10c3cc:	0c002f02 */ 	jal	viGetX
/*  f10c3d0:	00000000 */ 	nop
/*  f10c3d4:	00028400 */ 	sll	$s0,$v0,0x10
/*  f10c3d8:	00107403 */ 	sra	$t6,$s0,0x10
/*  f10c3dc:	0c002f06 */ 	jal	viGetY
/*  f10c3e0:	01c08025 */ 	or	$s0,$t6,$zero
/*  f10c3e4:	3c0f8008 */ 	lui	$t7,%hi(var8007fb18)
/*  f10c3e8:	3c0c8008 */ 	lui	$t4,%hi(var8007fb14)
/*  f10c3ec:	8d8cfb14 */ 	lw	$t4,%lo(var8007fb14)($t4)
/*  f10c3f0:	8deffb18 */ 	lw	$t7,%lo(var8007fb18)($t7)
/*  f10c3f4:	8fab012c */ 	lw	$t3,0x12c($sp)
/*  f10c3f8:	afac0014 */ 	sw	$t4,0x14($sp)
/*  f10c3fc:	afaf0010 */ 	sw	$t7,0x10($sp)
/*  f10c400:	8d78000c */ 	lw	$t8,0xc($t3)
/*  f10c404:	240d0001 */ 	addiu	$t5,$zero,0x1
/*  f10c408:	afad0028 */ 	sw	$t5,0x28($sp)
/*  f10c40c:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10c410:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10c414:	afb0001c */ 	sw	$s0,0x1c($sp)
/*  f10c418:	8fa40130 */ 	lw	$a0,0x130($sp)
/*  f10c41c:	27a50144 */ 	addiu	$a1,$sp,0x144
/*  f10c420:	27a60140 */ 	addiu	$a2,$sp,0x140
/*  f10c424:	27a70118 */ 	addiu	$a3,$sp,0x118
/*  f10c428:	0fc5580f */ 	jal	textRenderProjected
/*  f10c42c:	afb80018 */ 	sw	$t8,0x18($sp)
/*  f10c430:	8fa3012c */ 	lw	$v1,0x12c($sp)
/*  f10c434:	afa20130 */ 	sw	$v0,0x130($sp)
/*  f10c438:	93a50117 */ 	lbu	$a1,0x117($sp)
/*  f10c43c:	8c790004 */ 	lw	$t9,0x4($v1)
/*  f10c440:	272e0012 */ 	addiu	$t6,$t9,0x12
/*  f10c444:	afae0140 */ 	sw	$t6,0x140($sp)
/*  f10c448:	8c6f0000 */ 	lw	$t7,0x0($v1)
/*  f10c44c:	25ec003e */ 	addiu	$t4,$t7,0x3e
/*  f10c450:	18a00019 */ 	blez	$a1,.L0f10c4b8
/*  f10c454:	afac0144 */ 	sw	$t4,0x144($sp)
/*  f10c458:	00055880 */ 	sll	$t3,$a1,0x2
/*  f10c45c:	01655823 */ 	subu	$t3,$t3,$a1
/*  f10c460:	000b5880 */ 	sll	$t3,$t3,0x2
/*  f10c464:	3c048007 */ 	lui	$a0,%hi(menudialog_selectdifficulty3+0x12)
/*  f10c468:	008b2021 */ 	addu	$a0,$a0,$t3
/*  f10c46c:	0fc5b9f1 */ 	jal	langGet
/*  f10c470:	94841e66 */ 	lhu	$a0,%lo(menudialog_selectdifficulty3+0x12)($a0)
/*  f10c474:	93b80117 */ 	lbu	$t8,0x117($sp)
/*  f10c478:	3c048007 */ 	lui	$a0,%hi(menudialog_selectdifficulty3+0x14)
/*  f10c47c:	00408025 */ 	or	$s0,$v0,$zero
/*  f10c480:	00186880 */ 	sll	$t5,$t8,0x2
/*  f10c484:	01b86823 */ 	subu	$t5,$t5,$t8
/*  f10c488:	000d6880 */ 	sll	$t5,$t5,0x2
/*  f10c48c:	008d2021 */ 	addu	$a0,$a0,$t5
/*  f10c490:	0fc5b9f1 */ 	jal	langGet
/*  f10c494:	94841e68 */ 	lhu	$a0,%lo(menudialog_selectdifficulty3+0x14)($a0)
/*  f10c498:	3c057f1b */ 	lui	$a1,%hi(var7f1b350c)
/*  f10c49c:	24a5350c */ 	addiu	$a1,$a1,%lo(var7f1b350c)
/*  f10c4a0:	27a4009c */ 	addiu	$a0,$sp,0x9c
/*  f10c4a4:	02003025 */ 	or	$a2,$s0,$zero
/*  f10c4a8:	0c004dad */ 	jal	sprintf
/*  f10c4ac:	00403825 */ 	or	$a3,$v0,$zero
/*  f10c4b0:	10000006 */ 	b	.L0f10c4cc
/*  f10c4b4:	00000000 */ 	nop
.L0f10c4b8:
/*  f10c4b8:	0fc5b9f1 */ 	jal	langGet
/*  f10c4bc:	24045794 */ 	addiu	$a0,$zero,0x5794
/*  f10c4c0:	27a4009c */ 	addiu	$a0,$sp,0x9c
/*  f10c4c4:	0c004c4c */ 	jal	strcpy
/*  f10c4c8:	00402825 */ 	or	$a1,$v0,$zero
.L0f10c4cc:
/*  f10c4cc:	3c057f1b */ 	lui	$a1,%hi(var7f1b3514)
/*  f10c4d0:	24a53514 */ 	addiu	$a1,$a1,%lo(var7f1b3514)
/*  f10c4d4:	0c004c89 */ 	jal	strcat
/*  f10c4d8:	27a4009c */ 	addiu	$a0,$sp,0x9c
/*  f10c4dc:	0c002f02 */ 	jal	viGetX
/*  f10c4e0:	00000000 */ 	nop
/*  f10c4e4:	00028400 */ 	sll	$s0,$v0,0x10
/*  f10c4e8:	0010cc03 */ 	sra	$t9,$s0,0x10
/*  f10c4ec:	0c002f06 */ 	jal	viGetY
/*  f10c4f0:	03208025 */ 	or	$s0,$t9,$zero
/*  f10c4f4:	3c0e8008 */ 	lui	$t6,%hi(var8007fb10)
/*  f10c4f8:	3c0f8008 */ 	lui	$t7,%hi(var8007fb0c)
/*  f10c4fc:	8deffb0c */ 	lw	$t7,%lo(var8007fb0c)($t7)
/*  f10c500:	8dcefb10 */ 	lw	$t6,%lo(var8007fb10)($t6)
/*  f10c504:	8fac012c */ 	lw	$t4,0x12c($sp)
/*  f10c508:	afaf0014 */ 	sw	$t7,0x14($sp)
/*  f10c50c:	afae0010 */ 	sw	$t6,0x10($sp)
/*  f10c510:	8d8b000c */ 	lw	$t3,0xc($t4)
/*  f10c514:	afa00028 */ 	sw	$zero,0x28($sp)
/*  f10c518:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10c51c:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10c520:	afb0001c */ 	sw	$s0,0x1c($sp)
/*  f10c524:	8fa40130 */ 	lw	$a0,0x130($sp)
/*  f10c528:	27a50144 */ 	addiu	$a1,$sp,0x144
/*  f10c52c:	27a60140 */ 	addiu	$a2,$sp,0x140
/*  f10c530:	27a7009c */ 	addiu	$a3,$sp,0x9c
/*  f10c534:	0fc5580f */ 	jal	textRenderProjected
/*  f10c538:	afab0018 */ 	sw	$t3,0x18($sp)
/*  f10c53c:	8fb8012c */ 	lw	$t8,0x12c($sp)
/*  f10c540:	afa20130 */ 	sw	$v0,0x130($sp)
/*  f10c544:	8fae0140 */ 	lw	$t6,0x140($sp)
/*  f10c548:	8f0d0000 */ 	lw	$t5,0x0($t8)
/*  f10c54c:	8fac010c */ 	lw	$t4,0x10c($sp)
/*  f10c550:	25cf0001 */ 	addiu	$t7,$t6,0x1
/*  f10c554:	25b9003e */ 	addiu	$t9,$t5,0x3e
/*  f10c558:	afb90144 */ 	sw	$t9,0x144($sp)
/*  f10c55c:	1980000f */ 	blez	$t4,.L0f10c59c
/*  f10c560:	afaf0140 */ 	sw	$t7,0x140($sp)
/*  f10c564:	0fc5b9f1 */ 	jal	langGet
/*  f10c568:	24045795 */ 	addiu	$a0,$zero,0x5795
/*  f10c56c:	8fab0108 */ 	lw	$t3,0x108($sp)
/*  f10c570:	8fb80104 */ 	lw	$t8,0x104($sp)
/*  f10c574:	3c057f1b */ 	lui	$a1,%hi(var7f1b3518)
/*  f10c578:	24a53518 */ 	addiu	$a1,$a1,%lo(var7f1b3518)
/*  f10c57c:	27a4009c */ 	addiu	$a0,$sp,0x9c
/*  f10c580:	00403025 */ 	or	$a2,$v0,$zero
/*  f10c584:	8fa7010c */ 	lw	$a3,0x10c($sp)
/*  f10c588:	afab0010 */ 	sw	$t3,0x10($sp)
/*  f10c58c:	0c004dad */ 	jal	sprintf
/*  f10c590:	afb80014 */ 	sw	$t8,0x14($sp)
/*  f10c594:	1000000b */ 	b	.L0f10c5c4
/*  f10c598:	00000000 */ 	nop
.L0f10c59c:
/*  f10c59c:	0fc5b9f1 */ 	jal	langGet
/*  f10c5a0:	24045795 */ 	addiu	$a0,$zero,0x5795
/*  f10c5a4:	8fad0104 */ 	lw	$t5,0x104($sp)
/*  f10c5a8:	3c057f1b */ 	lui	$a1,%hi(var7f1b3528)
/*  f10c5ac:	24a53528 */ 	addiu	$a1,$a1,%lo(var7f1b3528)
/*  f10c5b0:	27a4009c */ 	addiu	$a0,$sp,0x9c
/*  f10c5b4:	00403025 */ 	or	$a2,$v0,$zero
/*  f10c5b8:	8fa70108 */ 	lw	$a3,0x108($sp)
/*  f10c5bc:	0c004dad */ 	jal	sprintf
/*  f10c5c0:	afad0010 */ 	sw	$t5,0x10($sp)
.L0f10c5c4:
/*  f10c5c4:	3c198008 */ 	lui	$t9,%hi(var8007fb0c)
/*  f10c5c8:	8f39fb0c */ 	lw	$t9,%lo(var8007fb0c)($t9)
/*  f10c5cc:	3c078008 */ 	lui	$a3,%hi(var8007fb10)
/*  f10c5d0:	8ce7fb10 */ 	lw	$a3,%lo(var8007fb10)($a3)
/*  f10c5d4:	27a40094 */ 	addiu	$a0,$sp,0x94
/*  f10c5d8:	27a50098 */ 	addiu	$a1,$sp,0x98
/*  f10c5dc:	27a6009c */ 	addiu	$a2,$sp,0x9c
/*  f10c5e0:	afa00014 */ 	sw	$zero,0x14($sp)
/*  f10c5e4:	0fc55cbe */ 	jal	textMeasure
/*  f10c5e8:	afb90010 */ 	sw	$t9,0x10($sp)
/*  f10c5ec:	0c002f02 */ 	jal	viGetX
/*  f10c5f0:	00000000 */ 	nop
/*  f10c5f4:	00028400 */ 	sll	$s0,$v0,0x10
/*  f10c5f8:	00107403 */ 	sra	$t6,$s0,0x10
/*  f10c5fc:	0c002f06 */ 	jal	viGetY
/*  f10c600:	01c08025 */ 	or	$s0,$t6,$zero
/*  f10c604:	3c0f8008 */ 	lui	$t7,%hi(var8007fb10)
/*  f10c608:	3c0c8008 */ 	lui	$t4,%hi(var8007fb0c)
/*  f10c60c:	8d8cfb0c */ 	lw	$t4,%lo(var8007fb0c)($t4)
/*  f10c610:	8deffb10 */ 	lw	$t7,%lo(var8007fb10)($t7)
/*  f10c614:	8fab012c */ 	lw	$t3,0x12c($sp)
/*  f10c618:	afac0014 */ 	sw	$t4,0x14($sp)
/*  f10c61c:	afaf0010 */ 	sw	$t7,0x10($sp)
/*  f10c620:	8d78000c */ 	lw	$t8,0xc($t3)
/*  f10c624:	afa00028 */ 	sw	$zero,0x28($sp)
/*  f10c628:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10c62c:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10c630:	afb0001c */ 	sw	$s0,0x1c($sp)
/*  f10c634:	8fa40130 */ 	lw	$a0,0x130($sp)
/*  f10c638:	27a50144 */ 	addiu	$a1,$sp,0x144
/*  f10c63c:	27a60140 */ 	addiu	$a2,$sp,0x140
/*  f10c640:	27a7009c */ 	addiu	$a3,$sp,0x9c
/*  f10c644:	0fc5580f */ 	jal	textRenderProjected
/*  f10c648:	afb80018 */ 	sw	$t8,0x18($sp)
/*  f10c64c:	8fad0140 */ 	lw	$t5,0x140($sp)
/*  f10c650:	8fae0144 */ 	lw	$t6,0x144($sp)
/*  f10c654:	3c057f1b */ 	lui	$a1,%hi(var7f1b3538)
/*  f10c658:	25b90001 */ 	addiu	$t9,$t5,0x1
/*  f10c65c:	25cf0001 */ 	addiu	$t7,$t6,0x1
/*  f10c660:	afa20130 */ 	sw	$v0,0x130($sp)
/*  f10c664:	afb90140 */ 	sw	$t9,0x140($sp)
/*  f10c668:	afaf0144 */ 	sw	$t7,0x144($sp)
/*  f10c66c:	24a53538 */ 	addiu	$a1,$a1,%lo(var7f1b3538)
/*  f10c670:	27a4009c */ 	addiu	$a0,$sp,0x9c
/*  f10c674:	0c004dad */ 	jal	sprintf
/*  f10c678:	8fa60100 */ 	lw	$a2,0x100($sp)
/*  f10c67c:	0c002f02 */ 	jal	viGetX
/*  f10c680:	00000000 */ 	nop
/*  f10c684:	00028400 */ 	sll	$s0,$v0,0x10
/*  f10c688:	00106403 */ 	sra	$t4,$s0,0x10
/*  f10c68c:	0c002f06 */ 	jal	viGetY
/*  f10c690:	01808025 */ 	or	$s0,$t4,$zero
/*  f10c694:	3c0b8008 */ 	lui	$t3,%hi(var8007fb08)
/*  f10c698:	3c188008 */ 	lui	$t8,%hi(var8007fb04)
/*  f10c69c:	8f18fb04 */ 	lw	$t8,%lo(var8007fb04)($t8)
/*  f10c6a0:	8d6bfb08 */ 	lw	$t3,%lo(var8007fb08)($t3)
/*  f10c6a4:	8fad012c */ 	lw	$t5,0x12c($sp)
/*  f10c6a8:	afb80014 */ 	sw	$t8,0x14($sp)
/*  f10c6ac:	afab0010 */ 	sw	$t3,0x10($sp)
/*  f10c6b0:	8db9000c */ 	lw	$t9,0xc($t5)
/*  f10c6b4:	afa00028 */ 	sw	$zero,0x28($sp)
/*  f10c6b8:	afa00024 */ 	sw	$zero,0x24($sp)
/*  f10c6bc:	afa20020 */ 	sw	$v0,0x20($sp)
/*  f10c6c0:	afb0001c */ 	sw	$s0,0x1c($sp)
/*  f10c6c4:	8fa40130 */ 	lw	$a0,0x130($sp)
/*  f10c6c8:	27a50144 */ 	addiu	$a1,$sp,0x144
/*  f10c6cc:	27a60140 */ 	addiu	$a2,$sp,0x140
/*  f10c6d0:	27a7009c */ 	addiu	$a3,$sp,0x9c
/*  f10c6d4:	0fc5580f */ 	jal	textRenderProjected
/*  f10c6d8:	afb90018 */ 	sw	$t9,0x18($sp)
/*  f10c6dc:	afa20130 */ 	sw	$v0,0x130($sp)
.L0f10c6e0:
/*  f10c6e0:	0fc54de0 */ 	jal	func0f153780
/*  f10c6e4:	8fa40130 */ 	lw	$a0,0x130($sp)
/*  f10c6e8:	10000042 */ 	b	.L0f10c7f4
/*  f10c6ec:	8fbf003c */ 	lw	$ra,0x3c($sp)
/*  f10c6f0:	240e0028 */ 	addiu	$t6,$zero,0x28
/*  f10c6f4:	1000003d */ 	b	.L0f10c7ec
/*  f10c6f8:	ae0e0000 */ 	sw	$t6,0x0($s0)
/*  f10c6fc:	8e020000 */ 	lw	$v0,0x0($s0)
/*  f10c700:	850f02d0 */ 	lh	$t7,0x2d0($t0)
/*  f10c704:	3c04800a */ 	lui	$a0,%hi(g_SoloSaveFile)
/*  f10c708:	00026080 */ 	sll	$t4,$v0,0x2
/*  f10c70c:	144f0008 */ 	bne	$v0,$t7,.L0f10c730
/*  f10c710:	01826023 */ 	subu	$t4,$t4,$v0
/*  f10c714:	0fc43da6 */ 	jal	savefileLoadDefaults
/*  f10c718:	24842200 */ 	addiu	$a0,$a0,%lo(g_SoloSaveFile)
/*  f10c71c:	3c048007 */ 	lui	$a0,%hi(g_FilemanEnterAgentNameMenuDialog)
/*  f10c720:	0fc3cbd3 */ 	jal	menuPushDialog
/*  f10c724:	24844d80 */ 	addiu	$a0,$a0,%lo(g_FilemanEnterAgentNameMenuDialog)
/*  f10c728:	10000031 */ 	b	.L0f10c7f0
/*  f10c72c:	00001025 */ 	or	$v0,$zero,$zero
.L0f10c730:
/*  f10c730:	000c60c0 */ 	sll	$t4,$t4,0x3
/*  f10c734:	010c1821 */ 	addu	$v1,$t0,$t4
/*  f10c738:	1060002c */ 	beqz	$v1,.L0f10c7ec
/*  f10c73c:	3c07800a */ 	lui	$a3,%hi(var800a22c0)
/*  f10c740:	8c6b0000 */ 	lw	$t3,0x0($v1)
/*  f10c744:	24e422c0 */ 	addiu	$a0,$a3,%lo(var800a22c0)
/*  f10c748:	24050064 */ 	addiu	$a1,$zero,0x64
/*  f10c74c:	ac8b0000 */ 	sw	$t3,0x0($a0)
/*  f10c750:	94780004 */ 	lhu	$t8,0x4($v1)
/*  f10c754:	00003025 */ 	or	$a2,$zero,$zero
/*  f10c758:	0fc42539 */ 	jal	func0f1094e4
/*  f10c75c:	a4980004 */ 	sh	$t8,0x4($a0)
/*  f10c760:	10000023 */ 	b	.L0f10c7f0
/*  f10c764:	00001025 */ 	or	$v0,$zero,$zero
/*  f10c768:	910d030a */ 	lbu	$t5,0x30a($t0)
/*  f10c76c:	25b90001 */ 	addiu	$t9,$t5,0x1
/*  f10c770:	1000001e */ 	b	.L0f10c7ec
/*  f10c774:	ae190000 */ 	sw	$t9,0x0($s0)
/*  f10c778:	8e020000 */ 	lw	$v0,0x0($s0)
/*  f10c77c:	910e030a */ 	lbu	$t6,0x30a($t0)
/*  f10c780:	00002025 */ 	or	$a0,$zero,$zero
/*  f10c784:	004e082b */ 	sltu	$at,$v0,$t6
/*  f10c788:	14200005 */ 	bnez	$at,.L0f10c7a0
/*  f10c78c:	00000000 */ 	nop
/*  f10c790:	0fc5b9f1 */ 	jal	langGet
/*  f10c794:	24045792 */ 	addiu	$a0,$zero,0x5792
/*  f10c798:	10000016 */ 	b	.L0f10c7f4
/*  f10c79c:	8fbf003c */ 	lw	$ra,0x3c($sp)
.L0f10c7a0:
/*  f10c7a0:	0fc421ae */ 	jal	func0f1086b8
/*  f10c7a4:	00403025 */ 	or	$a2,$v0,$zero
/*  f10c7a8:	10000012 */ 	b	.L0f10c7f4
/*  f10c7ac:	8fbf003c */ 	lw	$ra,0x3c($sp)
/*  f10c7b0:	8e020000 */ 	lw	$v0,0x0($s0)
/*  f10c7b4:	910f030a */ 	lbu	$t7,0x30a($t0)
/*  f10c7b8:	00002025 */ 	or	$a0,$zero,$zero
/*  f10c7bc:	004f082b */ 	sltu	$at,$v0,$t7
/*  f10c7c0:	14200005 */ 	bnez	$at,.L0f10c7d8
/*  f10c7c4:	00000000 */ 	nop
/*  f10c7c8:	850c02d0 */ 	lh	$t4,0x2d0($t0)
/*  f10c7cc:	00001025 */ 	or	$v0,$zero,$zero
/*  f10c7d0:	10000007 */ 	b	.L0f10c7f0
/*  f10c7d4:	ae0c0008 */ 	sw	$t4,0x8($s0)
.L0f10c7d8:
/*  f10c7d8:	0fc421ae */ 	jal	func0f1086b8
/*  f10c7dc:	00403025 */ 	or	$a2,$v0,$zero
/*  f10c7e0:	ae020008 */ 	sw	$v0,0x8($s0)
/*  f10c7e4:	10000002 */ 	b	.L0f10c7f0
/*  f10c7e8:	00001025 */ 	or	$v0,$zero,$zero
.L0f10c7ec:
/*  f10c7ec:	00001025 */ 	or	$v0,$zero,$zero
.L0f10c7f0:
/*  f10c7f0:	8fbf003c */ 	lw	$ra,0x3c($sp)
.L0f10c7f4:
/*  f10c7f4:	8fb00038 */ 	lw	$s0,0x38($sp)
/*  f10c7f8:	27bd0148 */ 	addiu	$sp,$sp,0x148
/*  f10c7fc:	03e00008 */ 	jr	$ra
/*  f10c800:	00000000 */ 	nop
);

s32 filemanMainMenuDialog(u32 operation, struct menudialog *dialog, union handlerdata *data)
{
	s32 i;

	switch (operation) {
	case MENUOP_OPEN:
		g_Menus[g_MpPlayerNum].data.filesel.filetypeplusone = 0;

		func0f110c5c(0, FILETYPE_SOLO);
		func0f188210();

		// Set MP player names to "Player 1" through 4 if blank
		for (i = 0; i < 4; i++) {
			if (g_MpPlayers[i].base.name[0] == '\0') {
				sprintf(g_MpPlayers[i].base.name, "%s %d\n", langGet(L_MISC(437)), i + 1);
			}
		}
		break;
	case MENUOP_CLOSE:
		func0f110bf8();
		break;
	}

	return 0;
}

bool filemanConsiderPushingFileSelectDialog(void)
{
	if (g_Menus[g_MpPlayerNum].unk83c == 0) {
		g_Menus[g_MpPlayerNum].playernum = 0;
		menuPushRootDialog(&g_FilemanMainMenuDialog, MENUROOT_FILESELECT);
		return true;
	}

	return false;
}

void pakPushPakMenuDialog(void)
{
	s32 prevplayernum = g_MpPlayerNum;
	g_MpPlayerNum = 0;
	menuPushRootDialog(&g_PakChoosePakMenuDialog, MENUROOT_BOOTPAKMGR);
	g_MpPlayerNum = prevplayernum;
}

struct menuitem g_FilemanSelectLocationMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0,                         0x00000010, L_OPTIONS(368), L_OPTIONS(369), NULL }, // "Where", "Spaces"
	{ MENUITEMTYPE_SEPARATOR,   0,                         0x00000000, 0x00000000, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  SAVEDEVICE_GAMEPAK,        0x00000000, (u32)&filemanMenuTextLocationName2, (u32)&filemanMenuTextSaveLocationSpaces, filemanSelectLocationMenuHandler },
	{ MENUITEMTYPE_SELECTABLE,  SAVEDEVICE_CONTROLLERPAK1, 0x00000000, (u32)&filemanMenuTextLocationName2, (u32)&filemanMenuTextSaveLocationSpaces, filemanSelectLocationMenuHandler },
	{ MENUITEMTYPE_SELECTABLE,  SAVEDEVICE_CONTROLLERPAK2, 0x00000000, (u32)&filemanMenuTextLocationName2, (u32)&filemanMenuTextSaveLocationSpaces, filemanSelectLocationMenuHandler },
	{ MENUITEMTYPE_SELECTABLE,  SAVEDEVICE_CONTROLLERPAK3, 0x00000000, (u32)&filemanMenuTextLocationName2, (u32)&filemanMenuTextSaveLocationSpaces, filemanSelectLocationMenuHandler },
	{ MENUITEMTYPE_SELECTABLE,  SAVEDEVICE_CONTROLLERPAK4, 0x00000000, (u32)&filemanMenuTextLocationName2, (u32)&filemanMenuTextSaveLocationSpaces, filemanSelectLocationMenuHandler },
	{ MENUITEMTYPE_SEPARATOR,   0,                         0x00000000, 0x00000000, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  0,                         0x00000000, L_OPTIONS(370), 0x00000000, filemanDeleteFilesForSaveMenuHandler }, // "Delete Files..."
	{ MENUITEMTYPE_SELECTABLE,  0,                         0x00000000, L_OPTIONS(371), 0x00000000, filemanCancelSaveMenuHandler }, // "Cancel"
	{ MENUITEMTYPE_END,         0,                         0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanSelectLocationMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(367), // "Select Location"
	g_FilemanSelectLocationMenuItems,
	NULL,
	0x00000080,
	NULL,
};

struct menuitem g_FilemanConfirmDeleteMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000030, (u32)&filemanMenuTextDeleteFileName, 0x00000000, filemanFileNameMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000230, (u32)&filemanMenuTextDeviceName, 0x00000000, filemanDeviceNameMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(380), 0x00000000, NULL }, // "Are you sure you want to delete this file?"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000028, L_OPTIONS(381), 0x00000000, NULL }, // "Cancel"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(382), 0x00000000, filemanConfirmDeleteMenuHandler }, // "OK"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanConfirmDeleteMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	L_OPTIONS(379), // "Warning"
	g_FilemanConfirmDeleteMenuItems,
	NULL,
	0x00000080,
	NULL,
};

struct menuitem g_FilemanFileInUseMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000030, (u32)&filemanMenuTextDeleteFileName, 0x00000000, filemanFileNameMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000230, (u32)&filemanMenuTextDeviceName, 0x00000000, filemanDeviceNameMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&filemanMenuTextFileInUseDescription, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000028, L_MPWEAPONS(161), 0x00000000, NULL }, // "Cancel"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanFileInUseMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	L_MPWEAPONS(159), // "Error"
	g_FilemanFileInUseMenuItems,
	NULL,
	0x00000080,
	NULL,
};

struct menuitem g_FilemanDeleteFileMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00004010, L_OPTIONS(377), 0x00000000, NULL }, // "Select a file to delete:"
	{ MENUITEMTYPE_LIST,        1, 0x00200000, 0x00000000, 0x00000000, filemanFileToDeleteListMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00004030, L_OPTIONS(378), 0x00000000, NULL }, // "Press B Button to exit."
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanDeleteFileMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(376), // "Delete File"
	g_FilemanDeleteFileMenuItems,
	filemanCopyOrDeleteListMenuDialog,
	0x00000000,
	NULL,
};

struct menuitem g_FilemanCopyListMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00004010, L_OPTIONS(374), 0x00000000, NULL }, // "Select a file to copy:"
	{ MENUITEMTYPE_LIST,        0, 0x00200000, 0x00000000, 0x00000000, filemanFileToCopyListMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00004030, L_OPTIONS(375), 0x00000000, NULL }, // "Press B Button to exit."
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanCopyListMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(373), // "Copy File"
	g_FilemanCopyListMenuItems,
	filemanCopyOrDeleteListMenuDialog,
	0x00000000,
	NULL,
};

struct pakdata *g_EditingPak = NULL;

struct menuitem g_PakDeleteGameNoteMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(384), 0x00000000, NULL }, // "Are you sure you want to delete this game note?"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000028, L_OPTIONS(385), 0x00000000, NULL }, // "No"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000020, L_OPTIONS(386), 0x00000000, pakDeleteGameNoteMenuHandler }, // "Yes"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_PakDeleteGameNoteMenuDialog = {
	MENUDIALOGTYPE_DANGER,
	L_OPTIONS(383), // "Delete Game Note"
	g_PakDeleteGameNoteMenuItems,
	NULL,
	0x00000000,
	NULL,
};

struct menuitem g_PakGameNotesMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(388), (u32)&pakMenuTextEditingPakName, NULL }, // "Delete Game Notes:"
	{ MENUITEMTYPE_SEPARATOR,   0, 0x00000000, 0x0000010e, 0x00000000, NULL },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(389), L_OPTIONS(390), NULL }, // "Note", "Pages"
	{ MENUITEMTYPE_LIST,        0, 0x00200000, 0x000000c8, 0x0000006e, pakGameNoteListMenuHandler },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&pakMenuTextPagesFree, (u32)&pakMenuTextPagesUsed, NULL },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, (u32)&pakMenuTextStatusMessage, 0x00000000, NULL },
	{ MENUITEMTYPE_LABEL,       0, 0x00000030, L_OPTIONS(391), 0x00000000, NULL }, // "Press the B Button to exit."
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_PakGameNotesMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(387), // "Game Notes"
	g_PakGameNotesMenuItems,
	pakGameNotesMenuDialog,
	0x00000000,
	NULL,
};

struct menuitem g_PakChoosePakMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(108), 0x00000000, NULL }, // "Use this menu to delete game notes from your Controller Pak"
	{ MENUITEMTYPE_SEPARATOR,   0, 0x00000000, 0x00000000, 0x00000000, NULL },
	{ MENUITEMTYPE_LABEL,       0, 0x00000010, L_OPTIONS(109), 0x00000000, NULL }, // "Choose Controller Pak to Edit:"
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000000, L_OPTIONS(112), 0x00000000, pakSelectionMenuHandler }, // "Controller Pak 1"
	{ MENUITEMTYPE_SELECTABLE,  1, 0x00000000, L_OPTIONS(113), 0x00000000, pakSelectionMenuHandler }, // "Controller Pak 2"
	{ MENUITEMTYPE_SELECTABLE,  2, 0x00000000, L_OPTIONS(114), 0x00000000, pakSelectionMenuHandler }, // "Controller Pak 3"
	{ MENUITEMTYPE_SELECTABLE,  3, 0x00000000, L_OPTIONS(115), 0x00000000, pakSelectionMenuHandler }, // "Controller Pak 4"
	{ MENUITEMTYPE_SEPARATOR,   0, 0x00000000, 0x00000000, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  0, 0x00000028, L_OPTIONS(110), 0x00000000, NULL }, // "Exit"
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_PakChoosePakMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(107), // "Controller Pak Menu"
	g_PakChoosePakMenuItems,
	pakChoosePakMenuDialog,
	0x00000000,
	NULL,
};

struct menuitem g_FilemanOperationsMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0,                 0x00004010, L_OPTIONS(100), 0x00000000, NULL }, // "Copy:"
	{ MENUITEMTYPE_SELECTABLE,  FILETYPE_SOLO,     0x00000000, L_OPTIONS(103), 0x00000000, filemanOpenCopyFileMenuHandler }, // "Single Player Agent File"
	{ MENUITEMTYPE_SELECTABLE,  FILETYPE_MPSETUP , 0x00000000, L_OPTIONS(104), 0x00000000, filemanOpenCopyFileMenuHandler }, // "Combat Simulator Settings File"
	{ MENUITEMTYPE_SELECTABLE,  FILETYPE_MPPLAYER, 0x00000000, L_OPTIONS(105), 0x00000000, filemanOpenCopyFileMenuHandler }, // "Combat Simulator Player File"
	{ MENUITEMTYPE_SEPARATOR,   0,                 0x00000000, 0x00000000, 0x00000000, NULL },
	{ MENUITEMTYPE_LABEL,       0,                 0x00004010, L_OPTIONS(101), 0x00000000, NULL }, // "Delete:"
	{ MENUITEMTYPE_SELECTABLE,  FILETYPE_SOLO,     0x00000000, L_OPTIONS(103), 0x00000000, filemanOpenDeleteFileMenuHandler }, // "Single Player Agent File"
	{ MENUITEMTYPE_SELECTABLE,  FILETYPE_MPSETUP,  0x00000000, L_OPTIONS(104), 0x00000000, filemanOpenDeleteFileMenuHandler }, // "Combat Simulator Settings File"
	{ MENUITEMTYPE_SELECTABLE,  FILETYPE_MPPLAYER, 0x00000000, L_OPTIONS(105), 0x00000000, filemanOpenDeleteFileMenuHandler }, // "Combat Simulator Player File"
	{ MENUITEMTYPE_SEPARATOR,   0,                 0x00000000, 0x00000000, 0x00000000, NULL },
	{ MENUITEMTYPE_SELECTABLE,  0,                 0x00000004, L_OPTIONS(102), 0x00000000, (void *)&g_PakChoosePakMenuDialog }, // "Delete Game Notes..."
	{ MENUITEMTYPE_END,         0,                 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanOperationsMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(99), // "Game Files"
	g_FilemanOperationsMenuItems,
	NULL,
	0x00000020,
	NULL,
};

struct menuitem g_FilemanEnterAgentNameMenuItems[] = {
	{ MENUITEMTYPE_KEYBOARD,    0, 0x00000000, 0x00000000, 0x00000001, filemanAgentNameKeyboardMenuHandler },
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanEnterAgentNameMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(401), // "Enter Agent Name"
	g_FilemanEnterAgentNameMenuItems,
	NULL,
	0x00000000,
	NULL,
};

struct menuitem g_FilemanMainMenuItems[] = {
	{ MENUITEMTYPE_LABEL,       0, 0x00004010, L_OPTIONS(96), 0x00000000, NULL }, // "Choose Your Reality"
	{ MENUITEMTYPE_LIST,        0, 0x00200000, 0x000000f5, 0x00000000, fileListChooseAgentMenuHandler },
	{ MENUITEMTYPE_END,         0, 0x00000000, 0x00000000, 0x00000000, NULL },
};

struct menudialog g_FilemanMainMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	L_OPTIONS(95), // "Perfect Dark"
	g_FilemanMainMenuItems,
	filemanMainMenuDialog,
	0x00000020,
	&g_FilemanOperationsMenuDialog,
};
