#include "main.hpp"
#include "paf.hpp"
#include "pagemgr.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "parser.hpp"
#include "configmgr.hpp"
#include "network.hpp"
#include "audiomgr.hpp"
#include "Archives.hpp"
#include "timemgr.hpp"

extern "C" {

    extern const char			sceUserMainThreadName[] = "BHBB_MAIN";
    extern const int			sceUserMainThreadPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    extern const unsigned int	sceUserMainThreadStackSize = SCE_KERNEL_THREAD_STACK_SIZE_DEFAULT_USER_MAIN;
    extern const unsigned int   sceLibcHeapSize = 0x180000;

    void __cxa_set_dso_handle_main(void *dso)
    {

    }

    int _sceLdTlsRegisterModuleInfo()
    {
        return 0;
    }

    int __aeabi_unwind_cpp_pr0()
    {
        return 9;
    }

    int __aeabi_unwind_cpp_pr1()
    {
        return 9;
    }

    int __at_quick_exit()
    {
        return 0;
    }
}

userConfig conf;
int loadFlags = 0;
int pageNum = 1;

int category;

THREAD(PageListThread);

int main()
{
    
    Utils::StartBGDL();

    if (sceAppMgrGrowMemory3(10 * 1024 * 1024, 1) >= 0)
        loadFlags |= LOAD_FLAGS_SCREENSHOTS;

    if (sceAppMgrGrowMemory3(5 * 1024 * 1024, 1) >= 0)
        loadFlags |= LOAD_FLAGS_ICONS;

	initPaf();
    initMusic();
	initPlugin();

    return sceKernelExitProcess(0);
}

BUTTON_CB(DisplayInfo)
{
    forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset);
    EventHandler::ResetBackButtonEvent();
    new InfoPage((homeBrewInfo *)userDat);
}

PAGE_CB(onListPageDelete)
{
    pageNum = 1;
    forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset);
    list.Clear(true);
}

BUTTON_CB(PageDecrease)
{
    Page *page = (Page *)userDat;

    if (pageNum > 1)
    {
        mainBackButton->Disable(0);
        forwardButton->Disable(0);
        pageNum--;
        mainBackButton->Enable(0);
        forwardButton->Enable(0);
    }

    if (pageNum == 1)
        EventHandler::ResetBackButtonEvent();

    page->jobs->AddTask(PageListThread, "PAGE_LIST_TASK");
}

BUTTON_CB(PageIncrease)
{
    Page *page = (Page *)userDat;
    if ((pageNum * APPS_PER_PAGE) < list.GetNumByCategory(category))
    {
        mainBackButton->Disable(0);
        forwardButton->Disable(0);
        pageNum++;
        mainBackButton->Enable(0);
        forwardButton->Enable(0);

        EventHandler::SetBackButtonEvent(PageDecrease, page);
        page->jobs->AddTask(PageListThread, "PAGE_LIST_TASK");
    }
}

PAGE_CB(RedisplayCB)
{
    if ((pageNum * APPS_PER_PAGE) < list.GetNumByCategory(category))
        forwardButton->PlayAnimation(0, Widget::Animation_Reset);
    
    if (pageNum > 1)
        EventHandler::SetBackButtonEvent(PageDecrease, callingPage);
}

BUTTON_CB(SetCategory)
{
    HomebrewListPage *page = (HomebrewListPage *)Page::GetCurrentPage();
    category = (int)userDat;
    page->SetCategoryColor(category);
    pageNum = 1;
    EventHandler::ResetBackButtonEvent();
    //Reload Page
    page->jobs->AddTask(PageListThread, "PAGE_LIST_TASK");
}

BUTTON_CB(Search)
{
    forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset);

    new SearchPage(&list);
}

