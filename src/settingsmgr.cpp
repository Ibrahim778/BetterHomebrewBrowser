#include "main.hpp"
#include "common.hpp"
#include "eventhandler.hpp"
#include "configmgr.hpp"
#include "pagemgr.hpp"
#include "utils.hpp"

extern userConfig conf;

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

//in main.cpp
extern void updateDarkMode();

BUTTON_CB(updateDarkMode)
{
    conf.darkMode = (bool)userDat;
    WriteConfig(&conf);
    updateDarkMode();
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

BUTTON_CB(DarkModeSelector)
{
    if(PopupMgr::showingDialog) return;

    PopupMgr::showDialog();

    PopupMgr::addDialogOption("Enabled", updateDarkMode, (void *)true, conf.darkMode);
    PopupMgr::addDialogOption("Disabled", updateDarkMode, (void *)false, !conf.darkMode);
}

void SettingsButtonEventHandler::onGet(SceInt32, Widget*s, SceInt32, ScePVoid)
{
    SelectionList *settingsPage = new SelectionList("Settings");
    settingsPage->AddOption("Source", DBSelector);
    settingsPage->AddOption("Download Icons After", IconTimeSelector);
    settingsPage->AddOption("Dark Mode", DarkModeSelector);
    settingsPage->busy->Stop();
} 