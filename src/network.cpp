#include <kernel.h>
#include <vshbridge.h>
#include <net.h>
#include <libsysmodule.h>
#include <libnetctl.h>
#include <libhttp.h>
#include <paf.h>
#include <shellsvc.h>
#include <message_dialog.h>

#include "network.h"
#include "utils.h"
#include "common.h"

#define NET_HEAP_SIZE  2 * 1024 * 1024
#define HTTP_HEAP_SIZE 2 * 1024 * 1024
#define SSL_HEAP_SIZE  2 * 1024 * 1024

static Network::CheckThread *checkThread = SCE_NULL;
static int lastError = 0;

static void (*CheckComplete)(void) = SCE_NULL;
static Network::Status CurrentStatus = Network::Status::Offline;

void Network::Init()
{
    /* NetCtl */
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

    /* CURL */
    curl_global_memmanager_set_np(sce_paf_malloc, sce_paf_free, sce_paf_realloc);

    lastError = 0;
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
}

void Network::Check(void (*onComplete)(void))
{
    CheckComplete = onComplete;
    
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

SceInt32 Network::GetLastError()
{
    return lastError;
}

Network::Status Network::GetCurrentStatus()
{
    /*
    if(checkThread)
        if(checkThread->IsAlive())
            checkThread->Join();
    */
    return CurrentStatus;
}

SceVoid Network::CheckThread::EntryFunction()
{
	SceInt32             ret = -1;
    paf::HttpFile        testFile;
    paf::HttpFile::OpenArg testHttp;

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    
	testHttp.SetUrl("https://www.google.com/index.html");
	testHttp.SetOpt(10000000, paf::HttpFile::OpenArg::Opt::Opt_ResolveTimeOut);
	testHttp.SetOpt(10000000, paf::HttpFile::OpenArg::Opt::Opt_ConnectTimeOut);

	ret = testFile.Open(&testHttp);
    CurrentStatus = ret == SCE_OK ? Online : Offline;
	
    if (ret == SCE_OK)
		testFile.Close();
    else lastError = ret;

	sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    if(CheckComplete)
        CheckComplete();

    Cancel();
}