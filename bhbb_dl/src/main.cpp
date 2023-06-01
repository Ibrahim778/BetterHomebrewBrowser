//Based of off bgvpk & download_enabler by SKGleba & TheFlow

#include <paf.h>
#include <kernel.h>
#include <libsysmodule.h>
#include <taihen.h>

#include "main.h"
#include "print.h"
#include "offsets.h"
#include "bhbb_dl.h"
#include "notice.h"
#include "installer.h"

SceUID hooks[7];
tai_hook_ref_t ExportFileRef;
tai_hook_ref_t GetFileTypeRef;
tai_hook_ref_t ToastRef;
const unsigned char nop32[4] = {0xaf, 0xf3, 0x00, 0x80};

int (*sceLsdbSendNotification)(SceLsdbNotificationParam *, int);

using namespace paf;

tai_hook_ref_t notifLog_ref;
SceUID         notifLog_id;
int sceLsdbSendNotificationLog(SceLsdbNotificationParam *pParam, int a2)
{
    if(pParam != nullptr)
    {
        print("notification %d:\n\ttitle: %s\n\tdesc: %s\n\ttitle_id: %s\n\texec_title_id: %s\n\titem_id: %s\n\tmsg_type 0x%X\n\taction_type: 0x%X\n\tnew_flag: %hhx\n\thash: %hhx\n\tmsg_arg0: %s\n\tmsg_arg1: %s\n\tmsg_arg2: %s\n\tmsg_arg3: %s\n\tmsg_arg4: %s\n\tmsg_arg5: %s\n\tmsg_arg6: %s\n\tmsg_arg7: %s\n\tmsg_arg8: %s\n\tiunk: 0x%X\n\tunk2[0]: 0x%x\n\tunk2[1]: 0x%x\n", a2, 
            pParam->title.c_str(), 
            pParam->desc.c_str(), 
            pParam->title_id.c_str(), 
            pParam->exec_titleid.c_str(), 
            pParam->item_id.c_str(),
            pParam->msg_type,
            pParam->action_type,
            pParam->new_flag,
            pParam->hash,
            pParam->msg_arg0.c_str(),
            pParam->msg_arg1.c_str(),
            pParam->msg_arg2.c_str(),
            pParam->msg_arg3.c_str(),
            pParam->msg_arg4.c_str(),
            pParam->msg_arg5.c_str(),
            pParam->msg_arg6.c_str(),
            pParam->msg_arg7.c_str(),
            pParam->msg_arg8.c_str(),
            pParam->iunk,
            pParam->unk2[0],
            pParam->unk2[1]
            );
    }
    return TAI_NEXT(sceLsdbSendNotificationLog, notifLog_ref, pParam, a2);
}

void Check(void *)
{
}

int ToastPatch(void *off, unsigned int arg)
{
    if(!arg && off != nullptr)
    {
        if(LocalFile::Exists(common::FormatString("ux0:/bgdl/t/%08x/bhbb.param_done", *(::uint32_t *)off).c_str()))
            return 0; // Notification should already be handled in export function
    }

    return TAI_NEXT(ToastPatch, ToastRef, off, arg);
}

