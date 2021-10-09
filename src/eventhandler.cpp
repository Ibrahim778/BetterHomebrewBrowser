#include "eventhandler.hpp"
#include "pagemgr.hpp"
#include "main.hpp"

extern Page *currPage;

static ECallback backButtonCB = NULL;
static ECallback forwardButtonCB = NULL;

#define DELETE_PAGE(Type, Class) case Type: { delete (Class *)currPage; break; }

EventHandler::EventHandler()
{
    eventHandler = onGet;
}

void EventHandler::onGet(SceInt32 e, Widget *s, SceInt32, ScePVoid puserData)
{
    eventcb *cb = (eventcb *)puserData;
    if(cb->Callback != NULL) cb->Callback(s, e, cb->dat);
}

BackButtonEventHandler::BackButtonEventHandler()
{
    eventHandler = onGet;
}

void EventHandler::SetBackButtonEvent(ECallback callback)
{
    backButtonCB = callback;
}

void EventHandler::SetForwardButtonEvent(ECallback callback)
{
    forwardButtonCB = callback;
}

void EventHandler::ResetForwardButtonEvent()
{
    forwardButtonCB = NULL;
}

BUTTON_CB(DefaultCB)
{
    currPage->skipAnimation = SCE_FALSE;

    //Just to call the correct destructors, if you have a better way to do this, please fix it
    switch (currPage->type)
    {

    DELETE_PAGE(PAGE_TYPE_SELECTION_LIST, SelectionList);
    DELETE_PAGE(PAGE_TYPE_TEXT_PAGE, TextPage);
    DELETE_PAGE(PAGE_TYPE_TEXT_PAGE_WITH_TITLE, TextPage);
    DELETE_PAGE(PAGE_TYPE_SELECTION_LIST_WITH_TITLE, SelectionList);
    DELETE_PAGE(PAGE_TYPE_LOADING_SCREEN, LoadingPage);
    DELETE_PAGE(PAGE_TYPE_PROGRESS_PAGE, ProgressPage);
    DELETE_PAGE(PAGE_TYPE_HOMBREW_INFO, InfoPage);
    DELETE_PAGE(PAGE_TYPE_PICTURE_PAGE, PicturePage);
    DELETE_PAGE(PAGE_TYPE_BLANK_PAGE, BlankPage);
    
    default:
        break;
    }
}

void EventHandler::ResetBackButtonEvent()
{
    backButtonCB = DefaultCB;
}

void BackButtonEventHandler::onGet(SceInt32 e, Widget *self, SceInt32, ScePVoid puserData)
{
    if(backButtonCB != NULL) backButtonCB(self, e, puserData);
}

ForwardButtonEventHandler::ForwardButtonEventHandler()
{
    eventHandler = onGet;
}

void ForwardButtonEventHandler::onGet(SceInt32 e, Widget *self, SceInt32, ScePVoid puserData)
{
    if(forwardButtonCB != NULL) forwardButtonCB(self, e, puserData);
}

SettingsButtonEventHandler::SettingsButtonEventHandler()
{
    eventHandler = onGet;
}

DiagButtonEventHandler::DiagButtonEventHandler()
{
    eventHandler = onGet;
}

void DiagButtonEventHandler::onGet(SceInt32 e, Widget *s, SceInt32, ScePVoid pUserData)
{
    eventcb *dat = (eventcb *)pUserData;    
    
    if(dat->Callback != NULL)
        dat->Callback(s, e, dat->dat);
    PopupMgr::hideDialog();
}