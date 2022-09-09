#include <kernel.h>
#include <paf.h>

#include "pages/page.h"
#include "utils.h"
#include "common.h"
#include "main.h"

using namespace paf;

generic::Page *generic::Page::currPage = SCE_NULL;
ui::Plane *generic::Page::templateRoot = SCE_NULL;

ui::CornerButton *g_backButton;
ui::CornerButton *g_forwardButton;
ui::BusyIndicator *g_busyIndicator;

generic::Page::ButtonEventCallback generic::Page::backCallback;
generic::Page::ButtonEventCallback generic::Page::forwardCallback;
void *generic::Page::backData;
void *generic::Page::forwardData;

generic::Page::Page(const char *pageName)
{
    this->prev = currPage;
    currPage = this;

    Plugin::TemplateInitParam tInit;
    rco::Element e;

    e.hash = Utils::GetHashById(pageName);
    
    //SceBool isMainThread = thread::IsMainThread();
    //if(!isMainThread && !thread::s_mainThreadMutex.TryLock())
    //    thread::s_mainThreadMutex.Lock();

    mainPlugin->TemplateOpen(templateRoot, &e, &tInit);
    root = (ui::Plane *)templateRoot->GetChild(templateRoot->childNum - 1);

	if (currPage->prev != NULL)
	{
		currPage->prev->root->PlayEffect(0, effect::EffectType_3D_SlideToBack1);
		if (prev->root->animationStatus & 0x80)
			prev->root->animationStatus &= ~0x80;

		g_backButton->PlayEffect(0, effect::EffectType_Reset);
        Page::SetBackButtonEvent(NULL, NULL);

		if (currPage->prev->prev != SCE_NULL)
		{
			currPage->prev->prev->root->PlayEffectReverse(0, effect::EffectType_Reset);
			if (prev->prev->root->animationStatus & 0x80)
				prev->prev->root->animationStatus &= ~0x80;
		}
	}
	else g_backButton->PlayEffectReverse(0, effect::EffectType_Reset);
	
    g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);

    root->PlayEffect(-50000, effect::EffectType_3D_SlideFromFront);
	if (root->animationStatus & 0x80)
		root->animationStatus &= ~0x80;

    //if(!isMainThread)
    //    thread::s_mainThreadMutex.Lock();
}

SceVoid generic::Page::OnRedisplay()
{

}

SceVoid generic::Page::OnDelete()
{

}

generic::Page::~Page()
{
    
	effect::Play(-100, this->root, effect::EffectType_3D_SlideFromFront, SCE_TRUE, SCE_TRUE);
	if (prev != SCE_NULL)
	{
		prev->root->PlayEffectReverse(0.0f, effect::EffectType_3D_SlideToBack1);
		prev->root->PlayEffect(0.0f, effect::EffectType_Reset);
		if (prev->root->animationStatus & 0x80)
			prev->root->animationStatus &= ~0x80;

		if (prev->prev != SCE_NULL) {
			prev->prev->root->PlayEffect(0.0f, effect::EffectType_Reset);
			if (prev->prev->root->animationStatus & 0x80)
				prev->prev->root->animationStatus &= ~0x80;
		}
	}
	currPage = this->prev;

    if (currPage != NULL && currPage->prev != SCE_NULL)
        g_backButton->PlayEffect(0, effect::EffectType_Reset);
    else g_backButton->PlayEffectReverse(0, effect::EffectType_Reset);

    if(currPage != SCE_NULL) currPage->OnRedisplay(); 
}

void generic::Page::Setup()
{
    if(templateRoot) return; //Already Initialised

    rco::Element e;
    Plugin::PageInitParam pInit;
    
    e.hash = Utils::GetHashById("page_main");
    ui::Widget *page =  mainPlugin->PageOpen(&e, &pInit);
    
    e.hash = Utils::GetHashById("template_plane");
    templateRoot = (ui::Plane *)page->GetChild(&e, 0);

    currPage = SCE_NULL;

    e.hash = Utils::GetHashById("back_button");
    g_backButton = (ui::CornerButton *)page->GetChild(&e, 0);

    backCallback = SCE_NULL;
    backData = SCE_NULL;

    forwardCallback = SCE_NULL;
    forwardData = SCE_NULL;

    g_backButton->PlayEffectReverse(0, effect::EffectType::EffectType_Reset);

    ui::EventCallback *backButtonEventCallback = new ui::EventCallback();
    backButtonEventCallback->eventHandler = generic::Page::BackButtonEventHandler;
    g_backButton->RegisterEventCallback(ui::EventMain_Decide, backButtonEventCallback, 0);

    e.hash = Utils::GetHashById("main_busy");
    g_busyIndicator = (ui::BusyIndicator *)page->GetChild(&e, 0);
    g_busyIndicator->Stop();

    e.hash = Utils::GetHashById("forward_button");
    g_forwardButton = (ui::CornerButton *)page->GetChild(&e, 0);
    g_forwardButton->PlayEffectReverse(0, effect::EffectType_Reset);

    ui::EventCallback *forwardButtonEventCallback = new ui::EventCallback;
    forwardButtonEventCallback->eventHandler = generic::Page::ForwardButtonEventHandler;
    g_forwardButton->RegisterEventCallback(ui::EventMain_Decide, forwardButtonEventCallback, 0);
}

void generic::Page::ResetBackButton()
{
    SetBackButtonEvent(SCE_NULL, SCE_NULL);

    if (currPage != NULL && currPage->prev != SCE_NULL)
        g_backButton->PlayEffect(0, effect::EffectType_Reset);
    else g_backButton->PlayEffectReverse(0, effect::EffectType_Reset);
}

void generic::Page::SetBackButtonEvent(ButtonEventCallback callback, void *data)
{
    backCallback = callback;
    backData = data;
}

void generic::Page::SetForwardButtonEvent(ButtonEventCallback callback, void *data)
{
    forwardCallback = callback;
    forwardData = data;
}

void generic::Page::BackButtonEventHandler(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid)
{
    if(backCallback)
        backCallback(eventID, self, unk, backData);
    else    
        generic::Page::DeleteCurrentPage();
}

void generic::Page::ForwardButtonEventHandler(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid)
{
    if(forwardCallback)
        forwardCallback(eventID, self, unk, forwardData);
}

void generic::Page::DeleteCurrentPage()
{
	delete currPage;
}