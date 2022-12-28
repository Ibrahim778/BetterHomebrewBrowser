#include <kernel.h>
#include <paf.h>

#include "pages/multi_page_app_list.h"
#include "common.h"
#include "db.h"
#include "print.h"
#include "utils.h"
#include "settings.h"

using namespace paf;
using namespace Utils;

generic::MultiPageAppList::MultiPageAppList(db::List *tList, const char *templateName):generic::Page(templateName),listRootPlane(SCE_NULL),currBody(SCE_NULL),targetList(tList),locked(false)
{   
    listRootPlane = Widget::GetChild<ui::Plane>(root, Misc::GetHash("plane_list_root"));
    CreateListWrapper();
    SetCategory(db::CategoryAll);
    GamePad::RegisterButtonUpCB(generic::MultiPageAppList::QuickCategoryCB, this);
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

SceVoid generic::MultiPageAppList::Lock()
{
    locked = true;
}

SceVoid generic::MultiPageAppList::Release()
{
    locked = false;
}

SceVoid generic::MultiPageAppList::OnCategoryChanged(int prev, int curr)
{
    //Dim all previous buttons
    thread::s_mainThreadMutex.Lock();
    auto plane = Widget::GetChild(root, Misc::GetHash("plane_category_buttons"));
    for(int i = 0; i < plane->childNum; i++)
        plane->GetChild(i)->SetColor(&paf::Rgba(1,1,1,0.4f));
    thread::s_mainThreadMutex.Unlock();

    for(int i = 0; i < db::info[Settings::GetInstance()->source].categoryNum; i++)
    {
        if(db::info[Settings::GetInstance()->source].categories[i].id == curr)
        {
            auto button = Widget::GetChild(root, Misc::GetHash(db::info[Settings::GetInstance()->source].categories[i].nameID));
            if(button) button->SetColor(&paf::Rgba(1,1,1,1));
            break;
        }
    }
}

SceVoid generic::MultiPageAppList::PopulatePage(ui::Widget *sb, void *userDat)
{
    
}

SceVoid generic::MultiPageAppList::OnPageDeleted(Body *userDat)
{

}

SceVoid generic::MultiPageAppList::OnPageDelete(Body *userDat)
{

}

ScePVoid generic::MultiPageAppList::DefaultNewPageData()
{
    
}

SceVoid generic::MultiPageAppList::OnForwardButtonPressed()
{
    NewPage();
    HandleForwardButton();
}

SceBool generic::MultiPageAppList::SetCategory(int _category)
{
    if(_category == category || locked) return SCE_FALSE;
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

    listWrapperPlane = Widget::Create<ui::Plane>("plane_list_wrapper", "plane", "_common_style_plane_transparent", listRootPlane);
    listWrapperPlane->SetPosition(&vect);
    
    vect.x = 960;
    vect.y = 544;
    listWrapperPlane->SetSize(&vect);
}

SceVoid generic::MultiPageAppList::OnRedisplay()
{
    HandleForwardButton();

    if(currBody && currBody->prev != SCE_NULL)
    {
        generic::Page::SetBackButtonEvent(generic::MultiPageAppList::BackCB, this);
        g_backButton->PlayEffect(0, effect::EffectType_Reset);
    }
}

SceVoid generic::MultiPageAppList::SetCategories(const std::vector<db::Category>& categoryList)
{
    //Delete all previous buttons
    thread::s_mainThreadMutex.Lock();
    auto plane = Widget::GetChild(root, Misc::GetHash("plane_category_buttons"));
    for(int i = 0; i < plane->childNum; i++)
        effect::Play(0, plane->GetChild(i), effect::EffectType_Reset, SCE_TRUE, SCE_TRUE);
    thread::s_mainThreadMutex.Unlock();
    
    Plugin::TemplateOpenParam tOpen;
    rco::Element e;
    SceFloat buttonSize = 844.0f / categoryList.size();
    
    e.hash = Misc::GetHash("category_button_template");
    thread::s_mainThreadMutex.Lock();

    int i = 0;
    for(const db::Category& category : categoryList)
    {
        mainPlugin->TemplateOpen(plane, &e, &tOpen);
        auto button = plane->GetChild(plane->childNum - 1);
        button->SetSize(&paf::Vector4(buttonSize, 58));
        button->SetPosition(&paf::Vector4(i * buttonSize, 1));
        button->SetAnchor(ui::Anchor_Left, ui::Anchor_None, ui::Anchor_None, ui::Anchor_None);
        button->SetAlign(ui::Align_Left, ui::Align_None, ui::Align_None, ui::Align_None);
        button->SetLabel(&paf::wstring(String::GetPFromID(category.nameID)));
        button->SetColor(this->category == category.id ? &paf::Rgba(1,1,1,1) : &paf::Rgba(1,1,1,0.4f));
        button->PlayEffect(0, effect::EffectType_Fadein1);
        button->elem.hash = Misc::GetHash(category.nameID);
        button->RegisterEventCallback(ui::EventMain_Decide, new generic::MultiPageAppList::CategoryCB(this), SCE_FALSE);
        i++;
    }
    thread::s_mainThreadMutex.Unlock();
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
        OnPageDelete(currBody);
        Body *prev = currBody->prev;
        OnPageDeleted(prev);
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

SceVoid generic::MultiPageAppList::NewPageJob::Run()
{
    callingPage->_NewPage(userDat, populate);
}

SceVoid generic::MultiPageAppList::DeletePageJob::Run()
{
    callingPage->_DeletePage(animate);
}

SceVoid generic::MultiPageAppList::ForwardButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    ((MultiPageAppList *)pUserData)->OnForwardButtonPressed();
}

SceVoid generic::MultiPageAppList::Redisplay(void *u)
{
    g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new ClearJob("MPAL::ClearJob", this)));
    g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new NewPageJob("MPAL::NewPageJob", this, !u ? DefaultNewPageData() : u, SCE_TRUE)));
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

