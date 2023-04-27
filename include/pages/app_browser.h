#ifndef _APP_PAGE_H_
#define _APP_PAGE_H_

#include <paf.h>

#include "page.h"
#include "db/source.h"

class AppBrowser : public page::Base
{
public:
    class ParseThread : public paf::thread::Thread
    {
    public:
        using paf::thread::Thread::Thread;

        void EntryFunction();
        
        ParseThread(AppBrowser *caller):
            paf::thread::Thread::Thread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_64KiB, "AppBrowser::ParseThread"),
            workPage(caller){}

        ~ParseThread();

    protected:
        AppBrowser *workPage;
    };

    class LoadJob : public paf::job::JobItem
    {
    public:
        using paf::job::JobItem::JobItem;

        void Run();
        void Finish();

        LoadJob(AppBrowser *caller):pPage(caller),paf::job::JobItem("AppBrowser::LoadJob"){}
    
    protected:
        AppBrowser *pPage;
    };

    class EntryFactory : public paf::ui::listview::ItemFactory
    {
    public:
        EntryFactory(AppBrowser *_workPage):workPage(_workPage)
        {

        }
        
        ~EntryFactory()
        {

        }

        paf::ui::ListItem *Create(CreateParam& param);
        
        void Start(StartParam& param)
        {
            param.list_item->Show(paf::common::transition::Type_Fadein2);
        }

        void Stop(StopParam& param)
        {
            param.list_item->Hide(paf::common::transition::Type_Fadein2);
        }

    protected:
        AppBrowser *workPage;        
    };

    enum PageMode 
    {
        PageMode_Browse,
        PageMode_Search
    };

    // Button callbacks
    static void SearchCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void RefreshCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void CategoryCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void QuickCategoryCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void EntryCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);

    // Cancel current jobs and reload from source
    void Load();

    // Set the page mode (and update ui)
    void SetMode(PageMode mode);

    // Set the source
    void SetSource(Source *source);

    // Set the category and update buttons
    bool SetCategory(int id);

    int GetCategory()
    {
        if(mode == PageMode_Search)
            return Source::CategoryAll;

        return category;
    }

    AppBrowser(Source *source);
    ~AppBrowser();

protected:

    void ClearList()
    {
        if(listView->GetCellNum(0) > 0)
            listView->DeleteCell(0, 0, listView->GetCellNum(0));
    }

    void CreateList()
    {
        listView->InsertCell(0, 0, targetList->GetSize(category));
    }

    bool loading;

    // Current page mode
    PageMode mode;

    // raw category
    int category;

    // Current source
    Source *source;

    // App list
    Source::List appList;

    // Search list
    Source::List searchList;

    // Target list (where list_view will pull from)
    Source::List *targetList;

    // Widgets
    paf::ui::BusyIndicator *busyIndicator;
    paf::ui::CornerButton *optionsButton;
    paf::ui::Button *refreshButton;
    paf::ui::Button *searchButton;
    paf::ui::Button *searchEnterButton;
    paf::ui::Button *searchBackButton;
    paf::ui::TextBox *searchBox;
    paf::ui::ListView *listView;

    paf::job::JobQueue pageJobs;
};

#endif