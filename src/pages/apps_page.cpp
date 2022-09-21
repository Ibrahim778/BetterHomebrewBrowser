#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <shellsvc.h>

#include "pages/apps_page.h"
#include "pages/text_page.h"
#include "print.h"
#include "utils.h"
#include "settings.h"
#include "common.h"
#include "dialog.h"

using namespace paf;

apps::Page::Page():generic::Page::Page("home_page_template"),listRootPlane(SCE_NULL),currBody(SCE_NULL),iconDownloadThread(SCE_NULL)
{
    ui::EventCallback *categoryCallback = new ui::EventCallback;
    categoryCallback->pUserData = this;
    categoryCallback->eventHandler = Page::CategoryButtonCB;

    Utils::GetChildByHash(root, Utils::GetHashById("game_button"))->RegisterEventCallback(ui::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("all_button"))->RegisterEventCallback(ui::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("emu_button"))->RegisterEventCallback(ui::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("port_button"))->RegisterEventCallback(ui::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("util_button"))->RegisterEventCallback(ui::EventMain_Decide, categoryCallback, 0);

    SetCategory(-1);

    ui::EventCallback *searchCallback = new ui::EventCallback();
    searchCallback->pUserData = this;
    searchCallback->eventHandler = Page::SearchCB;

    Utils::GetChildByHash(root, Utils::GetHashById("search_button"))->RegisterEventCallback(ui::EventMain_Decide, searchCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("search_enter_button"))->RegisterEventCallback(ui::EventMain_Decide, searchCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("search_back_button"))->RegisterEventCallback(ui::EventMain_Decide, searchCallback, 0);

    SetMode(PageMode_Browse);

    auto optionsButton = Utils::GetChildByHash(root, Utils::GetHashById("options_button"));
    auto optionsCallback = new ui::EventCallback();
    optionsCallback->eventHandler = Settings::OpenCB;

    optionsButton->RegisterEventCallback(ui::EventMain_Decide, optionsCallback, 0);

    listRootPlane = (ui::Plane *)Utils::GetChildByHash(root, Utils::GetHashById("plane_list_root"));
    CreateListWrapper();

    job::JobQueue::Option opt;
    opt.workerNum = 1;
    opt.workerOpt = NULL;
    opt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    opt.workerStackSize = SCE_KERNEL_16KiB;
    loadQueue = new job::JobQueue("BHBB::apps::Page:loadQueue", &opt);


    job::JobQueue::Option iconAssignOpt;
    opt.workerNum = 10;
    opt.workerOpt = NULL;
    opt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER - 10;
    opt.workerStackSize = SCE_KERNEL_16KiB;
    iconAssignQueue = new job::JobQueue("BHBB::apps::Page::iconAssignQueue", &iconAssignOpt);

    job::JobQueue::Option iconDownloadOpt;
    opt.workerNum = 10;
    opt.workerOpt = NULL;
    opt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER - 20;
    opt.workerStackSize = SCE_KERNEL_16KiB;
    iconDownloadQueue = new job::JobQueue("BHBB::apps::Page::iconDownloadQueue", &iconDownloadOpt);


    string iconEventFlagName = ccc::Sprintf("BHBB_IconEventFlag_0x%X", this); //Just to make it unique. Not ideal but works?
    iconFlags = sceKernelCreateEventFlag(iconEventFlagName.data(), SCE_KERNEL_ATTR_MULTI, 0, SCE_NULL);
    print("apps::Page::iconFlags = 0x%X\n", iconFlags);

    print("On Init done: %d\n", listRootPlane->childNum);
}

apps::Page::~Page()
{
    
}

SceVoid apps::Page::CategoryButtonCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(!db::info[Settings::GetInstance()->source].CategoriesSuppourted) return;
    apps::Page *page = (Page *)pUserData;
    int targetCategory = 0;
    switch (self->hash)
    {
    case Hash_All:
        targetCategory = -1;
        break;
    case Hash_Util:
        targetCategory = db::Category::UTIL;
        break;
    case Hash_Game:
        targetCategory = db::Category::GAME;
        break;
    case Hash_Emu:
        targetCategory = db::Category::EMULATOR;
        break;
    case Hash_Port:
        targetCategory = db::Category::PORT;
        break;
    }

    if(page->SetCategory(targetCategory)) // returns true if there's any change
        page->Redisplay();
}

SceVoid apps::Page::RedisplayJob::Run()
{
    callingPage->RedisplayInternal();
}

