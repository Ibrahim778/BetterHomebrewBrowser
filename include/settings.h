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

    enum Hash
    {
        Hash_Refresh = 0xDF43DB7A,
        Hash_nLoad = 0xA7E3A711,
        Hash_Source = 0x92EFFF4E,
    };

    Settings();
    ~Settings();

    static Settings *GetInstance();
    static sce::AppSettings *GetAppSettings();
    SceVoid Open();
    SceVoid Close();


    db::Id source;
    int nLoad;

private:
    static sce::AppSettings *appSettings;

    static SceVoid CBListChange(const char *elementId);

    static SceVoid CBListForwardChange(const char *elementId);

    static SceVoid CBListBackChange(const char *elementId);

    static SceInt32 CBIsVisible(const char *elementId, SceBool *pIsVisible);

    static SceInt32 CBElemInit(const char *elementId);

    static SceInt32 CBElemAdd(const char *elementId, paf::ui::Widget *widget);

    static SceInt32 CBValueChange(const char *elementId, const char *newValue);

    static SceInt32 CBValueChange2(const char *elementId, const char *newValue);

    static SceVoid CBTerm();

    static wchar_t *CBGetString(const char *elementId);

    static SceInt32 CBGetTex(paf::graph::Surface **tex, const char *elementId);


    const int d_settingsVersion = 1;
    const int d_source = db::CBPSDB;
    const int d_nLoad = 50;
};

#endif