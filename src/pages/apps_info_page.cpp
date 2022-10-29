#include <kernel.h>
#include <paf.h>

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
    
    Utils::GetChildByHash(root, button::ButtonHash_Download)->RegisterEventCallback(ui::EventMain_Decide,  new button::Callback(), SCE_FALSE);
    auto dataButton = Utils::GetChildByHash(root, button::ButtonHash_DataDownload);
    dataButton->RegisterEventCallback(ui::EventMain_Decide,  new button::Callback(), SCE_FALSE);

    if(info.dataURL.size() == 0)
       Utils::PlayEffectReverse(dataButton, 0, effect::EffectType_Reset);

    if(db::info[Settings::GetInstance()->source].ScreenshotsSuppourted & info.screenshotURL.size() > 0)
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
        effect::Play(0, Utils::GetChildByHash(root, Utils::GetHashById("screenshot_plane")), effect::EffectType_Reset, SCE_TRUE, SCE_TRUE);
        Utils::GetChildByHash(root, Utils::GetHashById("description_plane"))->SetSize(&paf::Vector4(960, 444));
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

    if(iconSurf)
    {
        Utils::GetChildByHash<ui::Plane>(root, Utils::GetHashById("icon_plane"))->SetSurface(&TransparentTex, 0);
        Utils::DeleteTexture(&iconSurf);
    }
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
        iconPlane->SetSurface(&BrokenTex, 0);
    else
        iconPlane->SetSurface(&callingPage->iconSurf, 0);
    
    iconPlane->SetColor(&paf::Rgba(1,1,1,1));

    busyIndicator->Stop();
    print("apps::info::Page::IconLoadThread FINISH\n");
    sceKernelExitDeleteThread(0);
}

SceVoid button::Callback::OnGet(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    switch(self->elem.hash)
    {
    case ButtonHash_Download:
        break;

    case ButtonHash_DataDownload:
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

    if(surface)
    {
        effect::Play(0, Utils::GetChildByHash(root, Utils::GetHashById("picture")), effect::EffectType_Reset, SCE_TRUE, SCE_TRUE);
        Utils::DeleteTexture(&surface);
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
    }
    else
    {
        plane->SetSurface(&callingPage->surface, 0);
        plane->SetColor(&paf::Rgba(1,1,1,1));
    }

    g_busyIndicator->Stop();
    g_backButton->PlayEffect(0, effect::EffectType_Reset);
    print("apps::info::screenshot::Page::LoadThread FINISH\n");
    sceKernelExitDeleteThread(0);
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
    sceKernelExitDeleteThread(0);
}