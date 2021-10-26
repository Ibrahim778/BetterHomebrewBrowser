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
}

userConfig conf;
int loadFlags = 0;
int pageNum = 1;
bool triedRedownloadIcons = false;

CB(PageListThread);

int main()
{
    
    BHBB::Utils::StartBGDL();

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

CB(onListPageDelete)
{
    pageNum = 1;
    forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset);
}

BUTTON_CB(PageDecrease)
{
    if (pageNum > 1)
        pageNum--;

    if (pageNum == 1)
        EventHandler::ResetBackButtonEvent();

    if (Page::GetCurrentPage()->pageThread->IsStarted())
    {
        Page::GetCurrentPage()->pageThread->EndThread = SCE_TRUE;
        Page::GetCurrentPage()->pageThread->Join();
    }
    
    delete Page::GetCurrentPage()->pageThread;
    Page::GetCurrentPage()->pageThread = new UtilThread(SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SCE_KERNEL_16KiB, "BHBB_PAGE_THREAD");
    Page::GetCurrentPage()->pageThread->Entry = PageListThread;
    Page::GetCurrentPage()->pageThread->EndThread = SCE_FALSE;
    Page::GetCurrentPage()->pageThread->Start();

}

BUTTON_CB(PageIncrease)
{
    if ((pageNum * APPS_PER_PAGE) < list.num)
    {
        pageNum++;

        EventHandler::SetBackButtonEvent(PageDecrease);
        if (Page::GetCurrentPage()->pageThread->IsStarted())
        {
            Page::GetCurrentPage()->pageThread->EndThread = SCE_TRUE;
            Page::GetCurrentPage()->pageThread->Join();
        }
        delete Page::GetCurrentPage()->pageThread;
        Page::GetCurrentPage()->pageThread = new UtilThread(SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SCE_KERNEL_16KiB, "BHBB_PAGE_THREAD");
        Page::GetCurrentPage()->pageThread->Entry = PageListThread;
        Page::GetCurrentPage()->pageThread->EndThread = SCE_FALSE;
        Page::GetCurrentPage()->pageThread->Start();
    }
}

CB(RedisplayCB)
{
    if ((pageNum * APPS_PER_PAGE) < list.num)
        forwardButton->PlayAnimation(0, Widget::Animation_Reset);
    
    if (pageNum > 1)
        EventHandler::SetBackButtonEvent(PageDecrease);
}

CB(PageListThread)
{
    SelectionList *listp = (SelectionList *)Page::GetCurrentPage();
    listp->Clear();
    listp->Hide();
    
    if (list.num == 0)
    {
        new TextPage("Why is there nothing here?");
        Page::DeletePage(listp, false);
        return;
    }

    Page::GetCurrentPage()->busy->Start();

    EventHandler::SetForwardButtonEvent(PageIncrease);

    bool enableIcons;
    switch (conf.db)
    {
    case CBPSDB:
        enableIcons = !BHBB::Utils::isDirEmpty(CBPSDB_ICON_SAVE_PATH);
        break;

    case VITADB:
        enableIcons = !BHBB::Utils::isDirEmpty(VITADB_ICON_SAVE_PATH);
        break;

    default:
        enableIcons = false;
        break;
    }

    enableIcons = enableIcons && (loadFlags & LOAD_FLAGS_ICONS);

    node *n = list.getByNum((pageNum - 1) * APPS_PER_PAGE);
    for (int i = 0; i < APPS_PER_PAGE && !Page::GetCurrentPage()->pageThread->EndThread && n != NULL; i++, n = n->next)
    {
        n->button = ((SelectionList *)Page::GetCurrentPage())->AddOption(&n->info.wstrtitle, DisplayInfo, &n->info, SCE_TRUE, enableIcons);
        sceKernelDelayThread(20000);
    }

    if(!Page::GetCurrentPage()->pageThread->EndThread)
    {
        Page::GetCurrentPage()->busy->Stop();
        listp->Show();
    }

    if ((pageNum * APPS_PER_PAGE) < list.num)
        forwardButton->PlayAnimation(0, Widget::Animation_Reset);
    else 
        forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset); 


    if (!(loadFlags & LOAD_FLAGS_ICONS))
        goto END;
    if (!enableIcons)
        goto END;
    if (Page::GetCurrentPage()->pageThread->EndThread)
        goto END;

    //Texture Loading
    switch (conf.db)
    {
    case VITADB:
    case CBPSDB:
    {
        n = list.getByNum((pageNum - 1) * APPS_PER_PAGE);
        for(int i = 0; i < APPS_PER_PAGE && !Page::GetCurrentPage()->pageThread->EndThread && n != NULL; i++, n = n->next)
        {
            if(n->tex.texSurface == NULL)
            {
                if(BHBB::Utils::CreateTextureFromFile(&n->tex, n->info.icon0Local.data))
                {
                    n->button->SetTextureBase(&n->tex);
                }
                else
                {
                    n->button->SetTextureBase(BrokenTex);
                }
            }
            else n->button->SetTextureBase(&n->tex);
        }
        break;
    }
    

    default:
        break;
    }

