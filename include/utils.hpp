#ifndef BHBB_UTILS_HPP
#define BHBB_UTILS_HPP

#include <paf.h>
#include <message_dialog.h>

typedef SceVoid (*ThreadCB)(void *);

class UtilThread : public paf::thread::Thread
{
private:
    ThreadCB Entry;
    void *parentPage;
public:
    bool End;

    using paf::thread::Thread::Thread;
    SceVoid EntryFunction();

    SceVoid Kill();
    SceVoid Delete();

    SceInt32 StartTask(ThreadCB task = NULL);

    UtilThread(ThreadCB Entry, void *callingPage, const char *name = "BHBB_PAGE_THREAD", SceInt32 initPriority = SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SceSize stackSize = SCE_KERNEL_16KiB);
};

namespace Utils
{
    bool isDirEmpty(const char *path);
    void MakeDataDirs();
    void NetInit();
    void NetTerm();
    void StartBGDL();
    int getStrtokNum(char splitter, char *str);
    char *strtok(char splitter, char *str);
    void ResetStrtok();
    void ToLowerCase(char *string);
    bool stringContains(char *string, char *contains);
    void InitMusic();
    void SetMemoryInfo();
    SceVoid GetStringFromID(const char *id, paf::string *out);
    SceVoid GetfStringFromID(const char *id, paf::string *out);
    SceUInt32 GetHashById(const char *id);    
    paf::Resource::Element GetParamWithHashFromId(const char *id);
    paf::Resource::Element GetParamWithHash(SceUInt32 hash);
    paf::ui::Widget *GetChildByHash(paf::ui::Widget *parent, SceUInt32 hash);
    SceInt32 DownloadFile(const char *url, const char *destination, void *callingPage, paf::ui::ProgressBar *progressBar = NULL);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, const char *text);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, paf::string *text);
    SceInt32 SetWidgetPosition(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0, SceFloat w = 0);
    SceInt32 SetWidgetSize(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0.0f, SceFloat w = 0.0f);
    SceInt32 SetWidgetColor(paf::ui::Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a);
    SceVoid DeleteTexture(paf::graphics::Surface **tex, bool DeletePointer = true);
    SceBool CreateTextureFromFile(paf::graphics::Surface **tex, const char *file);
    SceVoid DeleteWidget(paf::ui::Widget *widget);
    SceBool TestTexture(const char *path);

#ifdef _DEBUG
    SceVoid PrintAllChildren(paf::ui::Widget *widget, int offset = 0);
#endif

    namespace MsgDialog
    {
        void MessagePopup(const char *message, SceMsgDialogButtonType buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK);
        void MessagePopupFromID(const char *messageID,  SceMsgDialogButtonType buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK);
        void SystemMessage(SceMsgDialogSystemMessageType type);
        void EndMessage();
    };

};
#endif