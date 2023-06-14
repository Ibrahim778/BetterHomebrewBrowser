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
    
    if(!isMainThread)
        RMutex::main_thread_mutex.Unlock();

    new AsyncDescriptionLoader(app, root->FindChild(info_description_text));
}   

AppViewer::~AppViewer()
{

}

AppViewer::AsyncDescriptionLoader::AsyncDescriptionLoader(Source::Entry& entry, paf::ui::Widget *target, bool autoLoad)
{
    item = new Job(entry);
    item->target = target;
    item->workObj = this;

	target->AddEventListener(0x10000000, new TargetDeleteEventCallback(this));

    if(autoLoad)
        Load();
}

AppViewer::AsyncDescriptionLoader::~AsyncDescriptionLoader()
{
    Abort();
}

void AppViewer::AsyncDescriptionLoader::Load()
{
    SharedPtr<job::JobItem> itemParam(item);
    job::JobQueue::default_queue->Enqueue(itemParam);
}

void AppViewer::AsyncDescriptionLoader::Abort()
{
	if (item)
	{
		item->workObj = nullptr;
		item->Cancel();
		item = nullptr;
	}
}

void AppViewer::AsyncDescriptionLoader::Job::Run()
{
    int res = 0;
    
    paf::wstring desc;
    res = entry.pSource->GetDescription(entry, desc);
    
    if(IsCanceled())
        return;

    if(res < 0)
    {
        dialog::OpenError(g_appPlugin, res, g_appPlugin->GetString(msg_desc_error));
        target->SetString(g_appPlugin->GetString(msg_desc_error));
    }
    else 
    {
        if(entry.screenshotURL.size() == 0)
            desc += L"\n\n\n";
            
        target->SetString(desc);
    }
}

void AppViewer::AsyncDescriptionLoader::Job::Finish()
{
    if(workObj)
        workObj->item = nullptr;
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

void AppViewer::DownloadJob::Run()
{
    print("[AppViewer::DownloadJob] Run(START)\n");
    
    paf::string url;
    paf::string appName;
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
        sce_paf_strncpy(dlParam.fallback_icon, workPage->app.iconPath.c_str(), sizeof(dlParam.fallback_icon));
        break;

    case DownloadType_Data:
        ret = workPage->app.pSource->GetDataURL(workPage->app, url);
        dlParam.type = BGDLTarget_Zip;
        sce_paf_strncpy(dlParam.path, workPage->app.dataPath.c_str(), sizeof(dlParam.path));
        sce_paf_strncpy(dlParam.fallback_icon, workPage->app.iconPath.c_str(), sizeof(dlParam.fallback_icon));
        break; 
    }

    if(ret != SCE_PAF_OK)
    {
        print("[AppViewer::DownloadJob] Obtain URL FAIL! 0x%X\n", ret);
        dialog::Close();
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        return;
    }

    common::Utf16ToUtf8(workPage->app.title, &appName);
    
    ret = Downloader::GetCurrentInstance()->Enqueue(g_appPlugin, url.c_str(), appName.c_str(), workPage->app.iconPath.c_str(), &dlParam);

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
