#ifndef PAGEMGR_HPP
#define PAGEMGR_HPP

#include <paf.h>
#include "utils.hpp"
#include "parser.hpp" //For homebrewInfo type

typedef enum 
{
    PAGE_TYPE_SELECTION_LIST,
    PAGE_TYPE_TEXT_PAGE,
    PAGE_TYPE_TEXT_PAGE_WITH_TITLE,
    PAGE_TYPE_SELECTION_LIST_WITH_TITLE,
    PAGE_TYPE_LOADING_SCREEN,
    PAGE_TYPE_PROGRESS_PAGE,
    PAGE_TYPE_HOMBREW_INFO,
    PAGE_TYPE_PICTURE_PAGE,
    PAGE_TYPE_BLANK_PAGE
} pageType;

class Page
{
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

    UtilThread *pageThread;

    Page(pageType Type, SceBool = SCE_FALSE);
    ~Page();
};

class BlankPage : public Page
{
public:
    BlankPage();
    ~BlankPage();

    Widget *AddFromStyle(const char *refId, const char *style, const char *type, Widget *parent = SCE_NULL);
};

class PicturePage : public Page
{
private:
    SceInt32 pictureNum;
    graphics::Texture **pictures;
public:
    Box *listRoot;

    SceInt32 AddPictureFromFile(const char *path);
    SceInt32 AddPicture(graphics::Texture *src);


    PicturePage();
    ~PicturePage();
};

class InfoPage : public Page
{
private:
    graphics::Texture *iconTex;
public:
    int ScreenshotNum;
    char **ScreenShotURLS;
    char **ScreenshotPaths;
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
public:

    Plane *listRoot;
    Box *scrollViewBox;
    Text *TitleText;

    SelectionList(const char *title = SCE_NULL);
    ~SelectionList();

    ImageButton *AddOption(const char *text, ECallback onPress = NULL, void *userDat = NULL, SceBool isLong = SCE_FALSE, SceBool needImage = SCE_FALSE);
    ImageButton *AddOption(String *text, ECallback onPress = NULL, void *userDat = NULL, SceBool isLong = SCE_FALSE, SceBool needImage = SCE_FALSE);
    
    SceVoid DisableAllButtons();
    SceVoid EnableAllButtons();

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
};

class PopupMgr
{
public:
    static graphics::Texture *checkmark;
    static graphics::Texture *transparent;

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