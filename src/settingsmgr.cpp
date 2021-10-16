#include "main.hpp"
#include "common.hpp"
#include "eventhandler.hpp"
#include "configmgr.hpp"
#include "audiomgr.hpp"
#include "pagemgr.hpp"
#include <bgapputil.h>
#include "utils.hpp"
#include "bgdl.h"

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

BUTTON_CB(DBOptions)
{
    SelectionList *list = new SelectionList("DB Options");
    list->busy->Stop();
    list->AddOption("Vita DB", VitaDBSettings);
    list->AddOption("CBPS DB", CBPSDBSettings);
}

BUTTON_CB(CreditsPage)
{
    new TextPage("Better Homebrew Browser By M Ibrahim\nScePaf Reversing By Graphene\nLivearea Assets By SomonPC", "Credits");
}

BUTTON_CB(FixesPage)
{
    new TextPage("\n1. Fix CBPS DB\n2. Add Sorting\n3. Add Plugins\n4. Add Themes", "Upcoming Fixes and Features");
}

BUTTON_CB(InfoPage)
{
    SelectionList *list = new SelectionList("Info");
    list->busy->Stop();
    list->AddOption("Credits", CreditsPage);
    list->AddOption("Upcoming Fixes and Features", FixesPage);
}

BUTTON_CB(TermBGDL)
{
    termBhbbDl();
    new TextPage("Crashed BGDL Successfully!\nYou can now Update/Delete");
}

BUTTON_CB(OtherPage)
{
    SelectionList *s = new SelectionList("Info");
    s->busy->Stop();
    s->AddOption("Info", InfoPage);
    s->AddOption("Terminate BGDL", TermBGDL);
}

#ifdef _DEBUG

BUTTON_CB(DebugPage)
{
    Page *page = new Page();
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
    settingsPage->AddOption("DB Options", DBOptions);
    settingsPage->AddOption("Other", OtherPage);

    #ifdef _DEBUG
    settingsPage->AddOption("★DEBUG", DebugPage);
    #endif
    settingsPage->busy->Stop();
} 