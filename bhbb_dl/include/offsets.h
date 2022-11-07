#ifndef BHBB_DL_OFFSETS_H
#define BHBB_DL_OFFSETS_H

#include <psp2/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int GetShellOffsets(SceUInt32 nid, SceUInt32 *getOff, SceUInt32 *expOff, SceUInt32 *recOff, SceUInt32 *lockOff);

#ifdef __cplusplus
}
#endif
#endif