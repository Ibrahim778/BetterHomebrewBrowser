#ifndef _APP_VIEWER_H_
#define _APP_VIEWER_H_

#include <paf.h>

#include "page.h"
#include "db/source.h"
#include "app_browser.h"

class AppViewer : public page::Base
{
public:
    class AsyncDescriptionLoader // Could probably do this a little cleaner now but oh well
    {
    public:
        AsyncDescriptionLoader(Source::Entry& entry, paf::ui::Widget *target, bool autoLoad = true);
        ~AsyncDescriptionLoader();

        void Load();
        void Abort();
    
    private:
        class TargetDeleteEventCallback : public paf::ui::EventListener
        {
        public:
            TargetDeleteEventCallback(AsyncDescriptionLoader *parent)
            {
                workObj = parent;
            }

            virtual ~TargetDeleteEventCallback()
            {
                if(workObj)
                    delete workObj;
            }

			int32_t Do(int32_t type, paf::ui::Handler *self, paf::ui::Event *e){}

            AsyncDescriptionLoader *workObj;
        };

        class Job : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            Job(Source::Entry& e):JobItem("AsyncDescriptionLoader"),entry(e) {}
            ~Job() {}
            
            void Run();
            void Finish();

            AsyncDescriptionLoader *workObj;
            paf::ui::Widget *target;
            Source::Entry& entry;
        };

        Job *item;
    };

    class IconDownloadJob : public paf::job::JobItem
    {
    public:
        using paf::job::JobItem::JobItem;

        void Run();
        void Finish(){}

        AppViewer *workPage;
    };

    static void ScreenshotCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);
    static void IconButtonCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);

    AppViewer(Source::Entry& entry, AppBrowser::TexPool *pTexPool);
    ~AppViewer();

protected:
    Source::Entry &app;
    AppBrowser::TexPool *pool;
};

#endif