#ifndef BHBB_DL_MAIN_H
#define BHBB_DL_MAIN_H

#include <kernel.h>

SCE_CDECL_BEGIN

int module_start(size_t args, void *argp);
int module_stop(size_t args, void *argp);

SCE_CDECL_END

#endif