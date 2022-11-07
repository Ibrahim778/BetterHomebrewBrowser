#ifndef APPS_INFO_PAGE_H
#define APPS_INFO_PAGE_H

#include <kernel.h>
#include <paf.h>

#include "pages/page.h"
#include "db.h"

namespace apps 
{
    namespace info
    {
        class Page : public generic::Page
        {
        public:

            class IconLoadThread : public paf::thread::Thread
            {
            private:
                Page *callingPage;
            public:
                using paf::thread::Thread::Thread;

                SceVoid EntryFunction();

                IconLoadThread(Page *callingPage, SceInt32 initPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER, SceSize stackSize = SCE_KERNEL_16KiB, const char *name = "apps::info::Page::IconLoadThread"):paf::thread::Thread::Thread(initPriority, stackSize, name),callingPage(callingPage){}
            };

            class DescriptionLoadThread : public paf::thread::Thread
            {
            private:
                Page *callingPage;
            public:
                using paf::thread::Thread::Thread;

                SceVoid EntryFunction();

                DescriptionLoadThread(Page *callingPage, SceInt32 initPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER, SceSize stackSize = SCE_KERNEL_16KiB, const char *name = "apps::info::Page::DescriptionLoadThread"):paf::thread::Thread::Thread(initPriority, stackSize, name),callingPage(callingPage){}
            };


            Page(db::entryInfo& info);
            virtual ~Page();

            db::entryInfo& GetInfo();

        private:

            db::entryInfo &info;
            paf::graph::Surface *iconSurf;

            IconLoadThread *iconLoadThread;
            DescriptionLoadThread *descriptionLoadThread;
        };

        namespace screenshot
        {
            class Callback : public paf::ui::EventCallback
            {
            public:
                Callback(){
                    eventHandler = OnGet;
                }

                static SceVoid OnGet(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);;
            };

            class Page : public generic::Page
            {
            public:
                class LoadThread : public paf::thread::Thread
                {
                public:
                    using paf::thread::Thread::Thread;
                    SceVoid EntryFunction();

                    LoadThread(Page *caller, SceInt32 initPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER, SceSize stackSize = SCE_KERNEL_16KiB, const char *name = "apps::info::screenshot::Page::LoadThread"):paf::thread::Thread::Thread(initPriority, stackSize, name),callingPage(caller){}
                private:
                    Page *callingPage;
                };

                Page(paf::string &url);
                virtual ~Page();

            private:
                paf::string &url;
                LoadThread *loadThread;
                paf::graph::Surface *surface;
            };
        };
        
        namespace button
        {
            typedef enum
            {
                ButtonHash_DataDownload = 0xFADB190A,
                ButtonHash_Download = 0x1383EA9,
            } ButtonHash;

            class DataDownloadTask : public paf::job::JobItem
            {
            private:
                apps::info::Page *caller;
            public:
                using paf::job::JobItem::JobItem;
                DataDownloadTask(apps::info::Page *call, const char *name):paf::job::JobItem(name),caller(call){}
                SceVoid Run();
                SceVoid Finish(){}
            };

            class AppDownloadTask : public paf::job::JobItem
            {
            private:
                apps::info::Page *caller;
            public:
                using paf::job::JobItem::JobItem;
                AppDownloadTask(apps::info::Page *call, const char *name):paf::job::JobItem(name),caller(call){}
                SceVoid Run();
                SceVoid Finish(){}
            };

            class Callback : public paf::ui::EventCallback
            {
            public:
                Callback(void *userDat = SCE_NULL){
                    eventHandler = OnGet;
                    pUserData = userDat;
                }

                static SceVoid OnGet(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
            };
        };
    };
};

#endif