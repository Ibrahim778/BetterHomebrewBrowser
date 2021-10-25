#ifndef PAGEMGR_HPP
#define PAGEMGR_HPP

#include <paf.h>
#include "utils.hpp"
#include "parser.hpp" //For homebrewInfo type

#define DEFINE_PAGE(PageType, pageID) case PageType:{search = BHBB::Utils::GetParamWithHashFromId(pageID);break;}
#define DELETE_PAGE_TYPE(Type, Class) case Type: { delete (Class *)p; break; }

typedef enum 
{
    PAGE_TYPE_SELECTION_LIST,
    PAGE_TYPE_SELECTION_LIST_WITH_TITLE,
    PAGE_TYPE_TEXT_PAGE,
    PAGE_TYPE_TEXT_PAGE_WITH_TITLE,
    PAGE_TYPE_LOADING_PAGE,
    PAGE_TYPE_PROGRESS_PAGE,
    PAGE_TYPE_HOMBREW_INFO_PAGE,
    PAGE_TYPE_PICTURE_PAGE,
    PAGE_TYPE_BLANK_PAGE,
    PAGE_TYPE_DECISION_PAGE,
} pageType;

class Page
{
private:
    static Page *currPage;
    static SceInt32 pageDepth;
    SceVoid SetRoot();
public:
    pageType type;
    Page *prev;
    
    Plane *root;
    BusyIndicator *busy;

    bool skipAnimation;

    void (*OnDelete)(void);
    void (*AfterDelete)(void);
    void (*OnRedisplay)(void);

    static void Init();
    static void DeletePage(Page *p = Page::GetCurrentPage(), bool playAnimation = SCE_TRUE);
    static Page *GetCurrentPage();

    UtilThread *pageThread;

    Widget *AddFromStyle(const char *refId, const char *style, const char *type, Widget *parent = SCE_NULL);
    Widget *AddFromTemplate(SceInt32 id, Widget *targetRoot);
    Widget *AddFromTemplate(const char *id, Widget *targetRoot);

    Page *ChangeType(pageType toType);

    Page(pageType Type = PAGE_TYPE_BLANK_PAGE, SceBool = SCE_FALSE);
    ~Page();
};

class PicturePage : public Page
{
public:
    SceInt32 pictureNum;
    graphics::Texture **pictures;

    Box *listRoot;

    SceInt32 AddPictureFromFile(const char *path);
    SceInt32 AddPicture(graphics::Texture *src);


    PicturePage();
    ~PicturePage();
};

class InfoPage : public Page
{
public:
    int ScreenshotNum;
    char **ScreenShotURLS;
    char **ScreenshotPaths;
    graphics::Texture *iconTex;
    graphics::Texture *mainScreenshot;

    homeBrewInfo *Info;
    CompositeButton *ScreenShot;
    Button *DownloadButton;
    Text *TitleText;
    Text *Credits;
    Text *Description;
    Text *Version;
    Plane *Icon;

    InfoPage(homeBrewInfo *info, SceBool wait = SCE_FALSE);
    ~InfoPage();

};

class SelectionList : public Page
{
private:
    bool disabled;

    Plane *listRoot;
    Box *scrollViewBox;
    Text *TitleText;

    Plane *TopListRoot;
public:
    SelectionList(const char *title = SCE_NULL);
    ~SelectionList();

    ImageButton *AddOption(const char *text, ECallback onPress = NULL, void *userDat = NULL, SceBool isLong = SCE_FALSE, SceBool needImage = SCE_FALSE);
    ImageButton *AddOption(String *text, ECallback onPress = NULL, void *userDat = NULL, SceBool isLong = SCE_FALSE, SceBool needImage = SCE_FALSE);
    ImageButton *AddOption(WString *text, ECallback onPress = NULL, void *userDat = NULL, SceBool isLong = SCE_FALSE, SceBool needImage = SCE_FALSE);

    ImageButton *AddTopOption(const char *text, ECallback onPress = NULL, void *userDat = NULL);

    SceVoid DisableAllButtons();
    SceVoid EnableAllButtons();

    SceVoid Hide();
    SceVoid Show();

    SceVoid Clear();

};

class LoadingPage : public Page
{
public:

    Text *infoText;

    LoadingPage(const char *title);

    ~LoadingPage();
};

class ProgressPage : public Page
{
public:
    SceInt32 barNum;
    Text *infoText;
    ProgressBar **progressBars;
    BusyIndicator *busyIndicator;

    ProgressPage(const char *info, int barNum = 1);
    ~ProgressPage();
};

class TextPage : public Page
{
public:
    Text *TitleText;
    Text *InfoText;

    TextPage(const char *text, const char *title = SCE_NULL);
    ~TextPage(){}
};

class DecisionPage : public Page
{
private:
    Text *InfoText;
    Button *ConfirmButton;
    Button *DeclineButton;
public:

    enum DecisionType
    {
        DECISION_TYPE_YES_NO,
        DECISION_TYPE_OK_CANCEL,
    };

    SceVoid SetConfirm(ECallback onPress);
    SceVoid SetDecline(ECallback onPress);

    DecisionPage(const char *text, ECallback onConfirmPress = SCE_NULL, ECallback onDeclinePress = SCE_NULL, DecisionType type = DECISION_TYPE_YES_NO);
    ~DecisionPage(){}
};

class PopupMgr
{
public:
    static graphics::Texture *checkmark;

    static Plane *diagBG;
    static Dialog *diag;
    static Box *diagBox;
    static SceBool showingDialog;
    static void initDialog();
    static void showDialog();
    static void hideDialog();

    static void addDialogOption(const char *text, ECallback onPress = NULL, void *userDat = NULL, bool selected = false);
    static void clearOptions();
};

#endif