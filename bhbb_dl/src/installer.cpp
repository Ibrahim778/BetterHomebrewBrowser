/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2024 Muhammad Ibrahim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/


#include <paf.h>
#include <appmgr.h>
#include <common_gui_dialog.h>
#include <shellsvc.h>

#include "installer.h"
#include "print.h"
#include "promote.h"
#include "notice.h"
#include "dialog.h"
#include "compressed_file.h"

using namespace paf;

#define EXTRACT_PATH "ux0:data/bhbb_prom/"
#define SAVE_PATH "ux0:/bhbb_downloads/"

#define ERROR_LOW_SPACE     -0x50000001
#define ERROR_OPENING_CF    -0x50000002

void AppExtractCB(const char *fname, uint64_t curr, uint64_t total, void *pUserData)
{
    auto progressBar = (ui::ProgressBar *)pUserData;

    float prog = ((float)(curr + 1) / (float)total) * 95.0f;
    
    progressBar->SetValueAsync(prog, true);
}

void ZipExtractCB(const char *fname, uint64_t curr, uint64_t total, void *pUserData)
{
    auto progressBar = (ui::ProgressBar *)pUserData;

    float prog = ((float)(curr + 1) / (float)total) * 100.0f;
    print("%f\n", prog);
    progressBar->SetValueAsync(prog, true);

}

