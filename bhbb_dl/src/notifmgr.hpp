#ifndef NOTIFMGR_HPP
#define NOTIFMGR_HPP

#include <kernel.h>
#include <paf.h>

#define PROGRESS_EVT_CANCEL 0x00000001

extern "C" void charToWchar(const wchar_t *dest, const char *src);
class NotifMgr
{
private:
    static wchar_t currProgressNotifText[64];
    static wchar_t currProgressNotifSubText[64];
public:
    static bool currDlCanceled;
    static void SendNotif(const wchar_t *text);
    static void SendNotif(const char *text);
    static void MakeProgressNotif(const wchar_t *mainText, const wchar_t *subText, wchar_t *cancelText);
    static void MakeProgressNotif(const char *mainText, const char *subText, const char *cancelText);
    static void UpdateProgressNotif(float val, const char *subText = NULL, const char *text = NULL);
    static void Init();
    static void EndNotif(const char *txt, const char *subtxt);
};

#endif