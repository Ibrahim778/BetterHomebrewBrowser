#ifndef SETTINGS_MGR_H
#define SETTINGS_MGR_H

#include <kernel.h>
#include <app_settings.h>
#include <paf.h>

#include "db/source.h"
#include "bhbb_locale.h"

class Settings
{
public:
    enum
    {
        SettingsEvent = (paf::ui::Handler::CB_STATE + 0x30000),
    };

    enum SettingsEvent
    {
        SettingsEvent_ValueChange
    };

    Settings();
    ~Settings();

    static Settings *GetInstance();
    static sce::AppSettings *GetAppSettings();
    void Open();
    void Close();

    static void OpenCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *data);

    int source;

private:
    static sce::AppSettings *appSettings;

    static void CBOnStartPageTransition(const char *elementId, int32_t type);

    static void CBOnPageActivate(const char *elementId, int32_t type);

    static void CBOnPageDeactivate(const char *elementId, int32_t type);

    static int32_t CBOnCheckVisible(const char *elementId, int *pIsVisible);

    static int32_t CBOnPreCreate(const char *elementId, sce::AppSettings::Element *element);

    static int32_t CBOnPostCreate(const char *elementId, paf::ui::Widget *widget);

    static int32_t CBOnPress(const char *elementId, const char *newValue);

    static int32_t CBOnPress2(const char *elementId, const char *newValue);

    static void CBOnTerm(int32_t result);

    static wchar_t *CBOnGetString(const char *elementId);

    static int32_t CBOnGetSurface(paf::graph::Surface **surf, const char *elementId);

    const int d_settingsVersion = 2; 
    const int d_source = Source::VITA_DB;
};

#endif