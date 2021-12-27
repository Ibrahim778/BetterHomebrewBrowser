#include <kernel.h>
#include <paf.h>
#include <notification_util.h>
#include <libsysmodule.h>
#include <stdio.h>
#include <curl/curl.h>
#include <appmgr.h>
#include <promoterutil.h>

#include "main.hpp"
#include "Archives.hpp"
#include "notifmgr.hpp"
#include "bhbb_dl.h"

#define LIST_TYPE bhbbPacket
#include "../../common/queue.cpp"

#include "net.hpp"
#include "promote.hpp"

// Remaining functions that must be defined for ::Queue to work
LIST_TYPE *Queue::Find(const char *name)
{
    qnode *n = head;
    while(n != NULL)
    {
        if(sce_paf_strcmp(name, n->data.name) == 0) return &n->data;
        n = n->next;
    }
    return NULL;
}

void Queue::remove(const char *url)
{
    //Check head isn't NULL
    if (head == NULL)
        return;

    //First, handle the case where we free the head
    if (sce_paf_strcmp(head->data.url, url) == 0)
    {
        qnode *nodeToDelete = head;
        head = head->next;
        free(nodeToDelete);
        return;
    }

    //Bail out if the head is the only node
    if (head->next == NULL)
        return;

    //Else, try to locate node we're asked to remove
    qnode **pCurrentNodeNext = &head; //This points to the current node's `next` field (or to pHead)
    while (1)
    {
        if (sce_paf_strcmp((*pCurrentNodeNext)->data.url, url) == 0) //pCurrentNodeNext points to the pointer that points to the node we need to delete
            break;

        //If the next node's next is NULL, we reached the end of the list. Bail out.
        if ((*pCurrentNodeNext)->next == NULL)
            return;

        pCurrentNodeNext = &(*pCurrentNodeNext)->next;
    }
    qnode *nodeToDelete = *pCurrentNodeNext;
    *pCurrentNodeNext = (*pCurrentNodeNext)->next;
    free(nodeToDelete);

    num --;
}

extern "C" {

	extern unsigned int	sce_process_preload_disabled = (SCE_PROCESS_PRELOAD_DISABLED_LIBDBG \
		| SCE_PROCESS_PRELOAD_DISABLED_LIBCDLG | SCE_PROCESS_PRELOAD_DISABLED_LIBPERF);

	unsigned int sceLibcHeapSize = 2 * 1024 * 1024;

    extern const char			sceUserMainThreadName[] = "BHBB_MAIN_BG";
    extern const int			sceUserMainThreadPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    extern const unsigned int	sceUserMainThreadStackSize = SCE_KERNEL_THREAD_STACK_SIZE_DEFAULT_USER_MAIN;

    void __cxa_set_dso_handle_main(void *dso)
    {

    }

    int _sceLdTlsRegisterModuleInfo()
    {
        return 0;
    }

    int __aeabi_unwind_cpp_pr0()
    {
        return 9;
    }

    int __aeabi_unwind_cpp_pr1()
    {
        return 9;
    }

    int __at_quick_exit()
    {
        return 0;
    }
}

#define EXTRACT_PATH "ux0:temp/app"
#define VPK_DOWNLOAD_PATH "ux0:temp/bhbb_dl.vpk"

Queue queue;

SceBool Running = SCE_TRUE;
SceUID dlThreadID = SCE_UID_INVALID_UID;

int install(const char *file)
{
    sceIoRemove(EXTRACT_PATH);
    sceIoRemove("ux0:temp/new");
    sceIoRemove("ux0:appmeta/new");
    sceIoRemove("ux0:temp/promote");
    sceIoRemove("ux0:temp/game");

    sceIoRemove("ur0:temp/new");
    sceIoRemove("ur0:appmeta/new");
    sceIoRemove("ur0:temp/promote");
    sceIoRemove("ur0:temp/game");

    Zip *zfile;
    char txt[64];
	sce_paf_memset(txt, 0, sizeof(txt));
    
    if(NotifMgr::currDlCanceled)
        goto END;
    
    zfile = ZipOpen(file);
    ZipExtract(zfile, NULL, EXTRACT_PATH);
    ZipClose(zfile);

    NotifMgr::EndNotif(queue.head->data.name, "Promoting");
    
    if(NotifMgr::currDlCanceled) goto END;

    int res = promoteApp(EXTRACT_PATH);
    if(res == SCE_OK)
		sce_paf_snprintf(txt, 64, "Installed %s Successfully!", queue.head->data.name);
    else
		sce_paf_snprintf(txt, 64, "Error 0x%X", res);

    NotifMgr::SendNotif(txt);

END:
    sceIoRemove(EXTRACT_PATH);
    sceIoRemove(file);

    sceIoRemove("ux0:temp/new");
    sceIoRemove("ux0:appmeta/new");
    sceIoRemove("ux0:temp/promote");
    sceIoRemove("ux0:temp/game");

    sceIoRemove("ur0:temp/new");
    sceIoRemove("ur0:appmeta/new");
    sceIoRemove("ur0:temp/promote");
    sceIoRemove("ur0:temp/game");

    return 0;
}

