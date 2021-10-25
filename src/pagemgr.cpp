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

    default:
        search = BHBB::Utils::GetParamWithHashFromId(BLANK_PAGE_ID);
        break;
    }

    mainPlugin->AddWidgetFromTemplate(mainRoot, &search, &tini);
    root = (Plane *)mainRoot->GetChildByNum(mainRoot->childNum - 1);
}

Page *Page::GetCurrentPage()
{
    return currPage;
}

Page::Page(pageType page, SceBool wait)
{
    if(wait)
        if(currPage->pageThread != NULL)
            if(currPage->pageThread->IsStarted())
                currPage->pageThread->Join();
        
        
    type = page;

    this->prev = currPage;
    currPage = this;

    OnDelete = NULL;
    AfterDelete = NULL;
    OnRedisplay = NULL;

    pageDepth ++;
    skipAnimation = SCE_FALSE;

    currPage->pageThread = new UtilThread(SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SCE_KERNEL_16KiB, "BHBB_PAGE_THREAD");
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


    SetRoot();


    busy = (BusyIndicator *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(BUSY_INICATOR_ID));
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

    InfoText = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(TEXT_PAGE_TEXT));

    BHBB::Utils::SetWidgetLabel(InfoText, text);

    if(title != NULL)
    {
        TitleText = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(TEXT_PAGE_TITLE_TEXT));
        BHBB::Utils::SetWidgetLabel(TitleText, title);
    }
}

Page::~Page()
{
    this->root->SetAlpha(0.39f);

    if(this->pageThread != NULL)
    {
        this->pageThread->Delete();
    }


    if(OnDelete != NULL)
        OnDelete();

    common::Utils::WidgetStateTransition(-100, this->root, Widget::Animation_3D_SlideFromFront, SCE_TRUE, currPage == this ? skipAnimation : SCE_TRUE);


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
        if(currPage->OnRedisplay != NULL) currPage->OnRedisplay();
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
        AfterDelete();
}

ImageButton *SelectionList::AddOption(const char *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    if(isLong)
    {
        if(needImage)
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE_IMG);
        else
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE);
    }
    else
    {
        if(needImage)
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE_IMG);
        else
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE);
    }
    
    mainPlugin->AddWidgetFromTemplate(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);

    BHBB::Utils::SetWidgetLabel(button, text);
    if(onPress != NULL)
        BHBB::Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

ImageButton *SelectionList::AddOption(String *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    if(isLong)
    {
        if(needImage)
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE_IMG);
        else
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE);
    }
    else
    {
        if(needImage)
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE_IMG);
        else
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE);
    }
    
    mainPlugin->AddWidgetFromTemplate(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);

    if(text != NULL)
        BHBB::Utils::SetWidgetLabel(button, text);
    
    if(onPress != NULL)
        BHBB::Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

ImageButton *SelectionList::AddOption(WString *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
{
    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    if(isLong)
    {
        if(needImage)
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE_IMG);
        else
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_LONG_TEMPLATE);
    }
    else
    {
        if(needImage)
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE_IMG);
        else
            e = BHBB::Utils::GetParamWithHashFromId(LIST_BUTTON_TEMPLATE);
    }
    
    mainPlugin->AddWidgetFromTemplate(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);

    if(text != NULL)
        button->SetLabel(text);
    
    if(onPress != NULL)
        BHBB::Utils::AssignButtonHandler(button, onPress, userDat);

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
    
    Resource::Element search = BHBB::Utils::GetParamWithHashFromId(type == PAGE_TYPE_SELECTION_LIST_WITH_TITLE ? LIST_TEMPLATE_ID : LIST_TEMPLATE_ID_NO_TITLE);
    Plugin::TemplateInitParam tinit;

    mainPlugin->AddWidgetFromTemplate(root, &search, &tinit);

    listRoot = (Plane *)root->GetChildByNum(root->childNum - 1);
    scrollViewBox = (Box *)BHBB::Utils::GetChildByHash(listRoot, BHBB::Utils::GetHashById(LIST_SCROLL_BOX));

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
        String str;
        str.Set(title);

        WString wstr;
        str.ToWString(&wstr);

        TitleText = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(PAGE_TITLE_ID));
        TitleText->SetLabel(&wstr);
    }

    listRoot = (Plane *)AddFromTemplate(title != NULL ? LIST_TEMPLATE_ID : LIST_TEMPLATE_ID_NO_TITLE, root);

    scrollViewBox = (Box *)BHBB::Utils::GetChildByHash(listRoot, BHBB::Utils::GetHashById(LIST_SCROLL_BOX));

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

