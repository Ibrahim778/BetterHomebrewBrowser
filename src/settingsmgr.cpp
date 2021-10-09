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
extern int loadFlags;

BUTTON_CB(updateDBType)
{
    conf.db = (DB_Type)(int)(userDat);
    WriteConfig(&conf);
}

BUTTON_CB(updateIconTimeCBPSDB)
{
    conf.CBPSDBSettings.iconDownloadHourGap = (short)(int)userDat;
    WriteConfig(&conf);
}

BUTTON_CB(updateIconTimeVitaDB)
{
    conf.vitaDBSettings.iconDownloadHourGap = (short)(int)userDat;
    WriteConfig(&conf);
}

BUTTON_CB(DBSelector)
{
    if(PopupMgr::showingDialog) return;
    PopupMgr::showDialog();
    PopupMgr::addDialogOption("CBPS DB", updateDBType, (void *)CBPSDB, conf.db == CBPSDB);
    PopupMgr::addDialogOption("Vita DB", updateDBType, (void *)VITADB, conf.db == VITADB);
}

#define ADD_TIME_POPUP_OPTION_CBPSDB(hours) PopupMgr::addDialogOption(hours > 1 ? #hours " Hours" : #hours " Hour", updateIconTimeCBPSDB, (void *)hours, conf.CBPSDBSettings.iconDownloadHourGap == hours)

BUTTON_CB(IconTimeSelectorCBPSDB)
{
    if(PopupMgr::showingDialog) return;

    PopupMgr::showDialog();
    
    PopupMgr::addDialogOption("Never", updateIconTimeCBPSDB, (void *)-1, conf.CBPSDBSettings.iconDownloadHourGap == -1);
    PopupMgr::addDialogOption("Every Time", updateIconTimeCBPSDB, (void *)0, conf.CBPSDBSettings.iconDownloadHourGap == 0);

    ADD_TIME_POPUP_OPTION_CBPSDB(1);
    ADD_TIME_POPUP_OPTION_CBPSDB(3);
    ADD_TIME_POPUP_OPTION_CBPSDB(6);
    ADD_TIME_POPUP_OPTION_CBPSDB(12);
}

#define ADD_TIME_POPUP_OPTION_VITADB(hours) PopupMgr::addDialogOption(hours > 1 ? #hours " Hours" : #hours " Hour", updateIconTimeVitaDB, (void *)hours, conf.vitaDBSettings.iconDownloadHourGap == hours)

BUTTON_CB(IconTimeSelectorVITADB)
{
    if(PopupMgr::showingDialog) return;

    PopupMgr::showDialog();
    
    PopupMgr::addDialogOption("Never", updateIconTimeVitaDB, (void *)-1, conf.vitaDBSettings.iconDownloadHourGap == -1);
    PopupMgr::addDialogOption("Every Time", updateIconTimeVitaDB, (void *)0, conf.vitaDBSettings.iconDownloadHourGap == 0);

    ADD_TIME_POPUP_OPTION_VITADB(1);
    ADD_TIME_POPUP_OPTION_VITADB(3);
    ADD_TIME_POPUP_OPTION_VITADB(6);
    ADD_TIME_POPUP_OPTION_VITADB(12);
}

BUTTON_CB(CBPSDBSettings)
{
    SelectionList *page = new SelectionList("CBPS DB Settings");
    page->busy->Stop();
    page->AddOption("Download Icons After", IconTimeSelectorCBPSDB);
}

BUTTON_CB(VitaDBSettings)
{
    SelectionList *page = new SelectionList("Vita DB Settings");
    page->busy->Stop();
    page->AddOption("Download Icons After", IconTimeSelectorVITADB);
}

#ifdef _DEBUG
BUTTON_CB(slidebarRet)
{
    SlideBar *slidebar = (SlideBar *)self;

    int res =(int) slidebar->TypeSlideBar();
    print("Res = 0x%X\n", res);
}

BUTTON_CB(SlideBarTest)
{
    BlankPage *page = new BlankPage();

    SlideBar *slidebar = (SlideBar *)page->AddFromStyle("slidebartest", "_common_default_style_slidebar", "slidebar");
    Utils::SetWidgetSize(slidebar, 544, 80);
    Utils::SetWidgetPosition(slidebar, 0, 0);

    eventcb cb;
    cb.Callback = slidebarRet;
    cb.dat = slidebar;

    EventHandler *eh = new EventHandler();
    eh->pUserData = sce_paf_malloc(sizeof(cb));
    sce_paf_memcpy(eh->pUserData, &cb, sizeof(cb));

    slidebar->RegisterEventCallback(2, eh, 0);
    print("ret = 0x%X\n", slidebar->ScePafWidget_FDB59013(50, 0));
}

BUTTON_CB(DebugPage)
{
    BlankPage *page = new BlankPage();
    page->busy->Stop();

    Text *IconInfo = (Text *)page->AddFromStyle("BHBBIconInfo", "_common_default_style_text", "text");
    
    Utils::SetWidgetColor(IconInfo, 1,1,1,1);
    Utils::SetWidgetSize(IconInfo, 544, 80);
    Utils::SetWidgetPosition(IconInfo, 0, 40);

    char IconText[15] = {0};
    sce_paf_snprintf(IconText, 15, "Icons: %s", loadFlags & LOAD_FLAGS_ICONS ? "Enabled" : "Disabled");
    Utils::SetWidgetLabel(IconInfo, IconText);

    Text *ScreenShotInfo = (Text *)page->AddFromStyle("BHBBScreenShotInfo", "_common_default_style_text", "text");

    Utils::SetWidgetColor(ScreenShotInfo, 1,1,1,1);
    Utils::SetWidgetSize(ScreenShotInfo, 544, 80);
    Utils::SetWidgetPosition(ScreenShotInfo, 0, -40);

    char ScreenshotText[21] = {0};
    sce_paf_snprintf(ScreenshotText, 21, "Screenshots: %s\n", (loadFlags & LOAD_FLAGS_SCREENSHOTS) ? "Enabled" : "Disabled");
    Utils::SetWidgetLabel(ScreenShotInfo, ScreenshotText);
}

#endif

void SettingsButtonEventHandler::onGet(SceInt32, Widget*, SceInt32, ScePVoid)
{
    SelectionList *settingsPage = new SelectionList("Settings");
    settingsPage->AddOption("Source", DBSelector);
    settingsPage->AddOption("Vita DB", VitaDBSettings);
    settingsPage->AddOption("CBPS DB", CBPSDBSettings);

    #ifdef _DEBUG
    //settingsPage->AddOption("Slidebar Test", SlideBarTest);
    settingsPage->AddOption("★DEBUG", DebugPage);
    #endif
    settingsPage->busy->Stop();
} 