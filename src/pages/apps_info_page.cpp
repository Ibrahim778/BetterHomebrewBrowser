#include <kernel.h>
#include <paf.h>
#include <shellsvc.h>

#include "pages/apps_info_page.h"
#include "utils.h"
#include "print.h"
#include "common.h"
#include "settings.h"
#include "curl_file.h"
#include "cURLFile.h"
#include "dialog.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"

using namespace paf;
using namespace apps::info;
using namespace Utils;

Page::Page(db::entryInfo& entry):
    generic::Page::Page(
        app_info_page, 
        Plugin::PageOpenParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom), 
        Plugin::PageCloseParam(false, 200.0f, Plugin::PageEffectType_SlideFromLeft)
        ),
    info(entry),
    iconSurf(SCE_NULL),
    iconButton(SCE_NULL),
    iconLoadThread(SCE_NULL),
    descriptionLoadThread(SCE_NULL)
{
    Widget::SetLabel(Widget::GetChild(root, info_title_text), &info.title);
    Widget::SetLabel(Widget::GetChild(root, info_author_text), &info.author);
    Widget::SetLabel(Widget::GetChild(root, info_version_text), &info.version);
    
    Widget::GetChild(root, download_button)->RegisterEventCallback(ui::EventMain_Decide,  new button::Callback(this));
    Widget::GetChild(root, back_button)->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(generic::Page::DefaultBackButtonCB));
    
    iconButton = Widget::GetChild<ui::ImageButton>(root, icon_button);
    iconButton->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(IconInvokedDownloadJob::ButtonCB, this));

    auto dataButton = Widget::GetChild(root, data_download_button);
    dataButton->RegisterEventCallback(ui::EventMain_Decide, new button::Callback(this));

    if(info.dataURL.size() == 0)
       Widget::PlayEffectReverse(dataButton, 0, effect::EffectType_Reset);

    if(db::info[Settings::GetInstance()->source].ScreenshotsSupported && info.screenshotURL.size() > 0)
    {
        auto scrollBox = Widget::GetChild(root, screenshot_box);
        Plugin::TemplateOpenParam tOpen;
        rco::Element e;
        e.hash = info_screenshot_button_template;
        
        int size = info.screenshotURL.size();
        for(int i = 0; i < size; i++)
        {
            thread::s_mainThreadMutex.Lock();
            g_appPlugin->TemplateOpen(scrollBox, &e, &tOpen);
            thread::s_mainThreadMutex.Unlock();

            auto widget = scrollBox->GetChild(scrollBox->childNum - 1);
            auto cb = new screenshot::Callback();
            cb->pUserData = &info.screenshotURL;
            
            widget->elem.hash = i;
            widget->RegisterEventCallback(ui::EventMain_Decide, cb);
        }
    }
    else
    {
        thread::s_mainThreadMutex.Lock();
        effect::Play(0, Widget::GetChild(root, screenshot_plane), effect::EffectType_Reset, SCE_TRUE, SCE_TRUE);
        Widget::GetChild(root, description_plane)->SetSize(&paf::Vector4(960, 444));
        // Widget::GetChild<ui::Text>(root, 0)->SetFontSize;
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
    auto busyIndicator = Widget::GetChild<ui::BusyIndicator>(callingPage->root, icon_busy);
    auto iconButton = callingPage->iconButton;
    busyIndicator->Start();

    SceInt32 fileErr = SCE_OK;
    graph::Surface::Create(&callingPage->iconSurf, g_appPlugin->memoryPool, (SharedPtr<File> *)&LocalFile::Open(callingPage->info.iconPath.data(), SCE_O_RDONLY, 0, &fileErr));

    if(fileErr != SCE_OK || callingPage->iconSurf == SCE_NULL)
    {
        print("Error creating icon surf: 0x%X (0x%X)\n", fileErr, callingPage->iconSurf);
        iconButton->SetSurface(&g_brokenTex, 0);
    }
    else
    {
        callingPage->iconSurf->UnsafeRelease();
        iconButton->SetSurface(&callingPage->iconSurf, 0);
    }

    iconButton->SetColor(&paf::Rgba(1,1,1,1));

    busyIndicator->Stop();
    print("apps::info::Page::IconLoadThread FINISH\n");
    
    Cancel();
}

