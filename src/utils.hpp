#ifndef VHH_UTILS_HPP
#define VHH_UTILS_HPP
#include <paf.h>
#include <power.h>
using namespace paf;
using namespace widget;

#include <curl/curl.h>

class UtilThread : public paf::thread::Thread
{
public:
    bool EndThread;

    using thread::Thread::Thread;
    SceVoid EntryFunction();

    SceVoid (*Entry)(void);
};

class Utils
{
private:
    static CURL *curl;
public:
    static void OverClock();
    static void UnderClock();
    static void MakeDataDirs();
    static void NetInit();
    static SceUInt32 GetHashById(const char *id);    
    static Resource::Element GetParamWithHashFromId(const char *id);
    static Resource::Element GetParamWithHash(SceUInt32 hash);
    static Widget::Color makeSceColor(float r, float g, float b, float a);
    static Widget *GetChildByHash(Widget *parent, SceUInt32 hash);
    static SceInt32 DownloadFile(const char *url, const char *destination, ProgressBar *progressBar = NULL);
    static SceInt32 SetWidgetLabel(Widget *widget, const char *text);
};

extern "C" bool checkFileExist(const char *);

#define makeSceColorInt(r,g,b,a) Utils::makeSceColor(r##.0f, g##.0f, b##.0f, a##.0f)

#endif