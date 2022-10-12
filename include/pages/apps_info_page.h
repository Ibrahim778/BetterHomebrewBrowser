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

                IconLoadThread(Page *callingPage, SceInt32 initPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER, SceSize stackSize = SCE_KERNEL_4KiB, const char *name = "apps::info::Page::IconLoadThread"):paf::thread::Thread::Thread(initPriority, stackSize, name),callingPage(callingPage){}
            };

            class ScreenshotLoadThread : public paf::thread::Thread
            {
            private:
                Page *callingPage;
            public:
                using paf::thread::Thread::Thread;

                SceVoid EntryFunction();
                
                ScreenshotLoadThread(Page *callingPage, SceInt32 initPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER, SceSize stackSize = SCE_KERNEL_4KiB, const char *name = "apps::info::Page::ScreenshotLoadThread"):paf::thread::Thread::Thread(initPriority, stackSize, name),callingPage(callingPage){}
            };


            Page(db::entryInfo& info);
            virtual ~Page();

        private:
            db::entryInfo &info;
            paf::graph::Surface *iconSurf;
            std::vector<paf::graph::Surface *> screenshotSurfs;

            IconLoadThread *iconLoadThread;
            ScreenshotLoadThread *screenshotLoadThread;
        };
    };
};

#endif