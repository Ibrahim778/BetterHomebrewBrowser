#ifndef BHBB_DL_OFFSETS_H
#define BHBB_DL_OFFSETS_H

#include <kernel.h>

SCE_CDECL_BEGIN

int GetShellOffsets(SceUInt32 nid, SceUInt32 *getOff, SceUInt32 *expOff, SceUInt32 *recOff, SceUInt32 *lockOff);

SCE_CDECL_END

#endif