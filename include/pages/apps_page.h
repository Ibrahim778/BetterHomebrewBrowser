#ifndef BHBB_APPS_PAGE_H
#define BHBB_APPS_PAGE_H

#include <kernel.h>
#include <paf.h>

#include "page.h"
#include "db.h"
#include "dialog.h"

#define FLAG_ICON_DOWNLOAD       (1)
#define FLAG_ICON_LOAD_SURF      (2)
#define FLAG_ICON_ASSIGN_TEXTURE (4)

namespace apps
{
    class Page : public generic::Page
    {
    public:
        class IconAssignJob : public paf::job::JobItem 
        {
        public:
            
            struct Param {
                SceUInt32 widgetHash;
                paf::graph::Surface **texture;
                paf::string path;

                Param(SceUInt32 targetWidget, paf::graph::Surface **pTex, paf::string dPath):
                    widgetHash(targetWidget),texture(pTex),path(dPath){}
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
                paf::graph::Surface **texture;
                paf::string url, dest;

                Param(SceUInt32 targetWidget, paf::graph::Surface **pTex, paf::string sUrl, paf::string dPath):
                    widgetHash(targetWidget),texture(pTex),url(sUrl),dest(dPath){}
            };

            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

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
            SceVoid Finish();
        };

        class RedisplayJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            Page *callingPage;
            RedisplayJob(const char *name, Page *caller):job::JobItem(name),callingPage(caller){}
        };

        class LoadJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish();

            Page *callingPage;

            LoadJob(const char *name, Page *caller):job::JobItem(name),callingPage(caller){}
        };

        enum PageMode
        {
            PageMode_Browse,
            PageMode_Search
        };

        static SceVoid ForwardButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid IconDownloadDecideCB(Dialog::ButtonCode buttonResult, ScePVoid userDat);
        static SceVoid ErrorRetryCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid BackCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid SearchCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid CategoryButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);

        //Sets the category and updates button colour
        SceBool SetCategory(int category);
        //Sets page mode: Search or Browse
        SceVoid SetMode(PageMode mode);
        //Cancels any icon download jobs, Redownloads and parses index and calls Redisplay()
        SceVoid Load();
        //Redisplays the elements in the parsed db list according to category, also resets the current page number to 0 (1)
        SceVoid Redisplay();

        //Gets the current list widget
        paf::ui::Widget *GetCurrentPage();
        //Clears all lists
        SceVoid ClearPages();

        //Creates a new empty page
        template<class OnCompleteFunc>
        SceVoid NewPage(const OnCompleteFunc& onComplete);
        //Deletes the current list page
        SceVoid DeletePage(SceBool animate = SCE_TRUE);
        //Creates a new page and populates with buttons
        SceVoid CreatePopulatedPage();


        //Cancels current icon downloads and assignments, DO NOT CALL FROM MAIN THREAD
        SceVoid CancelIconJobs();

        //Returns the number of pages in the list, calculated using the page list (currBody)
        SceUInt32 GetPageCount();

        //Called whenever this page becomes the main page on screen. Handles forward button
        void OnRedisplay() override;

        Page();
        virtual ~Page();
    
    private:
        struct Body {
            paf::ui::Widget *widget;
            Body *prev;

            Body(Body *_prev = SCE_NULL):prev(_prev){}
        };

        enum {
            Hash_All = 0x59E75663,
            Hash_Game = 0x6222A81A,
            Hash_Emu = 0xD1A8D19,
            Hash_Port = 0xADC6272A,
            Hash_Util = 0x1EFEFBA6,

            Hash_SearchButton = 0xCCCE2527,
            Hash_SearchEnterButton = 0xAB8CB65E,
            Hash_SearchBackButton = 0x6A2C094C,
        } ButtonHash;

        SceVoid HandleForwardButton();
        SceVoid CreateListWrapper();
        SceVoid RedisplayInternal();
        //Starts adding new icon download *jobs*
        SceVoid StartIconDownloads();
        //Stops adding any new icon download *jobs* to cancel current jobs use CancelIconJobs()
        SceVoid CancelIconDownloads();

        PageMode mode;

        db::List appList;

        Body *currBody;

        paf::ui::Plane *listWrapperPlane;
        paf::ui::Plane *listRootPlane;

        paf::job::JobQueue *loadQueue;

        IconDownloadThread *iconDownloadThread;
        paf::job::JobQueue *iconDownloadQueue;
        paf::job::JobQueue *iconAssignQueue;
        SceUID iconFlags;

        SceInt32 category;
    };
}

#endif