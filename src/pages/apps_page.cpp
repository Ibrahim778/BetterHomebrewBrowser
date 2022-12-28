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
#include "error_codes.h"

using namespace paf;
using namespace Utils;

apps::Page::Page():generic::MultiPageAppList::MultiPageAppList(&appList, "home_page_template"),iconDownloadThread(SCE_NULL),mode((PageMode)db::CategoryAll)
{
    Widget::GetChild(root, Hash_SearchButton)->RegisterEventCallback(ui::EventMain_Decide, new Page::SearchCB(this), 0);
    Widget::GetChild(root, Hash_SearchEnterButton)->RegisterEventCallback(ui::EventMain_Decide, new Page::SearchCB(this), 0);
    Widget::GetChild(root, Hash_SearchBackButton)->RegisterEventCallback(ui::EventMain_Decide, new Page::SearchCB(this), 0);
    Widget::GetChild(root, Hash_SearchBox)->RegisterEventCallback(0x1000000B, new Page::SearchCB(this), 0); //Enter button on IME keyboard

    SetMode(PageMode_Browse);

    Widget::GetChild(root, Misc::GetHash("options_button"))->RegisterEventCallback(ui::EventMain_Decide, new Settings::OpenCallback(), 0);

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
    
    ui::Widget *categoriesPlane = Widget::GetChild(root, Misc::GetHash("plane_categories"));
    ui::Widget *searchPlane = Widget::GetChild(root, Misc::GetHash("plane_search"));
    ui::Widget *searchBox = Widget::GetChild(root, Misc::GetHash("search_box"));

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
        
        ui::Widget *textBox = Widget::GetChild(page->root, Misc::GetHash("search_box"));
        textBox->GetLabel(&keyString);
        
        if(keyString.length() == 0)
            break;

        paf::string key;
        ccc::UTF16toUTF8(&keyString, &key);

        String::ToLowerCase((char *)key.data());
        
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

            String::ToLowerCase((char *)titleID.data());
            String::ToLowerCase((char *)title.data());
            if(sce_paf_strstr(title.data(), key.data()) || sce_paf_strstr(titleID.data(), key.data()))
            {
                page->searchList.Add(*entry);
            }
        }

        page->SetTargetList(&page->searchList);
        page->SetCategory(db::CategoryAll);
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
    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, String::GetPFromID("msg_wait"));

    string titleTemplate;
    String::GetFromID("icons_dl_name", &titleTemplate);
    
    string title = ccc::Sprintf(titleTemplate.data(), db::info[Settings::GetInstance()->source].name);
    
    EnqueueCBGDLTask(title.data(), db::info[Settings::GetInstance()->source].iconsURL, db::info[Settings::GetInstance()->source].iconFolderPath);

    Dialog::Close();
    Dialog::OpenOk(mainPlugin, NULL, String::GetPFromID("msg_download_queued"));
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
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
        if(info->type != category && category != db::CategoryAll) i--;
    
    for(int i = 0; i < Settings::GetInstance()->nLoad && i < loadNum && info != end && !IsCanceled(); i++, info++)
    {
        if(info->type != category && category != db::CategoryAll)
        {
            i--;
            continue;
        }

        graph::Surface *surf = SCE_NULL;

        SceInt32 error = SCE_OK;
        graph::Surface::Create(&surf, mainPlugin->memoryPool, (SharedPtr<File> *)&LocalFile::Open(info->iconPath.data(), SCE_O_RDONLY, 0, &error));
        if(error != SCE_OK || !surf)
        {
            print("Icon create error: 0x%X (%p)\n", error, surf);
            if(error != SCE_PAF_ERROR_ERRNO_ENOENT && error != SCE_OK)
                Dialog::OpenError(mainPlugin, error, String::GetPFromID("msg_icon_error"));

            surf = BrokenTex;
        }
        else surf->UnsafeRelease();

        Widget::GetChild(callingPage->root, info->hash)->SetSurfaceBase(&surf);
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
    // iconAssignQueue->WaitEmpty(); // Not really needed, handled by Finish()

    iconDownloadQueue->Finish();
    print("------------------------------ CANCELLED ICON JOBS ------------------------------\n");

}