SceVoid button::DataDownloadTask::Run()
{
    print("DataDownloadTask START!\n");
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    Dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_wait));

    paf::string out;
    SceInt32 r = db::info[Settings::GetInstance()->source].GetDataUrl(caller->GetInfo(), out);
    if(r != SCE_OK)
    {
        Dialog::Close();
        Dialog::OpenOk(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_src_err));
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        return;
    }
    
    BGDLParam param;
    param.magic = (BHBB_DL_MAGIC | BHBB_DL_CFG_VER);
    sce_paf_strncpy(param.path, caller->GetInfo().dataPath.data(), sizeof(param.path));
    param.type = BGDLTarget::CustomPath;

    string titleTemplate;
    String::GetFromHash(data_dl_name, &titleTemplate);
    
    string title = ccc::Sprintf(titleTemplate.data(), caller->GetInfo().title.data());

    g_downloader->Enqueue(out.data(), title.data(), &param);

    Dialog::Close();
    Dialog::OpenOk(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_download_queued));
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    print("DataDownloadTask FINISH!\n");
}

SceVoid button::AppDownloadTask::Run()
{
    print("AppDownloadTask START!\n");
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    Dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_wait));

    paf::string out;
    SceInt32 r = db::info[Settings::GetInstance()->source].GetDownloadUrl(caller->GetInfo(), out);
    // print("URL Obtain result: 0x%X\n", r);
    if(r != SCE_OK)
    {
        Dialog::Close();
        Dialog::OpenOk(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_src_err));
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        return;
    }   
    
    BGDLParam param;
    sce_paf_memset(&param, 0, sizeof(param));
    param.magic = (BHBB_DL_MAGIC | BHBB_DL_CFG_VER);
    param.type = BGDLTarget::App;
    
    g_downloader->Enqueue(out.data(), caller->GetInfo().title.data(), &param);

    Dialog::Close();
    Dialog::OpenOk(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_download_queued));
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    print("AppDownloadTask FINISH!\n");
}

SceVoid button::Callback::OnGet(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    switch(self->elem.hash)
    {
    case download_button:
        g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new AppDownloadTask((apps::info::Page *)pUserData, "BHBB::AppDownloadTask")));
        break;

    case data_download_button:
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

screenshot::Page::Page(string &s):url(s),generic::Page(screenshot_page, Plugin::PageOpenParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom), Plugin::PageCloseParam(false, 200.0f, Plugin::PageEffectType_SlideFromBottom))
{
    busyIndicator = Widget::GetChild<ui::BusyIndicator>(root, busy);
    backButton = Widget::GetChild<ui::CornerButton>(root, back_button);
    backButton->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(generic::Page::DefaultBackButtonCB));

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

SceVoid screenshot::Page::ErrorCB(Dialog::ButtonCode button, ScePVoid pUserData)
{
    Page::DeleteCurrentPage();
}

SceVoid screenshot::Page::LoadThread::EntryFunction()
{
    print("apps::info::screenshot::Page::LoadThread START\n");
    callingPage->busyIndicator->Start();
    callingPage->backButton->PlayEffectReverse(0, effect::EffectType_Reset);

    auto plane = Widget::GetChild(callingPage->root, picture);
    
    SceInt32 fileErr = SCE_OK;

    print("%s\n", callingPage->url.data());

    graph::Surface::Create(&callingPage->surface, g_appPlugin->memoryPool, (SharedPtr<File> *)&CurlFile::Open(callingPage->url.data(), SCE_O_RDONLY, 0, &fileErr, true, true));
    print("surface created!\n");
    if(callingPage->surface == SCE_NULL || fileErr != SCE_OK)
    {
        print("Error making surf! fileErr: 0x%X callingPage->surface: 0x%X\n", fileErr, callingPage->surface);
        callingPage->busyIndicator->Stop();
        Dialog::OpenError(g_appPlugin, fileErr, String::GetPFromHash(msg_screenshot_error), ErrorCB);
        Cancel();
        return;
    }
    else
    {
        callingPage->surface->UnsafeRelease();
        plane->SetSurface(&callingPage->surface, 0);
        plane->SetColor(&paf::Rgba(1,1,1,1));
    }

    callingPage->busyIndicator->Stop();
    callingPage->backButton->PlayEffect(0, effect::EffectType_Reset);
    print("apps::info::screenshot::Page::LoadThread FINISH\n");
    Cancel();
}

