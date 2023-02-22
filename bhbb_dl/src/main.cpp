//Based of off bgvpk & download_enabler by SKGleba & TheFlow

#include <kernel.h>
#include <libsysmodule.h>
#include <notification_util.h>
#include <stdbool.h>
#include <psp2_compat/curl/curl.h>
#include <paf.h>

#include <taihen.h>

#include "notifmgr.h"
#include "main.h"
#include "print.h"
#include "offsets.h"
#include "bhbb_dl.h"
#include "zip.h"
#include "taskmgr.h"
#include "netutils.h"
#include "promote.h"
#include "notice.h"

SceUID taskMutex    =   SCE_UID_INVALID_UID;
SceUID taskThreadID =   SCE_UID_INVALID_UID;
SceUID pipeThreadID =   SCE_UID_INVALID_UID;
SceUID libcurlID    =   SCE_UID_INVALID_UID;

SceBool running = false;

SceUID hooks[6];
tai_hook_ref_t ExportFileRef;
tai_hook_ref_t GetFileTypeRef;
const unsigned char nop32[4] = {0xaf, 0xf3, 0x00, 0x80};

cBGDLItem currentTask;

SceInt32 (*sceLsdbSendNotification)(SceLsdbNotificationParam *, SceInt32);

int ExportFilePatched(uint32_t *data)
{
    int res = TAI_NEXT(ExportFilePatched, ExportFileRef, data);

    if(res == 0x80101A09) // Unsuppourted file
    {
        SceUInt32 bgdlID = *(SceUInt32 *)data[0];
        SceUInt16 urlLength = 0;

        SceUID fd = SCE_UID_INVALID_UID;

        char fileName[0x100];
        char buff[0x400];
        
        BGDLParam param;

        sce_paf_memset(fileName, 0, sizeof(fileName));
        sce_paf_memset(buff, 0, sizeof(buff));
        sce_paf_memset(&param, 0, sizeof(param));

        sce_paf_snprintf(buff, sizeof(buff), "ux0:bgdl/t/%08x/install_param.ini", bgdlID); //First get param file path from bhbb
        
        if(!paf::LocalFile::Exists(buff)) //Not a file from bhbb 
            return res;
    
        fd = sceIoOpen(buff, SCE_O_RDONLY, 0);
        if(fd < 0)
            return fd;
        
        sceIoRead(fd, &param, sizeof(param)); //Read the params
        sceIoClose(fd);

        sce_paf_memset(buff, 0, sizeof(buff));
        sce_paf_snprintf(buff, sizeof(buff), "ux0:bgdl/t/%08x/d0.pdb", bgdlID); //Get SceDownload data or smth
        
        fd = sceIoOpen(buff, SCE_O_RDONLY, 0);
        if(fd < 0)
            return fd;
        
        sceIoPread(fd, &urlLength, sizeof(SceUInt16), 0xD6); //Read the URL Length
		sceIoPread(fd, fileName, sizeof(fileName), 0xF7 + urlLength); //Read the filename
		sceIoClose(fd);

        sce_paf_memset(buff, 0, sizeof(buff));
        sce_paf_snprintf(buff, sizeof(buff), "ux0:bgdl/t/%08x/%s", bgdlID, fileName); //Now get the actual downloaded file path
        
        if(param.type == App) //App
        {
            install(buff);
        }
        else if(param.type == CustomPath) //Data
        {
            Zipfile *zfile = new Zipfile(paf::string(buff));
            SceInt32 result = zfile->Unzip(paf::string(param.path), SCE_NULL);
            delete zfile;
            if(result < 0)
            {
                print("Error extracting %s to %s -> %d\n", buff, param.path, result);
                return result;
            }
        }
        return 0;
    }

    return res;
}

int GetFileTypePatched(int unk, int *type, char **filename, char **mime_type)
{
    print("unk: %p type: %p fileName: %p mime_type: %s\n", unk, type, filename, *mime_type);
    int res = TAI_NEXT(GetFileTypePatched, GetFileTypeRef, unk, type, filename, mime_type);

    if (res == 0x80103A21)
    {
        if(type != SCE_NULL)
            *type = 1; //Type photo
        return 0;
    }

    return res;
}

