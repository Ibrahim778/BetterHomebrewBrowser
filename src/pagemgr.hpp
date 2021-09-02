#ifndef PAGEMGR_HPP
#define PAGEMGR_HPP

#include <paf.h>
#include "utils.hpp"
using namespace paf;
using namespace widget;

typedef enum 
{
    PAGE_TYPE_SELECTION_LIST,
    PAGE_TYPE_SELECTION_LIST_WITH_TITLE,
    PAGE_TYPE_INFO,
    PAGE_TYPE_LOADING_SCREEN,
    PAGE_TYPE_PROGRESS_PAGE
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

    static void Init();

    UtilThread *pageThread;

    Page(pageType Type);
    ~Page();
};

class SelectionList : public Page
{
public:

    Plane *listRoot;
    Box *scrollViewBox;
    Text *TitleText;

    SelectionList(const char *title = SCE_NULL);

    ImageButton *AddOption(const char *text, void(*onPress)(void *) = NULL, void *userDat = NULL, SceBool isLong = SCE_FALSE, SceBool needImage = SCE_FALSE);
    ImageButton *AddOption(String *text, void(*onPress)(void *) = NULL, void *userDat = NULL, SceBool isLong = SCE_FALSE, SceBool needImage = SCE_FALSE);

    ~SelectionList();
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

class PopupMgr
{
public:
    static graphics::Texture *checkmark;
    static graphics::Texture *transparent;

    static Plane *diagBG;
    static Dialog *diag;
    static Box *diagBox;
    static bool showingDialog;
    static void initDialog();
    static void showDialog();
    static void hideDialog();

    static void addDialogOption(const char *text, void (*onPress)(void *) = NULL, void *userDat = NULL, bool selected = false);
    static void clearOptions();
};

#endif