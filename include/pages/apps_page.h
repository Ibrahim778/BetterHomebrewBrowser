#ifndef BHBB_APPS_PAGE_H
#define BHBB_APPS_PAGE_H

#include <kernel.h>
#include <paf.h>
#include <vector>
#include <unordered_map>

#include "multi_page_app_list.h"
#include "db.h"
#include "dialog.h"

#define FLAG_ICON_DOWNLOAD       (1)
#define FLAG_ICON_LOAD_SURF      (2)
#define FLAG_ICON_ASSIGN_TEXTURE (4)

namespace apps
{
    class Page : public generic::MultiPageAppList
    {
    public:
        class IconAssignJob : public paf::job::JobItem 
        {
        public:
            
            struct Param {
                SceUInt32 widgetHash;
                paf::string path;

                Param(SceUInt32 targetWidget, paf::string dPath):
                    widgetHash(targetWidget),path(dPath){}
            };

            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            Page *callingPage;
            Param taskParam;

            IconAssignJob(const char *name, Page *caller, Param param):job::JobItem(name),callingPage(caller),taskParam(param){}
        };

        class IconDownloadJob : public paf::job::JobItem 
        {
        public:
            
            struct Param {
                SceUInt32 widgetHash;
                paf::string dest;

                Param(SceUInt32 targetWidget, paf::string dPath):
                    widgetHash(targetWidget),dest(dPath){}
            };

            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            static SceBool CancelCheck(Page *caller);

            Page *callingPage;
            Param taskParam;

            IconDownloadJob(const char *name, Page *caller, Param param):job::JobItem(name),callingPage(caller),taskParam(param){}
        
        };

        class IconDownloadThread : public paf::thread::Thread
        {
        private:
            Page *callingPage;
        public:
            paf::thread::Thread::Thread;

            SceVoid EntryFunction();

            IconDownloadThread(Page *callingPage, SceInt32 initPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER, SceSize stackSize = SCE_KERNEL_4KiB, const char *name = "apps::Page::IconDownloadThread"):paf::thread::Thread::Thread(initPriority, stackSize, name),callingPage(callingPage){}
        };

        class IconZipJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}
        };

        class IconAssignThread : public paf::thread::Thread
        {
        private:
            Page *callingPage;
        public:
            db::List *targetList;
            int category;
            int loadNum;
            int pageCountBeforeCreation;
            paf::thread::Thread::Thread;

            SceVoid EntryFunction();

            IconAssignThread(Page *callingPage, SceInt32 initPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER - 1, SceSize stackSize = SCE_KERNEL_8KiB, const char *name = "apps::Page::IconAssignThread"):paf::thread::Thread::Thread(initPriority, stackSize, name),callingPage(callingPage){}
        };

        class LoadJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            Page *callingPage;

            LoadJob(const char *name, Page *caller):job::JobItem(name),callingPage(caller){}
        };

        class SearchCB : public paf::ui::EventCallback
        {
        public:
            SearchCB(Page *page) {
                pUserData = page;
                eventHandler = OnGet;
            }

            static SceVoid OnGet(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        };

        class IconInvokedDownloadJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            Page *callingPage;
            SceUInt64 hash;

            SceBool DialogResult;

            static SceVoid DialogEventHandler(Dialog::ButtonCode res, ScePVoid pUserData);

            IconInvokedDownloadJob(const char *name, SceUInt64 _hash, Page *caller):job::JobItem(name),callingPage(caller),hash(_hash){}
        };

        enum PageMode
        {
            PageMode_Browse,
            PageMode_Search
        };

        static SceVoid IconDownloadDecideCB(Dialog::ButtonCode buttonResult, ScePVoid userDat);
        static SceVoid ErrorRetryCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);

        //Sets page mode: Search or Browse
        SceVoid SetMode(PageMode mode);
        //Cancels any icon download jobs, Redownloads and parses index and calls Redisplay()
        SceVoid Load();

        //Creates a new page and populates with buttons
        SceVoid PopulatePage(paf::ui::Widget *scrollBox, void *userDat) override;

        //Cancels current icon downloads and assignments, DO NOT CALL FROM MAIN THREAD
        SceVoid CancelIconJobs();
        //Enables icon downloads and assignments
        SceVoid StartIconJobs();

        Page();
        virtual ~Page();
    
    private:
        enum {
            Hash_SearchButton = 0xCCCE2527,
            Hash_SearchEnterButton = 0xAB8CB65E,
            Hash_SearchBackButton = 0x6A2C094C,
            Hash_SearchBox = 0x7BB7D799,
        } ButtonHash;

        //Starts adding new icon download *jobs*
        SceVoid StartIconDownloads();
        //Stops adding any new icon download *jobs* to cancel current jobs use CancelIconJobs()
        SceVoid CancelIconDownloads();
        
        SceVoid OnClear() override;
        SceVoid OnCleared() override;
        SceVoid OnPageDelete(generic::MultiPageAppList::Body *body) override;

        ScePVoid DefaultNewPageData() override;

        PageMode mode;
        
        db::List appList;
        db::List searchList;

        IconDownloadThread *iconDownloadThread; //Enqueues downloads
        paf::job::JobQueue *iconDownloadQueue; //Performs downloads
        paf::job::JobQueue *iconAssignQueue; //Performs texture loading and assignments
        SceUID iconFlags;
        std::vector<SceUInt64> textureJobs;
    };

    namespace button
    {
        class Callback : public paf::ui::EventCallback
        {
        public:
            Callback(apps::Page *page){
                eventHandler = OnGet;
                pUserData = page;
            }

            static void OnGet(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        };
    };
}

#endif