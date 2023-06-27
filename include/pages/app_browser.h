#ifndef _APP_PAGE_H_
#define _APP_PAGE_H_

#include <paf.h>

#include "page.h"
#include "db/source.h"
#include "tex_pool.h"

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
    
        bool forceRefresh;
    protected:
        AppBrowser *pPage;
    };

    class TexPool : public ::TexPool
    {
    public:
        using ::TexPool::TexPool;
        using ::TexPool::Add;

        ~TexPool()
        {

        }

        bool Add(Source::Entry *item, bool allowReplace = false, int *res = nullptr);
        
        bool AddAsync(Source::Entry *item, paf::ui::Widget *workList, uint32_t targetHash, bool allowReplace = false);

        class ListIconJob : public paf::job::JobItem
        {
        public:

            ListIconJob(Source::Entry *entry, paf::ui::Widget *targetList, uint32_t targetName):
                paf::job::JobItem::JobItem("AppPage::TexPool::ListIconJob"),
                workEntry(entry),
                workList(targetList),
                targetHash(targetName)
            {

            }

            ~ListIconJob()
            {

            }

            void Run()
            {
                if(workList->FindChild(targetHash) == nullptr)
                    return;

                bool result = workObj->Add(workEntry);
                if(workObj && workObj->cbPlugin && workList->FindChild(targetHash) != nullptr)
                {
                    AppBrowser::EntryFactory::TextureCB(result, workList->FindChild(targetHash), workEntry, workObj);
                }
            }   

            void Finish()
            {
            }

            TexPool *workObj;
            Source::Entry *workEntry; 
            paf::ui::Widget *workList;
            uint32_t targetHash;
        };
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

        static void TextureCB(bool success, paf::ui::Widget *target, Source::Entry *workItem, TexPool *workPool);
        static void TexPoolAddCbFun(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);

    protected:
        AppBrowser *workPage;        
    };

    enum PageMode 
    {
        PageMode_Browse,
        PageMode_Search
    };

    // callbacks
    static void SearchCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void RefreshCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void CategoryCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void QuickCategoryCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void EntryCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void SortSizeAdjustCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void SortButtonCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void SortButtonListCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData);
    static void SettingsCB(int id, paf::ui::Handler *handler, paf::ui::Event *event, void *pUserData);

    // Cancel current jobs and reload from source
    void Load(bool forceRefresh = false);

    // Set the page mode (and update ui)
    void SetMode(PageMode mode);

    // Set the source
    void SetSource(paf::common::SharedPtr<Source> source);

    // Update list header
    void UpdateListHeader();

    // Set the category and update buttons
    bool SetCategory(int id);

    // Wrapper for Source::List::Sort (Also updates ui button)
    void Sort(uint32_t hash = -1);

    int GetCategory()
    {
        if(mode == PageMode_Search)
            return Source::CategoryAll;

        return category;
    }

    AppBrowser(paf::common::SharedPtr<Source> source);
    ~AppBrowser();

protected:

    void ClearList()
    {
        if(listView->GetCellNum(0) > 0)
            listView->DeleteCell(0, 0, listView->GetCellNum(0));
        UpdateListHeader();
        listHeader->Hide(paf::common::transition::Type_Fadein1);
    }

    void CreateList()
    {
        listView->InsertCell(0, 0, targetList->GetSize(GetCategory()));
        UpdateListHeader();
        listHeader->Show(paf::common::transition::Type_Fadein1);
    }

    // current sort mode
    uint32_t sortMode;

    // loading flag
    bool loading;

    // Current page mode
    PageMode mode;

    // raw category
    int category;

    // Current source
    paf::common::SharedPtr<Source> source;

    // App list
    Source::List appList;

    // Search list
    Source::List searchList;

    // Target list (where list_view will pull from)
    Source::List *targetList;

    // Texture pool
    TexPool *texPool;

    // Widgets
    paf::ui::BusyIndicator *busyIndicator;
    paf::ui::CornerButton *optionsButton;
    paf::ui::Button *refreshButton;
    paf::ui::Button *searchButton;
    paf::ui::Button *searchEnterButton;
    paf::ui::Button *searchBackButton;
    paf::ui::TextBox *searchBox;
    paf::ui::ListView *listView;
    paf::ui::Plane *listHeader;
};

#endif