SceVoid apps::Page::Redisplay()
{
    auto jobPtr = SharedPtr<job::JobItem>(new RedisplayJob("BHBB::apps::Page::RedisplayJob", this));
    loadQueue->Enqueue(&jobPtr);
}

SceVoid apps::Page::RedisplayInternal()
{
    print("Redisplay!\n");

    sceKernelClearEventFlag(iconFlags, ~FLAG_ICON_ASSIGN_TEXTURE);
    ClearPages();
    CreatePopulatedPage();
    sceKernelSetEventFlag(iconFlags, FLAG_ICON_ASSIGN_TEXTURE);
    HandleForwardButton();

    sceKernelDelayThread(100); //Just to stabalise a little
}

SceVoid apps::Page::CreateListWrapper()
{
    listWrapperPlane = (ui::Plane *)Utils::CreateWidget("plane_list_wrapper", "plane", "_common_style_plane_transparent", listRootPlane);
    Utils::SetWidgetSize(listWrapperPlane, 960, 544);
    Utils::SetWidgetPosition(listWrapperPlane, 0, 0);
}

SceBool apps::Page::SetCategory(int _category)
{
    if(_category == category) return SCE_FALSE;
    category = _category;

    Rgba transparent, normal;
    transparent.r = 1;
    transparent.g = 1;
    transparent.b = 1;
    transparent.a = .4f;

    normal.r = 1;
    normal.g = 1;
    normal.b = 1;
    normal.a = 1;

    ui::Widget *allButton = Utils::GetChildByHash(root, Utils::GetHashById("all_button"));
    ui::Widget *gamesButton = Utils::GetChildByHash(root, Utils::GetHashById("game_button"));
    ui::Widget *emuButton = Utils::GetChildByHash(root, Utils::GetHashById("emu_button"));
    ui::Widget *portsButton = Utils::GetChildByHash(root, Utils::GetHashById("port_button"));
    ui::Widget *utilButton = Utils::GetChildByHash(root, Utils::GetHashById("util_button"));
    
    allButton->SetColor(&transparent);
    gamesButton->SetColor(&transparent);
    emuButton->SetColor(&transparent);
    portsButton->SetColor(&transparent);
    utilButton->SetColor(&transparent);

    switch(category)
    {
    case -1:
        allButton->SetColor(&normal);
        break;
    case db::Category::EMULATOR:
        emuButton->SetColor(&normal);
        break;
    case db::Category::GAME:
        gamesButton->SetColor(&normal);
        break;
    case db::Category::PORT:
        portsButton->SetColor(&normal);
        break;
    case db::Category::UTIL:
        utilButton->SetColor(&normal);
        break;
    }

    return SCE_TRUE;
}

SceVoid apps::Page::SetMode(PageMode targetMode)
{
    if(mode == targetMode) return;

    ui::Widget *categoriesPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_categories"));
    ui::Widget *searchPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_search"));
    ui::Widget *searchBox = Utils::GetChildByHash(root, Utils::GetHashById("search_box"));

    switch(targetMode)
    {
    case PageMode_Browse:
        searchPlane->PlayEffectReverse(0, effect::EffectType_Fadein1);
        categoriesPlane->PlayEffect(0, effect::EffectType_Fadein1);
        break;
    
    case PageMode_Search:
        categoriesPlane->PlayEffectReverse(0, effect::EffectType_Fadein1);
        searchPlane->PlayEffect(0, effect::EffectType_Fadein1);
        break;
    }

    mode = targetMode;
}

SceVoid apps::Page::SearchCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    Page *page = (Page *)pUserData;
    switch(self->hash)
    {
    case Hash_SearchButton:
        page->SetMode(PageMode_Search);
        break;

    case Hash_SearchBackButton:
        page->SetMode(PageMode_Browse);
        break;

    case Hash_SearchEnterButton:
        break;
    }
}

SceVoid apps::Page::IconZipJob::Run()
{
    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));

    BGDLParam param;
    sce_paf_memset(&param, 0, sizeof(param));
    param.magic = (BHBB_DL_CFG_VER | BHBB_DL_MAGIC);
    param.type = 0;
    sce_paf_strncpy(param.path, db::info[Settings::GetInstance()->source].iconFolderPath, sizeof(param.path));

    string titleTemplate;
    Utils::GetStringFromID("icons_dl_name", &titleTemplate);
    
    string title = ccc::Sprintf(titleTemplate.data(), db::info[Settings::GetInstance()->source].name);
    
    g_downloader->Enqueue(db::info[Settings::GetInstance()->source].iconsURL, title.data(), &param);
    Dialog::Close();

    Dialog::OpenOk(mainPlugin, NULL, Utils::GetStringPFromID("msg_download_queued"));
}

