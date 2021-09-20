#include "network.hpp"
#include "main.hpp"

#define NET_HEAP_SIZE  1 * 1024 * 1024
#define HTTP_HEAP_SIZE 1 * 1024 * 1024
#define SSL_HEAP_SIZE  1 * 1024 * 1024

static SceUID moduleID = SCE_UID_INVALID_UID;
static SceUID cLibID = SCE_UID_INVALID_UID;
static SceUID fios2ID = SCE_UID_INVALID_UID;

void loadPsp2CompatModule()
{
    //PSp2Compat needs libc, curl needs compat
	if (fios2ID <= 0)
		fios2ID = sceKernelLoadStartModule(FIOS2_PATH, 0, NULL, 0, NULL, NULL);
    if(cLibID <= 0)
        cLibID = sceKernelLoadStartModule(LIBC_PATH, 0, NULL, 0, NULL, NULL);
    if(moduleID <= 0)
        moduleID = sceKernelLoadStartModule(MODULE_PATH, 0, NULL, 0, NULL, NULL);
}

void netInit() 
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

	SceNetInitParam netInitParam;
	netInitParam.memory = sce_paf_malloc(NET_HEAP_SIZE);
	netInitParam.size = NET_HEAP_SIZE;
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
	sceHttpInit(HTTP_HEAP_SIZE);
	sceSslInit(SSL_HEAP_SIZE);
}

void httpTerm() 
{
	sceSslTerm();
	sceHttpTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
}