void Page::Init()
{
    pageDepth = 0;
    mainBackButton->RegisterEventCallback(ON_PRESS_EVENT_ID, new BackButtonEventHandler(), 0);
    settingsButton->RegisterEventCallback(ON_PRESS_EVENT_ID, new SettingsButtonEventHandler(), 0);
    forwardButton->RegisterEventCallback(ON_PRESS_EVENT_ID, new ForwardButtonEventHandler, 0);

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
    if(BHBB::Utils::CreateTextureFromFile(pictures[pictureNum], file))
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
    listRoot = (Box *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(LIST_SCROLL_BOX));
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
                BHBB::Utils::DeleteTexture(pictures[i]);

                delete pictures[i];
                pictures[i] = SCE_NULL;
            }
        }

        delete[] pictures;
    }
}

CB(DownloadRemainingScreenShotThread)
{
    InfoPage *info = (InfoPage *)Page::GetCurrentPage()->prev;
    for(int i = 1; i < info->ScreenshotNum && !Page::GetCurrentPage()->pageThread->EndThread; i++)
    {
        CURLcode r = (CURLcode)BHBB::Utils::DownloadFile(info->ScreenShotURLS[i], info->ScreenshotPaths[i]);

        if(paf::io::Misc::Exists(info->ScreenshotPaths[i]) && r == CURLE_OK)
        {
            ((PicturePage *)Page::GetCurrentPage())->AddPictureFromFile(info->ScreenshotPaths[i]);
        }
    }
    sceKernelExitThread(0);
}

BUTTON_CB(ShowScreenShotPage)
{
    graphics::Texture *screenShot1 = ((InfoPage *)Page::GetCurrentPage())->mainScreenshot;
    PicturePage *pp = new PicturePage();
    pp->AddPicture(screenShot1);
    pp->pageThread->Entry = DownloadRemainingScreenShotThread;
    pp->pageThread->Start();
}

CB(ScreenshotDownloadThread)
{
    InfoPage *page = (InfoPage *)Page::GetCurrentPage();
    BHBB::Utils::ResetStrtok();

    page->ScreenshotNum = BHBB::Utils::getStrtokNum(';', page->Info->screenshot_url.data);
    print("Strtoknum = %d\n", page->ScreenshotNum);
    page->ScreenShotURLS = (char **)sce_paf_malloc(page->ScreenshotNum * sizeof(char *));
    page->ScreenshotPaths = (char **)sce_paf_malloc(page->ScreenshotNum * sizeof(char *));

    //Download all textures and save the path in an array
    for(int i = 0; !Page::GetCurrentPage()->pageThread->EndThread; i++)
    {
        char *str = BHBB::Utils::strtok(';', page->Info->screenshot_url.data);
        if(str == NULL) break;
     
        page->ScreenshotPaths[i] = (char *)sce_paf_malloc(SCE_IO_MAX_PATH_BUFFER_SIZE);
        sce_paf_memset(page->ScreenshotPaths[i], 0, SCE_IO_MAX_PATH_BUFFER_SIZE);

        page->ScreenShotURLS[i] = (char *)sce_paf_malloc(250);
        sce_paf_memset(page->ScreenShotURLS[i], 0, 250);

        sce_paf_snprintf(page->ScreenshotPaths[i], SCE_IO_MAX_PATH_BUFFER_SIZE, DATA_PATH "/%s", str);
        sce_paf_snprintf(page->ScreenShotURLS[i], 250, "https://rinnegatamante.it/vitadb/%s", str);
        sce_paf_free(str);

    }
    CURLcode res = (CURLcode)BHBB::Utils::DownloadFile(page->ScreenShotURLS[0], page->ScreenshotPaths[0]);
    if(res != CURLE_OK)
    {
        if(!Page::GetCurrentPage()->pageThread->EndThread)
        {
            new TextPage(curl_easy_strerror(res), "Screenshot Download Error");
        }
        return;
    }

    if(paf::io::Misc::Exists(page->ScreenshotPaths[0]))
    {
        graphics::Texture *tex = new graphics::Texture();
        if(BHBB::Utils::CreateTextureFromFile(tex, page->ScreenshotPaths[0]))
        {
            page->mainScreenshot = tex;
            page->ScreenShot->SetTextureBase(page->mainScreenshot);
            BHBB::Utils::AssignButtonHandler(page->ScreenShot, ShowScreenShotPage);
        }
    }
    else
    {
        page->mainScreenshot = NULL;
        if(!Page::GetCurrentPage()->pageThread->EndThread)
            new TextPage("File Missing", "Screenshot Download Error");
    }
    sceKernelExitThread(0);
}