SceVoid apps::Page::StartIconJobs()
{
    sceKernelSetEventFlag(iconFlags, FLAG_ICON_DOWNLOAD | FLAG_ICON_LOAD_SURF | FLAG_ICON_ASSIGN_TEXTURE);
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

SceVoid apps::Page::OnPageDelete(generic::MultiPageAppList::Body *body)
{
    if(body && body->userDat)
    {
        IconAssignThread *thread = (IconAssignThread *)body->userDat;
        thread->Cancel();
        //thread::s_mainThreadMutex.Unlock(); // These are supposed to be here apparently but they cause a crash in LoadJob later on
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
    e.hash = Misc::GetHash("homebrew_button");
    Plugin::TemplateOpenParam tInit;
    db::List *targetList = GetTargetList();
    int category = GetCategory();

    SceUInt32 loadNum = (targetList->GetSize(category) - (pageCountBeforeCreation * Settings::GetInstance()->nLoad));
    if(loadNum > Settings::GetInstance()->nLoad)
        loadNum = Settings::GetInstance()->nLoad;

    auto info = targetList->entries.begin();
    auto end = targetList->entries.end();
    
    for(int i = 0; i < pageCountBeforeCreation * Settings::GetInstance()->nLoad && info != end;i++, info++)
        if(info->type != category && category != db::CategoryAll) i--;
    
    for(int i = 0; i < Settings::GetInstance()->nLoad && i < loadNum && info != end; i++, info++)
    {
        if(info->type != category && category != db::CategoryAll)
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

        Widget::SetLabel(createdWidget, info->title.data());

        createdWidget->RegisterEventCallback(ui::EventMain_Decide, new apps::button::Callback(this), SCE_FALSE);
        createdWidget->RegisterEventCallback(ui::EventMain_LongDecide, new apps::button::Callback(this), SCE_FALSE);

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

SceVoid apps::Page::LoadJob::Run()
{
    SceInt32 ret = SCE_OK;

    print("apps::Page::LoadJob START!\n");

    Settings::GetInstance()->Close();
    print("Closed settings...\n");
    callingPage->SetCategories(std::vector<db::Category>(db::info[Settings::GetInstance()->source].categories, db::info[Settings::GetInstance()->source].categories + db::info[Settings::GetInstance()->source].categoryNum));
    callingPage->SetCategory(db::CategoryAll);
    callingPage->Lock();
    print("Set Category!\n");
    callingPage->CancelIconDownloads();
    print("Canceled Icon Downloads!\n");
    callingPage->CancelIconJobs();
    print("Canceled Icon Jobs\n");
    callingPage->ClearPages();
    print("Cleared Pages!\n");
    g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);
    print("Forward button hidden!\n");
    Widget::GetChild(callingPage->root, Misc::GetHash("options_button"))->Disable(SCE_FALSE);
    print("Disabled settings button\n");
    
    //Get the previous time we downloaded from the last db in ticks
    rtc::Tick nextTick, prevTick = Time::GetPreviousDLTime(Settings::GetInstance()->source);
    print("previousTick: %llu\n", prevTick);
    
    rtc::TickAddHours(&nextTick, &prevTick, Settings::GetInstance()->downloadInterval);
    print("Target tick: %llu\n", nextTick);

    print("Download interval: %d\n", Settings::GetInstance()->downloadInterval);
    
    rtc::Tick currentTick;
    rtc::GetCurrentTick(&currentTick);
    print("Current Tick: %llu\n", currentTick);

    if(prevTick < currentTick) //The download time has passed (or was never set in the first place)
    {
        sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        print("Locked PS Button\n");

        Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, String::GetPFromID("msg_wait"));
        print("Opened pls wait\n");

        print("Saving: %s\n", db::info[Settings::GetInstance()->source].indexURL);

        ret = cURLFile::SaveFile(db::info[Settings::GetInstance()->source].indexURL, db::info[Settings::GetInstance()->source].indexPath);
        print("Saved 0x%X\n", ret);
 
        Dialog::Close();
        print("Closed\n");

        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);            
        print("Unlocked\n");
        if(ret != SCE_OK)
        {
            Dialog::OpenError(mainPlugin, ret, String::GetPFromID("msg_error_index"));

            string errorMsg;
            String::GetfFromID("msg_net_fix", &errorMsg);
            new text::Page(errorMsg.data());
            Page::SetBackButtonEvent(Page::ErrorRetryCB, callingPage);
            return;
        }

        paf::rtc::Tick currentTick;
        paf::rtc::GetCurrentTick(&currentTick);
        Time::SetPreviousDLTime(Settings::GetInstance()->source, currentTick);
    }

    if(!LocalFile::Exists(db::info[Settings::GetInstance()->source].iconFolderPath))
    {
        Dir::CreateRecursive(db::info[Settings::GetInstance()->source].iconFolderPath); 
        if(db::info[Settings::GetInstance()->source].iconsURL != SCE_NULL)
        {
            string dialogText;
            wstring wstrText;
            String::GetfFromID("msg_icon_pack_missing", &dialogText);
            ccc::UTF8toUTF16(&dialogText, &wstrText);

            Dialog::OpenYesNo(mainPlugin, NULL, (wchar_t *)wstrText.data(), IconDownloadDecideCB);
        }
    }

    print("Folder checked!\n");
    g_busyIndicator->Start();
    print("Started busy!\n");

    callingPage->SetTargetList(&callingPage->appList);
    print("Set list!\n");
    ret = db::info[Settings::GetInstance()->source].Parse(&callingPage->appList, db::info[Settings::GetInstance()->source].indexPath);
    if(ret != SCE_OK)
    {
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        
        Dialog::Close();
        Dialog::OpenError(mainPlugin, ret, String::GetPFromID("msg_error_parse"));

        string errorMsg;
        String::GetfFromID("msg_wait_fix", &errorMsg);
        new text::Page(errorMsg.data());
        Page::SetBackButtonEvent(Page::ErrorRetryCB, callingPage);
        return;   
    }
    
    print("Parsing complete!\n");

    callingPage->StartIconJobs();
    print("Set IconFlags!\n");
    callingPage->NewPage();
    print("Set New Page!\n");
    g_busyIndicator->Stop();
    print("Stopped busy!\n");
    callingPage->Release();
    callingPage->StartIconDownloads();
    print("Started Icon Downloads...\n");
    Widget::GetChild(callingPage->root, Misc::GetHash("options_button"))->Enable(SCE_FALSE);
    print("apps::Page::LoadJob END!\n");
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

        for(paf::string& url : entry.iconURL)
        {
            ret = cURLFile::SaveFile(url.data(), taskParam.dest.data(), (cURLFile::cancelCheck)IconDownloadJob::CancelCheck, callingPage);
            if(ret != SCE_OK)
            {
                print("\t[Error] Download Icon %s -> %s failed => 0x%X\n", url.data(), taskParam.dest.data(), ret);
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
    print("Icon Assign Job:\n\tPath: %s\n\tWidget: %p\n", taskParam.path.data(), taskParam.widgetHash);

    graph::Surface *surf;
LOAD_SURF:
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_LOAD_SURF, SCE_NULL, SCE_NULL) < 0 /* Surface loading disabled */)
    {
        print("\tSurface Loading aborted\n");
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
    }


ASSIGN_TEX:
    if(sceKernelPollEventFlag(callingPage->iconFlags, FLAG_ICON_ASSIGN_TEXTURE, SCE_NULL, SCE_NULL) < 0)
    {
        print("\tSurface assignment aborted.\n");
        goto EXIT;
    }

    //Assigning the appropriate surface to the widget
    {
        ui::Widget *widget = Widget::GetChild(callingPage->root, taskParam.widgetHash);
        if(widget == SCE_NULL)
        {
            print("\t[Skip] Widget 0x%X not found\n", taskParam.widgetHash);
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
    print("\tRemoving: 0x%X\n", taskParam.widgetHash);
    callingPage->textureJobs.erase(std::remove(callingPage->textureJobs.begin(), callingPage->textureJobs.end(), taskParam.widgetHash), callingPage->textureJobs.end());
    print("\tRemoved!\n");
    print("\tTask Completed\n");
    return;
}

SceVoid apps::Page::CancelIconDownloads()
{
    if(iconDownloadThread)
    {
        if(!iconDownloadThread->IsCanceled())
        {
            iconDownloadThread->Cancel();
            //thread::s_mainThreadMutex.Unlock();
            iconDownloadThread->Join();
            //thread::s_mainThreadMutex.Lock();

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
        if(LocalFile::Exists(info->iconPath.data()) || 
           std::find(callingPage->textureJobs.begin(), callingPage->textureJobs.end(), info->hash) != callingPage->textureJobs.end() || 
           info->iconURL.size() > 0) continue;

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

SceVoid apps::Page::IconInvokedDownloadJob::DialogEventHandler(Dialog::ButtonCode resCode, ScePVoid userDat)
{
    apps::Page::IconInvokedDownloadJob *job = (apps::Page::IconInvokedDownloadJob *)userDat;

    job->DialogResult = resCode == Dialog::ButtonCode::ButtonCode_Yes;
}

SceVoid apps::Page::IconInvokedDownloadJob::Run()
{
    auto &entry = callingPage->appList.Get(hash);
    if(entry.iconURL.size() == 0)
    {
        Dialog::OpenOk(mainPlugin, SCE_NULL, String::GetPFromID("msg_icon_no_source"));    
        return;
    }

    if(LocalFile::Exists(entry.iconPath.data()))
    {
        Dialog::OpenYesNo(mainPlugin, SCE_NULL, String::GetPFromID("msg_icon_redownload"), DialogEventHandler, this);
        Dialog::WaitEnd();
        if(!DialogResult) return; //Return if the user said no
    }

    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, String::GetPFromID("msg_downloading_icon"));
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
            graph::Surface::Create(&surf, mainPlugin->memoryPool, (SharedPtr<File> *)&file);

            file.get()->Close();    
        }


    ASSIGN_TEX:
        //Assigning the appropriate surface to the widget
        {
            ui::Widget *widget = Widget::GetChild(callingPage->root, hash);
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
                widget->SetSurfaceBase(&BrokenTex);
                thread::s_mainThreadMutex.Unlock();
                break;
            }
            surf->UnsafeRelease();
            widget->SetSurfaceBase(&surf);
            thread::s_mainThreadMutex.Unlock();
        } 

        DownloadSuccess = SCE_TRUE;
        break;
    } 
    
    Dialog::Close();
    print("Closed\n");
    print("DL Success: %s\n", DownloadSuccess ? "True" : "False");
    if(!DownloadSuccess)
        Dialog::OpenError(mainPlugin, ret, String::GetPFromID("msg_icon_error"));    
}

SceVoid apps::button::Callback::OnGet(SceInt32 eventId, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(self->elem.hash == 0x0)
        return;

    switch(eventId)
    {
        case ui::EventMain_Decide:
            new apps::info::Page(((apps::Page *)pUserData)->GetTargetList()->Get((SceUInt64)self->elem.hash));
            break;
        
        case ui::EventMain_LongDecide:
            g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new apps::Page::IconInvokedDownloadJob("BHBB::IconInvokedDownloadJob", self->elem.hash, (apps::Page *)pUserData)));
            break;
    }
}