#include <kernel.h>
#include <vshbridge.h>
#include <net.h>
#include <libsysmodule.h>
#include <libnetctl.h>
#include <libhttp.h>
#include <paf.h>
#include <shellsvc.h>
#include <message_dialog.h>

#include "network.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "common.hpp"

#define NET_HEAP_SIZE  2 * 1024 * 1024
#define HTTP_HEAP_SIZE 2 * 1024 * 1024
#define SSL_HEAP_SIZE  2 * 1024 * 1024

static SceUID moduleID = SCE_UID_INVALID_UID;
static SceUID cLibID = SCE_UID_INVALID_UID;
static SceUID fios2ID = SCE_UID_INVALID_UID;

static Network::CheckThread *checkThread = SCE_NULL;

void (*Network::OnReady)(void) = SCE_NULL;
Network::Status Network::CurrentStatus = Network::Status::Offline;

void Network::Init()
{
    /* NetCtl */
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

	SceNetInitParam netInitParam;
	netInitParam.memory = sce_paf_malloc(NET_HEAP_SIZE);
	netInitParam.size = NET_HEAP_SIZE;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);

	sceNetCtlInit();

    /* HTTPS */
	sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
	sceHttpInit(HTTP_HEAP_SIZE);
	sceSslInit(SSL_HEAP_SIZE);

    
}

void Network::Term()
{
    /* HTTPS */
	sceSslTerm();
	sceHttpTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);

    /* NetCtl */
	sceNetCtlTerm();
	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);

    /* PSP2Compat */
    if(moduleID > 0)
        moduleID = sceKernelStopUnloadModule(moduleID, 0, NULL, 0, NULL, NULL) == SCE_OK ? SCE_UID_INVALID_UID : moduleID;
    if(cLibID > 0)
        cLibID = sceKernelStopUnloadModule(cLibID, 0, NULL, 0, NULL, NULL) == SCE_OK ? SCE_UID_INVALID_UID : cLibID;
	if (fios2ID > 0)
		fios2ID = sceKernelStopUnloadModule(fios2ID, 0, NULL, 0, NULL, NULL) == SCE_OK ? SCE_UID_INVALID_UID : fios2ID;
}

void Network::Check(void (*onReady)(void))
{
    Network::OnReady = onReady;
    
    if(checkThread != NULL)
    {
        if(checkThread->IsAlive())
        {
            checkThread->Cancel();
            checkThread->Join();
        }
 
        delete checkThread;
        checkThread = NULL;
    }
    
    checkThread = new CheckThread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_4KiB, "BHBB::NetworkCheckThread");
    checkThread->Start();
}

Network::Status Network::GetCurrentStatus()
{
    /*
    if(checkThread)
        if(checkThread->IsAlive())
            checkThread->Join();
    */
    return Network::CurrentStatus;
}

SceVoid Network::CheckThread::EntryFunction()
{
	SceInt32                        ret = -1;

    paf::HttpFile                   testFile;
    paf::HttpFile::Param            testHttp;

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

	testHttp.SetUrl("https://www.google.com/index.html");
	testHttp.SetOpt(10000000, paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT);
	testHttp.SetOpt(10000000, paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT);

	ret = testFile.Open(&testHttp);
    Network::CurrentStatus = ret == SCE_OK ? Online : Offline;
	
    if (ret == SCE_OK)
		testFile.Close();

	sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    if(Network::OnReady)
        Network::OnReady();

    sceKernelExitDeleteThread(0);
}