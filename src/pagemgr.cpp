#include "pagemgr.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "main.hpp"
#include "eventhandler.hpp"

extern Plugin *mainPlugin;
extern Plane *mainRoot;
extern CornerButton *mainBackButton;
extern CornerButton *settingsButton;
extern Widget *mainScene;

int pageDepth;
Page *currPage = SCE_NULL;

Box * PopupMgr::diagBox = SCE_NULL;
Plane *PopupMgr::diagBG = SCE_NULL;
Dialog *PopupMgr::diag = SCE_NULL;
bool PopupMgr::showingDialog = SCE_FALSE;

graphics::Texture *PopupMgr::checkmark = SCE_NULL;
graphics::Texture *PopupMgr::transparent = SCE_NULL;

#define DEFINE_PAGE(pageType, pageID)\
case pageType:\
{\
search = Utils::GetParamWithHashFromId(pageID);\
break;\
}

Page::Page(pageType page)
{
    PrintFreeMem();
    type = page;

    this->prev = currPage;
    currPage = this;

    OnDelete = NULL;
    pageDepth ++;
    skipAnimation = SCE_FALSE;

    currPage->pageThread = new UtilThread(SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SCE_KERNEL_1MiB, "BHBB_PAGE_THREAD");
    currPage->pageThread->Entry = SCE_NULL;
    currPage->pageThread->EndThread = SCE_FALSE;

    if(pageDepth > 1)
    {
        prev->root->PlayAnimation(0, Widget::Animation_3D_SlideToBack1);
        if (prev->root->animationStatus & 0x80)
		    prev->root->animationStatus &= ~0x80;

        if(prev->prev != SCE_NULL)
        {
            prev->prev->root->PlayAnimationReverse(0, Widget::Animation_Reset);
            if (prev->root->animationStatus & 0x80)
                prev->root->animationStatus &= ~0x80;
        }
    }

    Resource::Element search;
    Plugin::TemplateInitParam tini;

    switch (type)
    {
    
    DEFINE_PAGE(PAGE_TYPE_PROGRESS_PAGE, PROGRESS_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_LOADING_SCREEN, LOADING_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_SELECTION_LIST, SELECTION_LIST_NO_TITLE_TEMPLATE)

    DEFINE_PAGE(PAGE_TYPE_SELECTION_LIST_WITH_TITLE, SELECTION_LIST_TEMPLATE)

    default:
        break;
    }

    mainPlugin->AddWidgetFromTemplate(mainRoot, &search, &tini);
    root = (Plane *)mainRoot->GetChildByNum(mainRoot->childNum - 1);


    busy = (BusyIndicator *)Utils::GetChildByHash(root, Utils::GetHashById(BUSY_INICATOR_ID));
    busy->Start();

    root->PlayAnimation(-5000, Widget::Animation_3D_SlideFromFront);
	if (root->animationStatus & 0x80)
		root->animationStatus &= ~0x80;

    if(pageDepth > 1)
        mainBackButton->PlayAnimation(0, Widget::Animation_Reset);
    else mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);

    if(pageDepth == 1)
        settingsButton->PlayAnimation(0, Widget::Animation_Reset);
    else settingsButton->PlayAnimationReverse(0, Widget::Animation_Reset);

}

