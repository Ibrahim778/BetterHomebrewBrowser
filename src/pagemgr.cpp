#include "pagemgr.hpp"
#include "configmgr.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "main.hpp"
#include "bgdl.h"
#include "eventhandler.hpp"

SceInt32 Page::pageDepth = 0;
Page *Page::currPage = SCE_NULL;

Box * PopupMgr::diagBox = SCE_NULL;
Plane *PopupMgr::diagBG = SCE_NULL;
Dialog *PopupMgr::diag = SCE_NULL;
SceBool PopupMgr::showingDialog = SCE_FALSE;

graphics::Texture *PopupMgr::checkmark = SCE_NULL;
graphics::Texture *transparentTex = SCE_NULL;

SceVoid Page::SetRoot()
{
    Resource::Element search;
    Plugin::TemplateInitParam tini;

    switch (type)
    {
    
    DEFINE_PAGE(PAGE_TYPE_PROGRESS_PAGE, PROGRESS_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_LOADING_PAGE, LOADING_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_SELECTION_LIST, SELECTION_LIST_NO_TITLE_TEMPLATE)

    DEFINE_PAGE(PAGE_TYPE_SELECTION_LIST_WITH_TITLE, SELECTION_LIST_TEMPLATE)

    DEFINE_PAGE(PAGE_TYPE_TEXT_PAGE, TEXT_PAGE_NO_TITLE_ID)
    
    DEFINE_PAGE(PAGE_TYPE_TEXT_PAGE_WITH_TITLE, TEXT_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_HOMBREW_INFO_PAGE, INFO_PAGE_ID)
    
    DEFINE_PAGE(PAGE_TYPE_PICTURE_PAGE, PICTURE_PAGE_ID)
    
    DEFINE_PAGE(PAGE_TYPE_DECISION_PAGE, DECISION_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_HOMEBREW_LIST_PAGE, HOMEBREW_LIST_PAGE_ID)
    
    DEFINE_PAGE(PAGE_TYPE_SEARCH_PAGE, SEARCH_PAGE_TEMPLATE_ID)

    default:
        search = Utils::GetParamWithHashFromId(BLANK_PAGE_ID);
        break;
    }

    mainPlugin->TemplateOpen(mainRoot, &search, &tini);
    root = (Plane *)mainRoot->GetChildByNum(mainRoot->childNum - 1);
}

Page *Page::GetCurrentPage()
{
    return currPage;
}

Page::Page(pageType pageType, const char *threadName)
{
    type = pageType;

    this->prev = currPage;
    currPage = this;
   
    OnDelete = NULL;
    AfterDelete = NULL;
    OnRedisplay = NULL;

    pageDepth ++;
    skipAnimation = SCE_FALSE;

    jobs = new UtilQueue(this);

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


    SetRoot();


    busy = (BusyIndicator *)Utils::GetChildByHash(root, Utils::GetHashById(BUSY_INICATOR_ID));
    busy->Start();

    root->PlayAnimation(-50000, Widget::Animation_3D_SlideFromFront);
	if (root->animationStatus & 0x80)
		root->animationStatus &= ~0x80;

    if(pageDepth > 1)
        mainBackButton->PlayAnimation(0, Widget::Animation_Reset);
    else mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);

    if(pageDepth == 1)
        settingsButton->PlayAnimation(0, Widget::Animation_Reset);
    else settingsButton->PlayAnimationReverse(0, Widget::Animation_Reset);

}

TextPage::TextPage(const char *text, const char *title):Page(title != SCE_NULL ? PAGE_TYPE_TEXT_PAGE_WITH_TITLE : PAGE_TYPE_TEXT_PAGE)
{
    busy->Stop();

    InfoText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(TEXT_PAGE_TEXT));

    Utils::SetWidgetLabel(InfoText, text);

    if(title != NULL)
    {
        TitleText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(TEXT_PAGE_TITLE_TEXT));
        Utils::SetWidgetLabel(TitleText, title);
    }
}