int InstallVPK(paf::common::SharedPtr<CompressedFile> zfile, ui::ProgressBar *progressbar, char *out_titleID)
{
    int res = SCE_OK;

    Dir::RemoveRecursive(EXTRACT_PATH);
    Dir::RemoveRecursive("ux0:temp/new");
    Dir::RemoveRecursive("ux0:appmeta/new");
    Dir::RemoveRecursive("ux0:temp/promote");
    Dir::RemoveRecursive("ux0:temp/game");

    Dir::RemoveRecursive("ur0:temp/new");
    Dir::RemoveRecursive("ur0:appmeta/new");
    Dir::RemoveRecursive("ur0:temp/promote");
    Dir::RemoveRecursive("ur0:temp/game");

    res = zfile->Decompress(EXTRACT_PATH, AppExtractCB, progressbar);
    
    if(res < 0)
    {
        print("[InstallVPK::Decompress()] res -> 0x%X (%d)\n", res, res);
        goto EXIT;
    }
    
    res = promoteApp(EXTRACT_PATH, out_titleID);

    progressbar->SetValue(100.0f, true);

EXIT:
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

void DecideDialogHandler(dialog::ButtonCode buttonCode, ScePVoid pUserArg)
{
    *((dialog::ButtonCode *)pUserArg) = buttonCode;
}

int SaveFile(const char *path) // This should NEVER fail NEVER EVER! (if it does then I cba to fix UB)
{
    auto short_name = common::StripFilename(path, "F");
    auto dir = common::StripFilename(path, "P");
    auto ext = common::StripFilename(path, "E");

    string save_path;

    int count = 0;
    while (1)
    {
        if (count == 0)
        {
            save_path = paf::common::FormatString(SAVE_PATH "%s.%s", short_name.c_str(), ext.c_str());
        }
        else
        {
            save_path = paf::common::FormatString(SAVE_PATH "%s (%d).%s", short_name.c_str(), count, ext.c_str());
        }

        if (!paf::LocalFile::Exists(save_path.c_str()))
        {
            break;
        }

        count++;
    }

    Dir::CreateRecursive(SAVE_PATH, 0006);
    LocalFile::RenameFile(path, save_path.c_str());

    auto indicator_plugin = Plugin::Find("indicator_plugin");

    wstring wstr_path;
    common::Utf8ToUtf16(save_path, &wstr_path);
    wstring sprint = indicator_plugin->GetString(0xdc0cc888);
    for(wchar_t *c = sprint.c_str(); *c != '\0'; c++)
        if(*c == '1') *c = 's'; // Change the string so it'll work with sprintf

    for(wchar_t *c = sprint.c_str(); *c != '\0'; c++)
        if(*(c+1) == '[') *c = '\n'; // Make it look a lil nicer
    
    common::String str;
    str.SetFormattedString(sprint.c_str(), wstr_path.c_str());

    dialog::OpenOk(indicator_plugin, nullptr, str.GetWString().c_str());
    dialog::WaitEnd();

    return 0;
}

int TitleSizeAdjustCB(::int32_t evtID, ui::Handler *handler, ui::Event *evt, void *pUserData)
{
    auto text = (ui::Text *)handler;
    auto icon = (ui::Plane *)pUserData;

    auto size = text->GetDrawObj(ui::Text::OBJ_TEXT)->GetSize();

    text->SetSize({ size.extract_x(), size.extract_y() });

    icon->SetPos({ -(size.extract_x() / 2) - (icon->GetSize(0)->extract_x() / 2) - 10, 150 });

    text->DeleteEventCallback(ui::Text::CB_STATE_READY, (ui::HandlerCB)TitleSizeAdjustCB, pUserData);
}

int ProcessExport(::uint32_t id, const char *name, const char *path, const char *icon_path, BGDLParam *param)
{   
    Plugin *indicator_plugin = Plugin::Find("indicator_plugin");
    SceLsdbNotificationParam notifParam;
    common::String str;
    wstring wtitle;
    rtc::Tick tick;    
    char installedTitleID[12];
    int ret = SCE_OK;

    if(!param)
        return 0xC0FFEE; // No param (how did we get here?)

    if(param->magic != (BHBB_DL_CFG_VER | BHBB_DL_MAGIC))
        return 0xC1FFEE;
    
    LocalFile::RenameFile( // Prevent SceDownload photo toast.
        common::FormatString("ux0:/bgdl/t/%08x/bhbb.param", id).c_str(), 
        common::FormatString("ux0:/bgdl/t/%08x/.installed", id).c_str()
    );

    common::Utf8ToUtf16(name, &wtitle);

    str.SetFormattedString(L"%ls\n%ls", wtitle.c_str(), indicator_plugin->GetString(0x501258e7 /*waiting to install*/));

    rtc::GetCurrentTick(&tick);

    intrusive_ptr<graph::Surface> iconTex;
    
    auto iconFile = LocalFile::Open(icon_path, SCE_O_RDONLY, 0, &ret);
    if(ret == SCE_PAF_OK)
        iconTex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), (common::SharedPtr<File>&)iconFile);
     
    // Preliminary notif param setup
    notifParam.title_id = "NPXS19999";
    notifParam.item_id = common::FormatString("BHBB_DL_%x%llx", IDParam(name).GetIDHash(), tick);

    if(InsideApp()) // Send notification "Ready to install"
    {
        notifParam.display_type = 1; // Toast in app
        notifParam.new_flag = 0;     // ^^
        notifParam.action_type = (SceLsdbNotificationParam::Action)0;
        notifParam.msg_type = SceLsdbNotificationParam::Custom;
        notifParam.title = str.GetString();
        notifParam.icon_path = icon_path;

        sceLsdbSendNotification(&notifParam, 0);
    }

    while(InsideApp())
        thread::Sleep(150);

    dialog::ButtonCode result;

    str.SetFormattedString(L"\n%s\nInstall - Install item now (may take a while)\nSave File - Save file to be manually installed later", indicator_plugin->GetString("msg_downloaded"));
    dialog::OpenTwoButton(indicator_plugin, nullptr, str.GetWString().c_str(), 0, 0, DecideDialogHandler, &result);
    
    // Me and @SonicMastr found this a long time ago
    // This will close the notification centre (if it was open) and suspend input for liveboard
    sceShellUtilLock((SceShellUtilLockType)0x801); 
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU); 

    // manually set button text because I can't figure out a better solution :)

    auto acceptButton = sce::CommonGuiDialog::Dialog::GetWidget(dialog::Current(), sce::CommonGuiDialog::REGISTER_ID_BUTTON_1);
    auto rejectButton = sce::CommonGuiDialog::Dialog::GetWidget(dialog::Current(), sce::CommonGuiDialog::REGISTER_ID_BUTTON_2);

    acceptButton->SetString(L"Install");
    rejectButton->SetString(L"Save File");

    // Might as well make our own title and icon because we can and it looks nicer.

    auto decideDiagPlane = sce::CommonGuiDialog::Dialog::GetWidget(dialog::Current(), sce::CommonGuiDialog::REGISTER_ID_PLANE_BODY);
    thread::Sleep(100);
    auto decideDiagIcon = indicator_plugin->CreateWidget(decideDiagPlane, "plane", "plane_diag_icon", "style_position");
    decideDiagIcon->SetSize({ 65, 65 });
    if(iconTex.get() != nullptr)
        decideDiagIcon->SetTexture(iconTex);

    auto decideDiagTitle = indicator_plugin->CreateWidget(decideDiagPlane, "text", "text_diag_title", "_common_style_text_dialog_title");//0x93e7966b);
    decideDiagTitle->SetAdjust(ui::Text::ADJUST_CONTENT, ui::Text::ADJUST_CONTENT, ui::Text::ADJUST_NONE);
    decideDiagTitle->AddEventCallback(ui::Handler::CB_STATE_READY, (ui::HandlerCB)TitleSizeAdjustCB, decideDiagIcon); 
    decideDiagTitle->SetPos({ 0, 150 });
    decideDiagTitle->SetString(wtitle);

    decideDiagIcon->Show(common::transition::Type_Fadein1);
    decideDiagTitle->Show(common::transition::Type_Fadein1);

    dialog::WaitEnd();

    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU); 
    sceShellUtilUnlock((SceShellUtilLockType)0x801);

    decideDiagIcon->Hide(common::transition::Type_Fadein1);
    decideDiagTitle->Hide(common::transition::Type_Fadein1);
    thread::Sleep(200); // Let dialog close animation complete

    if(result == dialog::ButtonCode_Button1) // Save only
    {
        ret = SaveFile(path);

        notifParam.msg_type = SceLsdbNotificationParam::DownloadComplete; // Send download complete notif
        notifParam.exec_titleid = "VITASHELL";
        notifParam.action_type = SceLsdbNotificationParam::AppOpen;
        notifParam.exec_mode = 0x20000;
        notifParam.icon_path = param->data_icon;
        notifParam.title = name;
        notifParam.new_flag = 1;
        notifParam.display_type = 1;
        
        sceLsdbSendNotification(&notifParam, 1);

        return ret;
    }

    // Me and @SonicMastr found this a long time ago
    // This will close the notification centre (if it was open) and suspend input for liveboard
    sceShellUtilLock((SceShellUtilLockType)0x801); 
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU); 
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_POWEROFF_MENU); 

    Plugin::InitParam pInit;
    pInit.caller_name = "__main__";
    pInit.name = "bhbb_dl_plugin";
    pInit.resource_file = "vs0:/vsh/game/gamecard_installer_plugin.rco";

    Plugin::LoadSync(pInit);

    auto bhbb_dl_plugin = Plugin::Find("bhbb_dl_plugin");
    print("bhbb_dl %p\n", bhbb_dl_plugin);

    Plugin::PageOpenParam pParam;
    pParam.overwrite_draw_priority = 1; // Draw on top of everything else
    auto page = bhbb_dl_plugin->PageOpen("page_bg", pParam); // This page has an invisible plane of size 960x544 to prevent touch input
    
    thread::Sleep(200); // Allow for app exit animation to complete

    Plugin::TemplateOpenParam tParam;
    bhbb_dl_plugin->TemplateOpen(page, 0x9ac337e5, tParam);

    auto diagBase = (ui::Dialog *)page->FindChild("dialog_base");

    bhbb_dl_plugin->TemplateOpen(diagBase, 0x388d5cd6, tParam);

    auto diagTitle = diagBase->FindChild("dialog_title");
    auto diagText = diagBase->FindChild("dialog_text1");
    auto diagProg = (ui::ProgressBar *)diagBase->FindChild("dialog_progressbar1");
    auto diagIcon = (ui::Plane *)diagBase->FindChild("dialog_image");

    diagTitle->SetString(wtitle);

    diagIcon->SetTexture(bhbb_dl_plugin->GetTexture(0x264c5084));
    diagText->SetString(bhbb_dl_plugin->GetString("msg_installing"));

    if(iconTex.get() != nullptr)
        diagIcon->SetTexture(iconTex);
    
    diagBase->Show(common::transition::Type_Popup1);
    
    // Actuall install stuff here
    
    uint64_t max_size;
    uint64_t free_space;
    
    sceAppMgrGetDevInfo("ux0:", &max_size, &free_space);

    size_t requiredSize;
    auto cfile = CompressedFile::Create(path);
    if(!cfile.get())
    {
        ret = ERROR_OPENING_CF;
        goto END;
    }

    cfile->CalculateUncompressedSize();
    requiredSize = cfile->GetUncompressedSize();

    if(free_space <= requiredSize)
    {
        ret = ERROR_LOW_SPACE;
        goto END;
    }

    if(param->type == BGDLTarget_App)
    {
        ret = InstallVPK(cfile, diagProg, installedTitleID);    
    }
    else if(param->type == BGDLTarget_CompressedFile)
    {
        ret = cfile->Decompress(param->path, ZipExtractCB, diagProg);
    }

    print("Install Complete -> ret = 0x%X (%d)\n", ret, ret);

