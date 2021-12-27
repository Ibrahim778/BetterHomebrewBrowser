#ifndef BHBB_PAF_INTERNAL_HPP
#define BHBB_PAF_INTERNAL_HPP

#include <paf.h>
#include <libsysmodule.h>
#include <ctrl.h>

using namespace paf;
using namespace ui;

typedef struct SceSysmoduleOpt {
	int flags;
	int *result;
	int unused[2];
} SceSysmoduleOpt;

typedef struct ScePafInit {
	SceSize global_heap_size;
	int a2;
	int a3;
	int cdlg_mode;
	int heap_opt_param1;
	int heap_opt_param2;
} ScePafInit; // size is 0x18

#endif
