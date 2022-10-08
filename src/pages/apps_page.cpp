#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <shellsvc.h>
#include <vector>
#include <algorithm>

#include "pages/apps_page.h"
#include "pages/text_page.h"
#include "print.h"
#include "utils.h"
#include "settings.h"
#include "common.h"
#include "dialog.h"

using namespace paf;

apps::Page::Page():generic::MultiPageAppList::MultiPageAppList(&appList, "home_page_template"),iconDownloadThread(SCE_NULL)
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

    job::JobQueue::Option iconAssignOpt;
    iconAssignOpt.workerNum = 10;
    iconAssignOpt.workerOpt = NULL;
    iconAssignOpt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER - 10;
    iconAssignOpt.workerStackSize = SCE_KERNEL_16KiB;
    iconAssignQueue = new job::JobQueue("BHBB::apps::Page::iconAssignQueue", &iconAssignOpt);

    job::JobQueue::Option iconDownloadOpt;
    iconDownloadOpt.workerNum = 10;
    iconDownloadOpt.workerOpt = NULL;
    iconDownloadOpt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER - 20;
    iconDownloadOpt.workerStackSize = SCE_KERNEL_16KiB;
    iconDownloadQueue = new job::JobQueue("BHBB::apps::Page::iconDownloadQueue", &iconDownloadOpt);

    string iconEventFlagName = ccc::Sprintf("BHBB_IconEventFlag_0x%X", this); //Just to make it unique. Not ideal but works?
    iconFlags = sceKernelCreateEventFlag(iconEventFlagName.data(), SCE_KERNEL_ATTR_MULTI, 0, SCE_NULL);
    print("apps::Page::iconFlags = 0x%X\n", iconFlags);

}

apps::Page::~Page()
{
    
}

SceVoid apps::Page::CategoryButtonCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(!db::info[Settings::GetInstance()->source].CategoriesSuppourted) return;
    apps::Page *page = (Page *)pUserData;
    int targetCategory = 0;
    switch (self->elem.hash)
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

SceVoid apps::Page::OnCategoryChanged(int prev, int _category)
{
    print("OnCategory changed...\n");
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

    switch(_category)
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
    switch(self->elem.hash)
    {
    case Hash_SearchButton:
        page->SetMode(PageMode_Search);
        break;

    case Hash_SearchBackButton:
        page->SetMode(PageMode_Browse);
        if(page->GetTargetList() != &page->appList)
        {
            page->SetTargetList(&page->appList);
            page->Redisplay();
        }
        break;

    case Hash_SearchEnterButton:
        paf::wstring keyString;
        
        ui::Widget *textBox = Utils::GetChildByHash(page->root, Utils::GetHashById("search_box"));
        textBox->GetLabel(&keyString);
        
        if(keyString.length() == 0)
            break;

        paf::string key;
        ccc::UTF16toUTF8(&keyString, &key);

        Utils::ToLowerCase((char *)key.data());
        
        //Get rid of trailing space char
        for(int i = key.length() - 1; i > 0; i--)
        {
            if(key.data()[i] == ' ')
            {
                ((char *)key.data())[i] = 0; //Shouldn't really do this with a paf::string but w/e
                break;
            } else break;
        }

        page->searchList.Clear();

        int i = 0;
        auto end = page->appList.entries.end();
        for(auto entry = page->appList.entries.begin(); entry != end; entry++)
        {
            string titleID = entry->titleID.data();
            string title = entry->title.data();

            Utils::ToLowerCase((char *)titleID.data());
            Utils::ToLowerCase((char *)title.data());
            if(sce_paf_strstr(title.data(), key.data()) || sce_paf_strstr(titleID.data(), key.data()))
            {
                page->searchList.Add(*entry);
            }
        }

        page->SetTargetList(&page->searchList);
        page->SetCategory(-1);
        page->Redisplay();

        break;
    }
}

SceVoid apps::Page::IconZipJob::Run()
{
    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));

    BGDLParam param;
    sce_paf_memset(&param, 0, sizeof(param));
    param.magic = (BHBB_DL_CFG_VER | BHBB_DL_MAGIC);
    param.type = BGDLParam::Target::CustomPath;
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
    job::s_defaultJobQueue->Enqueue(&jobPtr);
}

SceVoid apps::Page::OnClear()
{
    sceKernelClearEventFlag(iconFlags, ~FLAG_ICON_LOAD_SURF);
    iconAssignQueue->Finish();
}

SceVoid apps::Page::OnCleared()
{
    sceKernelSetEventFlag(iconFlags, FLAG_ICON_LOAD_SURF);
}

