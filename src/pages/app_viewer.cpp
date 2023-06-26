#include <paf.h>
#include <shellsvc.h>

#include "db/source.h"
#include "pages/app_viewer.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"
#include "common.h"
#include "print.h"
#include "dialog.h"
#include "pages/app_browser.h"
#include "pages/image_viewer.h"
#include "downloader.h"
#include "bhbb_dl.h"

using namespace paf;
using namespace common;
using namespace math;
using namespace thread;

AppViewer::AppViewer(Source::Entry& entry, AppBrowser::TexPool *pTexPool):
    page::Base(
        app_info_page,
        Plugin::PageOpenParam(false, 200, Plugin::TransitionType_SlideFromBottom),
        Plugin::PageCloseParam(true)
    ),app(entry),pool(pTexPool)
{
    auto isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);
    if(!isMainThread)
        RMutex::main_thread_mutex.Lock();

    busyIndicator = (ui::BusyIndicator *)root->FindChild(description_busy);
    descText = (ui::Text *)root->FindChild(info_description_text);

    root->FindChild(info_title_text)->SetString(app.title);
    root->FindChild(info_author_text)->SetString(app.author);

    String timeVer;

    wchar_t time[0x40];
    sce_paf_memset(time, 0, sizeof(time));

    entry.lastUpdated.Wcsftime(time, sizeof(time), L"%d/%m/%y");
    timeVer.SetFormattedString(L"(%ls) %ls", time, entry.version.c_str());
    root->FindChild(info_version_text)->SetString(timeVer.GetWString().c_str());

    if(app.screenshotURL.size() > 0)
    {
        Plugin::TemplateOpenParam tOpen;
        auto scrollBox = root->FindChild(screenshot_box);
        
        for(int i = 0; i < app.screenshotURL.size(); i++)
        {
            g_appPlugin->TemplateOpen(scrollBox, info_screenshot_button_template, tOpen);
            auto widget = scrollBox->GetChild(scrollBox->GetChildrenNum() - 1);
            widget->SetName(i);
            widget->AddEventCallback(ui::Button::CB_BTN_DECIDE, ScreenshotCB, &app.screenshotURL);
        }
    }
    else
    {
        transition::Do(0, root->FindChild(screenshot_plane), transition::Type_Reset, true, true);
        root->FindChild(description_plane)->SetSize(960, 444, 0, 0);
    }
    
    // Manage the icon button (For manual icon redownloads)
    auto iconButton = root->FindChild(icon_button);
    iconButton->SetTexture(pTexPool->Get(app.hash), 0, 0);
    iconButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, IconButtonCB, this);

    // Take care of our download buttons
    root->FindChild(download_button)->AddEventCallback(ui::Button::CB_BTN_DECIDE, DownloadButtonCB, this);
    
    auto dataButton = (ui::Button *)root->FindChild(data_download_button);
    if(app.dataURL.size() == 0) // Hide button
        dataButton->Hide(transition::Type_Reset);
    else // Assign callback
        dataButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, DownloadButtonCB, this);
    
    // Setup our information button
    auto infoButton = (ui::Button *)root->FindChild(info_button);
    infoButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, InfoButtonCB, this);

    if(!isMainThread)
        RMutex::main_thread_mutex.Unlock();

    busyIndicator->Start();

    descText->AddEventCallback(DescriptionEvent, DescriptionTextCB, this);

    auto itemParam = SharedPtr<job::JobItem>(new AsyncDescriptionJob(app, this));
    job::JobQueue::default_queue->Enqueue(itemParam);
}   

AppViewer::~AppViewer()
{

}

void AppViewer::InfoButtonCB(int id, ui::Handler *self, ui::Event *event, void *pUserData)
{
    auto workPage = (AppViewer *)pUserData;

    paf::wstring diagString;

    if(workPage->app.downloadSize == 0 && workPage->app.dataSize == 0)
        diagString = g_appPlugin->GetString(msg_download_nosize);
    else
    {
        common::String str;
        paf::wstring wsize;
        if(workPage->app.downloadSize != 0)
        {
            common::Utf8ToUtf16(common::FormatBytesize(workPage->app.downloadSize, 1), &wsize);

            str.SetFormattedString(L"%s: %s", g_appPlugin->GetString(db_category_single_app), wsize.c_str());
            diagString += str.GetWString();
        }
        if(workPage->app.dataSize != 0)
        {
            common::Utf8ToUtf16(common::FormatBytesize(workPage->app.dataSize, 1), &wsize);

            str.SetFormattedString(L"\n%s: %s", g_appPlugin->GetString(data_button_text), wsize.c_str());
            diagString += str.GetWString();
        }
    }

    dialog::OpenOk(g_appPlugin, nullptr, diagString.c_str());
}

void AppViewer::DescriptionTextCB(int id, ui::Handler *self, ui::Event *event, void *pUserData)
{
    auto target = (ui::Widget *)self;
    
    int res = event->GetValue(0);
    auto workPage = (AppViewer *)pUserData;
    auto desc = (paf::wstring *)(intptr_t)event->GetValue(1);

    if(res < 0)
    {
        dialog::OpenError(g_appPlugin, res, g_appPlugin->GetString(msg_desc_error));
        target->SetString(g_appPlugin->GetString(msg_desc_error));
    }
    else 
        target->SetString(*desc);
    

    workPage->busyIndicator->Stop();
}