BUTTON_CB(DownloadApp)
{
    homeBrewInfo *info = (homeBrewInfo *)userDat;
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

InfoPage::InfoPage(homeBrewInfo *info, SceBool wait):Page(PAGE_TYPE_HOMBREW_INFO_PAGE, wait)
{
    this->Info = info;

    TitleText = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(PAGE_TITLE_ID));
    Icon = (Plane *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(INFO_PAGE_ICON_ID));
    Description = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(INFO_PAGE_DESCRIPTION_TEXT_ID));
    ScreenShot = (CompositeButton *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(INFO_PAGE_SCREENSHOT_ID));
    Credits = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(INFO_PAGE_CREDITS_TEXT_ID));
    DownloadButton = (Button *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(INFO_PAGE_DOWNLOAD_BUTTON_ID));
    Version = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(INFO_PAGE_VERSION_TEXT_ID));

    if(conf.db == CBPSDB)
    {
        BHBB::Utils::DeleteWidget(Version);
    }
    else
    {
        BHBB::Utils::SetWidgetLabel(Version, &Info->version);
    }

    if(info->download_url.length != 0)
        BHBB::Utils::AssignButtonHandler(DownloadButton, DownloadApp, Info);

    BHBB::Utils::SetWidgetLabel(TitleText, &Info->title);
    BHBB::Utils::SetWidgetColor(DownloadButton, 1, 0.5490196078f, 0, 1);

    char credits[50];
	sce_paf_memset(credits, 0, sizeof(credits));
    sce_paf_snprintf(credits, 50, "By: %s", Info->credits.data);

    BHBB::Utils::SetWidgetLabel(Credits, credits);
    
    float y = (Info->description.length * 4.5f) / 5.0;
    if(Info->description.length <= 15) y = 50;

    BHBB::Utils::SetWidgetSize(Description, 960, y);

    BHBB::Utils::SetWidgetLabel(Description, &Info->description);

    if(loadFlags & LOAD_FLAGS_ICONS)
    {
        iconTex = new graphics::Texture();
        if(paf::io::Misc::Exists(info->icon0Local.data) && BHBB::Utils::CreateTextureFromFile(iconTex, info->icon0Local.data))
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
    
    if(Info->screenshot_url.length != 0 && (loadFlags & LOAD_FLAGS_SCREENSHOTS))
    {
        Page::GetCurrentPage()->pageThread->Entry = ScreenshotDownloadThread;
        Page::GetCurrentPage()->pageThread->Start();
    }
    else
    {
        BHBB::Utils::DeleteWidget(ScreenShot);
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
        BHBB::Utils::DeleteTexture(mainScreenshot);
        mainScreenshot = SCE_NULL;
    }

DELETE_ICON:
    if(iconTex != NULL)
    {
        Icon->SetTextureBase(transparentTex);
        BHBB::Utils::DeleteTexture(iconTex);
        iconTex = SCE_NULL;
    }

}

LoadingPage::LoadingPage(const char *info):Page(PAGE_TYPE_LOADING_PAGE)
{
    skipAnimation = SCE_TRUE;
    String str;
    str.Set(info);
    WString wstr;
    str.ToWString(&wstr);

    infoText = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(LOADING_PAGE_TEXT));
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
    Resource::Element search = BHBB::Utils::GetParamWithHashFromId(PROGRESS_PAGE_BAR_TEMPLATE);

    for (int i = 0; i < BarNum; i++)
    {
        mainPlugin->AddWidgetFromTemplate(root, &search, &tinit);
        Widget *plane = root->GetChildByNum(root->childNum - 1);


        progressBars[i] = (ProgressBar *)BHBB::Utils::GetChildByHash(plane, BHBB::Utils::GetHashById(PROGRESS_PAGE_BAR));

        SceFVector4 pos;
        pos.x = 0;
        pos.y = -40 + (-40 * i);
        pos.z = 0;
        pos.w = 0;

        progressBars[i]->SetPosition(&pos);
        progressBars[i]->SetProgress(0, 0, 0);

        if(i == 0)
            busyIndicator = (BusyIndicator *)BHBB::Utils::GetChildByHash(plane, BHBB::Utils::GetHashById(BUSY_INICATOR_ID));

        SceFVector4 pos2;
        pos2.x = 225;
        pos2.y = -40 + (-40 * i) / 2;
        pos2.z = 0;
        pos2.w = 0;

        busyIndicator->SetPosition(&pos2);
        busyIndicator->Start();
    }

    infoText = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(LOADING_PAGE_TEXT));
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

    ConfirmButton = (Button *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(DECISION_PAGE_CONFIRM_BUTTON_ID));
    DeclineButton = (Button *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(DECISION_PAGE_DECLINE_BUTTON_ID));
    InfoText = (Text *)BHBB::Utils::GetChildByHash(root, BHBB::Utils::GetHashById(DECISION_PAGE_TEXT_ID));

    if(text != NULL)
        BHBB::Utils::SetWidgetLabel(InfoText, text);

    if(onConfirmPress != NULL)
        BHBB::Utils::AssignButtonHandler(ConfirmButton, onConfirmPress);
    if(onDeclinePress != NULL)
        BHBB::Utils::AssignButtonHandler(DeclineButton, onDeclinePress);
    
    switch (type)
    {
    case DECISION_TYPE_OK_CANCEL:
        BHBB::Utils::SetWidgetLabel(ConfirmButton, "OK");
        BHBB::Utils::SetWidgetLabel(DeclineButton, "Cancel");
        break;

    default:
    case DECISION_TYPE_YES_NO:
        BHBB::Utils::SetWidgetLabel(ConfirmButton, "Yes");
        BHBB::Utils::SetWidgetLabel(DeclineButton, "No");
        break;
    }
}

