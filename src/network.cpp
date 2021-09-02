#include "network.hpp"
#include "main.hpp"

#define NET_INIT_SIZE 1024 * 1024

SceUID moduleID;
SceUID cLibID;

void loadPsp2CompatModule()
{
    //PSp2Compat needs libc, curl needs compat
    if(cLibID <= 0)
        cLibID = sceKernelLoadStartModule(LIBC_PATH, 0, NULL, 0, NULL, NULL);
    if(moduleID <= 0)
        moduleID = sceKernelLoadStartModule(MODULE_PATH, 0, NULL, 0, NULL, NULL);
}

void netInit() 
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

	SceNetInitParam netInitParam;
	int size = NET_INIT_SIZE;
	netInitParam.memory = sce_paf_malloc(size);
	netInitParam.size = size;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);

	sceNetCtlInit();
}

void netTerm() 
{
	sceNetCtlTerm();
	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
}

void httpInit() 
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
	sceHttpInit(NET_INIT_SIZE);
	sceSslInit(NET_INIT_SIZE);
}

void httpTerm() 
{
	sceSslTerm();
	sceHttpTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
}