#include "main.hpp"
#include "common.hpp"
#include "eventhandler.hpp"
#include "configmgr.hpp"
#include "audiomgr.hpp"
#include "pagemgr.hpp"
#include <bgapputil.h>
#include "utils.hpp"
#include "bgdl.h"

extern userConfig conf;
extern Plugin *mainPlugin;

BUTTON_CB(updateDBType)
{
    conf.db = (DB_Type)(int)(userDat);
    WriteConfig(&conf);
}

BUTTON_CB(updateIconTime)
{
    conf.iconDownloadHourGap = (short)(int)userDat;
    WriteConfig(&conf);
}

BUTTON_CB(updateMusicSelection)
{
    conf.enableMusic = (bool)userDat;
    WriteConfig(&conf);
    updateMusic();
}

BUTTON_CB(DBSelector)
{
    if(PopupMgr::showingDialog) return;
    PopupMgr::showDialog();
    PopupMgr::addDialogOption("CBPS DB", updateDBType, (void *)CBPSDB, conf.db == CBPSDB);
    PopupMgr::addDialogOption("Vita DB", updateDBType, (void *)VITADB, conf.db == VITADB);
}

#define ADD_TIME_POPUP_OPTION(hours) PopupMgr::addDialogOption(hours > 1 ? #hours " Hours" : #hours " Hour", updateIconTime, (void *)hours, conf.iconDownloadHourGap == hours)

BUTTON_CB(IconTimeSelector)
{
    if(PopupMgr::showingDialog) return;

    PopupMgr::showDialog();

    PopupMgr::addDialogOption("Never", updateIconTime, (void *)-1, conf.iconDownloadHourGap == -1);
    PopupMgr::addDialogOption("Every Time", updateIconTime, (void *)0, conf.iconDownloadHourGap == 0);

    ADD_TIME_POPUP_OPTION(1);
    ADD_TIME_POPUP_OPTION(3);
    ADD_TIME_POPUP_OPTION(6);
    ADD_TIME_POPUP_OPTION(12);
}

BUTTON_CB(MusicSelector)
{
    if(PopupMgr::showingDialog) return;

    PopupMgr::showDialog();

    PopupMgr::addDialogOption("Enabled", updateMusicSelection, (void *)true, conf.enableMusic);
    PopupMgr::addDialogOption("Disabled", updateMusicSelection, (void *)false, !conf.enableMusic);
}

BUTTON_CB(slidebarRet)
{
}

BUTTON_CB(SlideBarTest)
{
    BlankPage *page = new BlankPage();

    SlideBar *slidebar = (SlideBar *)page->AddFromStyle("slidebartest", "_common_default_style_slidebar", "slidebar", page->root);
    Utils::SetWidgetSize(slidebar, 544, 80);
    
    SceFVector4 pos;
    pos.x = 0;
    pos.y = 0;
    pos.w = 0;
    pos.z = 0;
    slidebar->SetPosition(&pos);

    eventcb cb;
    cb.Callback = slidebarRet;
    cb.dat = slidebar;

    EventHandler *eh = new EventHandler();
    eh->pUserData = sce_paf_malloc(sizeof(cb));
    sce_paf_memcpy(eh->pUserData, &cb, sizeof(cb));

    slidebar->RegisterEventCallback(2, eh, 0);

}

#ifdef _DEBUG

BUTTON_CB(UpdateSelectedIcons)
{
    conf.enableIcons = (bool)userDat;
    WriteConfig(&conf);
}

BUTTON_CB(EnableIconsSelector)
{
    if(PopupMgr::showingDialog) return;

    PopupMgr::showDialog();
    PopupMgr::addDialogOption("Enabled", UpdateSelectedIcons, (void *)true, conf.enableIcons);
    PopupMgr::addDialogOption("Disabled", UpdateSelectedIcons, (void *)false, !conf.enableIcons);
}

BUTTON_CB(UpdateSelectedScreenshots)
{
    conf.enableScreenshots = (bool)userDat;
    WriteConfig(&conf);
}

BUTTON_CB(EnableScreenshotsSelector)
{
    if(PopupMgr::showingDialog) return;

    PopupMgr::showDialog();
    PopupMgr::addDialogOption("Enabled", UpdateSelectedScreenshots, (void *)true, conf.enableScreenshots);
    PopupMgr::addDialogOption("Disabled", UpdateSelectedScreenshots, (void *)false, !conf.enableScreenshots);
}

BUTTON_CB(DebugPage)
{
    SelectionList *p = new SelectionList("★DEBUG");
    p->busy->Stop();

    p->AddOption("Enable Icons", EnableIconsSelector);
    p->AddOption("Enable Screenshots", EnableScreenshotsSelector);
}

#endif
void SettingsButtonEventHandler::onGet(SceInt32, Widget*, SceInt32, ScePVoid)
{
    SelectionList *settingsPage = new SelectionList("Settings");
    settingsPage->AddOption("Source", DBSelector);
    settingsPage->AddOption("Download Icons After", IconTimeSelector);
    settingsPage->AddOption("Enable Music", MusicSelector);

    #ifdef _DEBUG
    //settingsPage->AddOption("Slidebar Test", SlideBarTest);
    settingsPage->AddOption("★DEBUG", DebugPage);
    #endif
    settingsPage->busy->Stop();
} 