SceInt32 DownloadThread(SceSize args, void *argp)
{
    curlInit();
    
    while (Running)
    {
        if(queue.num == 0)
        {
            sceKernelDelayThread(100000);
            continue;
        }
        
        NotifMgr::Init();
        NotifMgr::MakeProgressNotif(queue.head->data.name, "Installing", "Are you sure you want to cancel the install?");

        int r = dlFile(queue.head->data.url, VPK_DOWNLOAD_PATH);

        if(!NotifMgr::currDlCanceled && r == CURLE_OK)    
        {
            NotifMgr::UpdateProgressNotif(0, "Extracting", queue.head->data.name);
            install(VPK_DOWNLOAD_PATH);
        }
        else
        {
            sceIoRemove(VPK_DOWNLOAD_PATH);
            if(r > 0) NotifMgr::EndNotif("CURL Error while downloading",curl_easy_strerror((CURLcode)r));
            else
            {
                char txt[64];
                sce_paf_memset(txt, 0, 64);
                sce_paf_snprintf(txt, 64, "0x%X", r);
                NotifMgr::EndNotif("An error ocourred before downloading", txt);
            }
        }
        queue.dequeue();
    };

    curlEnd();
    return sceKernelExitDeleteThread(0);
}

int initPaf()
{
    SceInt32 res = -1, load_res;

    ScePafInit initParam;
    SceSysmoduleOpt opt;

    initParam.global_heap_size = 6 * 1024 * 1024;

    initParam.a2 = 0x0000EA60;
    initParam.a3 = 0x00040000;

    initParam.cdlg_mode = SCE_FALSE;

    initParam.heap_opt_param1 = 0;
    initParam.heap_opt_param2 = 0;

    //Specify that we will pass some arguments
    opt.flags = 0;
    opt.result = &load_res;

    res = _sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(initParam), &initParam, &opt);

    if(res < 0 || load_res < 0)
    {
        LOG_ERROR("INIT_PAF", res);
        LOG_ERROR("INIT_PAF", load_res);
    }

    return res;
}

void endPaf()
{
    uint32_t buf = 0;
    _sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &buf);
}

int main()
{
    print("Loaded bhbb_dl\n");
    if(sceKernelOpenMsgPipe(BHBB_DL_PIPE_NAME) > 0) //Pipe already exists... a different instance in running!
        return -1;

    if(sceSysmoduleLoadModule(SCE_SYSMODULE_NOTIFICATION_UTIL) != SCE_OK)
        return -1;
    
    sceNotificationUtilBgAppInitialize();
    if(initPaf() < 0)
        return -1;

    Running = SCE_TRUE;

    sceKernelStartThread(dlThreadID = sceKernelCreateThread("bhbb_dl_thread", DownloadThread, SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SCE_KERNEL_128KiB, 0, SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL), 0, NULL);

    if(dlThreadID < 0)
    {
        Running = SCE_FALSE; //Just in case one of them succeeded it will shut down.
        return sceAppMgrDestroyAppByAppId(-2);
    }

    SceUID pipeID = sceKernelCreateMsgPipe(BHBB_DL_PIPE_NAME, SCE_KERNEL_MSG_PIPE_TYPE_USER_MAIN, SCE_KERNEL_ATTR_OPENABLE, SCE_KERNEL_4KiB, NULL);
    if(pipeID < 0)
    {
        Running = SCE_FALSE;
        return sceAppMgrDestroyAppByAppId(-2);
    }

    while (Running)
    {
        SceSize size = 0;
        bhbbPacket pkt;

        int receiveResult = sceKernelReceiveMsgPipe(pipeID, &pkt, sizeof(pkt), SCE_KERNEL_MSG_PIPE_MODE_WAIT | SCE_KERNEL_MSG_PIPE_MODE_FULL, &size, NULL);
        if(receiveResult != SCE_OK)
            print("[Error] Recieve data function has returned error 0x%X\n", receiveResult);

        if(size != sizeof(pkt))
            print("[Warning] Could not recieve all data!\n");
        
        if(size > 0)
        {
            switch (pkt.cmd)
            {
            case INSTALL:
                if(queue.Find(pkt.name) == NULL) //Avoid duplicates
                    queue.enqueue(&pkt, size);
                break;
            case CANCEL:
                queue.remove(pkt.url);
                break;
            case SHUTDOWN:
                Running = SCE_FALSE;
                sceKernelWaitThreadEnd(dlThreadID, NULL, NULL);
                break;
            default:
                print("[Error] Unknown Command!\n");
                break;
            }
        }
        sceKernelDelayThread(1000);
    }

    endPaf();
    sceKernelDeleteMsgPipe(pipeID);    

    return sceAppMgrDestroyAppByAppId(-2);
}