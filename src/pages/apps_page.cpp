#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <shellsvc.h>
#include <vector>
#include <algorithm>

#include "pages/apps_page.h"
#include "pages/text_page.h"
#include "pages/apps_info_page.h"
#include "print.h"
#include "utils.h"
#include "settings.h"
#include "common.h"
#include "dialog.h"
#include "curl_file.h"
#include "cURLFile.h"
#include "bgdl.h"

using namespace paf;

apps::Page::Page():generic::MultiPageAppList::MultiPageAppList(&appList, "home_page_template"),iconDownloadThread(SCE_NULL),mode((PageMode)-1)
{
    // Utils::GetChildByHash(root, Hash_Game)->RegisterEventCallback(ui::EventMain_Decide, new Page::CategoryCB(this), 0);
    // Utils::GetChildByHash(root, Hash_All)->RegisterEventCallback(ui::EventMain_Decide, new Page::CategoryCB(this), 0);
    // Utils::GetChildByHash(root, Hash_Emu)->RegisterEventCallback(ui::EventMain_Decide, new Page::CategoryCB(this), 0);
    // Utils::GetChildByHash(root, Hash_Port)->RegisterEventCallback(ui::EventMain_Decide, new Page::CategoryCB(this), 0);
    // Utils::GetChildByHash(root, Hash_Util)->RegisterEventCallback(ui::EventMain_Decide, new Page::CategoryCB(this), 0);
    SetCategories(db::info[Settings::GetInstance()->source].categories);
    SetCategory(-1);
    
    Utils::GetChildByHash(root, Hash_SearchButton)->RegisterEventCallback(ui::EventMain_Decide, new Page::SearchCB(this), 0);
    Utils::GetChildByHash(root, Hash_SearchEnterButton)->RegisterEventCallback(ui::EventMain_Decide, new Page::SearchCB(this), 0);
    Utils::GetChildByHash(root, Hash_SearchBackButton)->RegisterEventCallback(ui::EventMain_Decide, new Page::SearchCB(this), 0);
    Utils::GetChildByHash(root, Hash_SearchBox)->RegisterEventCallback(0x1000000B, new Page::SearchCB(this), 0); //Enter button on IME keyboard

    SetMode(PageMode_Browse);

    Utils::GetChildByHash(root, Utils::GetHashById("options_button"))->RegisterEventCallback(ui::EventMain_Decide, new Settings::OpenCallback(), 0);

    job::JobQueue::Option iconAssignOpt;
    iconAssignOpt.workerNum = 10;
    iconAssignOpt.workerOpt = SCE_NULL;
    iconAssignOpt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER + 15;
    iconAssignOpt.workerStackSize = SCE_KERNEL_16KiB;
    iconAssignQueue = new job::JobQueue("BHBB::apps::Page::iconAssignQueue", &iconAssignOpt);

    job::JobQueue::Option iconDownloadOpt;
    iconDownloadOpt.workerNum = 5;
    iconDownloadOpt.workerOpt = SCE_NULL;
    iconDownloadOpt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER + 20;
    iconDownloadOpt.workerStackSize = SCE_KERNEL_16KiB;
    iconDownloadQueue = new job::JobQueue("BHBB::apps::Page::iconDownloadQueue", &iconDownloadOpt);

    string iconEventFlagName = ccc::Sprintf("BHBB_IconEventFlag_0x%X", this); //Just to make it unique. Not ideal but works?
    iconFlags = sceKernelCreateEventFlag(iconEventFlagName.data(), SCE_KERNEL_ATTR_MULTI, 0, SCE_NULL);
    print("apps::Page::iconFlags = 0x%X\n", iconFlags);

}

apps::Page::~Page()
{
    
}

