#include <kernel.h>
#include <paf.h>
#include <shellsvc.h>

#include "pages/apps_info_page.h"
#include "utils.h"
#include "print.h"
#include "common.h"
#include "settings.h"
#include "curl_file.h"

using namespace paf;
using namespace apps::info;

Page::Page(db::entryInfo& entry):generic::Page::Page("info_page_template"),info(entry),iconSurf(SCE_NULL),iconLoadThread(SCE_NULL),descriptionLoadThread(SCE_NULL)
{
    Utils::SetWidgetLabel(Utils::GetChildByHash(root, Utils::GetHashById("info_title_text")), &info.title);
    Utils::SetWidgetLabel(Utils::GetChildByHash(root, Utils::GetHashById("info_author_text")), &info.author);
    Utils::SetWidgetLabel(Utils::GetChildByHash(root, Utils::GetHashById("info_version_text")), &info.version);
    
    Utils::GetChildByHash(root, button::ButtonHash_Download)->RegisterEventCallback(ui::EventMain_Decide,  new button::Callback(this), SCE_FALSE);
    auto dataButton = Utils::GetChildByHash(root, button::ButtonHash_DataDownload);
    dataButton->RegisterEventCallback(ui::EventMain_Decide,  new button::Callback(this), SCE_FALSE);

    if(info.dataURL.size() == 0)
       Utils::PlayEffectReverse(dataButton, 0, effect::EffectType_Reset);

    if(db::info[Settings::GetInstance()->source].ScreenshotsSupported & info.screenshotURL.size() > 0)
    {
        auto scrollBox = Utils::GetChildByHash(root, Utils::GetHashById("screenshot_box"));
        Plugin::TemplateOpenParam tOpen;
        rco::Element e;
        e.hash = Utils::GetHashById("info_screenshot_button_template");
        
        int size = info.screenshotURL.size();
        for(int i = 0; i < size; i++)
        {
            thread::s_mainThreadMutex.Lock();
            mainPlugin->TemplateOpen(scrollBox, &e, &tOpen);
            thread::s_mainThreadMutex.Unlock();

            auto widget = scrollBox->GetChild(scrollBox->childNum - 1);
            auto cb = new screenshot::Callback();
            cb->pUserData = &info.screenshotURL;
            
            widget->elem.hash = i;
            widget->RegisterEventCallback(ui::EventMain_Decide, cb, SCE_FALSE);
        }
    }
    else
    {
        thread::s_mainThreadMutex.Lock();
        effect::Play(0, Utils::GetChildByHash(root, Utils::GetHashById("screenshot_plane")), effect::EffectType_Reset, SCE_TRUE, SCE_TRUE);
        Utils::GetChildByHash(root, Utils::GetHashById("description_plane"))->SetSize(&paf::Vector4(960, 444));
        thread::s_mainThreadMutex.Unlock();
    }

    iconLoadThread = new IconLoadThread(this);
    iconLoadThread->Start();

    descriptionLoadThread = new DescriptionLoadThread(this);
    descriptionLoadThread->Start();
}

Page::~Page()
{
    if(iconLoadThread)
    {
        iconLoadThread->Cancel();
        thread::s_mainThreadMutex.Unlock();
        iconLoadThread->Join();
        thread::s_mainThreadMutex.Lock();
        delete iconLoadThread;

        iconLoadThread = SCE_NULL;
    }

    if(descriptionLoadThread)
    {
        descriptionLoadThread->Cancel();
        thread::s_mainThreadMutex.Unlock();
        descriptionLoadThread->Join();
        thread::s_mainThreadMutex.Lock();
        delete descriptionLoadThread;
        
        descriptionLoadThread = SCE_NULL;
    }
}

db::entryInfo& Page::GetInfo()
{
    return info;
}

SceVoid Page::IconLoadThread::EntryFunction()
{
    print("apps::info::Page::IconLoadThread START\n");
    auto busyIndicator = Utils::GetChildByHash<ui::BusyIndicator>(callingPage->root, Utils::GetHashById("icon_busy"));
    auto iconPlane = Utils::GetChildByHash<ui::Plane>(callingPage->root, Utils::GetHashById("icon_plane"));
    busyIndicator->Start();

    SceInt32 fileErr = SCE_OK;
    graph::Surface::Create(&callingPage->iconSurf, mainPlugin->memoryPool, (SharedPtr<File> *)&LocalFile::Open(callingPage->info.iconPath.data(), SCE_O_RDONLY, 0, &fileErr));

    if(fileErr != SCE_OK || callingPage->iconSurf == SCE_NULL)
    {
        print("Error creating icon surf: 0x%X (0x%X)\n", fileErr, callingPage->iconSurf);
        iconPlane->SetSurface(&BrokenTex, 0);
    }
    else
    {
        callingPage->iconSurf->UnsafeRelease();
        iconPlane->SetSurface(&callingPage->iconSurf, 0);
    }

    iconPlane->SetColor(&paf::Rgba(1,1,1,1));

    busyIndicator->Stop();
    print("apps::info::Page::IconLoadThread FINISH\n");
    
    Cancel();
}

