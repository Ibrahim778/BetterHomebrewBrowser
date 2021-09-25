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

extern Page *currPage;
extern CornerButton *mainBackButton;
extern CornerButton *settingsButton;
extern Plugin *mainPlugin;
extern Plane *mainRoot;
extern linked_list list;
extern graphics::Texture *BrokenTex;

extern Allocator *fwAllocator;

graphics::Texture **listTexs;

userConfig conf;


int main()
{
    GetConfig(&conf);
    if(conf.enableIcons) if(sceAppMgrGrowMemory3(10 * 1024 * 1024, 1) < 0) conf.enableIcons = false;
    if(conf.enableScreenshots) if(sceAppMgrGrowMemory3(10 * 1024 * 1024, 1) < 0) conf.enableScreenshots = false;
    initMusic();
    updateMusic();
    initPaf();
    initPlugin();

    return 0;
}

BUTTON_CB(DisplayInfo)
{
    new InfoPage((homeBrewInfo *)userDat, SCE_TRUE);
}

void DeleteTexs(void)
{
    for (int i = 0; i < list.num - 1; i++)
    {
        if(listTexs[i] != NULL)
        {
            listTexs[i]->texSurface = SCE_NULL;
            delete listTexs[i];
        }    
    }
    delete[] listTexs;
}

void PageListThread(void)
{
    SelectionList *listp = (SelectionList *)currPage;
    if(list.num == 0)
    {
        new TextPage("Why is there nothing here?");
        delete listp;
        return;
    }

    node *n = list.head;
    for (int i = 0; i < list.num && !currPage->pageThread->EndThread; i++, n = n->next)
    {
        n->button = ((SelectionList *)currPage)->AddOption(&n->widget.title, DisplayInfo, &n->widget, SCE_TRUE, conf.enableIcons);
        sceKernelDelayThread(8000); // To make sure it doesn't crash, paf is slow sometimes
    }
    currPage->busy->Stop();
    
    if(!conf.enableIcons) goto END;

    listTexs = new graphics::Texture *[list.num];

    //Texture Loading
    switch (conf.db)
    {
    case VITADB:
    case CBPSDB:
    {
        n = list.head;
        for (int i = 0; i < list.num && !currPage->pageThread->EndThread && n != NULL; i++, n = n->next)
        {
            while(currPage != listp) listp->pageThread->Sleep(1000);
            if(checkFileExist(n->widget.icon0Local.data))
            {
                Misc::OpenResult res;
                Misc::OpenFile(&res, n->widget.icon0Local.data, SCE_O_RDONLY, 0777, NULL);

                listTexs[i] = new graphics::Texture();

                graphics::Texture::CreateFromFile(listTexs[i], mainPlugin->memoryPool, &res);
                if(listTexs[i]->texSurface == NULL) new TextPage("Unkown Error Occourred", "Texture Loading Error");
                else n->button->SetTextureBase(listTexs[i]);
                
                delete res.localFile;
                sce_paf_free(res.unk_04);
            }
            else
            {
                listTexs[i] = SCE_NULL;
                n->button->SetTextureBase(BrokenTex);
            }
        }
        break;
    }

    default:
        break;
    }
    currPage->AfterDelete = DeleteTexs;

END:
    currPage->pageThread->Join();
}

void DownloadThread(void)
{
    switch (conf.db)
    {
    case VITADB:
    {
        SceInt32 res = Utils::DownloadFile(VITADB_URL, DATA_PATH "/vitadb.json");
        if(res < 0) break;

        parseJson(DATA_PATH "/vitadb.json");

        if(conf.enableIcons && (checkDownloadIcons() || !checkFileExist(VITADB_ICON_SAVE_PATH)))
        {
            Utils::SetWidgetLabel(((LoadingPage *)currPage)->infoText, "Downloading Icons");
            Utils::DownloadFile(VITADB_DOWNLOAD_ICONS_URL, VITADB_ICON_ZIP_SAVE_PATH);
            
            LoadingPage *oldPage = (LoadingPage *)currPage;
            ProgressPage *extractPage = new ProgressPage("Extracting");
            delete oldPage;

            extractPage->busy->Stop();

            Zip *zipFile = ZipOpen(VITADB_ICON_ZIP_SAVE_PATH);
            ZipExtract(zipFile, NULL, VITADB_ICON_SAVE_PATH, extractPage->progressBars[0]);
            ZipClose(zipFile);

            sceIoRemove(VITADB_ICON_ZIP_SAVE_PATH);

            saveCurrentTime();
        }
        break;
    }    
    case CBPSDB:
    {
        SceInt32 res = Utils::DownloadFile(CBPSDB_URL, DATA_PATH "/cbpsdb.csv");
        if(res < 0) break;

        parseCSV(DATA_PATH "/cbpsdb.csv");

        if(conf.enableIcons && (checkDownloadIcons() || !checkFileExist(CBPSDB_ICON_SAVE_PATH)))
        {
            sceIoMkdir(CBPSDB_ICON_SAVE_PATH, 0777);
            LoadingPage *DownloadIndexPage = (LoadingPage *)currPage;
            ProgressPage *iconDownloadPage = new ProgressPage("Downloading Icons", 2);

            delete DownloadIndexPage;

            mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);
            currPage->busy->Stop();

            node *n = list.head;
            for (int i = 0; i < list.num && n != NULL && !currPage->pageThread->EndThread; i++, n = n->next)
            {
                res = Utils::DownloadFile(n->widget.icon0.data, n->widget.icon0Local.data, ((ProgressPage *)currPage)->progressBars[1]);
                if(res != CURLE_OK)
                {
                    sceIoRemove(n->widget.icon0Local.data);
                    if(res != 6) break;
                }

                //Sometimes the page is deleted and the download functiion ends this could cause a crash
                if(!currPage->pageThread->EndThread)
                {
                    double pos = i + 1;
                    double progress = (double)pos/list.num * 100.0;
                    iconDownloadPage->progressBars[0]->SetProgress(progress, 0, 0);
                }
            }
            
            saveCurrentTime();
            mainBackButton->PlayAnimation(0, Widget::Animation_Reset);
        }
        break;
    }
    default:
        break;
    }

    if(currPage->pageThread->EndThread == SCE_TRUE) return;

    currPage->skipAnimation = SCE_FALSE;
    delete currPage;

    SelectionList *currlist = new SelectionList();
    currlist->pageThread->Entry = PageListThread;
    currlist->pageThread->Start();
}

BUTTON_CB(DownloadIndex)
{
    new LoadingPage("Downloading Index");

    currPage->pageThread->Entry = DownloadThread;
    currPage->pageThread->Start();
}

void onReady()
{
    Page::Init();
    Utils::StartBGDL();
    Utils::NetInit();
    Utils::MakeDataDirs();
    Utils::OverClock();

    sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);

    SelectionList *categoryPage = new SelectionList("Homebrew Type");
    categoryPage->AddOption("Apps", DownloadIndex);
    categoryPage->busy->Stop();
}

#ifdef _DEBUG
void PrintFreeMem(ScePVoid pUserData)
{
    printf("Free Mem: %lu\n", fwAllocator->GetFreeSize());
}
#endif