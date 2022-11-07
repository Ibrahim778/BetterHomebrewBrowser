#include "notifmgr.h"
#include <vdsuite/notification_util.h>
#include <stdbool.h>
#include <psp2/paf.h>
#include <psp2/kernel/clib.h>

#include <psp2/kernel/clib.h>
#include "print.h"

static SceWChar16 currProgressNotifText[64];
static SceWChar16 currProgressNotifSubText[64];

bool NotifMgr_currDlCanceled = false;

#define SET_TEXT(dest, src) sce_paf_wcsncpy(((wchar_t *)(dest)), ((wchar_t *)(src)), sizeof((dest)))
void cancelCallback(void *userData) 
{
    NotifMgr_currDlCanceled = true;
}

void NotifMgr_Init()
{
    NotifMgr_currDlCanceled = false;
}

void NotifMgr_EndNotif(const char *txt, const char *subtxt)
{
    SceNotificationUtilProgressFinishParam p;
    sce_paf_memset(&p, 0, sizeof(SceNotificationUtilProgressFinishParam));
    charToWchar(p.text, txt, sizeof(p.text));
    charToWchar(p.subText, subtxt, sizeof(p.subText));
    sceNotificationUtilProgressFinish(&p);
}

void NotifMgr_UpdateProgressNotif(float val, const char *subText, const char *text)
{
    SceNotificationUtilProgressUpdateParam p;
    sce_paf_memset(&p, 0, sizeof(SceNotificationUtilProgressUpdateParam));

    if(subText != NULL)
        charToWchar(p.subText, subText, sizeof(p.subText));
    if(text != NULL)
        charToWchar(p.text, text, sizeof(p.text));

    p.progress = val;
    sceNotificationUtilProgressUpdate(&p);
}

void NotifMgr_MakeProgressNotif(const char *mainText, const char *subText, const char *cancelText)
{
    print("Doing c to wc\n");
    charToWchar(currProgressNotifSubText, subText, sizeof(currProgressNotifSubText));
    charToWchar(currProgressNotifText, mainText, sizeof(currProgressNotifText));
    print("Done c to wc\n");
    SceNotificationUtilProgressInitParam pinit;
    sce_paf_memset(&pinit, 0, sizeof(SceNotificationUtilProgressInitParam));
    pinit.cancelCallback = cancelCallback;
    SET_TEXT(pinit.text, currProgressNotifText);
    print("Done first settext");
    SET_TEXT(pinit.subText, currProgressNotifSubText);
    charToWchar(pinit.cancelText, cancelText, sizeof(pinit.cancelText));

    print("Begin...\n");

    sceNotificationUtilProgressBegin(&pinit);
    print("Done!\n");
}


void NotifMgr_MakeWProgressNotif(const wchar_t *mainText, const wchar_t *subText, wchar_t *cancelText)
{
    SET_TEXT(currProgressNotifSubText, subText);
    SET_TEXT(currProgressNotifText, mainText);

    SceNotificationUtilProgressInitParam pinit;
    sce_paf_memset(&pinit, 0, sizeof(SceNotificationUtilProgressInitParam));
    pinit.cancelCallback = cancelCallback;
    SET_TEXT(pinit.text, currProgressNotifText);
    SET_TEXT(pinit.subText, currProgressNotifSubText);
    SET_TEXT(pinit.cancelText, cancelText);

    sceNotificationUtilProgressBegin(&pinit);
}

void NotifMgr_SendWNotif(const wchar_t *text)
{
    SceNotificationUtilSendParam s;
    sce_paf_memset(&s, 0, sizeof(SceNotificationUtilSendParam));
    SET_TEXT(s.text, text);
    sceNotificationUtilSendNotification(&s);
}

void NotifMgr_SendNotif(const char *text)
{
    SceNotificationUtilSendParam s;
    sce_paf_memset(&s, 0, sizeof(SceNotificationUtilSendParam));
    charToWchar(s.text, text, sizeof(s.text));
    sceNotificationUtilSendNotification(&s);
}

int charToWchar(SceWChar16 *utf16, const char *utf8, size_t len)
{
    int i = 0;
    for(; i < len && utf8[i] != '\0'; i++)
        utf16[i] = utf8[i];        
    
    utf16[i] = '\0';
    return len;
}