THREAD(PageListThread)
{
    HomebrewListPage *listp = (HomebrewListPage *)callingPage;

    listp->AssignAllButtonEvent(SetCategory, (void *)-1);
    listp->AssignEmulatorButtonEvent(SetCategory, (void *)EMULATOR);
    listp->AssignGameButtonEvent(SetCategory, (void *)GAME);
    listp->AssignPortButtonEvent(SetCategory, (void *)PORT);
    listp->AssignUtilitiesButtonEvent(SetCategory, (void *)UTIL);
    listp->AssignSearchButtonEvent(Search);

    listp->Clear();
    listp->Hide();

    sceKernelDelayThread(10000);
    
    if(conf.db == CBPSDB) listp->SetSearchMode();

    if (list.num == 0)
    {
        new TextPage("Why is there nothing here?");
        Page::DeletePage(listp, false);
        return;
    }

    listp->busy->Start();

    EventHandler::SetForwardButtonEvent(PageIncrease, listp);

    bool enableIcons;
    switch (conf.db)
    {
    case CBPSDB:
        enableIcons = !Utils::isDirEmpty(CBPSDB_ICON_SAVE_PATH);
        break;

    case VITADB:
        enableIcons = !Utils::isDirEmpty(VITADB_ICON_SAVE_PATH);
        break;

    default:
        enableIcons = false;
        break;
    }

    enableIcons = enableIcons && (loadFlags & LOAD_FLAGS_ICONS);
    
    node *n = list.GetByCategoryIndex((pageNum - 1) * APPS_PER_PAGE, category);
    for (int i = 0; i < APPS_PER_PAGE && !listp->jobs->End && !listp->jobs->WaitingTasks() && n != NULL; i++, n = n->next)
    {
        if(n->info.type != category && category != -1)
        {
            i--;
        }
        else
        {
            n->button = listp->AddOption(&n->info.wstrtitle, NULL, NULL, SCE_TRUE, enableIcons);
            //sceKernelDelayThread(20000);
            
            if(n->button != NULL)
            {
                n->button->hash = Utils::GetHashById(n->info.id.data);
                n->buttonCB = new HomebrewListButtonEventHandler();
                n->buttonCB->pUserData = &n->info;
                
                n->button->RegisterEventCallback(ON_PRESS_EVENT_ID, n->buttonCB, 0);
            }
            
        }
    }

    if(!listp->jobs->End && !listp->jobs->WaitingTasks())
    {
        listp->busy->Stop();
        listp->Show();
    }

    if ((pageNum * APPS_PER_PAGE) < list.GetNumByCategory(category))
        forwardButton->PlayAnimation(0, Widget::Animation_Reset);
    else 
        forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset); 

    if(listp->GetNum() < APPS_PER_PAGE && category != -1)
        forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset);


    if (!(loadFlags & LOAD_FLAGS_ICONS))
        goto END;
    if (!enableIcons)
        goto END;
    if (listp->jobs->End && !listp->jobs->WaitingTasks())
        goto END;

    //Texture Loading
    switch (conf.db)
    {
    case VITADB:
    case CBPSDB:
    {
        n = list.GetByCategoryIndex((pageNum - 1) * APPS_PER_PAGE, category);
        for(int i = 0; i < APPS_PER_PAGE && !listp->jobs->End && !listp->jobs->WaitingTasks() && n != NULL; i++, n = n->next)
        {
            bool triedDownload = true; 

            if(n->info.type != category && category != -1) 
            {
                i--;
            }
            else
            {
                if(n->tex->texSurface == NULL)
                {
ASSIGN:             
                    if(Utils::CreateTextureFromFile(n->tex, n->info.icon0Local.data))
                    {
                        n->button->SetTextureBase(n->tex);
                    }
                    else
                    {
                        if(!triedDownload)
                        {

                            switch (conf.db)
                            {
                            case CBPSDB:
                                if(!conf.CBPSDBSettings.downloadIconsDuringLoad) break;
                                Utils::DownloadFile(n->info.icon0.data, n->info.icon0Local.data, listp);
                                break;
                            
                            case VITADB:
                                if(!conf.vitaDBSettings.downloadIconsDuringLoad) break;
                                char url[0x400];
                                
                                sce_paf_memset(url, 0, sizeof(url));
                                sce_paf_snprintf(url, 0x400, VITADB_ICON_URL "%s", n->info.icon0);
                                Utils::DownloadFile(url, n->info.icon0Local.data, listp);

                                break;

                            default:
                                break;
                            }

                            triedDownload = true;
                            goto ASSIGN;
                        }
                        n->button->SetTextureBase(BrokenTex);
                    }
                }
                else n->button->SetTextureBase(n->tex);
            }
        }
        break;
    }
    

    default:
        break;
    }

END:
    listp->OnRedisplay = RedisplayCB;
    listp->OnDelete = onListPageDelete;
}

