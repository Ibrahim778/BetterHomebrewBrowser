#ifndef BHBB_UTILS_HPP
#define BHBB_UTILS_HPP
#include <paf.h>
#include <power.h>
#include "eventhandler.hpp"

using namespace paf;
using namespace widget;

#include <curl/curl.h>

class UtilThread : public paf::thread::Thread
{
public:
    bool EndThread;

    using thread::Thread::Thread;
    SceVoid EntryFunction();

    SceVoid Kill();
    SceVoid Delete();
    SceVoid (*Entry)(void);
};

class Utils
{
private:
    static CURL *curl;
    static SceInt32 currStok;
public:
    static void OverClock();
    static void UnderClock();
    static void MakeDataDirs();
    static void NetInit();
    static void StartBGDL();
    static int getStrtokNum(char splitter, char *str);
    static char *strtok(char splitter, char *str);
    static void ResetStrtok();
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
    static SceVoid DeleteTexture(graphics::Texture *tex);
    static SceBool CreateTextureFromFile(graphics::Texture *tex, const char *file);

#ifdef _DEBUG
    static SceVoid PrintAllChildren(Widget *widget, int offset = 0);
#endif

};

extern "C" bool isDirEmpty(const char *path);

#define makeSceColorInt(r,g,b,a) Utils::makeSceColor(r##.0f, g##.0f, b##.0f, a##.0f)

#endif