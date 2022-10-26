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

Page::Page(db::entryInfo& entry):generic::Page::Page("info_page_template"),info(entry),iconSurf(SCE_NULL),iconLoadThread(SCE_NULL),screenshotLoadThread(SCE_NULL)
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
        screenshotLoadThread = new ScreenshotLoadThread(this);
        screenshotLoadThread->Start();
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

apps::info::Page::~Page()
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

    if(screenshotLoadThread)
    {
        screenshotLoadThread->Cancel();
        thread::s_mainThreadMutex.Unlock();
        screenshotLoadThread->Join();
        thread::s_mainThreadMutex.Lock();
        delete screenshotLoadThread;
        
        screenshotLoadThread = SCE_NULL;
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

    effect::Play(0, Utils::GetChildByHash(root, Utils::GetHashById("screenshot_box")), effect::EffectType_Reset, SCE_TRUE, SCE_TRUE); //Just delete the screenshot buttons
    for(auto i = screenshotSurfs.begin(); i != screenshotSurfs.end(); i++)
    {
        graph::Surface *surf = *i;

        Utils::DeleteTexture(&surf);
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

SceVoid Page::ScreenshotLoadThread::EntryFunction()
{
    print("apps::info::Page::ScreenshotLoadThread START\n");
    
    auto scrollBox = Utils::GetChildByHash(callingPage->root, Utils::GetHashById("screenshot_box"));
    
    Plugin::TemplateOpenParam tOpen;
    rco::Element e;
    e.hash = Utils::GetHashById("info_screenshot_button_template");
    
    int x = 0;
    auto end = callingPage->info.thumbnailURL.end();
    for(auto i = callingPage->info.thumbnailURL.begin(); i != end && !IsCanceled(); i++, x++)
    {
        graph::Surface *currentSurf;
        SceInt32 fileErr = SCE_OK;
        
        SharedPtr<HttpFile> img = HttpFile::Open(i->data(), &fileErr, SCE_O_RDONLY);
        if(fileErr != SCE_OK)
        {
            print("Error opening %s -> 0x%X\n", i->data(), fileErr);
            continue;
        }
        SceSize imgSize = img.get()->GetFileSize();
        
        const ScePVoid memBuff = sce_paf_malloc(imgSize);
        sce_paf_memset(memBuff, 0, imgSize);
        
        SceUInt32 offset = 0;
        SceInt32 bytesRead = 0;
        do
        {
            bytesRead = img.get()->Read(memBuff + offset, 0x100);
            if(bytesRead < 0)
                print("Error reading file: 0x%X\n", bytesRead);
            else offset += bytesRead;
        } while(bytesRead > 0 && !IsCanceled());
        
        if(IsCanceled())
        {
            sce_paf_free(memBuff);
            break;
        }

        print("Read 0x%X / 0x%X bytes\n", offset, imgSize);
        
        graph::Surface::Create(&currentSurf, mainPlugin->memoryPool, memBuff, 0, imgSize);
        sce_paf_free(memBuff);

        if(currentSurf == SCE_NULL)
        {
            print("Error making surf! currentSurf: 0x%X\n", currentSurf);
            continue;
        }

        callingPage->screenshotSurfs.push_back(currentSurf);

        thread::s_mainThreadMutex.Lock();
        mainPlugin->TemplateOpen(scrollBox, &e, &tOpen);
        thread::s_mainThreadMutex.Unlock();

        auto widget = scrollBox->GetChild(scrollBox->childNum - 1);
        auto cb = new screenshot::Callback();
        cb->pUserData = &callingPage->info.screenshotURL;
        print("Assigning surf: %p\n", currentSurf);

        widget->elem.hash = x;
        widget->SetSurface(&currentSurf, 0);
        widget->RegisterEventCallback(ui::EventMain_Decide, cb, SCE_FALSE);

    }
    
    print("apps::info::Page::ScreenshotLoadThread FINISH\n");
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
        Utils::DeleteTexture(&surface);
}

SceVoid screenshot::Page::LoadThread::EntryFunction()
{
    print("apps::info::screenshot::Page::LoadThread START\n");
    g_busyIndicator->Start();
    g_backButton->PlayEffectReverse(0, effect::EffectType_Reset);

    auto plane = Utils::GetChildByHash(callingPage->root, Utils::GetHashById("picture"));
    
    SceInt32 fileErr = SCE_OK;

    print("%s\n", callingPage->url.data());

    graph::Surface::Create(&callingPage->surface, mainPlugin->memoryPool, (SharedPtr<File> *)&CurlFile::Open(callingPage->url.data(), SCE_O_RDONLY, 0, &fileErr));
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
    {
        print("%s\n", desc.data());
        Utils::SetWidgetLabel(descText, &desc);
    }
    else
    {
        string msgTemplate;
        Utils::GetfStringFromID("msg_net_fix", &msgTemplate);

        string errorMsg = ccc::Sprintf(msgTemplate.data(), ret);
        
        descText->SetLabel(&wstring(Utils::GetStringPFromID("msg_desc_error")));


    }
    busy->Stop();
    sceKernelExitDeleteThread(0);
}