SceVoid apps::Page::IconZipJob::Finish()
{

}

SceVoid apps::Page::IconDownloadDecideCB(Dialog::ButtonCode buttonResult, ScePVoid userDat)
{
    if(buttonResult == Dialog::ButtonCode::ButtonCode_Yes)
    {
        SharedPtr<job::JobItem> ptr = paf::SharedPtr<job::JobItem>(new IconZipJob("BHBB::IconZipJob"));
        job::s_defaultJobQueue->Enqueue(&ptr);
    }
}

SceVoid apps::Page::ErrorRetryCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    Page::DeleteCurrentPage();
    ((Page *)pUserData)->Load();
}

SceUInt32 apps::Page::GetPageCount()
{
    SceUInt32 num = 0;
    Body *b = currBody; 
    while(b != SCE_NULL)
    {
        b = b->prev;
        num++;
    }

    return num;
}

SceVoid apps::Page::ClearPages()
{
    while(currBody)
    {
        apps::Page::Body *prev = currBody->prev;
        delete currBody;
        currBody = prev;
    }

    effect::Play(-100, listWrapperPlane, effect::EffectType_3D_SlideFromFront, SCE_TRUE, SCE_FALSE);
    CreateListWrapper();

    generic::Page::ResetBackButton();
}

SceVoid apps::Page::CancelIconJobs()
{
    sceKernelClearEventFlag(iconFlags, 0);
    print("----------------------------- CANCELLING ICON JOBS! -----------------------------\n");
    iconAssignQueue->Finish();
    //iconAssignQueue->WaitEmpty(); // Not really needed, handled by Finish()

    iconDownloadQueue->Finish();
    print("------------------------------ CANCELLED ICON JOBS ------------------------------\n");

}

SceVoid apps::Page::Load()
{
    auto jobPtr = SharedPtr<job::JobItem>(new LoadJob("BHBB::apps::Page::LoadJob", this));
    loadQueue->Enqueue(&jobPtr);
}

SceVoid apps::Page::ForwardButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    ((Page *)pUserData)->CreatePopulatedPage();
    ((Page *)pUserData)->HandleForwardButton();
}

SceVoid apps::Page::HandleForwardButton()
{
    if(appList.GetSize(category) <= (GetPageCount() * Settings::GetInstance()->nLoad))
    {
        generic::Page::SetForwardButtonEvent(SCE_NULL, SCE_NULL);
        g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);
    }
    else
    {
        generic::Page::SetForwardButtonEvent(ForwardButtonCB, this);
        g_forwardButton->PlayEffect(0, effect::EffectType_Reset);
    }
}

SceVoid apps::Page::OnRedisplay()
{
    HandleForwardButton();
}

SceVoid apps::Page::CreatePopulatedPage()
{
    SceUInt32 pageCountBeforeCreation = GetPageCount();

    NewPage([=](ui::Widget *ScrollBox) {

        ScrollBox->SetAlpha(0);

        rco::Element e = Utils::GetParamWithHashFromId("homebrew_button");
        Plugin::TemplateInitParam tInit;

        SceUInt32 loadNum = (appList.GetSize(category) - (pageCountBeforeCreation * Settings::GetInstance()->nLoad)) < Settings::GetInstance()->nLoad ? (appList.GetSize(category) - (pageCountBeforeCreation * Settings::GetInstance()->nLoad)) : Settings::GetInstance()->nLoad;
        db::entryInfo *info = appList.Get(pageCountBeforeCreation * Settings::GetInstance()->nLoad, category);
        
        for(int i = 0; i < Settings::GetInstance()->nLoad && appList.IsValidEntry(info) && i < loadNum; i++, info++)
        {
            if(info->type != category && category != -1)
            {
                i--;
                continue;
            }

            mainPlugin->TemplateOpen(ScrollBox, &e, &tInit);

            ui::Widget *createdWidget = ScrollBox->GetChild(ScrollBox->childNum - 1);
            createdWidget->hash = info->hash;
            Utils::SetWidgetLabel(createdWidget, &info->title);

            if(info->tex != SCE_NULL)
            {
                createdWidget->SetSurfaceBase(&info->tex); 
            }
            else 
            {
                auto jobPtr = SharedPtr<job::JobItem>(
                    new IconAssignJob("BHBB::apps::Page::IconAssignJob", 
                                this, 
                                IconAssignJob::Param(
                                    info->hash, 
                                    &info->tex, 
                                    info->iconLocal)));
                        
                iconAssignQueue->Enqueue(&jobPtr);

            }
        } 

        ScrollBox->SetAlpha(1);
    });
}

