#ifndef NOTIFMGR_HPP
#define NOTIFMGR_HPP

#include <kernel.h>

#define PROGRESS_EVT_CANCEL 0x00000001


SCE_CDECL_BEGIN

extern SceBool NotifMgr_currDlCanceled;

void NotifMgr_SendWNotif(const wchar_t *text);
void NotifMgr_SendNotif(const char *text);
void NotifMgr_MakeWProgressNotif(const wchar_t *mainText, const wchar_t *subText, wchar_t *cancelText);
void NotifMgr_MakeProgressNotif(const char *mainText, const char *subText, const char *cancelText);
void NotifMgr_UpdateProgressNotif(float val, const char *subText, const char *text);
void NotifMgr_Init();
void NotifMgr_EndNotif(const char *txt, const char *subtxt);

int charToWchar(SceWChar16 *utf16, const char *utf8, size_t len);

SCE_CDECL_END

#endif