#include "main.hpp"
#include <kernel.h>
#include <paf.h>
#include <notification_util.h>
#include <libsysmodule.h>
#include "bhbb_dl.h"
#include "net.hpp"
#include <stdio.h>
#include "list.hpp"
#include <curl/curl.h>
#include "Archives.hpp"
#include "notifmgr.hpp"
#include <appmgr.h>
#include <promoterutil.h>

#define EXTRACT_PATH "ux0:temp/app"
extern "C" int promoteApp(const char* path);

extern unsigned int	sce_process_preload_disabled = (SCE_PROCESS_PRELOAD_DISABLED_LIBDBG \
	| SCE_PROCESS_PRELOAD_DISABLED_LIBCDLG | SCE_PROCESS_PRELOAD_DISABLED_LIBPERF \
	| SCE_PROCESS_PRELOAD_DISABLED_APPUTIL | SCE_PROCESS_PRELOAD_DISABLED_LIBSCEFT2 | SCE_PROCESS_PRELOAD_DISABLED_LIBPVF);

unsigned int sceLibcHeapSize = 1024 * 1024 * 6;

Queue queue;

SceBool Running = SCE_TRUE;
SceUID dlThreadID = SCE_UID_INVALID_UID;

int checkFileExist(const char *file)
{
    SceIoStat s;
    return sceIoGetstat(file, &s) >= 0;
}

int install(const char *file)
{
    Zip *zfile;
    char txt[64] = {0};
    
    if(NotifMgr::currDlCanceled)
        goto END;
    
    zfile = ZipOpen(file);
    ZipExtract(zfile, NULL, EXTRACT_PATH);
    ZipClose(zfile);

    NotifMgr::EndNotif(queue.head->packet.name, "Promoting");
    
    if(NotifMgr::currDlCanceled) goto END;

    {
        int res = promoteApp(EXTRACT_PATH);
        if(res == SCE_OK)
            snprintf(txt, 64, "Installed %s Successfully!", queue.head->packet.name);
        else
            snprintf(txt, 64, "Error 0x%X", res);

        NotifMgr::SendNotif(txt);
    }
END:
    sceIoRemove(EXTRACT_PATH);
    sceIoRemove(file);
    return 0;
}

SceInt32 DownloadThread(SceSize args, void *argp)
{
    curlInit();
    
    while (Running)
    {
        if(queue.num == 0)
        {
            sceKernelDelayThread(10000);
            continue;
        }
        
        NotifMgr::Init();
        NotifMgr::MakeProgressNotif(queue.head->packet.name, "Installing", "Are you sure you want to cancel the install?");


        char dest[SCE_IO_MAX_PATH_BUFFER_SIZE] = {0};
        snprintf(dest, SCE_IO_MAX_PATH_BUFFER_SIZE, "ux0:/temp/%s.vpk", queue.head->packet.name);
        dlFile(queue.head->packet.url, dest);

        if(!NotifMgr::currDlCanceled)    
        {
            NotifMgr::UpdateProgressNotif(0, "Extracting", queue.head->packet.name);
            install(dest);
        }
        else sceIoRemove(dest);

        queue.dequeue();
    };

    curlEnd();
    return sceKernelExitDeleteThread(0);
}

int main()
{
    int r = sceKernelOpenMsgPipe(BHBB_DL_PIPE_NAME);

    if(r > 0) //Pipe already exists... a different instance in running!
        return -1;

	if(sceSysmoduleLoadModule(SCE_SYSMODULE_NOTIFICATION_UTIL) != SCE_OK)
        return -1;
    sceNotificationUtilBgAppInitialize();

    Running = SCE_TRUE;

    sceKernelStartThread(dlThreadID = sceKernelCreateThread("bhbb_dl_thread", DownloadThread, SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SCE_KERNEL_128KiB, 0, SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL), 0, NULL);

    if(dlThreadID < 0)
    {
        Running = SCE_FALSE; //Just in case one of them succeeded it will shut down.
        return -1;
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
            printf("[Error] Recieve data function has returned error 0x%X\n", receiveResult);

        if(size != sizeof(pkt))
            printf("[Warning] Could not recieve all data!\n");
        
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
                break;
            default:
                printf("[Error] Unkown Command!\n");
                break;
            }
        }
        sceKernelDelayThread(1000);
    }

    sceKernelDeleteMsgPipe(pipeID);    

    return sceAppMgrDestroyAppByAppId(-2);
}