SceVoid apps::Page::LoadJob::Run()
{
    HttpFile            http;
    HttpFile::OpenArg   openArg;

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));
    Settings::GetInstance()->Close();
    callingPage->CancelIconDownloads();
    callingPage->CancelIconJobs();
    callingPage->ClearPages();
    g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);

    openArg.SetUrl(db::info[Settings::GetInstance()->source].indexURL);
    openArg.SetOpt(4000000, HttpFile::OpenArg::Opt_ResolveTimeOut);
	openArg.SetOpt(10000000, HttpFile::OpenArg::Opt_ConnectTimeOut);

    SceInt32 ret = http.Open(&openArg);
    if(ret != SCE_OK)
    {
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

        Dialog::Close();
        Dialog::WaitEnd();

        string msgTemplate;
        Utils::GetfStringFromID("msg_net_fix", &msgTemplate);

        string errorMsg = ccc::Sprintf(msgTemplate.data(), ret);
        
        new text::Page(errorMsg.data());
        
        Dialog::OpenError(mainPlugin, ret, Utils::GetStringPFromID("msg_error_index"));
        
        Page::SetBackButtonEvent(Page::ErrorRetryCB, callingPage);
        return;
    }

    string index;
    SceInt32 bytesRead;
    char buff[257]; //Leave 1 char for '\0'
    do
    {
        sce_paf_memset(buff, 0, sizeof(buff));

        bytesRead = http.Read(buff, 256);

        index += buff;
    } while(bytesRead > 0);

    http.Close();
    Dialog::Close();

    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    
    if(!LocalFile::Exists(db::info[Settings::GetInstance()->source].iconFolderPath))
    {
        Dir::CreateRecursive(db::info[Settings::GetInstance()->source].iconFolderPath); 

        string dialogText;
        wstring wstrText;
        Utils::GetfStringFromID("msg_icon_pack_missing", &dialogText);
        ccc::UTF8toUTF16(&dialogText, &wstrText);

        Dialog::OpenYesNo(mainPlugin, NULL, (wchar_t *)wstrText.data(), IconDownloadDecideCB);
    }

    g_busyIndicator->Start();

    db::info[Settings::GetInstance()->source].Parse(&callingPage->appList, index);
    print("Parsing complete!\n");
    
    sceKernelSetEventFlag(callingPage->iconFlags, FLAG_ICON_DOWNLOAD | FLAG_ICON_LOAD_SURF | FLAG_ICON_ASSIGN_TEXTURE);

    callingPage->CreatePopulatedPage();
    callingPage->HandleForwardButton();
    g_busyIndicator->Stop();

    callingPage->StartIconDownloads();

    print("LoadJob finished!\n");
}

SceVoid apps::Page::LoadJob::Finish()
{

}

SceVoid apps::Page::IconDownloadJob::Run()
{   
    //print("Icon Download Job:\n\tURL: %s\n\tDest: %s\n\tTexture: %p\n\tWidget: %p\n", taskParam.url.data(), taskParam.dest.data(), taskParam.texture, taskParam.widgetHash);
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_DOWNLOAD, SCE_NULL, SCE_NULL) < 0 /* Downloads disabled */ || LocalFile::Exists(taskParam.dest.data()) /* Already Downloaded */)
    {
        //print("\tDownload aborted\n");
        goto LOAD_SURF;
    }
    
    //Downloading
    {
        HttpFile             http;
        HttpFile::OpenArg    openArg;
        SharedPtr<LocalFile> file;
        SceInt32             ret = SCE_OK;
        
        openArg.SetOpt(4000000, HttpFile::OpenArg::Opt_ResolveTimeOut);
        openArg.SetOpt(10000000, HttpFile::OpenArg::Opt_ConnectTimeOut);
        openArg.SetUrl(taskParam.url.data());
        
        ret = http.Open(&openArg);
        if(ret != SCE_OK)
        {
            //print("\t[Error] Download Icon %s -> %s failed => 0x%X\n", taskParam.url.data(), taskParam.dest.data(), ret);
            goto LOAD_SURF;
        }

        file = LocalFile::Open(taskParam.dest.data(), SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0666, &ret);
        if(ret < 0)
        {
            //print("\t[Error] Open File %s failed => 0x%X\n", taskParam.dest.data(), ret);
            http.Close();
            goto LOAD_SURF;
        }

        char buff[0x100];
        do 
        {
            if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_DOWNLOAD, SCE_NULL, SCE_NULL) < 0)
            {
                //print("\tJob cancelled\n");
                break;
            }

            sce_paf_memset(buff, 0, sizeof(buff));
            ret = http.Read(buff, 0x100);

            file.get()->Write(buff, ret);
        } while(ret > 0);

        file.get()->Close();
        http.Close();
    }