Page::~Page()
{
    this->root->SetAlpha(0.39f);

    jobs->End = SCE_TRUE;


    if(OnDelete != NULL)
        OnDelete(this);

    common::Utils::WidgetStateTransition(-100, this->root, Widget::Animation_3D_SlideFromFront, SCE_TRUE, currPage == this ? skipAnimation : SCE_TRUE);


    if(currPage == this)
    {
        if (prev != SCE_NULL && !skipAnimation) 
        {
            prev->root->PlayAnimationReverse(0.0f, ui::Widget::Animation_3D_SlideToBack1);
            prev->root->PlayAnimation(0.0f, ui::Widget::Animation_Reset);
            if (prev->root->animationStatus & 0x80)
                prev->root->animationStatus &= ~0x80;
        
            if (prev->prev != SCE_NULL) {
                prev->prev->root->PlayAnimation(0.0f, ui::Widget::Animation_Reset);
                if (prev->prev->root->animationStatus & 0x80)
                    prev->prev->root->animationStatus &= ~0x80;
            }
        }
        currPage = this->prev;
        if(currPage->OnRedisplay != NULL) currPage->OnRedisplay(currPage);
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

    if(AfterDelete != NULL)
        AfterDelete(this);
}

ImageButton *SelectionList::AddOption(const char *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
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
    
    mainPlugin->TemplateOpen(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);

    Utils::SetWidgetLabel(button, text);
    if(onPress != NULL)
        Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

ImageButton *SelectionList::AddOption(String *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
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
    
    mainPlugin->TemplateOpen(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);

    if(text != NULL)
        Utils::SetWidgetLabel(button, text);
    
    if(onPress != NULL)
        Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

ImageButton *SelectionList::AddOption(WString *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
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
    
    mainPlugin->TemplateOpen(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);

    if(text != NULL)
        button->SetLabel(text);
    
    if(onPress != NULL)
        Utils::AssignButtonHandler(button, onPress, userDat);

    return button;

}

SceVoid SelectionList::Clear()
{
    for(int i = 0; i < scrollViewBox->childNum; i++)
    {
        Widget *w = scrollViewBox->GetChildByNum(i);
        if(w != NULL)
        {
            w->SetTextureBase(transparentTex);
        }
    }

    common::Utils::WidgetStateTransition(0, listRoot, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    
    Resource::Element search = Utils::GetParamWithHashFromId(type == PAGE_TYPE_SELECTION_LIST_WITH_TITLE ? LIST_TEMPLATE_ID : LIST_TEMPLATE_ID_NO_TITLE);
    Plugin::TemplateInitParam tinit;

    mainPlugin->TemplateOpen(root, &search, &tinit);

    listRoot = (Plane *)root->GetChildByNum(root->childNum - 1);
    scrollViewBox = (Box *)Utils::GetChildByHash(listRoot, Utils::GetHashById(LIST_SCROLL_BOX));

    disabled = false;
}

SceVoid SelectionList::DisableAllButtons()
{
    if(disabled) return;
    for (int i = 0; i < this->scrollViewBox->childNum; i++)
    {
        Widget *button = this->scrollViewBox->GetChildByNum(i);

        if(button == SCE_NULL)
            break;

        button->Disable(0);
    }

    disabled = true;
}

SceVoid SelectionList::EnableAllButtons()
{
    if(!disabled) return;
    for (int i = 0; i < this->scrollViewBox->childNum; i++)
    {
        Widget *button = this->scrollViewBox->GetChildByNum(i);

        if(button == SCE_NULL)
            break;

        button->Enable(0);
    }

    disabled = false;
}

SelectionList::SelectionList(const char *title):Page(title != NULL ? PAGE_TYPE_SELECTION_LIST_WITH_TITLE : PAGE_TYPE_SELECTION_LIST)
{
    if(title != NULL)
    {
        WString wstr;
        WString::CharToNewWString(title, &wstr);

        TitleText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(PAGE_TITLE_ID));
        TitleText->SetLabel(&wstr);
    }

    listRoot = (Plane *)AddFromTemplate(title != NULL ? LIST_TEMPLATE_ID : LIST_TEMPLATE_ID_NO_TITLE, root);

    scrollViewBox = (Box *)Utils::GetChildByHash(listRoot, Utils::GetHashById(LIST_SCROLL_BOX));

    disabled = false;
}

SelectionList::~SelectionList()
{
    for(int i = 0; i < scrollViewBox->childNum; i++)
    {
        Widget *w = scrollViewBox->GetChildByNum(i);
        if(w != NULL)
        {
            w->SetTextureBase(transparentTex);
        }
    }
}

SceVoid SelectionList::Hide()
{
    listRoot->PlayAnimationReverse(0, Widget::Animation_Reset);
}

SceVoid SelectionList::Show()
{
    listRoot->PlayAnimation(0, Widget::Animation_Reset);
}

SceInt32 SelectionList::GetNum()
{
    return scrollViewBox->childNum;   
}

HomebrewListPage::HomebrewListPage(const char *threadName):Page(PAGE_TYPE_HOMEBREW_LIST_PAGE, threadName)
{
    listPlane = (Plane *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_LIST_PLANE_ID));
    listRoot = (Plane *)AddFromTemplate(HOMEBREW_LIST_TEMPLATE_ID, listPlane);
    scrollBox = (Box *)Utils::GetChildByHash(listRoot, Utils::GetHashById(LIST_SCROLL_BOX));

    AllButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_ALL_BUTTON_ID));
    EmulatorButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_EMU_BUTTON_ID));
    PortButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_PORT_BUTTON_ID));
    GamesButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_GAME_BUTTON_ID));
    UtilitiesButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_UTIL_BUTTON_ID));
    SearchButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_SEARCH_BUTTON_ID));
}

SceVoid HomebrewListPage::AssignAllButtonEvent(ECallback c, void *d)
{
    Utils::AssignButtonHandler(AllButton, c, d);
}

SceVoid HomebrewListPage::AssignEmulatorButtonEvent(ECallback c, void *d)
{
    Utils::AssignButtonHandler(EmulatorButton, c, d);
}

SceVoid HomebrewListPage::AssignGameButtonEvent(ECallback c, void *d)
{
    Utils::AssignButtonHandler(GamesButton, c, d);
}

