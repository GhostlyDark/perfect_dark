#include <ultra64.h>
#include "constants.h"
#include "game/bg.h"
#include "game/pad.h"
#include "bss.h"
#include "lib/memp.h"
#include "lib/anim.h"
#include "data.h"
#include "types.h"

void coverAllocateSpecial(u16 *specialcovernums)
{
	s32 i;

	g_SpecialCoverNums = mempAlloc(ALIGN16(g_NumSpecialCovers * sizeof(u16)), MEMPOOL_STAGE);

	if (g_SpecialCoverNums != NULL) {
		for (i = 0; i < g_NumSpecialCovers; i++) {
			g_SpecialCoverNums[i] = specialcovernums[i];
		}
	}
}

void setupPrepareCover(void)
{
	s32 i;
	s32 numcovers = g_PadsFile->numcovers;
	s16 *roomsptr;
	f32 scale = 1;
	struct coord aimpos;
	struct cover cover;
	u16 specialcovernums[1024];
	s16 inrooms[21];
	s16 aboverooms[21];

	g_CoverFlags = mempAlloc(ALIGN16(numcovers * sizeof(u16)), MEMPOOL_STAGE);
	g_CoverRooms = mempAlloc(ALIGN16(numcovers * sizeof(s32)), MEMPOOL_STAGE);
	g_CoverCandidates = mempAlloc(ALIGN16(numcovers * 0x10), MEMPOOL_STAGE);

	g_NumSpecialCovers = 0;
	g_SpecialCoverNums = NULL;

	if (g_CoverFlags && g_CoverRooms && g_CoverCandidates) {
		for (i = 0; i < numcovers; i++) {
			roomsptr = NULL;
			g_CoverFlags[i] = 0;

			if (coverUnpack(i, &cover)
					&& (cover.look->x != 0.0f || cover.look->y != 0.0f || cover.look->z != 0.0f)) {
				if (coverIsSpecial(&cover)) {
					specialcovernums[g_NumSpecialCovers] = i;
					g_NumSpecialCovers++;
				}

				cover.pos->x *= scale;
				cover.pos->y *= scale;
				cover.pos->z *= scale;

				if (cover.look->x == 1.0f && cover.look->y == 1.0f && cover.look->z == 1.0f) {
					g_CoverFlags[i] |= COVERFLAG_OMNIDIRECTIONAL;
				} else if (!coverIsSpecial(&cover)) {
					struct coord *look = cover.look;
					look->y = 0;
					guNormalize(&look->x, &look->y, &look->z);
				}

				// Find room
				bgFindRoomsByPos(cover.pos, inrooms, aboverooms, 20, NULL);

				if (inrooms[0] != -1) {
					roomsptr = inrooms;
				} else if (aboverooms[0] != -1) {
					roomsptr = aboverooms;
				}

				g_CoverRooms[i] = -1;

				if (roomsptr != NULL) {
					s32 room = cd0002a400(cover.pos, roomsptr);

					if (room > 0) {
						g_CoverRooms[i] = (s16)room;
					} else {
						g_CoverRooms[i] = roomsptr[0];
					}
				}

				// Determine if aim is in the same room or not
				if (g_CoverRooms[i] < 0) {
					g_CoverFlags[i] |= COVERFLAG_AIMSAMEROOM;
				} else if ((g_CoverFlags[i] & COVERFLAG_OMNIDIRECTIONAL) == 0) {
					aimpos.x = cover.pos->x + cover.look->f[0] * 600;
					aimpos.y = cover.pos->y;
					aimpos.z = cover.pos->z + cover.look->f[2] * 600;

					bgFindRoomsByPos(&aimpos, inrooms, aboverooms, 20, NULL);

					if (inrooms[0] != -1) {
						roomsptr = inrooms;
					} else if (aboverooms[0] != -1) {
						roomsptr = aboverooms;
					}

					if (roomsptr) {
						s32 aimroom = cd0002a400(&aimpos, roomsptr);

						if (aimroom > 0) {
							g_CoverFlags[i] |= (g_CoverRooms[i] == (s16)aimroom) ? COVERFLAG_AIMSAMEROOM : COVERFLAG_AIMDIFFROOM;
						} else {
							g_CoverFlags[i] |= (g_CoverRooms[i] == roomsptr[0]) ? COVERFLAG_AIMSAMEROOM : COVERFLAG_AIMDIFFROOM;
						}
					} else {
						g_CoverFlags[i] |= COVERFLAG_AIMDIFFROOM;
					}
				}
			}
		}

		coverAllocateSpecial(specialcovernums);
	}
}
