#ifndef BHBB_PRINT_H
#define BHBB_PRINT_H
#ifndef __SNC__
#include <psp2/kernel/clib.h>
#endif

#ifdef _DEBUG
#define print(...) sceClibPrintf(__VA_ARGS__)
#else
#define print(...) {(void)SCE_NULL;}
#endif

#endif