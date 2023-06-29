#ifndef _APP_VIEWER_H_
#define _APP_VIEWER_H_

#include <paf.h>

#include "page.h"
#include "db/source.h"
#include "dialog.h"
#include "app_browser.h"

class AppViewer : public page::Base
{
public:
    enum
    {
        DescriptionEvent = (paf::ui::Handler::CB_STATE + 0x45000),
    };

    class AsyncDescriptionJob : public paf::job::JobItem
    {
    public:
        using paf::job::JobItem::JobItem;

        AsyncDescriptionJob(Source::Entry& e, AppViewer *_workPage):JobItem("AsyncDescriptionJob"),entry(e),workPage(_workPage) {}
        ~AsyncDescriptionJob() {}
        
        void Run();
        void Finish(){}

        Source::Entry& entry;
        AppViewer *workPage;
    };


    class IconDownloadJob : public paf::job::JobItem
    {
    public:
        using paf::job::JobItem::JobItem;

        void Run();
        void Finish(){}

        AppViewer *workPage;
    };

    class DownloadJob : public paf::job::JobItem
    {
    public:
        enum DownloadType
        {
            DownloadType_Data,
            DownloadType_App
        };

        using paf::job::JobItem::JobItem;

        void Run();
        void Finish(){}

        AppViewer *workPage;
        DownloadType type;
    };

    static void ScreenshotCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);
    static void IconButtonCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);
    static void InfoButtonCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);
    static void DownloadButtonCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);
    static void DescriptionTextCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData);

    AppViewer(Source::Entry& entry, AppBrowser::TexPool *pTexPool);
    ~AppViewer();

    ui::Text *descText;
    ui::BusyIndicator *busyIndicator;
    
protected:
    Source::Entry &app;
    AppBrowser::TexPool *pool;
};

#endif