int ExportFilePatched(unsigned int *data)
{
    int res = TAI_NEXT(ExportFilePatched, ExportFileRef, data);

    if(res == 0x80101A09) // Unsuppourted file
    {
        char buff[0x400];
        ::uint32_t bgdlID = *(::uint32_t *)data[0];
        pdb_flags_t flags;
        BGDLParam param;

        // BHBB files will have a vpk file as a flag.
        string bgdl_path = common::FormatString("ux0:bgdl/t/%08x/bhbb.param", bgdlID); 
        
        auto param_file = LocalFile::Open(bgdl_path.c_str(), SCE_O_RDONLY, 0, &res);
        if(res != SCE_PAF_OK) // prob doesn't exist
            return 0x80101A09; // We can leave this to other download plugins

        param_file.get()->Read(&param, sizeof(BGDLParam));
        param_file.get()->Close();
        param_file.release();

        bgdl_path = common::FormatString("ux0:bgdl/t/%08x/d0.pdb", bgdlID);
        
        string title;
        string icon_path;

        // offset of current entry's flags
        uint16_t offset = 0xD3;

        // paf::LocalFile would be nice but it doesn't have pread?! :( sony L
        SceUID fd = sceIoOpen(bgdl_path.c_str(), SCE_O_RDONLY, 0);
        if(fd < 0)
            return fd;

        sceIoPread(fd, &flags, sizeof(flags), offset); // Read first (title) flags
        
        sceIoPread(fd, buff, flags.size, offset + sizeof(flags)); // offset + sizeof(flags) = offset of entry itself
        
        title = buff;

        offset += flags.size + 0xC; // sizeof(flags) + 1
        sceIoPread(fd, &flags, sizeof(flags), offset);
        
        if(flags.size == 1) // Apparently this means that the title is the url and next one is file name so we skip? 
        {
            offset += flags.size + 0xC;
            sceIoPread(fd, &flags, sizeof(flags), offset);
        }

        sceIoPread(fd, buff, flags.size, offset + sizeof(flags)); // We now have the file_name
        bgdl_path = common::FormatString("ux0:bgdl/t/%08x/%s", bgdlID, buff);

        // Next entry is download url (we skip)
        offset += flags.size + 0xC;
        sceIoPread(fd, &flags, sizeof(flags), offset);

        offset += flags.size + 0xC;
        sceIoPread(fd, &flags, sizeof(flags), offset);

        sceIoPread(fd, buff, flags.size, offset + sizeof(flags));

        icon_path = buff;

        sceIoClose(fd);

        res = ProcessExport(title.c_str(), bgdl_path.c_str(), icon_path.c_str(), &param); 
        if(res == 0)
        {
            LocalFile::RenameFile(common::FormatString("ux0:bgdl/t/%08x/bhbb.param", bgdlID).c_str(), common::FormatString("ux0:bgdl/t/%08x/bhbb.param_done", bgdlID).c_str());
        }

        return res;
    }

    return res;
}

int GetFileTypePatched(int unk, int *type, char **filename, char **mime_type)
{
    print("unk: %p type: %p fileName: %p mime_type: %s\n", unk, type, filename, *mime_type);
    int res = TAI_NEXT(GetFileTypePatched, GetFileTypeRef, unk, type, filename, mime_type);

    if (res == 0x80103A21)
    {
        if(type != nullptr)
            *type = 1; //Type photo
        return 0;
    }

    return res;
}

int module_start(size_t args, void *argp)
{
    zipInitPsp2();
    
    tai_module_info_t info;
    info.size = sizeof(info);
    if(taiGetModuleInfo("SceShell", &info) < 0)
        return SCE_KERNEL_START_FAILED;

    unsigned int get_off, exp_off, rec_off, lock_off, notif_off;

    if (GetShellOffsets(info.module_nid, &exp_off, &rec_off, &notif_off) < 0)
        return SCE_KERNEL_START_FAILED;
    

    hooks[1] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, exp_off, 1, (void *)ExportFilePatched);
    hooks[2] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, rec_off, 1, (void *)GetFileTypePatched);
    hooks[6] = taiHookFunctionOffset(&ToastRef, info.modid, 0, notif_off, 1, (void *)ToastPatch);
    
    // notifLog_id = taiHookFunctionImport(&notifLog_ref, "SceShell", 0xFFFFFFFF, 0x315B9FD6, (void *)sceLsdbSendNotificationLog);

    int ret = taiGetModuleExportFunc("SceLsdb", 0xFFFFFFFF, 0x315B9FD6, (uintptr_t *)&sceLsdbSendNotification);    
    print("ret = 0x%X (%p)\n", ret, (uintptr_t *)sceLsdbSendNotification);

    common::MainThreadCallList::Register(Check, nullptr);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(size_t args, void *argp)
{
    if(hooks[6] >= 0)
        taiHookRelease(hooks[6], ToastRef);
    if (hooks[2] >= 0)
        taiHookRelease(hooks[2], GetFileTypeRef);
    if (hooks[1] >= 0)
        taiHookRelease(hooks[1], ExportFileRef);
    if (hooks[0] >= 0)
        taiInjectRelease(hooks[0]);

    // if(notifLog_id >= 0)
    //     taiHookRelease(notifLog_id, notifLog_ref);

    common::MainThreadCallList::Unregister(Check, nullptr);

    return SCE_KERNEL_STOP_SUCCESS; 
}
