#include "eventhandler.hpp"
#include "pagemgr.hpp"

extern Page *currPage;

EventHandler::EventHandler()
{
    eventHandler = onGet;
}

void EventHandler::onGet(SceInt32 , Widget *self, SceInt32, ScePVoid puserData)
{
    eventcb *cb = (eventcb *)puserData;
    if(cb->onPress != NULL) cb->onPress(cb->dat);
}

BackButtonEventHandler::BackButtonEventHandler()
{
    eventHandler = onGet;
}

void BackButtonEventHandler::onGet(SceInt32 , Widget *self, SceInt32, ScePVoid puserData)
{
    currPage->skipAnimation = SCE_FALSE;
    delete currPage;
}

SettingsButtonEventHandler::SettingsButtonEventHandler()
{
    eventHandler = onGet;
}

DiagButtonEventHandler::DiagButtonEventHandler()
{
    eventHandler = onGet;
}

void DiagButtonEventHandler::onGet(SceInt32, Widget *self, SceInt32, ScePVoid pUserData)
{
    eventcb *dat = (eventcb *)pUserData;    
    
    if(dat->onPress != NULL)
        dat->onPress(dat->dat);
    PopupMgr::hideDialog();
}