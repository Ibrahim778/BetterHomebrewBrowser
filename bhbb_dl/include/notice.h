#ifndef NOTICE_H
#define NOTICE_H

#include <paf.h>

//Based off of reversing done by @Princess-of-Sleeping and @Rinnegatamante along with field information in app.db
class SceLsdbNotificationParam 
{
public:

    // 0x3 = Download progress
    enum Type
    {
        AppInstallFailed = 0, // Happened when int wasnt set
        LiveAreaRefreshed = 0x900,
        AppInstalledSuccessfully = 0x52,
        DownloadComplete = 0x51,
        Custom = 0x100, // One single line of text (desc, or title, desc takes priority, used by SCE for testing?)
        UserDefined = 0x102, // type used by sceNotificationUtilSendNotification (same as Custom??)
    };

    enum Action
    {
        AppBound        = 1,
        AppOpen         = 2, // Opens LA
        AppHighlight    = 3,
        Unk             = 0xb // ?? found in RE (possibly some app launch)
    };// Other known: 0x8

    SceLsdbNotificationParam()
    {
        sce_paf_memset(unk, 0, sizeof(unk));
        sce_paf_memset(unk1, 0xFF, sizeof(unk1));
        sce_paf_memset(unk2, 0, sizeof(unk2));
        sce_paf_memset(unk3, 0, sizeof(unk3));

        msg_type = (Type)0;
        iunk = 0;
        new_flag = 0;
        display_type = 0;
        iUnk2 = 0;
        action_type = (Action)0;
        exec_mode = 0;
    }

    paf::string     title_id; // Usually titleID of the caller, can sometimes be empty
    paf::string     item_id; // Possibly paf::IDParam?
    int             unk[2]; // del flag? (0) 
    Type            msg_type; //see (@Type) 
    int             iunk; // unused? always 0
    Action          action_type; //action on press (see @Action)
    SceByte8        new_flag; // Blue highlight when opening centre
    SceByte8        display_type; // << possibly popup_no << 0 = no popup & no highlight in notif centre 1 = popup & no highlight, if 1 when @new_flag = 0 then popup
    SceChar8        unk1[2]; // Set to 0xFF
    paf::string     icon_path;
    int             unk2[2]; 
    //Some of these are covnerted to int's via sce_paf_atoi
    paf::string     msg_arg0; 
    paf::string     msg_arg1;
    paf::string     msg_arg2;
    paf::string     msg_arg3;
    paf::string     msg_arg4;
    paf::string     msg_arg5;
    paf::string     msg_arg6;
    paf::string     msg_arg7; // int (error code)
    paf::string     msg_arg8; // int
    paf::string     title;  
    paf::string     desc;
    int             exec_mode; 
    paf::string     exec_titleid; // basically work titleid. Will be used depending on opt (APP_HIGHTLIGHT or APP_OPEN) and will use it's icon if none specified in icon_path
    paf::string     exec_arg; //Launch arguments for the app upon launch
    paf::string     icon_data; // unverified
    int             unk3[3];
    int             iUnk2; //Princess set this to 0x10 ¿:/? (0 also works)
};

SCE_CDECL_BEGIN

int sceLsdbSendNotification(SceLsdbNotificationParam *param, int allowReplace);

SCE_CDECL_END

#endif