SceVoid apps::Page::PopulatePage(ui::Widget *ScrollBox)
{
    print("Populating page...\n");
    SceUInt32 pageCountBeforeCreation = GetPageCount() - 1;
    ScrollBox->SetAlpha(0);

    rco::Element e = Utils::GetParamWithHashFromId("homebrew_button");
    Plugin::TemplateInitParam tInit;
    db::List *targetList = GetTargetList();
    int category = GetCategory();

    SceUInt32 loadNum = (targetList->GetSize(category) - (pageCountBeforeCreation * Settings::GetInstance()->nLoad)) < Settings::GetInstance()->nLoad ? (targetList->GetSize(category) - (pageCountBeforeCreation * Settings::GetInstance()->nLoad)) : Settings::GetInstance()->nLoad;
    auto info = targetList->entries.begin();
    auto end = targetList->entries.end();
    
    for(int i = 0; i < pageCountBeforeCreation * Settings::GetInstance()->nLoad && info != end;i++, info++)
        if(info->type != category && category != -1) i--;
    
    for(int i = 0; i < Settings::GetInstance()->nLoad && i < loadNum && info != end; i++, info++)
    {
        if(info->type != category && category != -1)
        {
            i--;
            continue;
        }
        SceBool isMainThread = thread::IsMainThread();
        if(!isMainThread)
            thread::s_mainThreadMutex.Lock();
    
        mainPlugin->TemplateOpen(ScrollBox, &e, &tInit);
    
        ui::Widget *createdWidget = ScrollBox->GetChild(ScrollBox->childNum - 1);
        createdWidget->elem.hash = info->hash;
        print("0x%X\n", createdWidget->elem.hash);

        Utils::SetWidgetLabel(createdWidget, info->title.data());
        
        if(!isMainThread)
            thread::s_mainThreadMutex.Unlock();
        
        if(((ui::ImageButton *)createdWidget)->imageSurf == SCE_NULL && std::find(textureJobs.begin(), textureJobs.end(), info->hash) == textureJobs.end())
        {
            textureJobs.push_back(info->hash);
            iconAssignQueue->Enqueue(&SharedPtr<job::JobItem>(
                new IconAssignJob(
                    "apps::Page::IconAssignJob",
                    this,
                    IconAssignJob::Param(
                        info->hash,
                        info->iconLocal.data()
            ))));
        }
    } 
    

    
    ScrollBox->SetAlpha(1);
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
    callingPage->loadedTextures.Clear();
    g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);

    openArg.SetUrl(db::info[Settings::GetInstance()->source].indexURL);
    print("Opening: %s\n", db::info[Settings::GetInstance()->source].indexURL);
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

    callingPage->SetTargetList(&callingPage->appList);
    db::info[Settings::GetInstance()->source].Parse(&callingPage->appList, index);

    print("Parsing complete!\n");
    
    sceKernelSetEventFlag(callingPage->iconFlags, FLAG_ICON_DOWNLOAD | FLAG_ICON_LOAD_SURF | FLAG_ICON_ASSIGN_TEXTURE);

    callingPage->NewPage();
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

    // callingPage->iconAssignQueue->Enqueue(
    //     &SharedPtr<paf::job::JobItem>(
    //         new IconAssignJob(
    //             "apps::Page::IconAssignJob", 
    //             callingPage, 
    //             IconAssignJob::Param(
    //                 taskParam.widgetHash, 
    //                 taskParam.url.data(),
                
    // ))));
    return;

//     if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_LOAD_SURF, SCE_NULL, SCE_NULL) < 0 /* Surface loading disabled */)
//     {
//         //print("\tSurface Loading aborted\n");
//         goto ASSIGN_TEX;
//     }

//     if(*taskParam.texture != SCE_NULL /* Already loaded */)
//     {
//         //print("\tSurface already loaded\n");
//         goto ASSIGN_TEX;
//     }

//     //Surface loading
//     {
//         SceInt32 ret = SCE_OK;
//         SharedPtr<LocalFile> file = LocalFile::Open(taskParam.dest.data(), SCE_O_RDONLY, 0, &ret);
//         if(ret != SCE_OK)
//         {
//             //print("\t[Error] Open %s failed -> 0x%X\n", taskParam.dest.data(), ret);
//             goto ASSIGN_TEX;
//         }

//         graph::Surface::Create(taskParam.texture, mainPlugin->memoryPool, (SharedPtr<File> *)&file);

//         file.get()->Close();
//     }


