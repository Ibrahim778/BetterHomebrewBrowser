#include <kernel.h>
#include <paf.h>

#include <algorithm>
#include <vector>

#include "pages/page.h"
#include "common.h"
#include "print.h"
#include "bhbb_plugin.h"

using namespace paf;

std::vector<page::Base *> s_pageStack; //Why didn't I think of this? (Thanks Graphene)

page::Base::Base(uint32_t _hash, Plugin::PageOpenParam openParam, Plugin::PageCloseParam cParam):closeParam(cParam),backButton(SCE_NULL)
{
    if(_hash == 0)
        return; //¿How did we get here?

    IDParam param(_hash);
    root = g_appPlugin->PageOpen(param, openParam);
    param = back_button;
    backButton = (ui::CornerButton *)root->FindChild(param);
    
    if(!s_pageStack.empty()) //We have previous pages
    {
        page::Base *previousPage = s_pageStack.back(); //Get the previous menu
        previousPage->root->SetActivate(false); //Disable touch and button controls
        if(backButton)
        {
            backButton->Show(common::transition::Type_Reset);
            backButton->AddEventCallback(ui::CornerButton::CB_BTN_DECIDE, (ui::HandlerCB)page::Base::DefaultBackButtonCB);
        }
    }
    else if(backButton)
        backButton->Hide(common::transition::Type_Reset);

    s_pageStack.push_back(this); //Add our current page to the stack
}

page::Base *page::Base::GetCurrentPage()
{
    return s_pageStack.back();
}

uint32_t page::Base::GetHash()
{
    return root->GetName().GetIDHash();
}

page::Base::~Base()
{
    s_pageStack.erase(std::remove(s_pageStack.begin(), s_pageStack.end(), this), s_pageStack.end()); //Remove our page from the list.

    g_appPlugin->PageClose(root->GetName(), closeParam); //Delete our page

    if(!s_pageStack.empty()) //We have other pages
    {
        page::Base *previousPage = s_pageStack.back(); //Get our previous page
        previousPage->root->SetActivate(true); //Enable touch and button controls
    }
}

void page::Base::DefaultBackButtonCB(uint32_t type, ui::Handler *self, ui::Event *event, ScePVoid pUserData)
{
    page::Base::DeleteCurrentPage();
}

SceVoid page::Base::DeleteCurrentPage()
{
    auto button = page::Base::GetCurrentPage()->backButton;
    if(button)
        button->Hide(common::transition::Type_Reset);

	delete s_pageStack.back();
}