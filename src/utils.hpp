#ifndef BHBB_UTILS_HPP
#define BHBB_UTILS_HPP
#include <paf.h>
#include <power.h>
#include "eventhandler.hpp"

using namespace paf;
using namespace widget;

#include <curl/curl.h>

typedef SceVoid (*ThreadCB)(void *);

class UtilThread : public paf::thread::Thread
{
private:
    void *ParentPage;
public:
    bool EndThread;

    using thread::Thread::Thread;
    SceVoid EntryFunction();

    SceVoid Kill();
    SceVoid Delete();
    ThreadCB Entry;

    UtilThread::UtilThread(ThreadCB entry = SCE_NULL, const char *pName = "BHBB_PAGE_THREAD");
};

namespace BHBB
{
    class Utils
    {
    private:
        static CURL *curl;
        static SceInt32 currStok;
    public:
        static bool isDirEmpty(const char *path);
        static void OverClock();
        static void UnderClock();
        static void MakeDataDirs();
        static void NetInit();
        static void StartBGDL();
        static int getStrtokNum(char splitter, char *str);
        static char *strtok(char splitter, char *str);
        static void ResetStrtok();
        static void ToLowerCase(char *string);
        static bool StringContains(char *str1, char *str2);
        static SceUInt32 GetHashById(const char *id);    
        static Resource::Element GetParamWithHashFromId(const char *id);
        static Resource::Element GetParamWithHash(SceUInt32 hash);
        static Widget::Color makeSceColor(float r, float g, float b, float a);
        static Widget *GetChildByHash(Widget *parent, SceUInt32 hash);
        static Widget *AddWidgetFromTemplate(Widget *targetRoot, const char *id);
        static SceInt32 DownloadFile(const char *url, const char *destination, ProgressBar *progressBar = NULL);
        static SceInt32 SetWidgetLabel(Widget *widget, const char *text);
        static SceInt32 SetWidgetLabel(Widget *widget, String *text);
        static SceInt32 SetWidgetPosition(Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0, SceFloat w = 0);
        static SceInt32 SetWidgetSize(Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0.0f, SceFloat w = 0.0f);
        static SceInt32 SetWidgetColor(Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a);
        static SceInt32 AssignButtonHandler(Widget *widget, ECallback onPress = SCE_NULL, void *userDat = SCE_NULL, int id = ON_PRESS_EVENT_ID);
        static SceVoid DeleteTexture(graphics::Texture *tex, bool DeletePointer = true);
        static SceBool CreateTextureFromFile(graphics::Texture *tex, const char *file);
        static SceVoid DeleteWidget(Widget *widget);
        static SceBool TestTexture(const char *path);

    #ifdef _DEBUG
        static SceVoid PrintAllChildren(Widget *widget, int offset = 0);
    #endif

    };
}

#endif