#include <kernel.h>
#include <paf.h>

#include "pages/multi_page_app_list.h"
#include "common.h"
#include "db.h"
#include "print.h"
#include "utils.h"
#include "settings.h"

using namespace paf;

generic::MultiPageAppList::MultiPageAppList(db::List *tList, const char *templateName):generic::Page(templateName),listRootPlane(SCE_NULL),currBody(SCE_NULL),targetList(tList)
{
    listRootPlane = (ui::Plane *)Utils::GetChildByHash(root, Utils::GetHashById("plane_list_root"));
    CreateListWrapper();
}

generic::MultiPageAppList::~MultiPageAppList()
{

}

SceVoid generic::MultiPageAppList::OnClear()
{

}

SceVoid generic::MultiPageAppList::OnCleared()
{
    
}

SceVoid generic::MultiPageAppList::OnCategoryChanged(int prev, int curr)
{

}

SceVoid generic::MultiPageAppList::PopulatePage(ui::Widget *sb)
{
    
}

SceBool generic::MultiPageAppList::SetCategory(int _category)
{
    if(_category == category) return SCE_FALSE;
    int prev = category;
    category = _category;
    OnCategoryChanged(prev, category);
    return SCE_TRUE;
}

SceVoid generic::MultiPageAppList::SetTargetList(db::List *list)
{
    targetList = list;
}

db::List *generic::MultiPageAppList::GetTargetList()
{
    return targetList;
}

SceVoid generic::MultiPageAppList::BackCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    ((apps::Page *)pUserData)->DeletePage();
    ((apps::Page *)pUserData)->HandleForwardButton();
}

SceVoid generic::MultiPageAppList::ClearInternal()
{
    print("Redisplay!\n");

    ClearPages();
}

SceVoid generic::MultiPageAppList::CreateListWrapper()
{
    Vector4 vect;
    vect.x = 0;
    vect.y = 0;
    vect.z = 0;
    vect.w = 0;

    listWrapperPlane = (ui::Plane *)Utils::CreateWidget("plane_list_wrapper", "plane", "_common_style_plane_transparent", listRootPlane);
    listWrapperPlane->SetPosition(&vect);
    
    vect.x = 960;
    vect.y = 544;
    listWrapperPlane->SetSize(&vect);
}

SceVoid generic::MultiPageAppList::OnRedisplay()
{
    HandleForwardButton();

    if(currBody->prev != SCE_NULL)
    {
        generic::Page::SetBackButtonEvent(generic::MultiPageAppList::BackCB, this);
        g_backButton->PlayEffect(0, effect::EffectType_Reset);
    }
}

int generic::MultiPageAppList::GetCategory()
{
    return category;
}

SceUInt32 generic::MultiPageAppList::GetPageCount()
{
    SceUInt32 num = 0;
    Body *b = currBody; 
    while(b != SCE_NULL)
    {
        b = b->prev;
        num++;
    }

    return num;
}

SceVoid generic::MultiPageAppList::ClearPages()
{
    OnClear();
    while(currBody)
    {
        apps::Page::Body *prev = currBody->prev;
        delete currBody;
        currBody = prev;
    }

    SceBool isMainThread = thread::IsMainThread();
    if(!isMainThread)
        thread::s_mainThreadMutex.Lock();

    effect::Play(-100, listWrapperPlane, effect::EffectType_3D_SlideFromFront, SCE_TRUE, SCE_TRUE);
    CreateListWrapper();

    if(!isMainThread)
        thread::s_mainThreadMutex.Unlock();

    generic::Page::ResetBackButton();
    OnCleared();
}

SceVoid generic::MultiPageAppList::ClearJob::Run()
{
    callingPage->ClearInternal();
}

SceVoid generic::MultiPageAppList::ForwardButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    ((MultiPageAppList *)pUserData)->NewPage();
    ((MultiPageAppList *)pUserData)->HandleForwardButton();
}


SceVoid generic::MultiPageAppList::Redisplay()
{
    job::s_defaultJobQueue->Enqueue(&SharedPtr<job::JobItem>(new ClearJob("BHBB::apps::Page::ClearJob", this)));
    job::s_defaultJobQueue->Enqueue(&SharedPtr<job::JobItem>(new NewPageJob("MPAL::NewPageJob", this, SCE_TRUE)));
}

