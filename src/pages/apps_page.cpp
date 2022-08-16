#include <kernel.h>
#include <paf.h>

#include "pages/apps_page.h"
#include "print.h"
#include "utils.h"
#include "settings.h"
#include "common.h"

using namespace paf;

apps::Page::Page():generic::Page::Page("home_page_template"),body(SCE_NULL)
{
    ui::EventCallback *searchCallback = new ui::EventCallback();
    searchCallback->pUserData = this;
    searchCallback->eventHandler = Page::SearchCB;

    Utils::GetChildByHash(root, Utils::GetHashById("search_button"))->RegisterEventCallback(ui::EventMain_Decide, searchCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("search_enter_button"))->RegisterEventCallback(ui::EventMain_Decide, searchCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("search_back_button"))->RegisterEventCallback(ui::EventMain_Decide, searchCallback, 0);

    SetMode(PageMode_Browse);

    auto optionsButton = Utils::GetChildByHash(root, Utils::GetHashById("options_button"));
    auto optionsCallback = new ui::EventCallback();
    optionsCallback->eventHandler = Settings::OpenCB;

    optionsButton->RegisterEventCallback(ui::EventMain_Decide, optionsCallback, 0);
}

apps::Page::~Page()
{
    
}

SceVoid apps::Page::SetMode(PageMode targetMode)
{
    if(mode == targetMode) return;

    ui::Widget *categoriesPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_categories"));
    ui::Widget *searchPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_search"));
    ui::Widget *searchBox = Utils::GetChildByHash(root, Utils::GetHashById("search_box"));

    switch(targetMode)
    {
    case PageMode_Browse:
        searchPlane->PlayEffectReverse(0, effect::EffectType_Fadein1);
        categoriesPlane->PlayEffect(0, effect::EffectType_Fadein1);
        break;
    
    case PageMode_Search:
        categoriesPlane->PlayEffectReverse(0, effect::EffectType_Fadein1);
        searchPlane->PlayEffect(0, effect::EffectType_Fadein1);
        break;
    }

    mode = targetMode;
}

SceVoid apps::Page::SearchCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    Page *page = (Page *)pUserData;
    switch(self->hash)
    {
    case Hash_SearchButton:
        page->SetMode(PageMode_Search);
        break;

    case Hash_SearchBackButton:
        page->SetMode(PageMode_Browse);
        break;

    case Hash_SearchEnterButton:
        break;
    }
}

apps::Page::Body::Body(Page *_page):prev(SCE_NULL),next(SCE_NULL):page(_page)
{
    Plugin::TemplateInitParam tInit;
    rco::Element e = Utils::GetParamWithHashFromId("home_page_list_template");

    mainPlugin->TemplateOpen(_page->root, &e, &tInit);

    widget = (ui::Plane *)_page->root->GetChild(_page->root->childNum - 1);

    if(_page->body == SCE_NULL)
    {
        _page->body = this;

        widget->PlayEffect(-50000, effect::EffectType_3D_SlideFromFront);
        if (widget->animationStatus & 0x80)
            widget->animationStatus &= ~0x80;
        
        return;
    }


    Body *last = _page->body;
    while(last->next != SCE_NULL)
        last = last->next;

    last->next = this;
    prev = last;

    if(prev != SCE_NULL)
    {
        if(prev->prev != SCE_NULL)
        {
            widget->PlayEffectReverse(0, effect::EffectType_Reset);
            if (widget->animationStatus & 0x80)
                widget->animationStatus &= ~0x80;
        }
        else 
        {
            widget->PlayEffect(0, effect::EffectType_3D_SlideToBack1);
            if(widget->animationStatus & 0x80)
                widget->animationStatus &= ~0x80;
        }
    }
}

apps::Page::Body::~Body()
{
    page->body = next;
    next->prev = prev;

    if(page->body == this)
    {
        widget->PlayEffectReverse(0, effect::EffectType_Reset);
        if(widget->animationStatus & 0x80)
            widget->animationStatus &= ~0x80;
        
        
        return;
    }
}

SceVoid apps::Page::NewPage()
{
    new Body(this);
}

ui::Plane *apps::Page::Body::GetBody()
{
    return widget;
}

SceVoid apps::Page::Load()
{

}