SceVoid HomebrewListPage::AssignPortButtonEvent(ECallback c, void *d)
{
    Utils::AssignButtonHandler(PortButton, c, d);
}

SceVoid HomebrewListPage::AssignSearchButtonEvent(ECallback c, void *d)
{
    Utils::AssignButtonHandler(SearchButton, c, d);
}

SceVoid HomebrewListPage::AssignUtilitiesButtonEvent(ECallback c, void *d)
{
    Utils::AssignButtonHandler(UtilitiesButton, c, d);
}

SceVoid HomebrewListPage::Clear()
{
    for(int i = 0; i < scrollBox->childNum; i++)
    {
        Widget *w = scrollBox->GetChildByNum(i);
        if(w != NULL)
        {
            w->SetTextureBase(transparentTex);
        }
    }

    common::Utils::WidgetStateTransition(0, listRoot, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    listRoot = (Plane *)AddFromTemplate(HOMEBREW_LIST_TEMPLATE_ID, listPlane);
    scrollBox = (Box *)Utils::GetChildByHash(listRoot, Utils::GetHashById(LIST_SCROLL_BOX));
}

SceVoid HomebrewListPage::Hide()
{
    listRoot->PlayAnimationReverse(0, Widget::Animation_Reset);
}

SceVoid HomebrewListPage::Show()
{
    listRoot->PlayAnimation(0, Widget::Animation_Reset);
}

SceInt32 HomebrewListPage::GetNum()
{
    return scrollBox->childNum;
}

SceVoid HomebrewListPage::SetSearchMode()
{
    Utils::SetWidgetPosition(SearchButton, 0, 0);

    AllButton->PlayAnimationReverse(0, Widget::Animation_Reset);
    GamesButton->PlayAnimationReverse(0, Widget::Animation_Reset);
    EmulatorButton->PlayAnimationReverse(0, Widget::Animation_Reset);
    PortButton->PlayAnimationReverse(0, Widget::Animation_Reset);
    UtilitiesButton->PlayAnimationReverse(0, Widget::Animation_Reset);
}

SceVoid HomebrewListPage::SetCategoryColor(int category)
{
    Utils::SetWidgetColor(AllButton, 1,1,1,1);
    Utils::SetWidgetColor(GamesButton, 1,1,1,1);
    Utils::SetWidgetColor(EmulatorButton, 1,1,1,1);
    Utils::SetWidgetColor(PortButton, 1,1,1,1);
    Utils::SetWidgetColor(UtilitiesButton, 1,1,1,1);

    switch (category)
    {
    case -1:
        Utils::SetWidgetColor(AllButton, 1, 0.5490196078f, 0, 1);
        break;
    
    case GAME:
        Utils::SetWidgetColor(GamesButton, 1, 0.5490196078f, 0, 1);
        break;
    
    case EMULATOR:
        Utils::SetWidgetColor(EmulatorButton, 1, 0.5490196078f, 0, 1);
        break;
    
    case PORT:
        Utils::SetWidgetColor(PortButton, 1, 0.5490196078f, 0, 1);
        break;
    
    case UTIL:
        Utils::SetWidgetColor(UtilitiesButton, 1, 0.5490196078f, 0, 1);
        break;
    
    default:
        break;
    }
}

Box *HomebrewListPage::GetListBox()
{
    return scrollBox;
}

HomebrewListPage::~HomebrewListPage()
{
    for(int i = 0; i < scrollBox->childNum; i++)
    {
        Widget *w = scrollBox->GetChildByNum(i);
        if(w != NULL)
        {
            w->SetTextureBase(transparentTex);
        }
    }
}

ImageButton *HomebrewListPage::AddOption(const char *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
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
    
    mainPlugin->TemplateOpen(scrollBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollBox->GetChildByNum(scrollBox->childNum - 1);

    Utils::SetWidgetLabel(button, text);
    if(onPress != NULL)
        Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

ImageButton *HomebrewListPage::AddOption(String *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
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
    
    mainPlugin->TemplateOpen(scrollBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollBox->GetChildByNum(scrollBox->childNum - 1);

    if(text != NULL)
        Utils::SetWidgetLabel(button, text);
    
    if(onPress != NULL)
        Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

ImageButton *HomebrewListPage::AddOption(WString *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
    while(scrollBox == NULL) paf::thread::Sleep(10000);
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
    
    mainPlugin->TemplateOpen(scrollBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollBox->GetChildByNum(scrollBox->childNum - 1);

    if(text != NULL)
        button->SetLabel(text);
    
    if(onPress != NULL)
        Utils::AssignButtonHandler(button, onPress, userDat);

    return button;

}

BUTTON_CB(SearchPageSearchButtonCB)
{
    ((SearchPage *)Page::GetCurrentPage())->Search();
}

SearchPage::SearchPage(LinkedList *list):Page(PAGE_TYPE_SEARCH_PAGE)
{
    busy->Stop();
    listPlane = (Plane *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_LIST_PLANE_ID));
    listRoot = (Plane *)AddFromTemplate(HOMEBREW_LIST_TEMPLATE_ID, listPlane);
    scrollBox = (Box *)Utils::GetChildByHash(listRoot, Utils::GetHashById(LIST_SCROLL_BOX));

    SearchBox = (TextBox *)Utils::GetChildByHash(root, Utils::GetHashById(SEARCH_PAGE_SEARCH_BOX_ID));
    SearchButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(HOMEBREW_LIST_PAGE_SEARCH_BUTTON_ID));
    Utils::AssignButtonHandler(SearchButton, SearchPageSearchButtonCB);
    this->list = list;
}

int SearchPage::GetNum()
{
    return scrollBox->childNum;
}

ImageButton *SearchPage::GetEntryByIndex(int index)
{
    return (ImageButton *)scrollBox->GetChildByNum(index);
}

THREAD(SearchThread)
{
    SearchPage *currPage = (SearchPage *)callingPage;
    currPage->busy->Start();
    currPage->Clear();
    currPage->Hide();

    String keystring;
    currPage->GetKeyString(&keystring);
    
    if(keystring.length == 0)
    {
        currPage->busy->Stop();
        currPage->Show();
        return;
    }

    Utils::ToLowerCase(keystring.data);

    //Get rid of trailing space char
    for(int i = keystring.length; i > 0; i--)
    {
        if(keystring.data[i] == ' ')
        {
            keystring.data[i] = 0;
            break;
        }
    }

 /*
    HomebrewListButtonEventHandler *eh = new HomebrewListButtonEventHandler();

    node *n = currPage->list->head;
    for(int i = 0; i < list.num && n != NULL && !currPage->pageThread->EndThread; n = n->next, i++)
    {
        String title;
        String titleID;
        title = n->info.title.data;
        titleID = n->info.titleID.data;

        Utils::ToLowerCase(title.data);
        Utils::ToLowerCase(titleID.data);

        if(Utils::StringContains(title.data, keystring.data) || Utils::StringContains(titleID.data, keystring.data))
        {
            ImageButton *b = currPage->AddEntry(&n->info.wstrtitle, NULL, NULL, SCE_TRUE, SCE_TRUE);
            if(b != NULL)
            {
                eh->pUserData = &n->info;
                b->RegisterEventCallback(ON_PRESS_EVENT_ID, eh, 0);
            }
        }
    }

    currPage->busy->Stop();
    currPage->Show();
    int num = currPage->GetNum();
    for(int i = 0; i < num && !currPage->pageThread->EndThread; i++)
    {
        WString *wstr = new WString();
        String str;
        ImageButton *currButton = currPage->GetEntryByIndex(i);
        currButton->GetLabel(wstr);
        wstr->ToString(&str);

        delete wstr;

        node *n = list.Find(str.data);

        bool triedDownload = false; 
        if(n->tex->texSurface == NULL)
        {
ASSIGN:        
            if(Utils::CreateTextureFromFile(n->tex, n->info.icon0Local.data))
            {
                currButton->SetTextureBase(n->tex);
            }
            else
            {
                if(!triedDownload)
                {
                    char url[0x400];
                    sce_paf_memset(url, 0, sizeof(url));
                    sce_paf_snprintf(url, 0x400, VITADB_ICON_URL "%s", n->info.icon0);
                    Utils::DownloadFile(url, n->info.icon0Local.data, currPage);
                    triedDownload = true;
                    goto ASSIGN;
                }
                currButton->SetTextureBase(BrokenTex);
            }
        }
        else currButton->SetTextureBase(n->tex);
    }*/
}        

void SearchPage::Hide()
{
    listRoot->PlayAnimationReverse(0, Widget::Animation_Reset);
}

void SearchPage::Show()
{
    listRoot->PlayAnimation(0, Widget::Animation_Reset);
}

void SearchPage::Search()
{
    jobs->AddTask(SearchThread, "BHBB_SEARCH_TASK");
}

void SearchPage::GetKeyString(String *out)
{
    WString wstr;
    SearchBox->GetLabel(&wstr);
    wstr.ToString(out);
}

void SearchPage::Clear()
{
    for(int i = 0; i < scrollBox->childNum; i++)
    {
        Widget *w = scrollBox->GetChildByNum(i);
        if(w != NULL)
        {
            w->SetTextureBase(transparentTex);
        }
    }

    common::Utils::WidgetStateTransition(0, listRoot, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    listRoot = (Plane *)AddFromTemplate(HOMEBREW_LIST_TEMPLATE_ID, listPlane);
    scrollBox = (Box *)Utils::GetChildByHash(listRoot, Utils::GetHashById(LIST_SCROLL_BOX));
}

ImageButton *SearchPage::AddEntry(WString *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
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
    
    mainPlugin->TemplateOpen(scrollBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollBox->GetChildByNum(scrollBox->childNum - 1);

    button->SetLabel(text);

    if(onPress != NULL)
        Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

SearchPage::~SearchPage()
{
    for(int i = 0; i < scrollBox->childNum; i++)
    {
        Widget *w = scrollBox->GetChildByNum(i);
        if(w != NULL)
        {
            w->SetTextureBase(transparentTex);
        }
    }
}

void Page::Init()
{
    pageDepth = 0;
    mainBackButton->RegisterEventCallback(ON_PRESS_EVENT_ID, mainBackButtonEvent, 0);
    settingsButton->RegisterEventCallback(ON_PRESS_EVENT_ID, new SettingsButtonEventHandler(), 0);
    forwardButton->RegisterEventCallback(ON_PRESS_EVENT_ID, mainForwardButtonEvent, 0);

    //Not really needed but just to be sure
    currPage = SCE_NULL;

    forwardButton->PlayAnimationReverse(0, Widget::Animation_Reset);
    mainBackButton->PlayAnimationReverse(0, Widget::Animation_Reset);

    EventHandler::ResetBackButtonEvent();
}

void Page::DeletePage(Page *p, bool skipAnim)
{
    p->skipAnimation = !skipAnim;

    //Just to call the correct destructors, if you have a better way to do this, please fix it
    switch (p->type)
    {

    DELETE_PAGE_TYPE(PAGE_TYPE_SELECTION_LIST, SelectionList)
    DELETE_PAGE_TYPE(PAGE_TYPE_SELECTION_LIST_WITH_TITLE, SelectionList)
    DELETE_PAGE_TYPE(PAGE_TYPE_TEXT_PAGE, TextPage)
    DELETE_PAGE_TYPE(PAGE_TYPE_TEXT_PAGE_WITH_TITLE, TextPage)
    DELETE_PAGE_TYPE(PAGE_TYPE_LOADING_PAGE, LoadingPage)
    DELETE_PAGE_TYPE(PAGE_TYPE_PROGRESS_PAGE, ProgressPage)
    DELETE_PAGE_TYPE(PAGE_TYPE_HOMBREW_INFO_PAGE, InfoPage)
    DELETE_PAGE_TYPE(PAGE_TYPE_PICTURE_PAGE, PicturePage)
    DELETE_PAGE_TYPE(PAGE_TYPE_DECISION_PAGE, DecisionPage)
    DELETE_PAGE_TYPE(PAGE_TYPE_HOMEBREW_LIST_PAGE, HomebrewListPage)
    DELETE_PAGE_TYPE(PAGE_TYPE_SEARCH_PAGE, SearchPage)

    default:
        delete p;
        break;
    }
}

SceInt32 PicturePage::AddPictureFromFile(const char *file)
{
    if(Page::GetCurrentPage() != this) return -1;
    mainBackButton->Disable(0);
    if(pictureNum == 0)
        pictures = (graphics::Texture **)sce_paf_malloc(sizeof(graphics::Texture *));
    else if(pictureNum > 0) pictures = (graphics::Texture **)sce_paf_realloc(pictures, sizeof(graphics::Texture *) * (pictureNum + 1));

    pictures[pictureNum] = new graphics::Texture();

    SceInt32 r;
    if(Utils::CreateTextureFromFile(pictures[pictureNum], file))
    {
        Plane *p = (Plane *)AddFromTemplate(PICTURE_PAGE_PICTURE_TEMPLATE, listRoot);
        r = p->SetTextureBase(pictures[pictureNum]);
        pictureNum++;
    }
    else
    {
        delete pictures[pictureNum];
        if(pictureNum == 0)
        {
            sce_paf_free(pictures);
            pictures = SCE_NULL;
        }
        else if(pictures > 0)
        {
            pictures = (graphics::Texture **)sce_paf_realloc(pictures, sizeof(graphics::Texture *) * pictureNum);
        }

        r = -1;
    }

    mainBackButton->Enable(0);
    return r;
}

SceInt32 PicturePage::AddPicture(graphics::Texture *src)
{
    if(Page::GetCurrentPage() != this) return -1;
    if(src == NULL) return -1;

    return AddFromTemplate(PICTURE_PAGE_PICTURE_TEMPLATE, listRoot)->SetTextureBase(src);
}

PicturePage::PicturePage():Page(PAGE_TYPE_PICTURE_PAGE)
{
    pictures = NULL;
    pictureNum = 0;
    pictures = NULL;
    listRoot = (Box *)Utils::GetChildByHash(root, Utils::GetHashById(LIST_SCROLL_BOX));
    busy->Stop();
}

PicturePage::~PicturePage()
{
    for(int i = 0; i < listRoot->childNum; i++)
    {
        Widget *w = listRoot->GetChildByNum(i);
        if(w != NULL) w->SetTextureBase(transparentTex);
    }

    if(pictures != NULL)
    {
        for(int i = 0; i < pictureNum; i++)
        {
            if(pictures[i] != NULL)
            {
                Utils::DeleteTexture(pictures[i]);

                pictures[i] = SCE_NULL;
            }
        }

        delete[] pictures;
    }
}

THREAD(DownloadRemainingScreenShotThread)
{
    PicturePage *picturePage = (PicturePage *)callingPage;
    InfoPage *info = (InfoPage *)picturePage->prev;
    for(int i = 1; i < info->ScreenshotNum && !picturePage->jobs->End; i++)
    {
        CURLcode r = (CURLcode)Utils::DownloadFile(info->ScreenShotURLS[i], info->ScreenshotPaths[i], picturePage);

        if(paf::io::Misc::Exists(info->ScreenshotPaths[i]) && r == CURLE_OK)
        {
            picturePage->AddPictureFromFile(info->ScreenshotPaths[i]);
        }
    }
}

BUTTON_CB(ShowScreenShotPage)
{
    graphics::Texture *screenShot1 = ((InfoPage *)Page::GetCurrentPage())->mainScreenshot;
    PicturePage *pp = new PicturePage();
    pp->AddPicture(screenShot1);
    pp->jobs->AddTask(DownloadRemainingScreenShotThread, "BHBB_SCREENSHOT_DOWNLOAD_TASK");
}

THREAD(ScreenshotDownloadThread)
{
    InfoPage *page = (InfoPage *)callingPage;
    Utils::ResetStrtok();

    page->ScreenshotNum = Utils::getStrtokNum(';', page->Info->screenshot_url.data);
    print("Strtoknum = %d\n", page->ScreenshotNum);
    page->ScreenShotURLS = (char **)sce_paf_malloc(page->ScreenshotNum * sizeof(char *));
    page->ScreenshotPaths = (char **)sce_paf_malloc(page->ScreenshotNum * sizeof(char *));

    //Download all textures and save the path in an array
    for(int i = 0; !page->jobs->End; i++)
    {
        char *str = Utils::strtok(';', page->Info->screenshot_url.data);
        if(str == NULL) break;
     
        page->ScreenshotPaths[i] = (char *)sce_paf_malloc(SCE_IO_MAX_PATH_BUFFER_SIZE);
        sce_paf_memset(page->ScreenshotPaths[i], 0, SCE_IO_MAX_PATH_BUFFER_SIZE);

        page->ScreenShotURLS[i] = (char *)sce_paf_malloc(250);
        sce_paf_memset(page->ScreenShotURLS[i], 0, 250);

        sce_paf_snprintf(page->ScreenshotPaths[i], SCE_IO_MAX_PATH_BUFFER_SIZE, DATA_PATH "/%s", str);
        sce_paf_snprintf(page->ScreenShotURLS[i], 250, "https://rinnegatamante.it/vitadb/%s", str);
        sce_paf_free(str);

    }
    CURLcode res = (CURLcode)Utils::DownloadFile(page->ScreenShotURLS[0], page->ScreenshotPaths[0], page);
    if(res != CURLE_OK)
    {
        if(!page->jobs->End)
        {
            new TextPage(curl_easy_strerror(res), "Screenshot Download Error");
        }
        return;
    }

    if(paf::io::Misc::Exists(page->ScreenshotPaths[0]))
    {
        graphics::Texture *tex = new graphics::Texture();
        if(Utils::CreateTextureFromFile(tex, page->ScreenshotPaths[0]))
        {
            page->mainScreenshot = tex;
            page->ScreenShot->SetTextureBase(page->mainScreenshot);
            Utils::AssignButtonHandler(page->ScreenShot, ShowScreenShotPage);
        }
    }
    else
    {
        page->mainScreenshot = NULL;
        if(page->jobs->End)
            new TextPage("File Missing", "Screenshot Download Error");
    }
}

BUTTON_CB(DownloadApp)
{
    homeBrewInfo *info = (homeBrewInfo *)userDat;
    /*
    int size_needed = sce_paf_strtoul(info->size.data, NULL, 10) * 2;
    
    SceIoDevInfo devinfo;
    sceIoDevctl("ux0:", 0x3001, NULL, 0, &info, sizeof(info));
    
    if(devinfo.free_size < size_needed)
    {
        new TextPage("Not enough free space!");
        return;
    }
    */

    int r = 0;
    if(r = SendDlRequest(info->title.data, info->download_url.data) == 0)
    {
        new TextPage("Added to queue");
    }
    else
    {
        char txt[0x100];
        sce_paf_memset(txt, 0, 0x100);
        sce_paf_snprintf(txt, 0x100, "Error 0x%X", r);
        new TextPage(txt, "Error adding to queue");
    }
}

InfoPage::InfoPage(homeBrewInfo *info):Page(PAGE_TYPE_HOMBREW_INFO_PAGE)
{
    this->Info = info;

    TitleText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(PAGE_TITLE_ID));
    Icon = (Plane *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_ICON_ID));
    Description = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_DESCRIPTION_TEXT_ID));
    ScreenShot = (CompositeButton *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_SCREENSHOT_ID));
    Credits = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_CREDITS_TEXT_ID));
    DownloadButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_DOWNLOAD_BUTTON_ID));
    Version = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_VERSION_TEXT_ID));
    Size = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_SIZE_TEXT_ID));

    if(conf.db == CBPSDB)
    {
        Utils::DeleteWidget(Version);
    }
    else
    {
        Utils::SetWidgetLabel(Version, &Info->version);
    }

    if(info->download_url.length != 0)
        Utils::AssignButtonHandler(DownloadButton, DownloadApp, Info);

    Utils::SetWidgetLabel(TitleText, &Info->title);
    Utils::SetWidgetColor(DownloadButton, 1, 0.5490196078f, 0, 1);

    Utils::SetWidgetLabel(Credits, info->credits.data);
    
    float y = (Info->description.length * 4.5f) / 5.0;
    if(Info->description.length <= 15) y = 130;

    y += 80;

    Utils::SetWidgetSize(Description, 960, y);

    Utils::SetWidgetLabel(Description, &Info->description);

    if(loadFlags & LOAD_FLAGS_ICONS)
    {
        iconTex = new graphics::Texture();
        if(paf::io::Misc::Exists(info->icon0Local.data) && Utils::CreateTextureFromFile(iconTex, info->icon0Local.data))
        {
            Icon->SetTextureBase(iconTex);
        }
        else
        {
            delete iconTex;
            iconTex = SCE_NULL;
            Icon->SetTextureBase(BrokenTex);
        }
    }

    ScreenShotURLS = NULL;
    ScreenshotPaths = NULL;
    mainScreenshot = NULL;
    
    char sizeText[40];
    sce_paf_memset(sizeText, 0, sizeof(sizeText));
    
    int size = sce_paf_strtoul(Info->size.data, NULL, 10);

    char *extension = "B";
    if(size > 1024)
    {
        size = (int)(size / 1024.0);
        extension = "KB";
    }
    if(size > 1024)
    {
        size = (int)(size / 1024.0);
        extension = "MB";
    }
    if(size > 1024)
    {
        size = (int)(size / 1024.0);
        extension = "GB";
    }

    sce_paf_snprintf(sizeText, sizeof(sizeText), "%d %s", size, extension);
    Utils::SetWidgetLabel(Size, sizeText);

    if(Info->screenshot_url.length != 0 && (loadFlags & LOAD_FLAGS_SCREENSHOTS))
    {
        jobs->AddTask(ScreenshotDownloadThread, "SNAPSHOT_DOWNLOAD_TASK");
    }
    else
    {
        Utils::DeleteWidget(ScreenShot);
    }
    this->busy->Stop();
}

