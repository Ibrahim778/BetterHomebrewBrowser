#ifndef VHH_EVENTHANDLER_HPP
#define VHH_EVENTHANDLER_HPP

#include <paf.h>
#include <kernel.h>
using namespace paf;
using namespace widget;

class EventHandler : public Widget::EventCallback
{
public:
    EventHandler();

    static void onGet(SceInt32 , Widget *self, SceInt32, ScePVoid puserData);
};

class BackButtonEventHandler : public Widget::EventCallback
{
public:
    BackButtonEventHandler();

    static void onGet(SceInt32 , Widget *self, SceInt32, ScePVoid puserData);
};

class SettingsButtonEventHandler : public Widget::EventCallback
{
public:
    SettingsButtonEventHandler();

    static void onGet(SceInt32 , Widget *self, SceInt32, ScePVoid puserData);
};

class DiagButtonEventHandler : public Widget::EventCallback
{
public:
    DiagButtonEventHandler();

    static void onGet(SceInt32 , Widget *self, SceInt32, ScePVoid puserData);
};

#define ON_PRESS_EVENT_ID 0x10000008

typedef void (*ECallback)(Widget *, SceInt32, void*);;

typedef struct 
{
    ECallback Callback;
    void *dat;
} eventcb;

#endif