int install(const char *file)
{
    paf::Dir::RemoveRecursive(EXTRACT_PATH);
    paf::Dir::RemoveRecursive("ux0:temp/new");
    paf::Dir::RemoveRecursive("ux0:appmeta/new");
    paf::Dir::RemoveRecursive("ux0:temp/promote");
    paf::Dir::RemoveRecursive("ux0:temp/game");

    paf::Dir::RemoveRecursive("ur0:temp/new");
    paf::Dir::RemoveRecursive("ur0:appmeta/new");
    paf::Dir::RemoveRecursive("ur0:temp/promote");
    paf::Dir::RemoveRecursive("ur0:temp/game");

    // unZipTo(file, EXTRACT_PATH);
    
    auto zfile = new Zipfile(paf::string(file));
    SceInt32 result = zfile->Unzip(paf::string(EXTRACT_PATH), SCE_NULL);
    delete zfile;
    
    int res = promoteApp(EXTRACT_PATH);

    paf::Dir::RemoveRecursive(EXTRACT_PATH);

    paf::Dir::RemoveRecursive("ux0:temp/new");
    paf::Dir::RemoveRecursive("ux0:appmeta/new");
    paf::Dir::RemoveRecursive("ux0:temp/promote");
    paf::Dir::RemoveRecursive("ux0:temp/game");

    paf::Dir::RemoveRecursive("ur0:temp/new");
    paf::Dir::RemoveRecursive("ur0:appmeta/new");
    paf::Dir::RemoveRecursive("ur0:temp/promote");
    paf::Dir::RemoveRecursive("ur0:temp/game");

    return res;
}

void UnzipProgressCB(uint curr, uint total)
{
    double progress = curr + 1.0;
    double percent = (double)progress / total * 100.0;

    char subtxt[64];
    sce_paf_memset(subtxt, 0, sizeof(subtxt));
    sce_paf_snprintf(subtxt, 64, "Extracting %d%% Done (%d Files / %d Files)", (int)percent, curr + 1, total);
    
    NotifMgr_UpdateProgressNotif(percent, subtxt, currentTask.name);
}

SceInt32 TaskThread(SceSize args, void *argp)
{
    while(running)
    {
        // if(GetTaskNum() <= 0)
        //     continue;

        if(GetCurrentTask(&currentTask) != SCE_OK)
        {
            //Unload libcurl
            if(libcurlID != SCE_UID_INVALID_UID)
            {
                curlEnd();
                sceKernelStopUnloadModule(libcurlID, 0, SCE_NULL, 0, SCE_NULL, SCE_NULL);
                libcurlID = SCE_UID_INVALID_UID;
            }
            continue;
        }
        
        DequeueCurrentTask();

        print("Got Task:\n\t%s -> %s\n", currentTask.url, currentTask.dest);
        
        //Load libcurl (if needed), start DL and notif
        if(libcurlID == SCE_UID_INVALID_UID)
        {
            libcurlID = sceKernelLoadStartModule("ux0:app/BHBB00001/module/libcurl.suprx", 0, SCE_NULL, 0, SCE_NULL, SCE_NULL);
            if(libcurlID < 0)
            {
                print("Error loading libcurl -> 0%X!\n", libcurlID);
                running = false;
                continue;
            }

            curl_global_memmanager_set_np(sce_paf_malloc, sce_paf_free, sce_paf_realloc);
            curlInit();
        }

        NotifMgr_Init();
        print("making progress notif...\n");
        NotifMgr_MakeProgressNotif(currentTask.name, "Downloading", "Are you sure you want to cancel the download?");
        print("Made!\n");
        SceInt32 ret = dlFile(currentTask.url, CBGDL_DL_PATH);
        if(ret == SCE_OK && !NotifMgr_currDlCanceled)
        {
            //Extract
            NotifMgr_UpdateProgressNotif(0, "Preparing to install...", currentTask.name);
            
            Zipfile *zfile = new Zipfile(paf::string(CBGDL_DL_PATH));
            zfile->Unzip(currentTask.dest, UnzipProgressCB);

            NotifMgr_EndNotif(currentTask.name, "Installed successfully");

            sceIoRemove(CBGDL_DL_PATH);
        }
        else if(NotifMgr_currDlCanceled)
        {
            NotifMgr_EndNotif(currentTask.name, "Download canceled");
            sceIoRemove(CBGDL_DL_PATH);
            paf::Dir::RemoveRecursive(currentTask.dest);
        }
        else
        {
            sceIoRemove(CBGDL_DL_PATH);
            if(ret > 0)
            {
                print("Ret = 0x%X\n", ret);
                if(ret != CURLE_ABORTED_BY_CALLBACK) NotifMgr_EndNotif("CURL Error while downloading", curl_easy_strerror((CURLcode)ret));
                else
                {
                    NotifMgr_EndNotif(currentTask.name, "Cancelled");
                    sceNotificationUtilCleanHistory();
                    continue;
                }
            }
            else
            {
                char txt[64];
                sce_paf_memset(txt, 0, 64);
                sce_paf_snprintf(txt, 64, "0x%X", ret);
                NotifMgr_EndNotif("An error ocourred before downloading", txt);
            }
        }

        sce_paf_memset(&currentTask, 0, sizeof(cBGDLItem));
    }
    return sceKernelExitDeleteThread(0);
}