SceVoid apps::Page::SetMode(PageMode targetMode)
{
    if(mode == targetMode) return;
    
    ui::Widget *categoriesPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_categories"));
    ui::Widget *searchPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_search"));
    ui::Widget *searchBox = Utils::GetChildByHash(root, Utils::GetHashById("search_box"));

    if(categoriesPlane->animationStatus & 0x80)
        categoriesPlane->animationStatus &= ~0x80;

    if(searchPlane->animationStatus & 0x80)
        searchPlane->animationStatus &= ~0x80;
        
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

SceVoid apps::Page::SearchCB::OnGet(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
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

    case Hash_SearchBox:
    case Hash_SearchEnterButton:
    {
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
    default:
        print("Unknown hash (0x%X)! corrupted widget?\n", self->elem.hash);
        break;
    }
}

SceVoid apps::Page::IconZipJob::Run()
{
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));

    string titleTemplate;
    Utils::GetStringFromID("icons_dl_name", &titleTemplate);
    
    string title = ccc::Sprintf(titleTemplate.data(), db::info[Settings::GetInstance()->source].name);
    
    EnqueueCBGDLTask(title.data(), db::info[Settings::GetInstance()->source].iconsURL, db::info[Settings::GetInstance()->source].iconFolderPath);

    Dialog::Close();
    Dialog::OpenOk(mainPlugin, NULL, Utils::GetStringPFromID("msg_download_queued"));
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
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

SceVoid apps::Page::IconAssignThread::EntryFunction()
{
    print("IconAssignThread START\n");

    auto info = targetList->entries.begin();
    auto end = targetList->entries.end();
    
    for(int i = 0; i < pageCountBeforeCreation * Settings::GetInstance()->nLoad && info != end;i++, info++)
        if(info->type != category && category != -1) i--;
    
    for(int i = 0; i < Settings::GetInstance()->nLoad && i < loadNum && info != end && !IsCanceled(); i++, info++)
    {
        if(info->type != category && category != -1)
        {
            i--;
            continue;
        }
        if(callingPage->loadedTextures.Contains(info->hash))
        {
            auto surf = callingPage->loadedTextures.Get(info->hash);
            Utils::GetChildByHash(callingPage->root, info->hash)->SetSurfaceBase(&surf);
            continue;
        }

        if(/* Removing this check speeds it up alot but also makes it a little unstable */ LocalFile::Exists(info->iconPath.data()) && std::find(callingPage->textureJobs.begin(), callingPage->textureJobs.end(), info->hash) == callingPage->textureJobs.end())
        {
            print("ASSIGNING JOB!!\n");
            callingPage->textureJobs.push_back(info->hash);
            callingPage->iconAssignQueue->Enqueue(&SharedPtr<job::JobItem>(
                new IconAssignJob(
                    "apps::Page::IconAssignJob",
                    callingPage,
                    IconAssignJob::Param(
                        info->hash,
                        info->iconPath.data()
            ))));
            print("ASSIGNED!!\n");
        }
        else 
        {
            print("%s Job alreay present!\n", info->title.data());
        } 
    } 

    print("IconAssignThread FINISH\n");
    Cancel();
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
    g_mainQueue->Enqueue(&jobPtr);
}

SceVoid apps::Page::OnClear()
{
    sceKernelClearEventFlag(iconFlags, ~FLAG_ICON_LOAD_SURF);
    iconAssignQueue->Finish();
}

SceVoid apps::Page::OnPageDeleted(generic::MultiPageAppList::Body *body)
{
    if(body && body->userDat)
    {
        IconAssignThread *thread = (IconAssignThread *)body->userDat;
        thread->Cancel();
        //thread::s_mainThreadMutex.Unlock();
        thread->Join();
        //thread::s_mainThreadMutex.Lock();
        delete thread;
        body->userDat = SCE_NULL;   
    }
}

ScePVoid apps::Page::DefaultNewPageData()
{
    return new apps::Page::IconAssignThread(this);
}

SceVoid apps::Page::OnCleared()
{
    sceKernelSetEventFlag(iconFlags, FLAG_ICON_LOAD_SURF);
}

