#ifndef BHBB_UTILS_HPP
#define BHBB_UTILS_HPP

#include <paf.h>
#include <power.h>
#include "eventhandler.hpp"

typedef SceVoid (*JobCB)(void *);

using namespace paf;
using namespace ui;

#include <curl/curl.h>

class UtilQueue : public thread::JobQueue
{
private:
    void *parentPage;
public:
    int num;
    bool End;

    using thread::JobQueue::JobQueue;

    SceVoid AddTask(JobCB Task, const char *name);
    SceBool WaitingTasks();

    UtilQueue(void *callingPage, const char *name = "BHBB_UTIL_QUEUE");
    ~UtilQueue();
};

class UtilJob : public thread::JobQueue::Item
{
private:
    JobCB task;
    void *parentPage;
    UtilQueue *caller;

public:
    using thread::JobQueue::Item::Item;

    UtilJob(JobCB task, void *callingPage, UtilQueue *caller, const char *name = "BHBB_UTIL_JOB");
    ~UtilJob() {}

    SceVoid Run();
    SceVoid Finish();

    static SceVoid JobKiller(thread::JobQueue::Item *job);
};



namespace Utils
{
    bool isDirEmpty(const char *path);
    void OverClock();
    void UnderClock();
    void MakeDataDirs();
    void NetInit();
    void StartBGDL();
    int getStrtokNum(char splitter, char *str);
    char *strtok(char splitter, char *str);
    void ResetStrtok();
    void ToLowerCase(char *string);
    bool StringContains(char *str1, char *str2);
    SceUInt32 GetHashById(const char *id);    
    Resource::Element GetParamWithHashFromId(const char *id);
    Resource::Element GetParamWithHash(SceUInt32 hash);
    Widget::Color makeSceColor(float r, float g, float b, float a);
    Widget *GetChildByHash(Widget *parent, SceUInt32 hash);
    SceInt32 DownloadFile(const char *url, const char *destination, void *callingPage, ProgressBar *progressBar = NULL);
    SceInt32 SetWidgetLabel(Widget *widget, const char *text);
    SceInt32 SetWidgetLabel(Widget *widget, String *text);
    SceInt32 SetWidgetPosition(Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0, SceFloat w = 0);
    SceInt32 SetWidgetSize(Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0.0f, SceFloat w = 0.0f);
    SceInt32 SetWidgetColor(Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a);
    SceInt32 AssignButtonHandler(Widget *widget, ECallback onPress = SCE_NULL, void *userDat = SCE_NULL, int id = ON_PRESS_EVENT_ID);
    SceVoid DeleteTexture(graphics::Texture *tex, bool DeletePointer = true);
    SceBool CreateTextureFromFile(graphics::Texture *tex, const char *file);
    SceVoid DeleteWidget(Widget *widget);
    SceBool TestTexture(const char *path);

#ifdef _DEBUG
    SceVoid PrintAllChildren(Widget *widget, int offset = 0);
#endif

};
#endif