SceInt32 PipeThread(SceSize args, void *argp)
{
    SceUID pipeID = sceKernelCreateMsgPipe(BHBB_DL_PIPE_NAME, SCE_KERNEL_MSG_PIPE_TYPE_USER_MAIN, SCE_KERNEL_ATTR_OPENABLE, 0x1000, NULL);
    if(pipeID < 0)
    {
        print("Error creating MsgPipe! 0x%X\n", pipeID);
        running = SCE_FALSE;
    }
    // print("Pipe thread started! %s\n", running ? "true" : "false");

    while(running)
    {
        SceSize size = 0;
        SceUInt32 waitTimeout = 50000;
        cBGDLItem pkt;
        
        int receiveResult = sceKernelReceiveMsgPipe(pipeID, &pkt, sizeof(pkt), SCE_KERNEL_MSG_PIPE_MODE_WAIT | SCE_KERNEL_MSG_PIPE_MODE_FULL, &size, &waitTimeout);
        if(receiveResult == 0x80028005)
            goto SKIP;

#ifdef _DEBUG
        if(receiveResult != SCE_OK)
            print("[Error] Recieve data function has returned error 0x%X\n", receiveResult);
        if(size != sizeof(pkt))
        {
            print("[Warning] Could not recieve all data!\n");
        }
#endif // _DEBUG
        if(size > 0)
            if(!DoesTaskExist(pkt.name))
                EnqueueTask(&pkt);
        
SKIP: 
        sceKernelDelayThread(1000);
    }
    print("Pipe thread exiting\n");
    sceKernelDeleteMsgPipe(pipeID);
    return sceKernelExitDeleteThread(0);
}