THREAD(DownloadThread)
{
    ScrollView
    LoadingPage *currPage = (LoadingPage *)callingPage;
    SceInt32 res = 0;
    switch (conf.db)
    {
    case VITADB:
    {
        res = Utils::DownloadFile(VITADB_URL, DATA_PATH "/vitadb.json", currPage);
        if (res != CURLE_OK)
            break;
        Utils::SetWidgetLabel(currPage->infoText, "Parsing");

        parseJson(DATA_PATH "/vitadb.json");

        if ((loadFlags & LOAD_FLAGS_ICONS) && checkDownloadIcons())
        {
            Utils::SetWidgetLabel(currPage->infoText, "Downloading Icons");
            res = Utils::DownloadFile(VITADB_DOWNLOAD_ICONS_URL, VITADB_ICON_ZIP_SAVE_PATH, currPage);
            if(!currPage->jobs->End && res == CURLE_OK)
            {
                LoadingPage *oldPage = currPage;
                ProgressPage *extractPage = new ProgressPage("Extracting");
                Page::DeletePage(oldPage, false);

                mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);
                extractPage->busy->Stop();

                Zip *zipFile = ZipOpen(VITADB_ICON_ZIP_SAVE_PATH);
                ZipExtract(zipFile, NULL, VITADB_ICON_SAVE_PATH, extractPage->progressBars[0]);
                ZipClose(zipFile);

                sceIoRemove(VITADB_ICON_ZIP_SAVE_PATH);

                saveCurrentTime();
            }
            mainBackButton->PlayAnimation(0, Widget::Animation_Reset);
        }
        break;
    }
    case CBPSDB:
    {
        SceInt32 res = Utils::DownloadFile(CBPSDB_URL, DATA_PATH "/cbpsdb.csv", currPage);
        if (res != CURLE_OK)
            break;

        Utils::SetWidgetLabel(currPage->infoText, "Parsing");
        parseCSV(DATA_PATH "/cbpsdb.csv");

        if ((loadFlags & LOAD_FLAGS_ICONS) && checkDownloadIcons())
        {
            sceIoMkdir(CBPSDB_ICON_SAVE_PATH, 0666);
            Utils::SetWidgetLabel(currPage->infoText, "Downloading Icons");
            res = Utils::DownloadFile(CBPSDB_DOWNLOAD_ICONS_URL, CBPSDB_ICON_ZIP_SAVE_PATH, currPage);

            if(!currPage->jobs->End && res == CURLE_OK)
            {
                LoadingPage *oldPage =currPage;
                ProgressPage *extractPage = new ProgressPage("Extracting");
                Page::DeletePage(oldPage, false);

                mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);
                extractPage->busy->Stop();

                Zip *zipFile = ZipOpen(CBPSDB_ICON_ZIP_SAVE_PATH);
                ZipExtract(zipFile, NULL, CBPSDB_ICON_SAVE_PATH, extractPage->progressBars[0]);
                ZipClose(zipFile);

                sceIoRemove(CBPSDB_ICON_ZIP_SAVE_PATH);
                saveCurrentTime();
            }

            mainBackButton->PlayAnimation(0, Widget::Animation_Reset);
        }
        break;
    }
    default:
        break;
    }

    if (currPage->jobs->End == SCE_TRUE)
        return;

    currPage->skipAnimation = SCE_TRUE;

    if(res != CURLE_OK)
    {
        new TextPage("Download DB Error");
        Page::DeletePage((Page *)callingPage);
        return;
    }

    print("Done download index\n");
    sceKernelDelayThread(1000);

    HomebrewListPage *currlist = new HomebrewListPage("PAGE_LIST_THREAD");
    currlist->Hide();
    category = -1;
    currlist->SetCategoryColor(category);
    Page::DeletePage((Page *)callingPage);
    currlist->jobs->AddTask(PageListThread, "PAGE_LIST_TASK");
}

BUTTON_CB(DownloadIndex)
{
    LoadingPage *page = new LoadingPage("Downloading Index");

    page->jobs->AddTask(DownloadThread, "DOWNLOAD_INDEX_TASK");
}

void onReady()
{
    Utils::MakeDataDirs();
    GetConfig(&conf);
    
    Page::Init();
    Utils::NetInit();

    sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);

    pageNum = 1;
    print("Set Defaults\n");
    print("Creating Page....\n");
    SelectionList *categoryPage = new SelectionList("Homebrew Type");
    print("Done\n");
    categoryPage->AddOption("Apps", DownloadIndex);
    categoryPage->busy->Stop();
}

#ifdef _DEBUG
void PrintFreeMem(ScePVoid pUserData)
{
    print("Free Mem: %lu\n", fwAllocator->GetFreeSize());
}
#endif