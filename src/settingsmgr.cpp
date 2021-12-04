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

BUTTON_CB(DBSelector)
{
    if(PopupMgr::showingDialog) return;
    PopupMgr::ShowDialog();
    PopupMgr::AddDialogOption("CBPS DB", updateDBType, (void *)CBPSDB, conf.db == CBPSDB);
    PopupMgr::AddDialogOption("Vita DB", updateDBType, (void *)VITADB, conf.db == VITADB);
}

BUTTON_CB(SetVitaDbDownloadIconsOnLoad)
{
    conf.vitaDBSettings.downloadIconsDuringLoad = (bool)userDat;
    WriteConfig(&conf);
}

BUTTON_CB(SetCBPSDbDownloadIconsOnLoad)
{
    conf.CBPSDBSettings.downloadIconsDuringLoad = (bool)userDat;
    WriteConfig(&conf);
}

BUTTON_CB(VitaDBOnLoadSetter)
{
    if(PopupMgr::showingDialog) return;
    PopupMgr::ShowDialog();

    PopupMgr::AddDialogOption("Enabled", SetVitaDbDownloadIconsOnLoad, (void *)true, conf.vitaDBSettings.downloadIconsDuringLoad);
    PopupMgr::AddDialogOption("Disabled", SetVitaDbDownloadIconsOnLoad, (void *)false, !conf.vitaDBSettings.downloadIconsDuringLoad);

}

BUTTON_CB(CBPSDBOnLoadSetter)
{
    if(PopupMgr::showingDialog) return;
    PopupMgr::ShowDialog();

    PopupMgr::AddDialogOption("Enabled", SetCBPSDbDownloadIconsOnLoad, (void *)true, conf.CBPSDBSettings.downloadIconsDuringLoad);
    PopupMgr::AddDialogOption("Disabled", SetCBPSDbDownloadIconsOnLoad, (void *)false, !conf.CBPSDBSettings.downloadIconsDuringLoad);

}

BUTTON_CB(CBPSDBSettings)
{
    SelectionList *page = new SelectionList("CBPS DB Settings");
    page->busy->Stop();

    page->AddOption("Download Icons During Load", CBPSDBOnLoadSetter);
}

BUTTON_CB(VitaDBSettings)
{
    SelectionList *page = new SelectionList("Vita DB Settings");
    page->busy->Stop();

    page->AddOption("Download Icons During Load", VitaDBOnLoadSetter);
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
    new TextPage("Better Homebrew Browser By M Ibrahim\nScePaf Reversing By Graphene\nLivearea Assets By SomonPC\nV0.61B", "Credits");
}

BUTTON_CB(FixesPage)
{
    new TextPage("1. Fix Extraction on some apps\n2. Add Plugins\n3. Add Themes", "Upcoming Fixes and Features");
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
    SelectionList *s = new SelectionList("Other");
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
    
    BHBB::Utils::SetWidgetColor(IconInfo, 1,1,1,1);
    BHBB::Utils::SetWidgetSize(IconInfo, 544, 80);
    BHBB::Utils::SetWidgetPosition(IconInfo, 0, 40);

    char IconText[16];
    sce_paf_memset(IconText, 0, 16);
    sce_paf_snprintf(IconText, 15, "Icons: %s", loadFlags & LOAD_FLAGS_ICONS ? "Enabled" : "Disabled");
    BHBB::Utils::SetWidgetLabel(IconInfo, IconText);

    Text *ScreenShotInfo = (Text *)page->AddFromStyle("BHBBScreenShotInfo", "_common_default_style_text", "text");

    BHBB::Utils::SetWidgetColor(ScreenShotInfo, 1,1,1,1);
    BHBB::Utils::SetWidgetSize(ScreenShotInfo, 544, 80);
    BHBB::Utils::SetWidgetPosition(ScreenShotInfo, 0, -40);

    char ScreenshotText[22];
    sce_paf_memset(ScreenshotText, 0, 22);
    sce_paf_snprintf(ScreenshotText, 21, "Screenshots: %s\n", (loadFlags & LOAD_FLAGS_SCREENSHOTS) ? "Enabled" : "Disabled");
    BHBB::Utils::SetWidgetLabel(ScreenShotInfo, ScreenshotText);
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