BUTTON_CB(DialogBackEvent)
{
    PopupMgr::hideDialog();
}

void PopupMgr::showDialog()
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
    e.hash = BHBB::Utils::GetHashById(POPUP_DIALOG_BOX);
    
    mainPlugin->AddWidgetFromTemplate(diag, &e, &tinit);

    diagBox = (Box *)diag->GetChildByNum(diag->childNum - 1);
}

void PopupMgr::hideDialog()
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

void PopupMgr::initDialog()
{
    Resource::Element searchParam;
	checkmark = new graphics::Texture();
	searchParam.hash = BHBB::Utils::GetHashById("_common_texture_check_mark");
	Plugin::LoadTexture(checkmark, Plugin::Find("__system__common_resource"), &searchParam);

	transparentTex = new graphics::Texture();
	searchParam.hash = BHBB::Utils::GetHashById("_common_texture_transparent");
	Plugin::LoadTexture(transparentTex, Plugin::Find("__system__common_resource"), &searchParam);

    diagBG = (Plane *)BHBB::Utils::GetChildByHash(mainScene, BHBB::Utils::GetHashById(POPUP_DIALOG_BG));
    if(diagBG == NULL) return;

    diag = (Dialog *)BHBB::Utils::GetChildByHash(diagBG, BHBB::Utils::GetHashById(POPUP_DIALOG_ID));
    showingDialog = true;
    return hideDialog();
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

    searchRequest.id.Set(refId);
    winfo.hash = winfo.GetHashById(&searchRequest);
    
    searchRequest.id.Set(style);
    sinfo.hash = sinfo.GetHashById(&searchRequest);    
    
    Widget *newWidget = mainPlugin->CreateWidgetWithStyle(parent != NULL ? parent : root, type, &winfo, &sinfo);
    if(newWidget == NULL) print("Error can't make widget with refID %s, type %s, style = %s\n", refId, type, style);
    if(newWidget == NULL || newWidget < 0) return NULL;
    return newWidget;
}

Widget *Page::AddFromTemplate(SceInt32 id, Widget *targetRoot)
{
    Resource::Element e = BHBB::Utils::GetParamWithHash(id);
    Plugin::TemplateInitParam tinit;

    mainPlugin->AddWidgetFromTemplate(targetRoot, &e, &tinit);
    return targetRoot->GetChildByNum(targetRoot->childNum - 1);
}

Widget *Page::AddFromTemplate(const char *id, Widget *targetRoot)
{
    Resource::Element e = BHBB::Utils::GetParamWithHashFromId(id);
    Plugin::TemplateInitParam tinit;

    mainPlugin->AddWidgetFromTemplate(targetRoot, &e, &tinit);
    return targetRoot->GetChildByNum(targetRoot->childNum - 1);
}

void PopupMgr::addDialogOption(const char *text, ECallback onPress, void *userDat, bool selected)
{
    if(!showingDialog) return;
    
    Plugin::TemplateInitParam tinit;
    Resource::Element e;
    e.hash = BHBB::Utils::GetHashById(POPUP_DIALOG_BUTTON);

    mainPlugin->AddWidgetFromTemplate(diagBox, &e, &tinit);

    ImageButton *currbutton = (ImageButton *)diagBox->GetChildByNum(diagBox->childNum - 1);
    
    String str;
    str.Set(text);
    WString wstr;
    str.ToWString(&wstr);

    currbutton->SetLabel(&wstr);
    if(selected) currbutton->SetTextureBase(checkmark);
    else currbutton->SetTextureBase(transparentTex);

    eventcb cb;
    cb.dat = userDat;
    cb.Callback = onPress;


    DiagButtonEventHandler *eh = new DiagButtonEventHandler();
    eh->pUserData = sce_paf_malloc(sizeof(cb));
    sce_paf_memcpy(eh->pUserData, &cb, sizeof(cb));
    currbutton->RegisterEventCallback(ON_PRESS_EVENT_ID, eh, 1);

}