END:
    diagBase->Hide(common::transition::Type_Popup1);

    thread::Sleep(250); // Let the animation play (I am quite surprised this works, I thought it'd hang but it doesnt!)

    bhbb_dl_plugin->PageClose("page_bg", Plugin::PageCloseParam());

    Plugin::UnloadAsync("bhbb_dl_plugin");
    
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_POWEROFF_MENU); 
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU); 
    sceShellUtilUnlock((SceShellUtilLockType)0x801);
    
    if(ret < 0)
    {
        if(ret == ERROR_LOW_SPACE) // Display low space dialog
        {
            wstring wreqsize;
            string reqSizeStr = common::FormatBytesize((requiredSize >= free_space) ? (requiredSize - free_space) : 0, 1);
            
            common::Utf8ToUtf16(reqSizeStr, &wreqsize);

            wstring sprint = bhbb_dl_plugin->GetString(0x3babf33c);
            for(wchar_t *c = sprint.c_str(); *c != '\0'; c++)
                if(*c == '1') *c = 's'; // Change the string so it'll work with sprintf

            str.SetFormattedString(sprint.c_str(), wreqsize.c_str());
            
            dialog::OpenError(indicator_plugin, ret, str.GetWString().c_str());      
        }
        else // Display generic error dialog
        {
            dialog::OpenError(indicator_plugin, ret, indicator_plugin->GetString(0x77f396a2));
        }
        
        dialog::WaitEnd();

        print("Install failed: %d (0x%X)\nResorting to file save!", ret, ret);
        ret = SaveFile(path);

        notifParam.msg_type = SceLsdbNotificationParam::DownloadComplete;
        notifParam.exec_titleid = "VITASHELL";
        notifParam.action_type = SceLsdbNotificationParam::AppOpen;
        notifParam.exec_mode = 0x20000;
        notifParam.icon_path = param->data_icon;
        notifParam.title = name;
        notifParam.new_flag = 1;
        notifParam.display_type = 1;
        sceLsdbSendNotification(&notifParam, 1);
    }
    else  // operation success
    {
        // Send notification installed successfully 
        if(param->type == BGDLTarget_App)
        {
            notifParam.title.clear();
            notifParam.exec_titleid = installedTitleID;
            notifParam.msg_type = SceLsdbNotificationParam::AppInstalledSuccessfully;
            notifParam.display_type = 1; // Highlight
            notifParam.new_flag = 1;     // ^^
            notifParam.action_type = SceLsdbNotificationParam::AppHighlight;
            notifParam.icon_path.clear();
        }
        else if(param->type == BGDLTarget_CompressedFile)
        {
            notifParam.msg_type = SceLsdbNotificationParam::DownloadComplete; // Send download complete notif
            notifParam.exec_titleid = "VITASHELL";
            notifParam.action_type = SceLsdbNotificationParam::AppOpen;
            notifParam.exec_mode = 0x20000;
            notifParam.icon_path = param->data_icon;
            notifParam.title = name;
            notifParam.new_flag = 1;
            notifParam.display_type = 1;
        }

        sceLsdbSendNotification(&notifParam, 1);
    }

    return ret;
}  


