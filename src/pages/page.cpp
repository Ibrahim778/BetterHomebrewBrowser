#include <kernel.h>
#include <paf.h>

#include "pages/page.hpp"
#include "utils.hpp"
#include "common.hpp"
#include "main.hpp"

generic::Page *generic::Page::currPage = SCE_NULL;
paf::ui::Plane *generic::Page::templateRoot = SCE_NULL;

paf::ui::CornerButton *g_backButton;

generic::Page::Page(const char *pageName)
{
    this->prev = currPage;
    currPage = this;

    paf::Plugin::TemplateInitParam tInit;
    paf::Resource::Element e;

    e.hash = Utils::GetHashById(pageName);
    mainPlugin->TemplateOpen(templateRoot, &e, &tInit);
    root = (paf::ui::Plane *)templateRoot->GetChildByNum(templateRoot->childNum - 1);

	if (currPage->prev != NULL)
	{
		currPage->prev->root->PlayAnimation(0, paf::ui::Widget::Animation_3D_SlideToBack1);
		if (prev->root->animationStatus & 0x80)
			prev->root->animationStatus &= ~0x80;

		g_backButton->PlayAnimation(0, paf::ui::Widget::Animation_Reset);

		if (currPage->prev->prev != SCE_NULL)
		{
			currPage->prev->prev->root->PlayAnimationReverse(0, paf::ui::Widget::Animation_Reset);
			if (prev->prev->root->animationStatus & 0x80)
				prev->prev->root->animationStatus &= ~0x80;
		}
	}
	else g_backButton->PlayAnimationReverse(0, paf::ui::Widget::Animation_Reset);
	
    root->PlayAnimation(-50000, paf::ui::Widget::Animation_3D_SlideFromFront);
	if (root->animationStatus & 0x80)
		root->animationStatus &= ~0x80;

}

generic::Page::~Page()
{
	paf::common::Utils::WidgetStateTransition(-100, this->root, paf::ui::Widget::Animation_3D_SlideFromFront, SCE_TRUE, SCE_TRUE);
	if (prev != SCE_NULL)
	{
		prev->root->PlayAnimationReverse(0.0f, paf::ui::Widget::Animation_3D_SlideToBack1);
		prev->root->PlayAnimation(0.0f, paf::ui::Widget::Animation_Reset);
		if (prev->root->animationStatus & 0x80)
			prev->root->animationStatus &= ~0x80;

		if (prev->prev != SCE_NULL) {
			prev->prev->root->PlayAnimation(0.0f, paf::ui::Widget::Animation_Reset);
			if (prev->prev->root->animationStatus & 0x80)
				prev->prev->root->animationStatus &= ~0x80;
		}
	}
	currPage = this->prev;

     if (currPage->prev != SCE_NULL)
        g_backButton->PlayAnimation(0, paf::ui::Widget::Animation_Reset);
     else g_backButton->PlayAnimationReverse(0, paf::ui::Widget::Animation_Reset);
}

void generic::Page::Init()
{
    if(templateRoot) return; //Already Initialised

    paf::Resource::Element e;
    paf::Plugin::PageInitParam pInit;
    
    e.hash = Utils::GetHashById("page_main");
    paf::ui::Widget *page =  mainPlugin->PageOpen(&e, &pInit);
    
    e.hash = Utils::GetHashById("template_plane");
    templateRoot = (paf::ui::Plane *)page->GetChildByHash(&e, 0);

    currPage = SCE_NULL;

    e.hash = Utils::GetHashById("back_button");
    g_backButton = (paf::ui::CornerButton *)page->GetChildByHash(&e, 0);

    g_backButton->PlayAnimationReverse(0, paf::ui::Widget::Animation::Animation_Reset);

    paf::ui::Widget::EventCallback *backButtonEventCallback = new paf::ui::Widget::EventCallback();
    backButtonEventCallback->eventHandler = generic::Page::BackButtonEventHandler;
    g_backButton->RegisterEventCallback(paf::ui::Widget::EventMain_Decide, backButtonEventCallback, 0);
}

void generic::Page::BackButtonEventHandler(SceInt32, paf::ui::Widget *, SceInt32, ScePVoid)
{
    generic::Page::DeleteCurrentPage();
}

void generic::Page::DeleteCurrentPage()
{
	delete currPage;
}