void AppViewer::AsyncDescriptionJob::Run()
{
    int res = 0;
    paf::wstring desc;

    res = entry.pSource->GetDescription(entry, desc);
    if(entry.screenshotURL.size() == 0)
        desc += L"\n\n\n";
        
    event::BroadcastGlobalEvent(g_appPlugin, AppViewer::DescriptionEvent, res, (uintptr_t)&desc);
    thread::Sleep(100); // This is enough to keep this stack frame from dying so that the processing in the event can happen
}

void AppViewer::ScreenshotCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData)
{
    auto url = (std::vector<paf::string> *)pUserData;
    auto workWidget = (ui::Widget *)self; // ID Hash is index of url in above vector
    
    new ImageViewer(url->at(workWidget->GetName().GetIDHash()).c_str());
}

void AppViewer::IconButtonCB(int id, ui::Handler *self, ui::Event *event, void *pUserData)
{
    auto job = new IconDownloadJob("AppViewer::IconDownloadJob");
    job->workPage = (AppViewer *)pUserData;
    auto jobParam = SharedPtr<job::JobItem>(job);
    job::JobQueue::default_queue->Enqueue(jobParam);
}

void AppViewer::DownloadButtonCB(int id, ui::Handler *self, ui::Event *event, void *pUserData)
{
    auto pWidget = (ui::Widget *)self;

    auto job = new DownloadJob("AppViewer::DownloadJob");
    job->type = pWidget->GetName().GetIDHash() == download_button ? DownloadJob::DownloadType_App : DownloadJob::DownloadType_Data;
    job->workPage = (AppViewer *)pUserData;
    auto jobParam = SharedPtr<job::JobItem>(job);
    job::JobQueue::default_queue->Enqueue(jobParam);
}

void AppViewer::IconDownloadJob::Run()
{
    dialog::OpenPleaseWait(g_appPlugin, nullptr, g_appPlugin->GetString(msg_downloading_icon));
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    LocalFile::RemoveFile(workPage->app.iconPath.c_str());

    int res = 0;
    if(!workPage->pool->Add(&workPage->app, true, &res))
    {
        // Failed
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        dialog::Close();
        dialog::OpenError(g_appPlugin, res, g_appPlugin->GetString(msg_icon_error));
        return;
    }

    workPage->root->FindChild(icon_button)->SetTexture(workPage->pool->Get(workPage->app.hash), 0, 0);
    event::BroadcastGlobalEvent(g_appPlugin, ui::Handler::CB_STATE_READY_CACHEIMAGE, workPage->app.hash);

    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    dialog::Close();
}

void AppViewer::DownloadJob::DialogCB(dialog::ButtonCode bc, void *pUserData)
{
    *((dialog::ButtonCode *)pUserData) = bc;
}

void AppViewer::DownloadJob::Run()
{
    print("[AppViewer::DownloadJob] Run(START)\n");
    
    paf::string url;
    common::String appName;
    BGDLParam dlParam;

    sce_paf_memset(&dlParam, 0, sizeof(dlParam));

    dlParam.magic = (BHBB_DL_CFG_VER | BHBB_DL_MAGIC);

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    dialog::OpenPleaseWait(g_appPlugin, nullptr, g_appPlugin->GetString(msg_wait));

    int ret = 0;
    switch(type)
    {
    case DownloadType_App:
        ret = workPage->app.pSource->GetDownloadURL(workPage->app, url);
        dlParam.type = BGDLTarget_App;
        sce_paf_strncpy(dlParam.data_icon, workPage->app.iconPath.c_str(), sizeof(dlParam.data_icon));
        appName = workPage->app.title;
        break;

    case DownloadType_Data:
        ret = workPage->app.pSource->GetDataURL(workPage->app, url);
        if(ret == 0 && sce_paf_strstr(url.c_str(), "tar.gz") != nullptr)
        {
            sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
            dialog::Close();
            
            dialog::ButtonCode bc;

            dialog::OpenTwoButton(g_appPlugin, nullptr, g_appPlugin->GetString(msg_download_tgz), IDParam("msg_ok").GetIDHash(), IDParam("msg_cancel_vb").GetIDHash(), DialogCB, &bc);
            dialog::WaitEnd();
            
            
            if(bc == dialog::ButtonCode_No)
                return;

            dialog::OpenPleaseWait(g_appPlugin, nullptr, g_appPlugin->GetString(msg_wait));
            sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        } 
        dlParam.type = BGDLTarget_CompressedFile;
        sce_paf_strncpy(dlParam.path, workPage->app.dataPath.c_str(), sizeof(dlParam.path));
        sce_paf_strncpy(dlParam.data_icon, workPage->app.iconPath.c_str(), sizeof(dlParam.data_icon));
        appName.SetFormattedString(g_appPlugin->GetString(data_dl_name), workPage->app.title.c_str());
        break; 
    }

    if(ret != SCE_PAF_OK)
    {
        print("[AppViewer::DownloadJob] Obtain URL FAIL! 0x%X\n", ret);
        dialog::Close();
        dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_download_error));
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        return;
    }
    
    ret = Downloader::GetCurrentInstance()->Enqueue(g_appPlugin, url.c_str(), appName.GetString().c_str(), workPage->app.iconPath.c_str(), &dlParam);

    dialog::Close();
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    
    if(ret != SCE_OK)
    {
        dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_download_error));
        print("[AppViewer::DownloadJob] Start download FAIL! 0x%X\n", ret);
        return;
    }

    dialog::OpenOk(g_appPlugin, nullptr, g_appPlugin->GetString(msg_download_queued));

    print("[AppViewer::DownloadJob] Run(END)\n");
}
