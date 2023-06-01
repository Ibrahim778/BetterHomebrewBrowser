#ifndef BHBB_DL_OFFSETS_H
#define BHBB_DL_OFFSETS_H

#include <kernel.h>

SCE_CDECL_BEGIN

int GetShellOffsets(uint32_t nid, uint32_t *exp_off, uint32_t *rec_off, uint32_t *notif_off);

SCE_CDECL_END

#endif