Page::~Page()
{
    PrintFreeMem();

    if(this->pageThread != NULL)
    {
        if(this->pageThread->IsStarted())
        {
            this->pageThread->EndThread = true;
            this->pageThread->Join();
            if(this->pageThread != NULL) delete this->pageThread;
        }
    }

    if(OnDelete != NULL)
        OnDelete();
    
    common::Utils::WidgetStateTransition(-1000.0f, this->root, Widget::Animation_3D_SlideFromFront, SCE_TRUE, currPage == this ? skipAnimation : SCE_TRUE);
    
    if(currPage == this)
    {
        if (prev != SCE_NULL && !skipAnimation) 
        {
            prev->root->PlayAnimationReverse(0.0f, widget::Widget::Animation_3D_SlideToBack1);
            prev->root->PlayAnimation(0.0f, widget::Widget::Animation_Reset);
            if (prev->root->animationStatus & 0x80)
                prev->root->animationStatus &= ~0x80;
        
            if (prev->prev != SCE_NULL) {
                prev->prev->root->PlayAnimation(0.0f, widget::Widget::Animation_Reset);
                if (prev->prev->root->animationStatus & 0x80)
                    prev->prev->root->animationStatus &= ~0x80;
            }
        }
        currPage = this->prev;
    }
    else
    {
        currPage->prev = this->prev;
        if(currPage->prev != SCE_NULL)
        {
            currPage->prev->root->PlayAnimation(0, Widget::Animation_Reset);
            if (currPage->prev->root->animationStatus & 0x80)
                    currPage->prev->root->animationStatus &= ~0x80;
        }
    }

    pageDepth--;

    //Settings button
    if(pageDepth == 1)
        settingsButton->PlayAnimation(0, Widget::Animation_Reset);
    else settingsButton->PlayAnimationReverse(0, Widget::Animation_Reset);

    //Back Button
    if(pageDepth > 1)
        mainBackButton->PlayAnimation(0, Widget::Animation_Reset);
    else mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);

}

SelectionList::~SelectionList()
{}

ImageButton *SelectionList::AddOption(const char *text, void(*onPress)(void *), void *userDat, SceBool isLong, SceBool needImage)
{
    String str;
    str.Set(text);
    WString wstr;
    str.ToWString(&wstr);

    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    if(isLong)
    {
        if(needImage)
            e = Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE_IMG);
        else
            e = Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE);
    }
    else
    {
        if(needImage)
            e = Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE_IMG);
        else
            e = Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE);
    }
    
    mainPlugin->AddWidgetFromTemplate(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);
    button->SetLabel(&wstr);
    
    Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

ImageButton *SelectionList::AddOption(String *text, void(*onPress)(void *), void *userDat, SceBool isLong, SceBool needImage)
{
    return this->AddOption(text->data, onPress, userDat, isLong, needImage);
}

SelectionList::SelectionList(const char *title):Page(title != NULL ? PAGE_TYPE_SELECTION_LIST_WITH_TITLE : PAGE_TYPE_SELECTION_LIST)
{
    if(title != NULL)
    {
        String str;
        str.Set(title);

        WString wstr;
        str.ToWString(&wstr);

        TitleText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(PAGE_TITLE_ID));
        TitleText->SetLabel(&wstr);
    }

    Resource::Element search = Utils::GetParamWithHashFromId(title != NULL ? LIST_TEMPLATE_ID : LIST_TEMPLATE_ID_NO_TITLE);
    Plugin::TemplateInitParam tini;

    mainPlugin->AddWidgetFromTemplate(root, &search, &tini);

    listRoot = (Plane *)root->GetChildByNum(root->childNum - 1);

    scrollViewBox = (Box *)Utils::GetChildByHash(listRoot, Utils::GetHashById(LIST_SCROLL_BOX));
}

void Page::Init()
{
    pageDepth = 0;
    mainBackButton->RegisterEventCallback(ON_PRESS_EVENT_ID, new BackButtonEventHandler(), 0);
    settingsButton->RegisterEventCallback(ON_PRESS_EVENT_ID, new SettingsButtonEventHandler(), 0);

    //Not really needed but just to be sure
    currPage = SCE_NULL;

    mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);
}

LoadingPage::LoadingPage(const char *info):Page(PAGE_TYPE_LOADING_SCREEN)
{
    skipAnimation = SCE_TRUE;
    String str;
    str.Set(info);
    WString wstr;
    str.ToWString(&wstr);

    infoText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(LOADING_PAGE_TEXT));
    infoText->SetLabel(&wstr);
}

LoadingPage::~LoadingPage()
{
    busy->Stop();
}