SceInt32 module_start(SceSize args, void *argp)
{
    zipInitPsp2();
    sceSysmoduleLoadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
    taskMutex = sceKernelCreateMutex("bhbb_dl_taskMutex", SCE_KERNEL_MUTEX_ATTR_TH_FIFO | SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 0, SCE_NULL);
    if(taskMutex < 0)
    {
        print("Error creating bhbb_dl_taskMutex -> 0x%X\n", taskMutex);
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
        return SCE_KERNEL_START_NO_RESIDENT;
    }

    running = true;

    taskThreadID = sceKernelCreateThread("bhbb_dl_taskThread", TaskThread, 250, 0x4000, 0, 0, SCE_NULL);
    if(taskThreadID < 0)
    {
        print("Error creating bhbb_dl_taskThread -> 0x%X\n", taskThreadID);
        sceKernelDeleteMutex(taskMutex);
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
        return SCE_KERNEL_START_NO_RESIDENT;
    }

    if(sceKernelStartThread(taskThreadID, 0, SCE_NULL) < 0)
    {
        print("Error failed to start bhbb_dl_taskThread!\n");
        running = false;
        sceKernelDeleteMutex(taskMutex);
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
        return SCE_KERNEL_START_NO_RESIDENT;
    }

    pipeThreadID = sceKernelCreateThread("bhbb_dl_pipeThread", PipeThread, 249, 0x1000, 0, 0, SCE_NULL);
    if(pipeThreadID < 0)
    {
        print("Error creating bhbb_dl_pipeThread -> 0x%X\n", pipeThreadID);
        running = false;
        sceKernelWaitThreadEnd(taskThreadID, SCE_NULL, SCE_NULL);
        sceKernelDeleteCallback(taskMutex);
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
        return SCE_KERNEL_START_NO_RESIDENT;
    }

    if(sceKernelStartThread(pipeThreadID, 0, SCE_NULL) < 0) //Don't u just love cleanups
    {
        print("Error failed to start bhbb_dl_pipeThread!\n");
        running = false;
        sceKernelWaitThreadEnd(taskThreadID, SCE_NULL, SCE_NULL);
        sceKernelDeleteMutex(taskMutex);
        sceKernelDeleteThread(pipeThreadID);
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
        return SCE_KERNEL_START_NO_RESIDENT;   
    }

    tai_module_info_t info;
    info.size = sizeof(info);

    SceUInt32 get_off, exp_off, rec_off, lock_off;
    if (taiGetModuleInfo("SceShell", &info) < 0 || GetShellOffsets(info.module_nid, &get_off, &exp_off, &rec_off, &lock_off) < 0)
        return SCE_KERNEL_START_FAILED;
    
    hooks[0] = taiInjectData(info.modid, 0, get_off, "GET", 4);

    hooks[1] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, exp_off, 1, (void *)ExportFilePatched);
    hooks[2] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, rec_off, 1, (void *)GetFileTypePatched);
    hooks[3] = taiInjectData(info.modid, 0, lock_off, nop32, 4);
    hooks[4] = taiInjectData(info.modid, 0, lock_off + 8, nop32, 4);
    hooks[5] = taiInjectData(info.modid, 0, lock_off + 16, nop32, 4);
    
    print("Running gef\n");
    SceInt32 ret = taiGetModuleExportFunc("SceLsdb", 0xFFFFFFFF, 0x315B9FD6, (uintptr_t *)&sceLsdbSendNotification);    
    print("ret = 0x%X (%p)\n", ret, (uintptr_t *)sceLsdbSendNotification);
    
    return SCE_KERNEL_START_SUCCESS;
}

SceInt32 module_stop(SceSize args, void *argp)
{
    if (hooks[5] >= 0)
        taiInjectRelease(hooks[5]);
    if (hooks[4] >= 0)
        taiInjectRelease(hooks[4]);
    if (hooks[3] >= 0)
        taiInjectRelease(hooks[3]);
    if (hooks[2] >= 0)
        taiHookRelease(hooks[2], GetFileTypeRef);
    if (hooks[1] >= 0)
        taiHookRelease(hooks[1], ExportFileRef);
    if (hooks[0] >= 0)
        taiInjectRelease(hooks[0]);

    NotifMgr_currDlCanceled = true;
    running = false;
    print("Waiting pipeThread\n");
    sceKernelWaitThreadEnd(pipeThreadID, 0, SCE_NULL);
    print("Done!\nWaiting taskThread\n");
    sceKernelWaitThreadEnd(taskThreadID, 0, SCE_NULL);
    print("Done!\nDeleting Mutex\n");
    sceKernelDeleteMutex(taskMutex);
    print("Done! Unloading libcurl\n");

    if(libcurlID != SCE_UID_INVALID_UID)
        sceKernelStopUnloadModule(libcurlID, 0, SCE_NULL, 0, SCE_NULL, SCE_NULL);
    
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);
    return SCE_KERNEL_STOP_SUCCESS; 
}
