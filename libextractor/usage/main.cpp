#include <stdio.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>

#include "libextractor.h"
extern "C" int sceSysmoduleLoadModuleInternalWithArg(unsigned int id, SceSize args, void *argp, void *option);
extern "C" int sceSysmoduleUnloadModuleInternalWithArg(unsigned int id, SceSize args, void * argp, void * option);

int loadScePaf() {
  static uint32_t argp[] = { 0x180000, -1, -1, 1, -1, -1 };

  int result = -1;

  uint32_t buf[4];
  buf[0] = sizeof(buf);
  buf[1] = (uint32_t)&result;
  buf[2] = -1;
  buf[3] = -1;

  return sceSysmoduleLoadModuleInternalWithArg(0x80000008, sizeof(argp), argp, buf);
}

int unloadScePaf() {
  uint32_t buf = 0;
  return sceSysmoduleUnloadModuleInternalWithArg(0x80000008, 0, NULL, &buf);
}


int main()
{
    sceClibPrintf("Hello World!\n");
    if(loadScePaf() < 0)
    {
        sceClibPrintf("Error loading ScePaf!\n");
        return -1;
    }
 
    SceUID modid = sceKernelLoadStartModule("app0:module/libextractor.suprx", 0, NULL, 0, NULL, NULL);
    if(modid < 0)
    {
        sceClibPrintf("Error module load failed 0x%X\n", modid);
        return -1;
    }
    sceClibPrintf("Module loaded with id 0x%X\n", modid);

    Zipfile file = Zipfile("ux0:usage.vpk");
    int result = file.Unzip("ux0:data/extracted");
    if(result == 0)
    {
        sceClibPrintf("Extracted Successfully!\n");
    }
    else 
    {
        sceClibPrintf("Extraction Failed! %d\n", result);
        return -1;
    }
    return 0;
}