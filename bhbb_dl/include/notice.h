#ifndef NOTICE_H
#define NOTICE_H

#include <kernel.h>
#include <paf.h>

enum SceLsdbNotificationAction
{
    AppOpen         = 2,
    AppHighlight    = 3
};
enum SceLsdbNotificationPreset
{
    LiveAreaRefreshed = 0x900,
    AppInstalledSuccessfully = 0x52,
    Custom = 0x100, //Maybe idk. One single line of text.
};

//Based off of reversing done by @Princess-of-Sleeping and @Rinnegatamante
struct SceLsdbNotificationParam {
    paf::string     ownerTitleID; // Usually titleID of the caller
    paf::string     unkStr_Category; //Could be category or smth idk, known used: "g00", "LAUPDATE" and "0", also seen "LOGSTATUS0"
    SceInt32        unk[2]; //Set to 0xFF
    SceInt32        preset; //see (SceLsdbNotificationPreset) 
    SceInt32        iUnk0;
    SceUInt32       action; //action on press (see SceLsdbNotificationAction)
    SceByte8        bUnk0; //always set to 1, but 2 and 3 also work
    SceByte8        bUnk1;
    SceChar8        unk1[2];
    paf::string     iconPath;
    SceInt32        unk2[2];
    //Some of these are covnerted to int's via sce_paf_atoi
    paf::string     unkStr1;
    paf::string     unkStr2;
    paf::string     unkStr3;
    paf::string     unkStr4;
    paf::string     unkStr5;
    paf::string     unkStr6;
    paf::string     unkStr7;
    paf::string     unkStr8;
    paf::string     unkStr9;
    paf::string     unkStr10;
    paf::string     text;
    SceInt32        launchArgs; //*supposedly* Launch arguments for the app upon launch
    paf::string     titleID; // basically work titleid. Will be used depending on opt (APP_HIGHTLIGHT or APP_OPEN) and will use it's icon if none specified in iconPath
    paf::string     arg;
    paf::string     unkStr11;
    SceInt32        unk3[3];
    SceInt32        iUnk2; //Princess set this to 0x10 ¿:/? (0 also works)

    SceLsdbNotificationParam()
    {
        sce_paf_memset(unk, 0, sizeof(unk));
        sce_paf_memset(unk1, 0xFF, sizeof(unk1));
        sce_paf_memset(unk2, 0, sizeof(unk2));
        sce_paf_memset(unk3, 0, sizeof(unk3));

        preset = 0;
        iUnk2 = 0;
        bUnk1 = 1;
        bUnk0 = 0;
        iUnk0 = 0;
        action = 0;
        launchArgs = 0;
    }
};

SCE_CDECL_BEGIN

// extern SceInt32 sceLsdbSendNotification(sceLsdbNotificationParam *param, SceInt32 unk);

SCE_CDECL_END

#endif