SceVoid apps::Page::PopulatePage(ui::Widget *ScrollBox, void *userDat)
{
    print("PopulatePage START\n");
    SceUInt32 pageCountBeforeCreation = GetPageCount() - 1;
    ScrollBox->SetAlpha(0);

    rco::Element e;
    e.hash = Utils::GetHashById("homebrew_button");
    Plugin::TemplateOpenParam tInit;
    db::List *targetList = GetTargetList();
    int category = GetCategory();

    SceUInt32 loadNum = (targetList->GetSize(category) - (pageCountBeforeCreation * Settings::GetInstance()->nLoad));
    if(loadNum > Settings::GetInstance()->nLoad)
        loadNum = Settings::GetInstance()->nLoad;

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

        Utils::SetWidgetLabel(createdWidget, info->title.data());
        auto callback = new apps::button::Callback();
        callback->pUserData = this;
        createdWidget->RegisterEventCallback(ui::EventMain_Decide, callback, SCE_FALSE);

        if(!isMainThread)
            thread::s_mainThreadMutex.Unlock();
    } 

    if(userDat)
    {
        IconAssignThread *thread = (IconAssignThread *)userDat;
        thread->category = category;
        thread->loadNum = loadNum;
        thread->targetList = targetList;
        thread->pageCountBeforeCreation = pageCountBeforeCreation;
        ((apps::Page::IconAssignThread *)userDat)->Start();
    }

    ScrollBox->SetAlpha(1);    
    print("PopulatePage END\n");
}

SceVoid apps::Page::OnForwardButtonPressed()
{
    NewPage();
    HandleForwardButton();
}

SceVoid apps::Page::LoadJob::Run()
{
    print("apps::Page::LoadJob START!\n");
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    print("Locked PS Button");

    Settings::GetInstance()->Close();
    print("Closed settings...\n");
    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));
    print("Opened pls wait\n");
    callingPage->CancelIconDownloads();
    print("Canceled Icon Downloads!\n");
    callingPage->CancelIconJobs();
    print("Canceled Icon Jobs\n");
    callingPage->ClearPages();
    print("Cleared Pages!\n");
    callingPage->loadedTextures.Clear();
    print("Cleared Tex's\n");
    g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);
    print("Forward button hidden!\n");
    Utils::GetChildByHash(callingPage->root, Utils::GetHashById("options_button"))->Disable(SCE_FALSE);
    print("Disabled settings button\n");

    print("Opening: %s\n", db::info[Settings::GetInstance()->source].indexURL);
    cURLFile file(db::info[Settings::GetInstance()->source].indexURL);

    SceInt32 ret = file.Read();    
    print("Read 0x%X\n", file.GetSize());
    if(ret != SCE_OK || file.GetSize() == 0)
    {
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        
        Dialog::Close();
        Dialog::OpenError(mainPlugin, ret, Utils::GetStringPFromID("msg_error_index"));

        string errorMsg;
        Utils::GetfStringFromID("msg_net_fix", &errorMsg);
        new text::Page(errorMsg.data());
        Page::SetBackButtonEvent(Page::ErrorRetryCB, callingPage);
        return;
    }
    
    string index = string(file.GetData(), file.GetSize());
    
    Dialog::Close();
    print("Closed\n");
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    print("Unlocked\n");
    if(!LocalFile::Exists(db::info[Settings::GetInstance()->source].iconFolderPath))
    {
        Dir::CreateRecursive(db::info[Settings::GetInstance()->source].iconFolderPath); 
        if(db::info[Settings::GetInstance()->source].iconsURL != SCE_NULL)
        {
            string dialogText;
            wstring wstrText;
            Utils::GetfStringFromID("msg_icon_pack_missing", &dialogText);
            ccc::UTF8toUTF16(&dialogText, &wstrText);

            Dialog::OpenYesNo(mainPlugin, NULL, (wchar_t *)wstrText.data(), IconDownloadDecideCB);
        }
    }
    print("Folder checked!\n");
    g_busyIndicator->Start();
    print("Started busy!\n");

    callingPage->SetTargetList(&callingPage->appList);
    print("Set list!\n");
    ret = db::info[Settings::GetInstance()->source].Parse(&callingPage->appList, index);
    if(ret != SCE_OK)
    {
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        
        Dialog::Close();
        Dialog::OpenError(mainPlugin, ret, Utils::GetStringPFromID("msg_error_parse"));

        string errorMsg;
        Utils::GetfStringFromID("msg_wait_fix", &errorMsg);
        new text::Page(errorMsg.data());
        Page::SetBackButtonEvent(Page::ErrorRetryCB, callingPage);
        return;   
    }
    print("Parsing complete!\n");
    
    sceKernelSetEventFlag(callingPage->iconFlags, FLAG_ICON_DOWNLOAD | FLAG_ICON_LOAD_SURF | FLAG_ICON_ASSIGN_TEXTURE);
    print("Set IconFlags!\n");
    callingPage->SetCategory(-1);
    print("Set Category!\n");
    callingPage->NewPage();
    print("Set New Page!\n");
    g_busyIndicator->Stop();
    print("Stopped busy!\n");
    callingPage->StartIconDownloads();
    print("Started Icon Downloads...\n");
    Utils::GetChildByHash(callingPage->root, Utils::GetHashById("options_button"))->Enable(SCE_FALSE);
    print("apps::Page::LoadJob END!\n");
}