SceVoid generic::MultiPageAppList::HandleForwardButton()
{
    if(targetList->GetSize(category) <= (GetPageCount() * Settings::GetInstance()->nLoad))
    {
        generic::Page::SetForwardButtonEvent(SCE_NULL, SCE_NULL);
        g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);
    }
    else
    {
        generic::Page::SetForwardButtonEvent(ForwardButtonCB, this);
        g_forwardButton->PlayEffect(0, effect::EffectType_Reset);
    }
}

SceVoid generic::MultiPageAppList::NewPageJob::Run()
{
    callingPage->_NewPage(populate);
}

SceVoid generic::MultiPageAppList::DeletePageJob::Run()
{
    callingPage->_DeletePage(animate);
}

SceVoid generic::MultiPageAppList::NewPage(SceBool populate)
{
    //job::s_defaultJobQueue->Enqueue(&SharedPtr<job::JobItem>(new NewPageJob("MPAL::NewPageJob", this, populate)));
    _NewPage(populate);
}

SceVoid generic::MultiPageAppList::DeletePage(SceBool animate)
{
    //job::s_defaultJobQueue->Enqueue(&SharedPtr<job::JobItem>(new DeletePageJob("MPAL::DeletePageJob", this, animate)));
    _DeletePage(animate);
}

SceVoid generic::MultiPageAppList::_NewPage(SceBool populate)
{
    Plugin::TemplateOpenParam tInit;
    rco::Element e;
    e.hash = Utils::GetHashById("home_page_list_template");

    currBody = new Body(currBody);

    ui::Widget *widget = SCE_NULL;
    
    SceBool isMainThread = thread::IsMainThread();
    if(!isMainThread)
       thread::s_mainThreadMutex.Lock();
    
    mainPlugin->TemplateOpen(listWrapperPlane, &e, &tInit);
    
    currBody->widget = widget = listWrapperPlane->GetChild(listWrapperPlane->childNum - 1);

    if(currBody->prev != SCE_NULL)
    {
        Utils::PlayEffect(currBody->prev->widget, 0, effect::EffectType_3D_SlideToBack1);

        g_backButton->PlayEffect(0, effect::EffectType_Reset);
        generic::Page::SetBackButtonEvent(generic::MultiPageAppList::BackCB, this);

        if(currBody->prev->prev != SCE_NULL)
        {
            Utils::PlayEffectReverse(currBody->prev->prev->widget, 0, effect::EffectType_Reset);
        }
    }

    if(!isMainThread)
       thread::s_mainThreadMutex.Unlock();
    
    if(listWrapperPlane->childNum == 1)
        generic::Page::ResetBackButton();
    
    auto scrollBox = Utils::GetChildByHash(widget, Utils::GetHashById("list_scroll_box"));
    e.hash = Utils::GetHashById("homebrew_button");
    
    if(populate)
        PopulatePage(scrollBox);

    Utils::PlayEffect(widget, -5000, effect::EffectType_3D_SlideFromFront);
    HandleForwardButton();
}

SceVoid generic::MultiPageAppList::_DeletePage(SceBool animate)
{   
    if(currBody == SCE_NULL) return;

    SceBool isMainThread = thread::IsMainThread();
    if(!isMainThread)
       thread::s_mainThreadMutex.Lock();

    effect::Play(animate ? -100 : 0, currBody->widget, effect::EffectType_3D_SlideFromFront, SCE_TRUE, !animate);
    
    if(currBody->prev != SCE_NULL)
    {
        currBody->prev->widget->PlayEffectReverse(0, effect::EffectType_3D_SlideToBack1);
        Utils::PlayEffect(currBody->prev->widget, 0, effect::EffectType_Reset);

        if(currBody->prev->prev != SCE_NULL)
            Utils::PlayEffect(currBody->prev->prev->widget, 0, effect::EffectType_Reset);
        else 
            generic::Page::ResetBackButton();
    }

    if(!isMainThread)
       thread::s_mainThreadMutex.Unlock();
    
    Body *prev = currBody->prev;
    delete currBody;
    currBody = prev;
}