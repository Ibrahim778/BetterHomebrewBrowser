//Based of off bgvpk & download_enabler by SKGleba & TheFlow

#include <paf.h>
#include <kernel.h>
#include <libsysmodule.h>
#include <taihen.h>

#include "main.h"
#include "print.h"
#include "offsets.h"
#include "bhbb_dl.h"
#include "installer.h"

using namespace paf;

void OnPAFLoad();

tai_hook_ref_t ToastRef;
SceUID ToastID = SCE_UID_INVALID_UID;
int ToastPatch(void *off, unsigned int arg)
{
    if(!arg && off != nullptr)
    {
        if(LocalFile::Exists(common::FormatString("ux0:/bgdl/t/%08x/.installed", *(::uint32_t *)off).c_str()))
            return 0; // Notification should already be handled in export function
    }

    return TAI_NEXT(ToastPatch, ToastRef, off, arg);
}

tai_hook_ref_t ExportFileRef;
SceUID ExportFileID = SCE_UID_INVALID_UID;
int ExportFilePatched(unsigned int *data)
{
    int res = TAI_NEXT(ExportFilePatched, ExportFileRef, data);

    if(res == 0x80101A09) // Unsupported file
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
        // Read next flags
        offset += flags.size + 0xC;
        sceIoPread(fd, &flags, sizeof(flags), offset);
        // Read icon path
        sceIoPread(fd, buff, flags.size, offset + sizeof(flags));

        icon_path = buff;

        sceIoClose(fd);

        return ProcessExport(bgdlID, title.c_str(), bgdl_path.c_str(), icon_path.c_str(), &param); 
    }

    return res;
}

tai_hook_ref_t GetFileTypeRef;
SceUID GetFileTypeID = SCE_UID_INVALID_UID;
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

SceUID sysmoduleHookID = SCE_UID_INVALID_UID;
tai_hook_ref_t sysmoduleHookRef;
int sceSysmoduleLoadModuleInternalWithArgPatched(SceSysmoduleInternalModuleId id, size_t size, void *argp, void *opt)
{
    int res = TAI_NEXT(sceSysmoduleLoadModuleInternalWithArgPatched, sysmoduleHookRef, id, size, argp, opt);
    if(res == 0 && id == SCE_SYSMODULE_INTERNAL_PAF)
    {
        OnPAFLoad();
        taiHookRelease(sysmoduleHookID, sysmoduleHookRef);
        sysmoduleHookID = SCE_UID_INVALID_UID;
    }
    return res;
}

void OnPAFLoad()
{
    zipInitPsp2();
}

int module_start(size_t args, void *argp)
{    
    tai_module_info_t info;
    
    sceClibMemset(&info, 0, sizeof(info));
    info.size = sizeof(info);

    if(taiGetModuleInfo("ScePaf", &info) < 0)
    {
        // Hook sysmodule load to catch when PAF is loaded
        sysmoduleHookID = taiHookFunctionImport(&sysmoduleHookRef, (const char *)TAI_MAIN_MODULE, 0x03FCF19D, 0xC3C26339, sceSysmoduleLoadModuleInternalWithArgPatched);
    }
    else 
    {
        OnPAFLoad();
    }

    sceClibMemset(&info, 0, sizeof(info));
    info.size = sizeof(info);

    if(taiGetModuleInfo("SceShell", &info) < 0)
        return SCE_KERNEL_START_FAILED;

    unsigned int get_off, exp_off, rec_off, lock_off, notif_off;

    if(GetShellOffsets(info.module_nid, &exp_off, &rec_off, &notif_off) < 0)
        return SCE_KERNEL_START_FAILED;
    
    ExportFileID = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, exp_off, 1, (void *)ExportFilePatched);
    GetFileTypeID = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, rec_off, 1, (void *)GetFileTypePatched);
    ToastID = taiHookFunctionOffset(&ToastRef, info.modid, 0, notif_off, 1, (void *)ToastPatch);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(size_t args, void *argp)
{
    if(ToastID >= 0)
        taiHookRelease(ToastID, ToastRef);
    if(GetFileTypeID >= 0)
        taiHookRelease(GetFileTypeID, GetFileTypeRef);
    if(ExportFileID >= 0)
        taiHookRelease(ExportFileID, ExportFileRef);
    if(sysmoduleHookID >= 0)
        taiHookRelease(sysmoduleHookID, sysmoduleHookRef);

    return SCE_KERNEL_STOP_SUCCESS; 
}
