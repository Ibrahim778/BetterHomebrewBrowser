#include <kernel.h>
#include <paf.h>

#include "pages/apps_page.h"
#include "print.h"
#include "utils.h"
#include "settings.h"
#include "common.h"

using namespace paf;

apps::Page::Page():generic::Page::Page("home_page_template")
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

    ResetList();
}

apps::Page::~Page()
{
    
}

SceVoid apps::Page::ResetList()
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

SceVoid apps::Page::Load()
{

}