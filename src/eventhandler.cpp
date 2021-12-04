#include "eventhandler.hpp"
#include "pagemgr.hpp"
#include "main.hpp"
#include "common.hpp"

static ECallback backButtonCB = NULL;
static ECallback forwardButtonCB = NULL;

BackButtonEventHandler *mainBackButtonEvent;
ForwardButtonEventHandler *mainForwardButtonEvent;
HomebrewListButtonEventHandler *mainHomebrewListButtonEvent;

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

void EventHandler::SetBackButtonEvent(ECallback callback, void *userDat)
{
    backButtonCB = callback;
    mainBackButtonEvent->pUserData = userDat;
}

void EventHandler::SetForwardButtonEvent(ECallback callback, void *userDat)
{
    forwardButtonCB = callback;
    mainForwardButtonEvent->pUserData = userDat;
}

void EventHandler::ResetForwardButtonEvent()
{
    forwardButtonCB = NULL;
    mainForwardButtonEvent->pUserData = NULL;
}

BUTTON_CB(DefaultBackButtonCB)
{
    Page::DeletePage(Page::GetCurrentPage(), true);
}

void EventHandler::ResetBackButtonEvent()
{
    backButtonCB = DefaultBackButtonCB;
    mainBackButtonEvent->pUserData = NULL;
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

HomebrewListButtonEventHandler::HomebrewListButtonEventHandler()
{
    eventHandler = onGet;
}

void HomebrewListButtonEventHandler::onGet(SceInt32 e, Widget *self, SceInt32, ScePVoid puserData)
{
    print("it is %p\n", puserData);
    if(puserData == NULL) return;
    Box *listBox = (Box *)puserData;
    print("getting %p\n", listBox);
    
    for(int i = 0; i < listBox->childNum; i++) //Find the child number
    {
        Widget *w = listBox->GetChildByNum(i);
        if(w == NULL)
        {
            print("Err nullll\n");
            return;
        }
        if(w->hash == self->hash)
        {
            node *n = list.GetByCategoryIndex(((pageNum - 1) * APPS_PER_PAGE) + i, category);
            print("Num = %d n = %p\n", ((pageNum - 1) * APPS_PER_PAGE) + i, n);
            forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset);
            EventHandler::ResetBackButtonEvent();
            new InfoPage(&n->info);
            return;
        }
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
    PopupMgr::HideDialog();
}