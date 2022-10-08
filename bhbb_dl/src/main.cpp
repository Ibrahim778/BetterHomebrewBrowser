//Based of off bgvpk & download_enabler by SKGleba & TheFlow

#include <kernel.h>
#include <moduleinfo.h>
#include <taihen.h>
#include <libsysmodule.h>
#include <incoming_dialog.h>
#include <paf.h>
#include <ces.h>
#include <appmgr.h>
#include <quickmenureborn/qm_reborn.h>
#include <notification_util.h>
#include <promoterutil.h>
#include <shellsvc.h>
#include <libextractor.h>

#include "main.h"
#include "print.h"
#include "offsets.h"
#include "bhbb_dl.h"

using namespace paf;

SceUID hooks[6];
tai_hook_ref_t ExportFileRef;
tai_hook_ref_t GetFileTypeRef;
const unsigned char nop32[4] = {0xaf, 0xf3, 0x00, 0x80};

SceBool IsOnLiveArea()
{
    SceInt32 result = SCE_OK;
    
    SceUID appIDs[20];
    sce_paf_memset(appIDs, SCE_UID_INVALID_UID, sizeof(appIDs));

    SceSize count = result = sceAppMgrGetRunningAppIdListForShell(appIDs, 20);
    if(result < 0)
    {
        print("sceAppMgrGetRunningAppIdListForShell() -> 0x%X\n", result);
        return false;
    }

    for(int i = 0; i < count; i++)
    {
        char name[0x10];
        sce_paf_memset(name, 0, sizeof(name));

        result = sceAppMgrGetNameById(sceAppMgrGetProcessIdByAppIdForShell(appIDs[i]), name);
        if(result != SCE_OK)
        {
            print("sceAppMgrGetNameById (0x%X) -> 0x%X\n", appIDs[i], result);
            continue;
        }

        SceAppMgrAppStatus status;
        result = sceAppMgrGetStatusByName(name, &status); 
        if(result < 0)
        {
            print("sceAppMgrGetStatusByName() -> 0x%X\n", result);
            continue;
        }

        if(status.isShellProcess)
            return false;
    } 

    return true;
}

//From BGFTP, too lazy to rewrite if it works lol
//Send a notification with formatting :D
void sendNotification(const char *text, ...)
{
	SceNotificationUtilSendParam param;
	uint32_t inSize, outSize;

	char buf[SCE_NOTIFICATION_UTIL_TEXT_MAX * 2];
	va_list argptr;
	va_start(argptr, text);
	sceClibVsnprintf(buf, sizeof(buf), text, argptr); //oh no
	va_end(argptr);

	sce_paf_memset(&param, 0, sizeof(SceNotificationUtilSendParam));

	SceCesUcsContext context;
	sceCesUcsContextInit(&context);
	sceCesUtf8StrToUtf16Str(
		&context,
		(uint8_t *)buf,
		SCE_NOTIFICATION_UTIL_TEXT_MAX * 2,
		&inSize,
		(uint16_t *)param.text,
		SCE_NOTIFICATION_UTIL_TEXT_MAX,
		&outSize);

	sceNotificationUtilSendNotification(&param);
}

int ExportFilePatched(uint32_t *data)
{
    int res = TAI_NEXT(ExportFilePatched, ExportFileRef, data);

    if(res == 0x80101A09) // Unsuppourted file
    {
        SceUInt32 bgdlID = *(SceUInt32 *)data[0];
        SceUInt16 urlLength = 0;

        char fileName[0x100];
        
        string paramPath = ccc::Sprintf("ux0:bgdl/t/%08x/install_param.ini", bgdlID);
        
        if(!paf::LocalFile::Exists(paramPath.data())) //Not a file from bhbb 
            return res;
    
        string pdbPath = ccc::Sprintf("ux0:bgdl/t/%08x/d0.pdb", bgdlID);
        
        SceUID fd = sceIoOpen(pdbPath.data(), SCE_O_RDONLY, 0);
        if(fd < 0)
            return fd;
        
        sceIoPread(fd, &urlLength, sizeof(SceUInt16), 0xD6);
		sceIoPread(fd, fileName, sizeof(fileName), 0xF7 + urlLength);
		sceIoClose(fd);

        string filePath = ccc::Sprintf("ux0:bgdl/t/%08x/%s", bgdlID, fileName);
        
        BGDLParam param;
        sce_paf_memset(&param, 0, sizeof(param));

        fd = sceIoOpen(paramPath.data(), SCE_O_RDONLY, 0);
        if(fd < 0)
            return fd;
        
        sceIoRead(fd, &param, sizeof(param));
        sceIoClose(fd);

        SceUID moduleID = sceKernelLoadStartModule("ux0:app/BHBB00001/module/libextractor.suprx", 0, SCE_NULL, 0, SCE_NULL, SCE_NULL);
        if(moduleID < 0)
        {
            print("sceKernelLoadStartModule(\"ux0:app/BHBB00001/module/libextractor.suprx\", 0, SCE_NULL, 0, SCE_NULL, SCE_NULL) -> 0x%X\n", moduleID);
            return moduleID;
        }
        else print("Module started with ID 0x%X\n", moduleID);

        if(param.type == BGDLParam::Target::App) //App
        {
        
        }
        else if(param.type == BGDLParam::Target::CustomPath) //Data
        {
            Zipfile zFile = Zipfile(filePath.data());
            SceInt32 result = zFile.Unzip(param.path);
            if(result < 0)
                print("Error extracting %s to %s -> %d\n", filePath, param.path, result);
        }

        moduleID = sceKernelStopUnloadModule(moduleID, 0, SCE_NULL, 0, SCE_NULL, SCE_NULL);
        if(moduleID != SCE_OK)
        {
            print("Error unloading module -> 0x%X\n", moduleID);
            return moduleID;
        }

        return 0;
    }

    return res;
}

int GetFileTypePatched(int unk, int *type, char **filename, char **mime_type)
{
    int res = TAI_NEXT(GetFileTypePatched, GetFileTypeRef, unk, type, filename, mime_type);
    print("Type = %d res = 0x%X %s %s\n", *type, res, *filename, *mime_type);
    if (res == 0x80103A21)
    {
        *type = 1; //Type photo
        return 0;
    }

    return res;
}

SceInt32 module_start(SceSize args, void *argp)
{
    sceSysmoduleLoadModule(SCE_SYSMODULE_NOTIFICATION_UTIL);

    tai_module_info_t info;
    info.size = sizeof(info);

    SceUInt32 get_off, exp_off, rec_off, lock_off;
    if (taiGetModuleInfo("SceShell", &info) < 0 || GetShellOffsets(info.module_nid, &get_off, &exp_off, &rec_off, &lock_off) < 0)
        return SCE_KERNEL_START_FAILED;
    
    hooks[0] = taiInjectData(info.modid, 0, get_off, "GET", 4);

    hooks[1] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, exp_off, 1, ExportFilePatched);
    hooks[2] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, rec_off, 1, GetFileTypePatched);
    hooks[3] = taiInjectData(info.modid, 0, lock_off, nop32, 4);
    hooks[4] = taiInjectData(info.modid, 0, lock_off + 8, nop32, 4);
    hooks[5] = taiInjectData(info.modid, 0, lock_off + 16, nop32, 4);
    
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

    return SCE_KERNEL_STOP_SUCCESS; 
}