SceVoid Page::DescriptionLoadThread::EntryFunction()
{
    auto busyIndicator = Widget::GetChild<ui::BusyIndicator>(callingPage->root, description_busy);

    busyIndicator->Start();
    
    string desc;
    SceInt32 ret = db::info[Settings::GetInstance()->source].GetDescription(callingPage->info, desc);
    auto descText = Widget::GetChild<ui::Text>(callingPage->root, info_description_text);
    if(ret == SCE_OK)
        Widget::SetLabel(descText, &desc);
    else
    {
        Dialog::OpenError(g_appPlugin, ret, String::GetPFromHash(msg_desc_error));
        descText->SetLabel(&wstring(String::GetPFromHash(msg_desc_error)));
    }

    busyIndicator->Stop();
    Cancel();
}

SceVoid Page::IconInvokedDownloadJob::DialogEventHandler(Dialog::ButtonCode resCode, ScePVoid userDat)
{
    Page::IconInvokedDownloadJob *job = (Page::IconInvokedDownloadJob *)userDat;

    job->DialogResult = (resCode == Dialog::ButtonCode::ButtonCode_Yes);
}

SceVoid Page::IconInvokedDownloadJob::ButtonCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new IconInvokedDownloadJob("IconInvokedDownloadJob", ((Page *)pUserData)->GetInfo().hash, (Page *)pUserData)));
}

SceVoid Page::IconInvokedDownloadJob::Run()
{
    auto &entry = callingPage->info;
    if(entry.iconURL.size() == 0)
    {
        Dialog::OpenOk(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_icon_no_source));    
        return;
    }

    if(LocalFile::Exists(entry.iconPath.data()))
    {
        Dialog::OpenYesNo(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_icon_redownload), DialogEventHandler, this);
        Dialog::WaitEnd();
        if(!DialogResult) return; //Return if the user said no
    }

    Dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, String::GetPFromHash(msg_downloading_icon));
    print("Opened pls wait\n");
    
    SceInt32 ret = SCE_OK;
    SceBool DownloadSuccess = SCE_FALSE; 

    int x = 0;
    for(paf::string& url : entry.iconURL)
    {
        x++;
        print("Trying source %d / %d\n", x, entry.iconURL.size());
        ret = cURLFile::SaveFile(url.data(), entry.iconPath.data());
        print("SaveFile ended!\n");
        if(ret != SCE_OK)
        {
            print("[Error] Download Icon %s -> %s failed => 0x%X\n", url.data(), entry.iconPath.data(), ret);
            continue;    
        }

        graph::Surface *surf = SCE_NULL;
        //Surface loading
        {
            SceInt32 ret = SCE_OK;
            SharedPtr<LocalFile> file = LocalFile::Open(entry.iconPath.data(), SCE_O_RDONLY, 0, &ret);
            if(ret != SCE_OK)
            {
                print("\t[Error] Open %s failed -> 0x%X\n", entry.iconPath.data(), ret);
                goto ASSIGN_TEX;
            }
            graph::Surface::Create(&surf, g_appPlugin->memoryPool, (SharedPtr<File> *)&file);

            file.get()->Close();    
        }


    ASSIGN_TEX:
        //Assigning the appropriate surface to the widget
        {
            ui::Widget *widget = Widget::GetChild(callingPage->root, icon_button);
            if(widget == SCE_NULL)
            {
                print("\t[Skip] Widget 0x%X not found\n", hash);
                break;
            }
            thread::s_mainThreadMutex.Lock();
            if(surf == SCE_NULL)
            {
                print("[Error] Surface creation failed\n");
                if(x < entry.iconURL.size()) continue;
                widget->SetSurface(&g_brokenTex, 0);
                thread::s_mainThreadMutex.Unlock();
                break;
            }
            widget->SetSurface(&surf, 0);
            surf->Release();
            thread::s_mainThreadMutex.Unlock();
        } 

        DownloadSuccess = SCE_TRUE;
        break;
    } 
    
    Dialog::Close();
    print("Closed\n");
    print("DL Success: %s\n", DownloadSuccess ? "True" : "False");
    if(!DownloadSuccess)
        Dialog::OpenError(g_appPlugin, ret, String::GetPFromHash(msg_icon_error));    
}