SceVoid apps::Page::LoadJob::Finish()
{

}

SceBool apps::Page::IconDownloadJob::CancelCheck(apps::Page *caller)
{
    return sceKernelPollEventFlag(caller->iconFlags, FLAG_ICON_DOWNLOAD, SCE_NULL, SCE_NULL) < 0;
}

SceVoid apps::Page::IconDownloadJob::Run()
{   
    SceBool DownloadSuccess = SCE_FALSE;
    print("Icon Download Job:\n\tDest: %s\n\tWidget: %p\n", taskParam.dest.data(), taskParam.widgetHash);
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_DOWNLOAD, SCE_NULL, SCE_NULL) < 0 /* Downloads disabled */ || LocalFile::Exists(taskParam.dest.data()) /* Already Downloaded */)
    {
        print("\tDownload aborted\n");
        goto LOAD_SURF;
    }

    //Downloading
    {
        auto entry = callingPage->appList.Get((SceUInt64)taskParam.widgetHash);
        
        SceInt32 ret = SCE_OK;

        auto end = entry.iconURL.end();
        for(auto i = entry.iconURL.begin(); i != end; i++)
        {
            ret = cURLFile::SaveFile(i->data(), taskParam.dest.data(), (cURLFile::cancelCheck)IconDownloadJob::CancelCheck, callingPage);
            if(ret != SCE_OK)
            {
                print("\t[Error] Download Icon %s -> %s failed => 0x%X\n", i->data(), taskParam.dest.data(), ret);
                continue;    
            }
            DownloadSuccess = SCE_TRUE;
            break;
        }
    }

LOAD_SURF:
    if(!DownloadSuccess) return;

    callingPage->iconAssignQueue->Enqueue(
        &SharedPtr<paf::job::JobItem>(
            new IconAssignJob(
                "apps::Page::IconAssignJob", 
                callingPage, 
                IconAssignJob::Param(
                    taskParam.widgetHash,
                    taskParam.dest            
    ))));
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

        widget->SetSurfaceBase(&surf);
        thread::s_mainThreadMutex.Unlock();
    }
EXIT:
    print("Removing: 0x%X\n", taskParam.widgetHash);
    callingPage->textureJobs.erase(std::remove(callingPage->textureJobs.begin(), callingPage->textureJobs.end(), taskParam.widgetHash), callingPage->textureJobs.end());
    print("Removed!\n");
    //print("\tTask Completed\n");
    return;
}

SceVoid apps::Page::CancelIconDownloads()
{
    if(iconDownloadThread && !iconDownloadThread->IsCanceled())
    {
        iconDownloadThread->Cancel();
        //thread::s_mainThreadMutex.Unlock();
        iconDownloadThread->Join();
        //thread::s_mainThreadMutex.Lock();
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
        if(LocalFile::Exists(info->iconPath.data()) || 
            std::find(callingPage->textureJobs.begin(), callingPage->textureJobs.end(), info->hash) != callingPage->textureJobs.end()) continue;

        callingPage->textureJobs.push_back(info->hash);
        auto jobPtr = SharedPtr<job::JobItem>(
            new IconDownloadJob("BHBB::apps::Page::IconDownloadJob", 
                        callingPage, 
                        IconDownloadJob::Param(
                            info->hash, 
                            info->iconPath)));

        callingPage->textureJobs.push_back(info->hash);
        callingPage->iconDownloadQueue->Enqueue(&jobPtr);
    }

    print("apps::Page::IconDownloadThread END\n");
    Cancel();
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

apps::button::Callback::Callback()
{
    eventHandler = OnGet;   
}

SceVoid apps::button::Callback::OnGet(SceInt32 eventId, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(self->elem.hash != 0x0)
    {
        new apps::info::Page(((apps::Page *)pUserData)->GetTargetList()->Get((SceUInt64)self->elem.hash));
    }
}