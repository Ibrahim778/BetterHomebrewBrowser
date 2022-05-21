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

//void (*Network::OnReady)(void) = SCE_NULL;
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

    /* PSP2Compat */
    //PSp2Compat needs libc, curl needs compat
	if (fios2ID <= 0)
		fios2ID = sceKernelLoadStartModule(FIOS2_PATH, 0, NULL, 0, NULL, NULL);
    if(cLibID <= 0)
        cLibID = sceKernelLoadStartModule(LIBC_PATH, 0, NULL, 0, NULL, NULL);
    if(moduleID <= 0)
        moduleID = sceKernelLoadStartModule(MODULE_PATH, 0, NULL, 0, NULL, NULL);
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

void Network::Check(/*void (*onReady)(void)*/)
{
    //Network::OnReady = onReady;
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
	SceMsgDialogParam				msgParam;
	SceMsgDialogSystemMessageParam	sysMsgParam;
	SceMsgDialogUserMessageParam    userMsgParam;
	SceCommonDialogStatus           status;
	SceInt32                        ret = -1;

    paf::HttpFile                   testFile;
    paf::HttpFile::Param            testHttp;

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    sceMsgDialogParamInit(&msgParam);
    msgParam.mode = SCE_MSG_DIALOG_MODE_SYSTEM_MSG;

    sce_paf_memset(&sysMsgParam, 0, sizeof(SceMsgDialogSystemMessageParam));
	msgParam.sysMsgParam = &sysMsgParam;
	msgParam.sysMsgParam->sysMsgType = SCE_MSG_DIALOG_SYSMSG_TYPE_WAIT_SMALL;

    sceMsgDialogInit(&msgParam);

	testHttp.SetUrl("https://www.google.com/index.html");
	testHttp.SetOpt(10000000, paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT);
	testHttp.SetOpt(10000000, paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT);

	ret = testFile.Open(&testHttp);
    Network::CurrentStatus = ret == SCE_OK ? Online : Offline;
	
    if (ret == SCE_OK)
		testFile.Close();

	status = sceMsgDialogGetStatus();

	while (status != SCE_COMMON_DIALOG_STATUS_RUNNING) {
		status = sceMsgDialogGetStatus();
		paf::thread::Sleep(10);
	}

	sceMsgDialogClose();

	while (status != SCE_COMMON_DIALOG_STATUS_FINISHED) {
		status = sceMsgDialogGetStatus();
		paf::thread::Sleep(10);
	}

    sceMsgDialogTerm();

    if(ret != SCE_OK)
    {
        msgParam.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
        msgParam.sysMsgParam = SCE_NULL;

        sce_paf_memset(&userMsgParam, 0, sizeof(SceMsgDialogUserMessageParam));
        msgParam.userMsgParam = &userMsgParam;
        msgParam.userMsgParam->buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;

        paf::string str;
		if (ret == SCE_HTTP_ERROR_SSL)
            Utils::GetfStringFromID("msg_error_ssl", &str);
		else
            Utils::GetStringFromID("msg_error_net", &str);

        msgParam.userMsgParam->msg = (SceChar8 *)str.data;

		sceMsgDialogInit(&msgParam);

		status = sceMsgDialogGetStatus();

		while (status != SCE_COMMON_DIALOG_STATUS_FINISHED) 
        {
			status = sceMsgDialogGetStatus();
			paf::thread::Sleep(100);
		}

		sceMsgDialogTerm();        
    }

	sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    //if(Network::OnReady)
    //    Network::OnReady();

    sceKernelExitDeleteThread(0);
}