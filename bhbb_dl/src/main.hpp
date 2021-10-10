#ifndef BHBB_DL_MAIN
#define BHBB_DL_MAIN

#include <scetypes.h>

#define SET_TEXT(dest, src) sce_paf_wcsncpy(((wchar_t *)(dest)), ((wchar_t *)(src)), sizeof((dest)))

#ifdef _DEBUG
#define print sceClibPrintf
#define LOG_ERROR(prefix, error_code) print("[%s] Got Error: 0x%X\n", prefix, error_code);
#else
#define print (void)NULL;
#define LOG_ERROR(prefix, error_code) (void)NULL;
#endif
typedef struct ScePafInit {
	SceSize global_heap_size;
	int a2;
	int a3;
	int cdlg_mode;
	int heap_opt_param1;
	int heap_opt_param2;
} ScePafInit; // size is 0x18

typedef struct SceSysmoduleOpt {
	int flags;
	int *result;
	int unused[2];
} SceSysmoduleOpt;
#endif