ProgressPage::ProgressPage(const char *info, int BarNum):Page(PAGE_TYPE_PROGRESS_PAGE)
{
    barNum = BarNum;
    skipAnimation = SCE_TRUE;
    String str;
    str.Set(info);
    WString wstr;
    str.ToWString(&wstr);

    progressBars = new ProgressBar *[BarNum];

    Plugin::TemplateInitParam tinit;
    Resource::Element search = Utils::GetParamWithHashFromId(PROGRESS_PAGE_BAR_TEMPLATE);

    for (int i = 0; i < BarNum; i++)
    {
        mainPlugin->AddWidgetFromTemplate(root, &search, &tinit);
        Widget *plane = root->GetChildByNum(root->childNum - 1);


        progressBars[i] = (ProgressBar *)Utils::GetChildByHash(plane, Utils::GetHashById(PROGRESS_PAGE_BAR));

        SceFVector4 pos;
        pos.x = 0;
        pos.y = -40 + (-40 * i);
        pos.z = 0;
        pos.w = 0;

        progressBars[i]->SetPosition(&pos);
        progressBars[i]->SetProgress(0, 0, 0);

        if(i == 0)
            busyIndicator = (BusyIndicator *)Utils::GetChildByHash(plane, Utils::GetHashById(BUSY_INICATOR_ID));

        SceFVector4 pos2;
        pos2.x = 225;
        pos2.y = -40 + (-40 * i) / 2;
        pos2.z = 0;
        pos2.w = 0;

        busyIndicator->SetPosition(&pos2);
        busyIndicator->Start();
    }

    infoText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(LOADING_PAGE_TEXT));
    infoText->SetLabel(&wstr);
}

ProgressPage::~ProgressPage()
{
    for(int i = 0; i < barNum; i++) progressBars[i]->SetProgress(100, 0, 0);
    delete[] progressBars;
}

void PopupMgr::showDialog()
{
    if(showingDialog) return;
    mainBackButton->Disable(SCE_FALSE);
    diagBG->PlayAnimation(0, Widget::Animation_3D_SlideFromFront);
    showingDialog = true;

    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    e.hash = Utils::GetHashById(POPUP_DIALOG_BOX);
    
    mainPlugin->AddWidgetFromTemplate(diag, &e, &tinit);

    diagBox = (Box *)diag->GetChildByNum(diag->childNum - 1);
}

void PopupMgr::hideDialog()
{
    if(!showingDialog) return;
    mainBackButton->Enable(SCE_FALSE);
    diagBG->PlayAnimationReverse(500, Widget::Animation_3D_SlideFromFront);
    common::Utils::WidgetStateTransition(0, diagBox, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    showingDialog = false;
}

void PopupMgr::initDialog()
{
    Resource::Element searchParam;
	checkmark = new graphics::Texture();
	searchParam.hash = Utils::GetHashById("_common_texture_check_mark");
	Plugin::LoadTexture(checkmark, Plugin::Find("__system__common_resource"), &searchParam);

	transparent = new graphics::Texture();
	searchParam.hash = Utils::GetHashById("_common_texture_transparent");
	Plugin::LoadTexture(transparent, Plugin::Find("__system__common_resource"), &searchParam);

    diagBG = (Plane *)Utils::GetChildByHash(mainScene, Utils::GetHashById(POPUP_DIALOG_BG));
    if(diagBG == NULL) return;

    diag = (Dialog *)Utils::GetChildByHash(diagBG, Utils::GetHashById(POPUP_DIALOG_ID));
    showingDialog = true;
    return hideDialog();
}

void PopupMgr::addDialogOption(const char *text, void (*onPress)(void *), void *userDat, bool selected)
{
    if(!showingDialog) return;
    
    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    e.hash = Utils::GetHashById(POPUP_DIALOG_BUTTON);

    mainPlugin->AddWidgetFromTemplate(diagBox, &e, &tinit);

    ImageButton *currbutton = (ImageButton *)diagBox->GetChildByNum(diagBox->childNum - 1);
    
    String str;
    str.Set(text);
    WString wstr;
    str.ToWString(&wstr);

    currbutton->SetLabel(&wstr);
    if(selected) currbutton->SetTextureBase(checkmark);
    else currbutton->SetTextureBase(transparent);

    eventcb cb;
    cb.dat = userDat;
    cb.onPress = onPress;

    DiagButtonEventHandler *eh = new DiagButtonEventHandler();
    eh->pUserData = sce_paf_malloc(sizeof(cb));
    sce_paf_memcpy(eh->pUserData, &cb, sizeof(cb));
    currbutton->RegisterEventCallback(ON_PRESS_EVENT_ID, eh, 1);

}