InfoPage::~InfoPage()
{
    if(!(loadFlags & LOAD_FLAGS_SCREENSHOTS)) goto DELETE_ICON;

    if(ScreenshotPaths != NULL) 
    {
        for(int i = 0; i < ScreenshotNum; i++)
        {
            if(ScreenshotPaths[i] != NULL) sce_paf_free(ScreenshotPaths[i]);
        }
        sce_paf_free(ScreenshotPaths);
        ScreenshotPaths = SCE_NULL;
    }
    
    if(ScreenShotURLS != NULL)
    {
        for (int i = 0; i < ScreenshotNum; i++)
        {
            if(ScreenShotURLS[i] != NULL) sce_paf_free(ScreenShotURLS[i]);
        }
        sce_paf_free(ScreenShotURLS);
        ScreenShotURLS = SCE_NULL;
    }
    
    if(mainScreenshot != NULL)
    {
        ScreenShot->SetTextureBase(transparentTex);
        Utils::DeleteTexture(mainScreenshot);
        mainScreenshot = SCE_NULL;
    }

DELETE_ICON:
    if(iconTex != NULL)
    {
        Icon->SetTextureBase(transparentTex);
        Utils::DeleteTexture(iconTex);
        iconTex = SCE_NULL;
    }

}

LoadingPage::LoadingPage(const char *info):Page(PAGE_TYPE_LOADING_PAGE)
{
    skipAnimation = SCE_TRUE;
    WString wstr;
    WString::CharToNewWString(info, &wstr);

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

    WString wstr;
    WString::CharToNewWString(info, &wstr);

    progressBars = new ProgressBar *[BarNum];

    Plugin::TemplateInitParam tinit;
    Resource::Element search = Utils::GetParamWithHashFromId(PROGRESS_PAGE_BAR_TEMPLATE);

    for (int i = 0; i < BarNum; i++)
    {
        mainPlugin->TemplateOpen(root, &search, &tinit);
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

DecisionPage::DecisionPage(const char *text, ECallback onConfirmPress, ECallback onDeclinePress, DecisionType type):Page(PAGE_TYPE_DECISION_PAGE)
{
    busy->Stop();

    ConfirmButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(DECISION_PAGE_CONFIRM_BUTTON_ID));
    DeclineButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(DECISION_PAGE_DECLINE_BUTTON_ID));
    InfoText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(DECISION_PAGE_TEXT_ID));

    if(text != NULL)
        Utils::SetWidgetLabel(InfoText, text);

    if(onConfirmPress != NULL)
        Utils::AssignButtonHandler(ConfirmButton, onConfirmPress);
    if(onDeclinePress != NULL)
        Utils::AssignButtonHandler(DeclineButton, onDeclinePress);
    
    switch (type)
    {
    case DECISION_TYPE_OK_CANCEL:
        Utils::SetWidgetLabel(ConfirmButton, "OK");
        Utils::SetWidgetLabel(DeclineButton, "Cancel");
        break;

    default:
    case DECISION_TYPE_YES_NO:
        Utils::SetWidgetLabel(ConfirmButton, "Yes");
        Utils::SetWidgetLabel(DeclineButton, "No");
        break;
    }
}

