#include <kernel.h>
#include <libsysmodule.h>
#include <moduleinfo.h>
#include <appmgr.h>

SCE_MODULE_INFO(libScePafPreload, SCE_MODULE_ATTR_NONE, 1, 2)

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


int __module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

int __module_exit(SceSize args, void * argp)
{
    return SCE_KERNEL_STOP_SUCCESS;
}

int __module_start(SceSize argc, void *args)
{
    SceInt32 load_res;

    ScePafInit initParam;
    SceSysmoduleOpt opt;

    initParam.global_heap_size = 5 * 1024 * 1024;
    if (sceAppMgrGrowMemory3(16 * 1024 * 1024, 1) >= 0)
        initParam.global_heap_size += 5 * 1024 * 1024;
    if (sceAppMgrGrowMemory3(2 * 1024 * 1024, 1) >= 0)
        initParam.global_heap_size += 2 * 1024 * 1024;

    initParam.a2 = 0x0000EA60;
    initParam.a3 = 0x00040000;

    initParam.cdlg_mode = SCE_FALSE;

    initParam.heap_opt_param1 = 0;
    initParam.heap_opt_param2 = 0;

    //Specify that we will pass some arguments
    opt.flags = 0;
    opt.result = &load_res;

    return _sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(initParam), &initParam, &opt) == 0 ? SCE_KERNEL_START_NO_RESIDENT : SCE_KERNEL_START_FAILED;
}