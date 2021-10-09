#include "notifmgr.hpp"
#include <notification_util.h>
#include "main.hpp"
#include "list.hpp"

extern Queue queue;

bool NotifMgr::currDlCanceled = SCE_FALSE;
wchar_t NotifMgr::currProgressNotifSubText[64] = {0};
wchar_t NotifMgr::currProgressNotifText[64] = {0};

void cancelCallback(void *userData) 
{
    NotifMgr::currDlCanceled = true;
}

void NotifMgr::Init()
{
    currDlCanceled = false;
}

void NotifMgr::EndNotif(const char *txt, const char *subtxt)
{
    SceNotificationUtilProgressFinishParam p = {0};
    charToWchar((wchar_t *)p.text, txt);
    charToWchar((wchar_t *)p.subText, subtxt);
    sceNotificationUtilProgressFinish(&p);
}

void NotifMgr::UpdateProgressNotif(float val, const char *subText, const char *text)
{
    SceNotificationUtilProgressUpdateParam p = {0};

    if(subText != NULL)
        charToWchar((wchar_t *)p.subText, subText);
    if(text != NULL)
        charToWchar((wchar_t *)p.text, text);

    p.progress = val;
    sceNotificationUtilProgressUpdate(&p);
}

void NotifMgr::MakeProgressNotif(const char *mainText, const char *subText, const char *cancelText)
{
    charToWchar(currProgressNotifSubText, subText);
    charToWchar(currProgressNotifText, mainText);

    SceNotificationUtilProgressInitParam pinit = {0};
    pinit.cancelCallback = cancelCallback;
    SET_TEXT(pinit.text, currProgressNotifText);
    SET_TEXT(pinit.subText, currProgressNotifSubText);
    charToWchar((wchar_t *)pinit.cancelText, cancelText);

    sceNotificationUtilProgressBegin(&pinit);
}


void NotifMgr::MakeProgressNotif(const wchar_t *mainText, const wchar_t *subText, wchar_t *cancelText)
{
    SET_TEXT(currProgressNotifSubText, subText);
    SET_TEXT(currProgressNotifText, mainText);

    SceNotificationUtilProgressInitParam pinit = {0};
    pinit.cancelCallback = cancelCallback;
    SET_TEXT(pinit.text, currProgressNotifText);
    SET_TEXT(pinit.subText, currProgressNotifSubText);
    SET_TEXT(pinit.cancelText, cancelText);

    sceNotificationUtilProgressBegin(&pinit);
}

void NotifMgr::SendNotif(const wchar_t *text)
{
    SceNotificationUtilSendParam s = {0};
    SET_TEXT(s.text, text);
    sceNotificationUtilSendNotification(&s);
}

void NotifMgr::SendNotif(const char *text)
{
    SceNotificationUtilSendParam s = {0};
    charToWchar((wchar_t *)s.text, text);
    sceNotificationUtilSendNotification(&s);
}

void charToWchar(const wchar_t *dest, const char *src)
{
    int len = sce_paf_strlen(src) + 1;
    for(int i = 0; i < len; i++)
    {
       ((wchar_t *)dest)[i] = src[i];
    }
}