BUTTON_CB(DialogBackEvent)
{
    PopupMgr::HideDialog();
}

void PopupMgr::ShowDialog()
{
    if(showingDialog) return;

    if(Page::GetCurrentPage() != NULL)
    {
        Page::GetCurrentPage()->root->SetAlpha(0.39f);
    }
    
    EventHandler::SetBackButtonEvent(DialogBackEvent);
    diagBG->PlayAnimation(0, Widget::Animation_3D_SlideFromFront);
    showingDialog = true;

    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    e.hash = Utils::GetHashById(POPUP_DIALOG_BOX);
    
    mainPlugin->TemplateOpen(diag, &e, &tinit);

    diagBox = (Box *)diag->GetChildByNum(diag->childNum - 1);
}

void PopupMgr::HideDialog()
{
    if(!showingDialog) return;

    if(Page::GetCurrentPage() != NULL)
    {
        Page::GetCurrentPage()->root->SetAlpha(1.0f);
    }

    EventHandler::ResetBackButtonEvent();
    diagBG->PlayAnimationReverse(500, Widget::Animation_3D_SlideFromFront);
    common::Utils::WidgetStateTransition(0, diagBox, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    showingDialog = false;
}

void PopupMgr::InitDialog()
{
    Resource::Element searchParam;
	checkmark = new graphics::Texture();
	searchParam.hash = Utils::GetHashById("_common_texture_check_mark");
	Plugin::LoadTexture(checkmark, Plugin::Find("__system__common_resource"), &searchParam);

	transparentTex = new graphics::Texture();
	searchParam.hash = Utils::GetHashById("_common_texture_transparent");
	Plugin::LoadTexture(transparentTex, Plugin::Find("__system__common_resource"), &searchParam);

    diagBG = (Plane *)Utils::GetChildByHash(mainScene, Utils::GetHashById(POPUP_DIALOG_BG));
    if(diagBG == NULL) return;

    diag = (Dialog *)Utils::GetChildByHash(diagBG, Utils::GetHashById(POPUP_DIALOG_ID));
    showingDialog = true;
    return HideDialog();
}

//From QuickMenuReborn
Widget *Page::AddFromStyle(const char *refId, const char *style, const char *type, Widget *parent)
{
    //WidgetInfo
    paf::Resource::Element winfo;
    //StyleInfo
    paf::Resource::Element sinfo;
    //Search Request
    paf::Resource::Element searchRequest;

    searchRequest.id = refId;
    winfo.hash = winfo.GetHashById(&searchRequest);
    
    searchRequest.id = style;
    sinfo.hash = sinfo.GetHashById(&searchRequest);    
    
    Widget *newWidget = mainPlugin->CreateWidgetWithStyle(parent != NULL ? parent : root, type, &winfo, &sinfo);
    if(newWidget == NULL) print("Error can't make widget with refID %s, type %s, style = %s\n", refId, type, style);
    return newWidget;
}

Widget *Page::AddFromTemplate(SceInt32 id, Widget *targetRoot)
{
    Resource::Element e = Utils::GetParamWithHash(id);
    Plugin::TemplateInitParam tinit;

    mainPlugin->TemplateOpen(targetRoot == NULL ? root : targetRoot, &e, &tinit);
    return targetRoot == NULL ? root->GetChildByNum(root->childNum - 1) : targetRoot->GetChildByNum(targetRoot->childNum - 1);
}

Widget *Page::AddFromTemplate(const char *id, Widget *targetRoot)
{
    Resource::Element e = Utils::GetParamWithHashFromId(id);
    Plugin::TemplateInitParam tinit;

    mainPlugin->TemplateOpen(targetRoot == NULL ? root : targetRoot, &e, &tinit);
    return targetRoot == NULL ? root->GetChildByNum(root->childNum - 1) : targetRoot->GetChildByNum(targetRoot->childNum - 1);
}

void PopupMgr::AddDialogOption(const char *text, ECallback onPress, void *userDat, bool selected)
{
    if(!showingDialog) return;
    
    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    e.hash = Utils::GetHashById(POPUP_DIALOG_BUTTON);

    mainPlugin->TemplateOpen(diagBox, &e, &tinit);

    ImageButton *currbutton = (ImageButton *)diagBox->GetChildByNum(diagBox->childNum - 1);

    Utils::SetWidgetLabel(currbutton, text);    

    if(selected) currbutton->SetTextureBase(checkmark);
    else currbutton->SetTextureBase(transparentTex);

    eventcb cb;
    cb.dat = userDat;
    cb.Callback = onPress;


    DiagButtonEventHandler *eh = new DiagButtonEventHandler();
    eh->pUserData = sce_paf_malloc(sizeof(cb));
    sce_paf_memcpy(eh->pUserData, &cb, sizeof(cb));
    currbutton->RegisterEventCallback(ON_PRESS_EVENT_ID, eh, 0);

}
