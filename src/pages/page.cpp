#include <kernel.h>
#include <paf.h>

#include <algorithm>

#include "pages/page.h"
#include "utils.h"
#include "common.h"
#include "print.h"
#include "bhbb_plugin.h"

using namespace paf;

paf::vector<generic::Page *> s_pageStack; //Why didn't I think of this? (Thanks Graphene)

generic::Page::Page(SceInt32 _hash, Plugin::PageOpenParam openParam, Plugin::PageCloseParam cParam):hash(_hash),closeParam(cParam)
{
    rco::Element searchParam;

    if(hash == 0)
        return; //¿How did we get here?

    searchParam.hash = _hash;

    root = g_appPlugin->PageOpen(&searchParam, &openParam);

    if(!s_pageStack.empty()) //We have previous pages
    {
        generic::Page *previousPage = s_pageStack.back(); //Get the previous menu
        ui::Widget::SetControlFlags(previousPage->root, 0); //Disable touch and button controls
    }

    s_pageStack.push_back(this); //Add our current page to the stack
}

generic::Page *generic::Page::GetCurrentPage()
{
    return s_pageStack.back();
}

SceUInt64 generic::Page::GetHash()
{
    return hash;
}

// SceVoid generic::Page::OnRedisplay()
// {

// }

// SceVoid generic::Page::OnDelete()
// {

// }

generic::Page::~Page()
{
    s_pageStack.erase(std::remove(s_pageStack.begin(), s_pageStack.end(), this), s_pageStack.end()); //Remove our page from the list.

    g_appPlugin->PageClose(&root->elem, &closeParam); //Delete our page

    if(!s_pageStack.empty()) //We have other pages
    {
        generic::Page *previousPage = s_pageStack.back(); //Get our previous page
        ui::Widget::SetControlFlags(previousPage->root, 1); //Enable touch and button controls
    }
}

SceVoid generic::Page::DefaultBackButtonCB(SceInt32 hash, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    auto button = Utils::Widget::GetChild(GetCurrentPage()->root, back_button);
    if(button)
        button->PlayEffectReverse(0, effect::EffectType_Reset);
    generic::Page::DeleteCurrentPage();
}

SceVoid generic::Page::DeleteCurrentPage()
{
	delete s_pageStack.back();
}