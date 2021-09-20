#include "eventhandler.hpp"
#include "pagemgr.hpp"

extern Page *currPage;

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

void BackButtonEventHandler::onGet(SceInt32 , Widget *self, SceInt32, ScePVoid puserData)
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