LOAD_SURF:
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_LOAD_SURF, SCE_NULL, SCE_NULL) < 0 /* Surface loading disabled */)
    {
        //print("\tSurface Loading aborted\n");
        goto ASSIGN_TEX;
    }

    if(*taskParam.texture != SCE_NULL /* Already loaded */)
    {
        //print("\tSurface already loaded\n");
        goto ASSIGN_TEX;
    }

    //Surface loading
    {
        SceInt32 ret = SCE_OK;
        SharedPtr<LocalFile> file = LocalFile::Open(taskParam.dest.data(), SCE_O_RDONLY, 0, &ret);
        if(ret != SCE_OK)
        {
            //print("\t[Error] Open %s failed -> 0x%X\n", taskParam.dest.data(), ret);
            goto ASSIGN_TEX;
        }

        graph::Surface::Create(taskParam.texture, mainPlugin->memoryPool, (SharedPtr<File> *)&file);

        file.get()->Close();
    }


ASSIGN_TEX:
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_ASSIGN_TEXTURE, SCE_NULL, SCE_NULL) < 0)
    {
        //print("\tSurface assignment aborted.\n");
        goto EXIT;
    }

    //Assigning the appropriate surface to the widget
    {
        ui::Widget *widget = Utils::GetChildByHash(callingPage->root, taskParam.widgetHash);
        if(widget == SCE_NULL)
        {
            //print("\t[Skip] Widget 0x%X not found\n", taskParam.widgetHash);
            goto EXIT;
        }

        if(*taskParam.texture == SCE_NULL)
        {
            widget->SetSurfaceBase(&BrokenTex);
            goto EXIT;
        }

        widget->SetSurfaceBase(taskParam.texture);
    }

EXIT:
    //print("\tTask completed\n");
    return;
}

SceVoid apps::Page::IconAssignJob::Run()
{
    print("Icon Assign Job:\n\tPath: %s\n\tTexture: %p\n\tWidget: %p\n", taskParam.path.data(), taskParam.texture, taskParam.widgetHash);
    //wstring str;
    //Utils::GetChildByHash(callingPage->root, taskParam.widgetHash)->GetLabel(&str);
    //print("\tTitle: %ls\n", str.data());
LOAD_SURF:
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_LOAD_SURF, SCE_NULL, SCE_NULL) < 0 /* Surface loading disabled */)
    {
        print("\tSurface Loading aborted\n");
        goto ASSIGN_TEX;
    }

    if(*taskParam.texture != SCE_NULL /* Already loaded */)
    {
        print("\tSurface already loaded\n");
        goto ASSIGN_TEX;
    }

    //Surface loading
    {
        SceInt32 ret = SCE_OK;
        SharedPtr<LocalFile> file = LocalFile::Open(taskParam.path.data(), SCE_O_RDONLY, 0, &ret);
        if(ret != SCE_OK)
        {
            print("\t[Error] Open %s failed -> 0x%X\n", taskParam.path.data(), ret);
            goto ASSIGN_TEX;
        }

        graph::Surface::Create(taskParam.texture, mainPlugin->memoryPool, (SharedPtr<File> *)&file);

        file.get()->Close();
    }


