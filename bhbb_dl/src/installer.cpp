#include <paf.h>
#include <appmgr.h>
#include <common_gui_dialog.h>
#include <shellsvc.h>

#include "zip.h"
#include "installer.h"
#include "print.h"
#include "promote.h"
#include "notice.h"

using namespace paf;

#define EXTRACT_PATH "ux0:data/bhbb_prom/"

extern int (*sceLsdbSendNotification)(SceLsdbNotificationParam *, int);

void AppExtractCB(::uint32_t curr, ::uint32_t total, void *pUserData)
{
    auto progressBar = (ui::ProgressBar *)pUserData;

    float prog = ((float)(curr + 1) / (float)total) * 80.0f;
    progressBar->SetValueAsync(prog, true);
}

void ZipExtractCB(::uint32_t curr, ::uint32_t total, void *pUserData)
{
    auto progressBar = (ui::ProgressBar *)pUserData;

    float prog = ((float)(curr + 1) / (float)total) * 100.0f;
    progressBar->SetValueAsync(prog, true);
}

int install(const char *file, ui::ProgressBar *progressbar, char *out_titleID)
{
    Dir::RemoveRecursive(EXTRACT_PATH);
    Dir::RemoveRecursive("ux0:temp/new");
    Dir::RemoveRecursive("ux0:appmeta/new");
    Dir::RemoveRecursive("ux0:temp/promote");
    Dir::RemoveRecursive("ux0:temp/game");

    Dir::RemoveRecursive("ur0:temp/new");
    Dir::RemoveRecursive("ur0:appmeta/new");
    Dir::RemoveRecursive("ur0:temp/promote");
    Dir::RemoveRecursive("ur0:temp/game");

    auto zfile = Zipfile(file);
    SceInt32 result = zfile.Unzip(EXTRACT_PATH, AppExtractCB, progressbar);
    
    int res = promoteApp(EXTRACT_PATH, out_titleID);

    progressbar->SetValue(100.0f, true);

    Dir::RemoveRecursive(EXTRACT_PATH);

    Dir::RemoveRecursive("ux0:temp/new");
    Dir::RemoveRecursive("ux0:appmeta/new");
    Dir::RemoveRecursive("ux0:temp/promote");
    Dir::RemoveRecursive("ux0:temp/game");

    Dir::RemoveRecursive("ur0:temp/new");
    Dir::RemoveRecursive("ur0:appmeta/new");
    Dir::RemoveRecursive("ur0:temp/promote");
    Dir::RemoveRecursive("ur0:temp/game");

    return res;
}

bool InsideApp()
{
    SceAppMgrAppStatus status;
    char titleid[10];
    titleid[9] = '\0';

    SceUID runningApps[0x10];
    sce_paf_memset(runningApps, 0, sizeof(runningApps));

    int res = sceAppMgrGetRunningAppIdListForShell(runningApps, 0x10);
    for(int i = 0; i < res; i++)
    {
        sceAppMgrGetNameById(sceAppMgrGetProcessIdByAppIdForShell(runningApps[i]), titleid);
        sce_paf_memset(&status, 0, sizeof(status));
        sceAppMgrGetStatusByName(titleid, &status);
        if(status.isShellProcess) // This is true only if is a proper app (returns false for bg app)
            return true;
    }

    return false;
}

int ProcessExport(const char *name, const char *path, const char *icon_path, BGDLParam *param)
{   
    char installedTitleID[12];
    int ret = SCE_OK;

    while(InsideApp())
        thread::Sleep(200);

    thread::Sleep(200); // Allow for app exit animation to complete

    if(!param)
        return 0xC0FFEE; // No param (how did we get here?)

    if(param->magic != (BHBB_DL_CFG_VER | BHBB_DL_MAGIC))
        return 0xC1FFEE;

    // Me and @SonicMastr found this a long time ago
    // This will close the notification centre (if it was open) and suspend input in the livearea
    sceShellUtilLock((SceShellUtilLockType)0x801); 

    Plugin::InitParam pInit;
    pInit.caller_name = "__main__";
    pInit.name = "bhbb_dl_plugin";
    pInit.resource_file = "vs0:/vsh/game/gamecard_installer_plugin.rco";

    Plugin::LoadSync(pInit);

    auto bhbb_dl_plugin = Plugin::Find("bhbb_dl_plugin");
    print("bhbb_dl %p\n", bhbb_dl_plugin);

    Plugin::PageOpenParam pParam;
    pParam.overwrite_draw_priority = 1;
    auto page = bhbb_dl_plugin->PageOpen("page_bg", pParam); // This page has an invisible plane of size 960x544 to prevent touch input
    
    Plugin::TemplateOpenParam tParam;
    bhbb_dl_plugin->TemplateOpen(page, 0x9ac337e5, tParam);

    auto diagBase = page->FindChild("dialog_base");

    bhbb_dl_plugin->TemplateOpen(diagBase, 0x388d5cd6, tParam);

    auto diagTitle = diagBase->FindChild("dialog_title");
    auto diagText = diagBase->FindChild("dialog_text1");
    auto diagProg = (ui::ProgressBar *)diagBase->FindChild("dialog_progressbar1");
    auto diagIcon = (ui::Plane *)diagBase->FindChild("dialog_image");

    wstring wtitle;
    common::Utf8ToUtf16(name, &wtitle);
    diagTitle->SetString(wtitle);

    diagIcon->SetTexture(bhbb_dl_plugin->GetTexture(0x264c5084));
    diagText->SetString(bhbb_dl_plugin->GetString("msg_installing"));

    auto iconFile = LocalFile::Open(icon_path, SCE_O_RDONLY, 0, &ret);
    if(ret == SCE_PAF_OK)
    {
        auto tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), (common::SharedPtr<File>&)iconFile);
        if(tex.get())
            diagIcon->SetTexture(tex);
    }    
    
    diagBase->Show(common::transition::Type_Popup1);
    
    // Actuall install stuff here
    
    if(param->type == App)
        ret = install(path, diagProg, installedTitleID); 
    else if(param->type == Zip)
    {
        Zipfile zip(path);
        zip.Unzip(param->path, ZipExtractCB, diagProg);
    }

    diagBase->Hide(common::transition::Type_Popup1);

    thread::Sleep(200); // Let the animation play (I am quite surprised this works, I thought it'd hang but it doesnt!)

    bhbb_dl_plugin->PageClose("page_bg", Plugin::PageCloseParam());

    Plugin::UnloadAsync("bhbb_dl_plugin");
    
    sceShellUtilUnlock((SceShellUtilLockType)0x801);
    
    rtc::Tick tick;
    rtc::GetCurrentTick(&tick);

    SceLsdbNotificationParam toastParam;
    toastParam.title_id = "NPXS19999";
    toastParam.item_id = common::FormatString("BHBB_DL_%s%llu", installedTitleID, param, tick);
    toastParam.new_flag = 3;
    toastParam.action_type = AppHighlight;
    toastParam.exec_titleid = installedTitleID;
    toastParam.msg_type = AppInstalledSuccessfully;

    sceLsdbSendNotification(&toastParam, 0);

    return ret;
}  