SceVoid apps::info::button::DataDownloadTask::Run()
{
    print("DataDownloadTask START!\n");
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));

    paf::string out;
    SceInt32 r = db::info[Settings::GetInstance()->source].GetDataUrl(caller->GetInfo(), out);
    if(r != SCE_OK)
    {
        Dialog::Close();
        Dialog::OpenOk(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_src_err"));
        return;
    }   
    
    BGDLParam param;
    param.magic = (BHBB_DL_MAGIC | BHBB_DL_CFG_VER);
    sce_paf_strncpy(param.path, caller->GetInfo().dataPath.data(), sizeof(param.path));
    param.type = BGDLTarget::CustomPath;

    string titleTemplate;
    Utils::GetStringFromID("data_dl_name", &titleTemplate);
    
    string title = ccc::Sprintf(titleTemplate.data(), caller->GetInfo().title.data());

    g_downloader->Enqueue(out.data(), title.data(), &param);

    Dialog::Close();
    Dialog::OpenOk(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_download_queued"));
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    print("DataDownloadTask FINISH!\n");
}

SceVoid button::AppDownloadTask::Run()
{
    print("AppDownloadTask START!\n");
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));

    paf::string out;
    SceInt32 r = db::info[Settings::GetInstance()->source].GetDownloadUrl(caller->GetInfo(), out);
    if(r != SCE_OK)
    {
        Dialog::Close();
        Dialog::OpenOk(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_src_err"));
        return;
    }   
    
    BGDLParam param;
    sce_paf_memset(&param, 0, sizeof(param));
    param.magic = (BHBB_DL_MAGIC | BHBB_DL_CFG_VER);
    param.type = BGDLTarget::App;
    
    g_downloader->Enqueue(out.data(), caller->GetInfo().title.data(), &param);

    Dialog::Close();
    Dialog::OpenOk(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_download_queued"));
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    print("AppDownloadTask FINISH!\n");
}

SceVoid button::Callback::OnGet(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    switch(self->elem.hash)
    {
    case ButtonHash_Download:
        g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new AppDownloadTask((apps::info::Page *)pUserData, "BHBB::AppDownloadTask")));
        break;

    case ButtonHash_DataDownload:
        g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new DataDownloadTask((apps::info::Page *)pUserData, "BHBB::DataDownloadTask")));
        break;

    default:
        print("Unknown hash! 0x%X\n", self->elem.hash);
        break;
    }
}

SceVoid screenshot::Callback::OnGet(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    std::vector<paf::string> *urls = (std::vector<paf::string> *)pUserData;
    new screenshot::Page(urls->at(self->elem.hash));
}

screenshot::Page::Page(string &s):url(s),generic::Page("picture_page_template")
{
    loadThread = new LoadThread(this);
    loadThread->Start();
}

screenshot::Page::~Page()
{
    if(loadThread)
    {
        loadThread->Cancel();
        thread::s_mainThreadMutex.Unlock();
        loadThread->Join();
        thread::s_mainThreadMutex.Lock();
        delete loadThread;
        
        loadThread = SCE_NULL;
    }
}

SceVoid screenshot::Page::LoadThread::EntryFunction()
{
    print("apps::info::screenshot::Page::LoadThread START\n");
    g_busyIndicator->Start();
    g_backButton->PlayEffectReverse(0, effect::EffectType_Reset);

    auto plane = Utils::GetChildByHash(callingPage->root, Utils::GetHashById("picture"));
    
    SceInt32 fileErr = SCE_OK;

    print("%s\n", callingPage->url.data());

    graph::Surface::Create(&callingPage->surface, mainPlugin->memoryPool, (SharedPtr<File> *)&CurlFile::Open(callingPage->url.data(), SCE_O_RDONLY, 0, &fileErr, true, true));
    print("surface created!\n");
    if(callingPage->surface == SCE_NULL || fileErr != SCE_OK)
    {
        print("Error making surf! fileErr: 0x%X callingPage->surface: 0x%X\n", fileErr, callingPage->surface);
        Dialog::OpenOk(mainPlugin, NULL, Utils::GetStringPFromID("msg_download_queued"));
        Page::DeleteCurrentPage();
        Cancel();
        return;
    }
    else
    {
        callingPage->surface->UnsafeRelease();
        plane->SetSurface(&callingPage->surface, 0);
        plane->SetColor(&paf::Rgba(1,1,1,1));
    }

    g_busyIndicator->Stop();
    g_backButton->PlayEffect(0, effect::EffectType_Reset);
    print("apps::info::screenshot::Page::LoadThread FINISH\n");
    Cancel();
}

SceVoid Page::DescriptionLoadThread::EntryFunction()
{
    auto busy = Utils::GetChildByHash<ui::BusyIndicator>(callingPage->root, Utils::GetHashById("description_busy"));

    busy->Start();
    
    string desc;
    SceInt32 ret = db::info[Settings::GetInstance()->source].GetDescription(callingPage->info, desc);
    auto descText = Utils::GetChildByHash<ui::Text>(callingPage->root, Utils::GetHashById("info_description_text"));
    if(ret == SCE_OK)
        Utils::SetWidgetLabel(descText, &desc);    
    else
        descText->SetLabel(&wstring(Utils::GetStringPFromID("msg_desc_error")));
    busy->Stop();
    Cancel();
}