// SceVoid generic::MultiPageAppList::NewPageJob::Run()
// {
//     callingPage->_NewPage(populate);
// }

// SceVoid generic::MultiPageAppList::DeletePageJob::Run()
// {
//     callingPage->_DeletePage(animate);
// }

SceVoid generic::MultiPageAppList::NewPage(SceVoid *userDat, SceBool populate)
{
    //g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new NewPageJob("MPAL::NewPageJob", this, populate)));
    _NewPage(!userDat ? DefaultNewPageData() : userDat, populate);
}

SceVoid generic::MultiPageAppList::DeletePage(SceBool animate)
{
    //g_mainQueue->Enqueue(&SharedPtr<job::JobItem>(new DeletePageJob("MPAL::DeletePageJob", this, animate)));
    _DeletePage(animate);
}

SceVoid generic::MultiPageAppList::_NewPage(void *userDat, SceBool populate)
{
    Plugin::TemplateOpenParam tInit;
    rco::Element e;
    e.hash = Misc::GetHash("home_page_list_template");

    currBody = new Body(currBody, userDat);

    ui::Widget *widget = SCE_NULL;
    
    SceBool isMainThread = thread::IsMainThread();
    if(!isMainThread)
       thread::s_mainThreadMutex.Lock();
    
    mainPlugin->TemplateOpen(listWrapperPlane, &e, &tInit);
    
    currBody->widget = widget = listWrapperPlane->GetChild(listWrapperPlane->childNum - 1);

    if(currBody->prev != SCE_NULL)
    {
        Widget::PlayEffect(currBody->prev->widget, 0, effect::EffectType_3D_SlideToBack1);

        g_backButton->PlayEffect(0, effect::EffectType_Reset);
        generic::Page::SetBackButtonEvent(generic::MultiPageAppList::BackCB, this);

        if(currBody->prev->prev != SCE_NULL)
        {
            Widget::PlayEffectReverse(currBody->prev->prev->widget, 0, effect::EffectType_Reset);
        }
    }

    if(!isMainThread)
       thread::s_mainThreadMutex.Unlock();
    
    if(listWrapperPlane->childNum == 1)
        generic::Page::ResetBackButton();
    
    auto scrollBox = Widget::GetChild(widget, Misc::GetHash("list_scroll_box"));
    e.hash = Misc::GetHash("homebrew_button");
    
    if(populate)
        PopulatePage(scrollBox, userDat);

    Widget::PlayEffect(widget, -5000, effect::EffectType_3D_SlideFromFront);
    HandleForwardButton();
}

SceVoid generic::MultiPageAppList::_DeletePage(SceBool animate)
{   
    if(currBody == SCE_NULL) return;

    SceBool isMainThread = thread::IsMainThread();
    if(!isMainThread)
       thread::s_mainThreadMutex.Lock();
    
    OnPageDelete(currBody);
    print("OnPageDelete completed!\n");
    effect::Play(animate ? -100 : 0, currBody->widget, effect::EffectType_3D_SlideFromFront, SCE_TRUE, !animate);
    
    if(currBody->prev != SCE_NULL)
    {
        currBody->prev->widget->PlayEffectReverse(0, effect::EffectType_3D_SlideToBack1);
        Widget::PlayEffect(currBody->prev->widget, 0, effect::EffectType_Reset);

        if(currBody->prev->prev != SCE_NULL)
            Widget::PlayEffect(currBody->prev->prev->widget, 0, effect::EffectType_Reset);
        else 
            generic::Page::ResetBackButton();
    }

    if(!isMainThread)
       thread::s_mainThreadMutex.Unlock();
    
    Body *prev = currBody->prev;
    OnPageDeleted(currBody);
    delete currBody;
    currBody = prev;
}

void generic::MultiPageAppList::QuickCategoryCB(input::GamePad::GamePadData *data, ScePVoid pUserData)
{
    generic::MultiPageAppList *page = (generic::MultiPageAppList *)pUserData;
    if(generic::Page::GetCurrentPage() != pUserData)
        return;
        
    //Find current index in category array according to id
    int target = page->GetCategory();
    int i = 0;

    for(const db::Category &cat : db::info[Settings::GetInstance()->source].categories)
    {
        if(cat.id == target)
            break;
        i++;
    }

    if(data->buttons & (SCE_PAF_CTRL_R1 | SCE_PAF_CTRL_R))    
        i++;
    else if(data->buttons & (SCE_PAF_CTRL_L1 | SCE_PAF_CTRL_L))
        i--;

    print("Target: %d\n", i);

    if(i < 0) 
        return;
    
    print("Higher than 0\n");

    if(i > (db::info[Settings::GetInstance()->source].categoryNum - 1))
        return;

    print("Lower than max\n");
    if(page->SetCategory(db::info[Settings::GetInstance()->source].categories[i].id))
        page->Redisplay();
}

SceVoid generic::MultiPageAppList::CategoryCB::OnGet(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(!db::info[Settings::GetInstance()->source].CategoriesSupported) return;
    MultiPageAppList *page = (MultiPageAppList *)pUserData;

    //Loop through and find category ID from button hash (same as category hash)
    for(int i = 0; i < db::info[Settings::GetInstance()->source].categoryNum; i++)
    {
        if(Misc::GetHash(db::info[Settings::GetInstance()->source].categories[i].nameID) == self->elem.hash)
        {
            if(page->SetCategory(db::info[Settings::GetInstance()->source].categories[i].id)) // returns true if there's any change
                page->Redisplay();
            break;
        }
    }
}