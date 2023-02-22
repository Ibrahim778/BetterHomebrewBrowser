#ifndef SETTINGS_MGR_H
#define SETTINGS_MGR_H

#include <kernel.h>
#include <app_settings.h>
#include <paf.h>

#include "db.h"

class Settings
{
public:
    class OpenCallback : public paf::ui::EventCallback
    {
    public:
        OpenCallback();

        static SceVoid OnGet(SceInt32 id, paf::ui::Widget *widget, SceInt32 unk, ScePVoid data);
    };

    Settings();
    ~Settings();

    static Settings *GetInstance();
    static sce::AppSettings *GetAppSettings();
    SceVoid Open();
    SceVoid Close();

    db::Id source;
    int downloadInterval;

private:
    static sce::AppSettings *appSettings;

    static SceVoid CBListChange(const char *elementId, SceInt32 type);

    static SceVoid CBListForwardChange(const char *elementId, SceInt32 type);

    static SceVoid CBListBackChange(const char *elementId, SceInt32 type);

    static SceInt32 CBIsVisible(const char *elementId, SceBool *pIsVisible);

    static SceInt32 CBElemInit(const char *elementId, sce::AppSettings::Element *element);

    static SceInt32 CBElemAdd(const char *elementId, paf::ui::Widget *widget);

    static SceInt32 CBValueChange(const char *elementId, const char *newValue);

    static SceInt32 CBValueChange2(const char *elementId, const char *newValue);

    static SceVoid CBTerm(SceInt32 result);

    static wchar_t *CBGetString(const char *elementId);

    static SceInt32 CBGetTex(paf::graph::Surface **tex, const char *elementId);

    const int d_settingsVersion = 4;
    const int d_source = db::CBPSDB;
    const int d_downloadInterval = 6;
};

#endif