END:
    listp->OnRedisplay = RedisplayCB;
    listp->OnDelete = onListPageDelete;
    sceKernelExitDeleteThread(0);
}

CB(DownloadThread)
{
    SceInt32 res = 0;
    switch (conf.db)
    {
    case VITADB:
    {
        res = BHBB::Utils::DownloadFile(VITADB_URL, DATA_PATH "/vitadb.json");
        if (res != CURLE_OK)
            break;
        BHBB::Utils::SetWidgetLabel(((LoadingPage *)Page::GetCurrentPage())->infoText, "Parsing");

        parseJson(DATA_PATH "/vitadb.json");

        if ((loadFlags & LOAD_FLAGS_ICONS) && checkDownloadIcons())
        {
            BHBB::Utils::SetWidgetLabel(((LoadingPage *)Page::GetCurrentPage())->infoText, "Downloading Icons");
            res = BHBB::Utils::DownloadFile(VITADB_DOWNLOAD_ICONS_URL, VITADB_ICON_ZIP_SAVE_PATH);
            if(!Page::GetCurrentPage()->pageThread->EndThread && res == CURLE_OK)
            {
                LoadingPage *oldPage = (LoadingPage *)Page::GetCurrentPage();
                new ProgressPage("Extracting");
                Page::DeletePage(oldPage, false);

                mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);
                Page::GetCurrentPage()->busy->Stop();

                Zip *zipFile = ZipOpen(VITADB_ICON_ZIP_SAVE_PATH);
                ZipExtract(zipFile, NULL, VITADB_ICON_SAVE_PATH, ((ProgressPage *)Page::GetCurrentPage())->progressBars[0]);
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
        SceInt32 res = BHBB::Utils::DownloadFile(CBPSDB_URL, DATA_PATH "/cbpsdb.csv");
        if (res != CURLE_OK)
            break;

        BHBB::Utils::SetWidgetLabel(((LoadingPage *)Page::GetCurrentPage())->infoText, "Parsing");
        parseCSV(DATA_PATH "/cbpsdb.csv");

        if ((loadFlags & LOAD_FLAGS_ICONS) && checkDownloadIcons())
        {
            sceIoMkdir(CBPSDB_ICON_SAVE_PATH, 0666);
            BHBB::Utils::SetWidgetLabel(((LoadingPage *)Page::GetCurrentPage())->infoText, "Downloading Icons");
            res = BHBB::Utils::DownloadFile(CBPSDB_DOWNLOAD_ICONS_URL, CBPSDB_ICON_ZIP_SAVE_PATH);

            if(!Page::GetCurrentPage()->pageThread->EndThread && res == CURLE_OK)
            {
                LoadingPage *oldPage = (LoadingPage *)Page::GetCurrentPage();
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

            saveCurrentTime();
            mainBackButton->PlayAnimation(0, Widget::Animation_Reset);
        }
        break;
    }
    default:
        break;
    }

    if (Page::GetCurrentPage()->pageThread->EndThread == SCE_TRUE)
        return;

    Page::GetCurrentPage()->skipAnimation = SCE_TRUE;

    if(res != CURLE_OK)
    {
        new TextPage("Download DB Error");
        return;
    }

    print("Done download index\n");
    sceKernelDelayThread(1000);

    LoadingPage *oldPage = (LoadingPage *)Page::GetCurrentPage();
    SelectionList *currlist = new SelectionList();
    Page::DeletePage(oldPage);
    currlist->pageThread->Entry = PageListThread;
    currlist->pageThread->Start();
}

BUTTON_CB(DownloadIndex)
{
    new LoadingPage("Downloading Index");

    Page::GetCurrentPage()->pageThread->Entry = DownloadThread;
    Page::GetCurrentPage()->pageThread->Start();
}

void onReady()
{
    BHBB::Utils::MakeDataDirs();
    GetConfig(&conf);
    
    Page::Init();
    BHBB::Utils::NetInit();

    sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);

    pageNum = 1;
    SelectionList *categoryPage = new SelectionList("Homebrew Type");
    categoryPage->AddOption("Apps", DownloadIndex);
    categoryPage->busy->Stop();
}

#ifdef _DEBUG
void PrintFreeMem(ScePVoid pUserData)
{
    print("Free Mem: %lu\n", fwAllocator->GetFreeSize());
}
#endif