// ASSIGN_TEX:
//     if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_ASSIGN_TEXTURE, SCE_NULL, SCE_NULL) < 0)
//     {
//         //print("\tSurface assignment aborted.\n");
//         goto EXIT;
//     }

//     //Assigning the appropriate surface to the widget
//     {
//         ui::Widget *widget = Utils::GetChildByHash(callingPage->root, taskParam.widgetHash);
//         if(widget == SCE_NULL)
//         {
//             //print("\t[Skip] Widget 0x%X not found\n", taskParam.widgetHash);
//             goto EXIT;
//         }

//         if(*taskParam.texture == SCE_NULL)
//         {
//             widget->SetSurfaceBase(&BrokenTex);
//             goto EXIT;
//         }

//         widget->SetSurfaceBase(taskParam.texture);
//     }

// EXIT:
//     //print("\tTask completed\n");
//     return;
}

SceVoid apps::Page::IconAssignJob::Run()
{
    //print("Icon Assign Job:\n\tPath: %s\n\tWidget: %p\n", taskParam.path.data(), taskParam.widgetHash);

    graph::Surface *surf = callingPage->loadedTextures.Get(taskParam.widgetHash);//*taskParam.texture; //temporary variable to store surface    

LOAD_SURF:
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_LOAD_SURF, SCE_NULL, SCE_NULL) < 0 /* Surface loading disabled */)
    {
        //print("\tSurface Loading aborted\n");
        goto ASSIGN_TEX;
    }

    if(surf != SCE_NULL /* Already loaded */)
    {
        //print("\tSurface already loaded\n");
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

        graph::Surface::Create(&surf, mainPlugin->memoryPool, (SharedPtr<File> *)&file);

        file.get()->Close();    
        callingPage->loadedTextures.Add(taskParam.widgetHash, surf);
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
        thread::s_mainThreadMutex.Lock();
        if(surf == SCE_NULL)
        {
            widget->SetSurfaceBase(&BrokenTex);
            thread::s_mainThreadMutex.Unlock();
            goto EXIT;
        }
        print("%d\n", surf->base.refCount);

        widget->SetSurfaceBase(&surf);
        thread::s_mainThreadMutex.Unlock();
    }
EXIT:
    //print("Removing: 0x%X\n", taskParam.widgetHash);
    callingPage->textureJobs.erase(std::remove(callingPage->textureJobs.begin(), callingPage->textureJobs.end(), taskParam.widgetHash), callingPage->textureJobs.end());
    //print("Removed!\n");
    //print("\tTask Completed\n");
    return;
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
    //This is apparently a LOT faster in a separate thread?
    print("apps::Page::IconDownloadThread START\n"); 
    db::List *targetList = callingPage->GetTargetList();
    
    auto end = targetList->entries.end();
    for(auto info = targetList->entries.begin(); info != end && !IsCanceled(); info++)
    {
        if(LocalFile::Exists(info->iconLocal.data()) || 
            std::find(callingPage->textureJobs.begin(), callingPage->textureJobs.end(), info->hash) != callingPage->textureJobs.end()) continue;

        callingPage->textureJobs.push_back(info->hash);
        auto jobPtr = SharedPtr<job::JobItem>(
            new IconDownloadJob("BHBB::apps::Page::IconDownloadJob", 
                        callingPage, 
                        IconDownloadJob::Param(
                            info->hash, 
                            info->icon, 
                            info->iconLocal)));

        callingPage->iconDownloadQueue->Enqueue(&jobPtr);
    }

    print("apps::Page::IconDownloadThread END\n");
}

SceVoid apps::Page::TextureList::Add(SceUInt64 hash, graph::Surface *tex)
{
    list.push_back(Node(hash, tex));
}

SceVoid apps::Page::TextureList::Clear(SceBool deleteTex)
{
    if(deleteTex)
    {
        auto end = list.end();
        for(auto entry = list.begin(); entry != list.end(); entry++)
        {
            Utils::DeleteTexture(&entry->surf);
        }
    }

    list.clear();
}

SceBool apps::Page::TextureList::Contains(SceUInt64 hash)
{
    auto end = list.end();
    for(auto entry = list.begin(); entry != list.end(); entry++)
        if(entry->hash == hash) return SCE_TRUE;

    return SCE_FALSE;
}

graph::Surface *apps::Page::TextureList::Get(SceUInt64 hash)
{
    auto end = list.end();
    for(auto entry = list.begin(); entry != list.end(); entry++)
        if(entry->hash == hash) return entry->surf;

    return SCE_NULL;
}