ASSIGN_TEX:
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_ASSIGN_TEXTURE, SCE_NULL, SCE_NULL) < 0)
    {
        print("\tSurface assignment aborted.\n");
        goto EXIT;
    }

    //Assigning the appropriate surface to the widget
    {
        ui::Widget *widget = Utils::GetChildByHash(callingPage->root, taskParam.widgetHash);
        if(widget == SCE_NULL)
        {
            print("\t[Skip] Widget 0x%X not found\n", taskParam.widgetHash);
            goto EXIT;
        }

        thread::s_mainThreadMutex.Lock();
        if(*taskParam.texture == SCE_NULL)
        {
            widget->SetSurfaceBase(&BrokenTex);
            thread::s_mainThreadMutex.Unlock();
            goto EXIT;
        }

        widget->SetSurfaceBase(taskParam.texture);
        thread::s_mainThreadMutex.Unlock();
    }
EXIT:
    print("\tTask Completed\n");
    return;
}

SceVoid apps::Page::BackCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    ((apps::Page *)pUserData)->DeletePage();
    ((apps::Page *)pUserData)->HandleForwardButton();
}

SceVoid apps::Page::CancelIconDownloads()
{
    if(iconDownloadThread)
    {
        if(iconDownloadThread->IsAlive())
        {
            iconDownloadThread->Cancel();
            iconDownloadThread->Join();
        }
        delete iconDownloadThread;
        iconDownloadThread = SCE_NULL;
    }
}

SceVoid apps::Page::StartIconDownloads()
{
    iconDownloadThread = new IconDownloadThread(this, SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_16KiB);
    iconDownloadThread->Start();
}

SceVoid apps::Page::IconDownloadThread::EntryFunction()
{
    //This is apparently a LOT faster in a thread?
    print("apps::Page::IconDownloadThread START\n"); 

    db::entryInfo *info = callingPage->appList.Get(0);
    for(int i = 0; i < callingPage->appList.GetSize() && callingPage->appList.IsValidEntry(info) && !IsCanceled(); i++, info++)
    {
        if(LocalFile::Exists(info->iconLocal.data())) continue;

        auto jobPtr = SharedPtr<job::JobItem>(
            new IconDownloadJob("BHBB::apps::Page::IconDownloadJob", 
                        callingPage, 
                        IconDownloadJob::Param(
                            info->hash, 
                            &info->tex, 
                            info->icon, 
                            info->iconLocal)));

        callingPage->iconDownloadQueue->Enqueue(&jobPtr);
    }

    print("apps::Page::IconDownloadThread END\n");
}

template<class OnCompleteFunc>
SceVoid apps::Page::NewPage(const OnCompleteFunc& onComplete)
{
    Plugin::TemplateInitParam tInit;
    rco::Element e = Utils::GetParamWithHashFromId("home_page_list_template");

    currBody = new Body(currBody);

    ui::Widget *widget = SCE_NULL;
    
    mainPlugin->TemplateOpen(listWrapperPlane, &e, &tInit);
    currBody->widget = widget = listWrapperPlane->GetChild(listWrapperPlane->childNum - 1);

    if(currBody->prev != SCE_NULL)
    {
        Utils::PlayEffect(currBody->prev->widget, 0, effect::EffectType_3D_SlideToBack1);

        g_backButton->PlayEffect(0, effect::EffectType_Reset);
        generic::Page::SetBackButtonEvent(apps::Page::BackCB, this);

        if(currBody->prev->prev != SCE_NULL)
        {
            Utils::PlayEffectReverse(currBody->prev->prev->widget, 0, effect::EffectType_Reset);
        }
    }

    if(listWrapperPlane->childNum == 1)
        generic::Page::ResetBackButton();
    
    auto scrollBox = Utils::GetChildByHash(widget, Utils::GetHashById("list_scroll_box"));
    e.hash = Utils::GetHashById("homebrew_button");

    onComplete(scrollBox);

    Utils::PlayEffect(widget, -5000, effect::EffectType_3D_SlideFromFront);
}

SceVoid apps::Page::DeletePage(SceBool animate)
{   
    if(currBody == SCE_NULL) return;

    effect::Play(animate ? -100 : 0, currBody->widget, effect::EffectType_3D_SlideFromFront, SCE_TRUE, !animate);
    
    if(currBody->prev != SCE_NULL)
    {
        currBody->prev->widget->PlayEffectReverse(0, effect::EffectType_3D_SlideToBack1);
        Utils::PlayEffect(currBody->prev->widget, 0, effect::EffectType_Reset);

        if(currBody->prev->prev != SCE_NULL)
            Utils::PlayEffect(currBody->prev->prev->widget, 0, effect::EffectType_Reset);
        else 
            generic::Page::ResetBackButton();
    }

    Body *prev = currBody->prev;
    delete currBody;
    currBody = prev;
}