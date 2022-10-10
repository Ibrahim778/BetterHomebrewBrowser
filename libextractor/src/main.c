#include <psp2/kernel/modulemgr.h>

int _start(SceSize s, void *a) __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize s, void *a)
{
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize s, void *a)
{
    return SCE_KERNEL_STOP_SUCCESS;
}