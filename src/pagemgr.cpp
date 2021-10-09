#include "pagemgr.hpp"
#include "configmgr.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "main.hpp"
#include "bgdl.h"
#include "eventhandler.hpp"

extern Plugin *mainPlugin;
extern Plane *mainRoot;
extern CornerButton *mainBackButton;
extern CornerButton *settingsButton;
extern CornerButton *forwardButton;
extern Widget *mainScene;

extern graphics::Texture *BrokenTex;

extern userConfig conf;
extern int loadFlags;

SceInt32 pageDepth;
Page *currPage = SCE_NULL;

Box * PopupMgr::diagBox = SCE_NULL;
Plane *PopupMgr::diagBG = SCE_NULL;
Dialog *PopupMgr::diag = SCE_NULL;
SceBool PopupMgr::showingDialog = SCE_FALSE;

graphics::Texture *PopupMgr::checkmark = SCE_NULL;
graphics::Texture *PopupMgr::transparent = SCE_NULL;

#define DEFINE_PAGE(PageType, pageID) case PageType:{search = Utils::GetParamWithHashFromId(pageID);break;}

Page::Page(pageType page, SceBool wait)
{
    if(wait)
    {
        if(currPage->pageThread != NULL)
        {
            if(currPage->pageThread->IsStarted())
            {
                //currPage->pageThread->EndThread = SCE_TRUE;
                currPage->pageThread->Join();
            }
        }
    }

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

    Resource::Element search;
    Plugin::TemplateInitParam tini;

    switch (type)
    {
    
    DEFINE_PAGE(PAGE_TYPE_PROGRESS_PAGE, PROGRESS_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_LOADING_SCREEN, LOADING_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_SELECTION_LIST, SELECTION_LIST_NO_TITLE_TEMPLATE)

    DEFINE_PAGE(PAGE_TYPE_SELECTION_LIST_WITH_TITLE, SELECTION_LIST_TEMPLATE)

    DEFINE_PAGE(PAGE_TYPE_TEXT_PAGE, TEXT_PAGE_NO_TITLE_ID)
    
    DEFINE_PAGE(PAGE_TYPE_TEXT_PAGE_WITH_TITLE, TEXT_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_HOMBREW_INFO, INFO_PAGE_ID)
    
    DEFINE_PAGE(PAGE_TYPE_PICTURE_PAGE, PICTURE_PAGE_ID)

    DEFINE_PAGE(PAGE_TYPE_BLANK_PAGE, BLANK_PAGE_ID)

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
    if(this->pageThread != NULL)
    {
        if(this->pageThread->IsStarted())
        {
            this->pageThread->EndThread = true;
            this->pageThread->Join();
        }
        if(this->pageThread != NULL) delete this->pageThread;
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

SelectionList::~SelectionList(){}

ImageButton *SelectionList::AddOption(const char *text, ECallback onPress, void *userDat, SceBool isLong, SceBool needImage)
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

    //Utils::SetWidgetLabel(button, text);
    button->SetLabel(&wstr);
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
    
    mainPlugin->AddWidgetFromTemplate(scrollViewBox, &e, &tinit);
    ImageButton *button = (ImageButton *)scrollViewBox->GetChildByNum(scrollViewBox->childNum - 1);
 
    Utils::SetWidgetLabel(button, text);   
    Utils::AssignButtonHandler(button, onPress, userDat);

    return button;
}

SceVoid SelectionList::Clear()
{
    common::Utils::WidgetStateTransition(0, listRoot, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    
    Resource::Element search = Utils::GetParamWithHashFromId(type == PAGE_TYPE_SELECTION_LIST_WITH_TITLE ? LIST_TEMPLATE_ID : LIST_TEMPLATE_ID_NO_TITLE);
    Plugin::TemplateInitParam tinit;

    mainPlugin->AddWidgetFromTemplate(root, &search, &tinit);

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

    disabled = false;
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

SceInt32 PicturePage::AddPictureFromFile(const char *file)
{
    if(currPage != this) return -1;
    mainBackButton->Disable(0);
    if(pictureNum == 0)
        pictures = (graphics::Texture **)sce_paf_malloc(sizeof(graphics::Texture *));
    else if(pictureNum > 0) pictures = (graphics::Texture **)sce_paf_realloc(pictures, sizeof(graphics::Texture *) * (pictureNum + 1));

    pictures[pictureNum] = new graphics::Texture();
    Misc::OpenResult res;
    Misc::OpenFile(&res, file, SCE_O_RDONLY, 0777, NULL);

    graphics::Texture::CreateFromFile(pictures[pictureNum], mainPlugin->memoryPool, &res);
    if(pictures[pictureNum]->texSurface == NULL)
        new TextPage("Unknown Error", "Create Texture Error");

    delete res.localFile;
    sce_paf_free(res.unk_04);

    Plugin::TemplateInitParam tinit;
    Resource::Element search = Utils::GetParamWithHashFromId(PICTURE_PAGE_PICTURE_TEMPLATE);

    mainPlugin->AddWidgetFromTemplate(listRoot, &search, &tinit);

    Plane *p = (Plane *)listRoot->GetChildByNum(listRoot->childNum - 1);
    SceInt32 r = p->SetTextureBase(pictures[pictureNum]);
    pictureNum++;
    mainBackButton->Enable(0);
    return r;
}

SceInt32 PicturePage::AddPicture(graphics::Texture *src)
{
    if(currPage != this) return -1;
    if(src == NULL) return -1;
    mainBackButton->Disable(0);
    Plugin::TemplateInitParam tinit;
    Resource::Element search = Utils::GetParamWithHashFromId(PICTURE_PAGE_PICTURE_TEMPLATE);

    mainPlugin->AddWidgetFromTemplate(listRoot, &search, &tinit);

    Plane *p = (Plane *)listRoot->GetChildByNum(listRoot->childNum - 1);
    mainBackButton->Enable(0);
    return p->SetTextureBase(src);
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
    if(pictures != NULL)
    {
        for(int i = 0; i < pictureNum; i++)
        {
            if(pictures[i] != NULL)
            {
                if(pictures[i]->texSurface != NULL)
                {
                    delete pictures[i]->texSurface;
                    pictures[i]->texSurface = SCE_NULL;
                }
                delete pictures[i];
            }
        }
    }

    delete[] pictures;
}

void DownloadRemainingScreenShotThread(void)
{
    InfoPage *info = (InfoPage *)currPage->prev;
    for(int i = 1; i < info->ScreenshotNum - 1 && !currPage->pageThread->EndThread; i++)
    {
        CURLcode r = (CURLcode)Utils::DownloadFile(info->ScreenShotURLS[i], info->ScreenshotPaths[i]);

        if(checkFileExist(info->ScreenshotPaths[i]) && r == CURLE_OK)
        {
            ((PicturePage *)currPage)->AddPictureFromFile(info->ScreenshotPaths[i]);
        }
    }
}

BUTTON_CB(ShowScreenShotPage)
{
    graphics::Texture *screenShot1 = ((InfoPage *)currPage)->mainScreenshot;
    PicturePage *pp = new PicturePage();
    pp->AddPicture(screenShot1);
    pp->pageThread->Entry = DownloadRemainingScreenShotThread;
    pp->pageThread->Start();
}

void ScreenshotDownloadThread(void)
{
    InfoPage *page = (InfoPage *)currPage;
    Utils::ResetStrtok();

    page->ScreenshotNum = Utils::getStrtokNum(';', page->Info->screenshot_url.data);
    page->ScreenShotURLS = (char **)sce_paf_malloc(page->ScreenshotNum * sizeof(char *));
    page->ScreenshotPaths = (char **)sce_paf_malloc(page->ScreenshotNum * sizeof(char *));

    //Download all textures and save the path in an array
    for(int i = 0; !currPage->pageThread->EndThread; i++)
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
    CURLcode res = (CURLcode)Utils::DownloadFile(page->ScreenShotURLS[0], page->ScreenshotPaths[0]);
    if(res != CURLE_OK)
    {
        sceIoRemove(page->ScreenshotPaths[0]);
        if(!currPage->pageThread->EndThread)
        {
            new TextPage(curl_easy_strerror(res), "Screenshot Download Error");
            return;
        }
    }

    if(checkFileExist(page->ScreenshotPaths[0]))
    {
        Misc::OpenResult res;
        SceInt32 err = 0;
        Misc::OpenFile(&res, page->ScreenshotPaths[0], SCE_O_RDONLY, 0777, &err);
        
        graphics::Texture *tex = new graphics::Texture();
        graphics::Texture::CreateFromFile(tex, mainPlugin->memoryPool, &res);

        if(tex->texSurface == NULL)
            new TextPage("Unknown Error Occourred", "Create Texture Error");

        delete res.localFile;
        sce_paf_free(res.unk_04);

        page->mainScreenshot = tex;
        page->ScreenShot->SetTextureBase(page->mainScreenshot);
        Utils::AssignButtonHandler(page->ScreenShot, ShowScreenShotPage);
    }
    else
    {
        page->mainScreenshot = NULL;
        if(!currPage->pageThread->EndThread)
            new TextPage("File Missing", "Screenshot Download Error");
    }
}

BUTTON_CB(DownloadApp)
{
    homeBrewInfo *info = (homeBrewInfo *)userDat;

    SendDlRequest(info->title.data, info->download_url.data);
}

InfoPage::InfoPage(homeBrewInfo *info, SceBool wait):Page(PAGE_TYPE_HOMBREW_INFO, wait)
{
    this->Info = info;

    TitleText = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(PAGE_TITLE_ID));
    Icon = (Plane *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_ICON_ID));
    Description = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_DESCRIPTION_TEXT_ID));
    ScreenShot = (CompositeButton *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_SCREENSHOT_ID));
    Credits = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_CREDITS_TEXT_ID));
    DownloadButton = (Button *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_DOWNLOAD_BUTTON_ID));
    Version = (Text *)Utils::GetChildByHash(root, Utils::GetHashById(INFO_PAGE_VERSION_TEXT_ID));

    if(conf.db == CBPSDB)
    {
        Version->PlayAnimationReverse(0, Widget::Animation_Reset);
    }
    else
    {
        Utils::SetWidgetLabel(Version, &Info->version);
    }

    if(info->download_url.length != 0)
        Utils::AssignButtonHandler(DownloadButton, DownloadApp, Info);

    Utils::SetWidgetLabel(TitleText, &Info->title);
    Utils::SetWidgetColor(DownloadButton, 1, 0.5490196078f, 0, 1);

    char credits[50] = {0};
    sce_paf_snprintf(credits, 50, "By: %s", Info->credits.data);

    Utils::SetWidgetLabel(Credits, credits);

    float y = ((float)Info->description.length / 75.0) * 70.0;
    if(Info->description.length <= 10) y = 50;

    Utils::SetWidgetSize(Description, 960, y);
    Utils::SetWidgetLabel(Description, &Info->description);

    if(loadFlags & LOAD_FLAGS_ICONS)
    {
        if(checkFileExist(info->icon0Local.data))
        {
            Misc::OpenResult res;
            Misc::OpenFile(&res, info->icon0Local.data, SCE_O_RDONLY, 0777, NULL);
        
            iconTex = new graphics::Texture();

            graphics::Texture::CreateFromFile(iconTex, mainPlugin->memoryPool, &res);
            Icon->SetTextureBase(iconTex);        
            delete res.localFile;
            sce_paf_free(res.unk_04);
        }
        else
        {
            Icon->SetTextureBase(BrokenTex);
        }
    }

    iconTex = NULL;
    ScreenShotURLS = NULL;
    ScreenshotPaths = NULL;
    mainScreenshot = NULL;
    
    if(Info->screenshot_url.length != 0 && (loadFlags & LOAD_FLAGS_SCREENSHOTS))
    {
        currPage->pageThread->Entry = ScreenshotDownloadThread;
        currPage->pageThread->Start();
    }
    else
    {
        common::Utils::WidgetStateTransition(0, ScreenShot, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    }
    this->busy->Stop();
}

InfoPage::~InfoPage()
{
    if(!(loadFlags & LOAD_FLAGS_SCREENSHOTS)) goto DELETE_ICON;
    if(ScreenshotPaths != NULL) 
    {
        for(int i = 0; i < ScreenshotNum ; i++)
        {
            if(ScreenshotPaths[i] != NULL) sce_paf_free(ScreenshotPaths[i]);
        }
        sce_paf_free(ScreenshotPaths);
    }
    
    if(ScreenShotURLS != NULL)
    {
        for (int i = 0; i < ScreenshotNum; i++)
        {
            if(ScreenShotURLS[i] != NULL) sce_paf_free(ScreenShotURLS[i]);
        }
        sce_paf_free(ScreenShotURLS);
    }

    if(mainScreenshot != NULL)
    {
        delete mainScreenshot->texSurface;
        mainScreenshot->texSurface = SCE_NULL;
        delete mainScreenshot;
    }

DELETE_ICON:
    if(iconTex != NULL)
    {
        delete iconTex->texSurface;
        iconTex->texSurface = SCE_NULL;
        delete iconTex;
    }
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

BlankPage::BlankPage():Page(PAGE_TYPE_BLANK_PAGE)
{
    busy->Stop();
}

BlankPage::~BlankPage(){}

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

BUTTON_CB(DialogBackEvent)
{
    PopupMgr::hideDialog();
}

void PopupMgr::showDialog()
{
    if(showingDialog) return;

    if(currPage != NULL)
    {
        currPage->root->SetAlpha(0.39f);
    }
    
    EventHandler::SetBackButtonEvent(DialogBackEvent);
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

    if(currPage != NULL)
    {
        currPage->root->SetAlpha(1.0f);
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

//From QuickMenuReborn
Widget *BlankPage::AddFromStyle(const char *refId, const char *style, const char *type, Widget *parent)
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

void PopupMgr::addDialogOption(const char *text, ECallback onPress, void *userDat, bool selected)
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
    cb.Callback = onPress;


    DiagButtonEventHandler *eh = new DiagButtonEventHandler();
    eh->pUserData = sce_paf_malloc(sizeof(cb));
    sce_paf_memcpy(eh->pUserData, &cb, sizeof(cb));
    currbutton->RegisterEventCallback(ON_PRESS_EVENT_ID, eh, 1);

}
