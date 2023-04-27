#include <paf.h>

#include "db/source.h"
#include "pages/app_viewer.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"
#include "common.h"
#include "print.h"
#include "dialog.h"

using namespace paf;
using namespace common;
using namespace math;
using namespace thread;

AppViewer::AppViewer(Source::Entry& entry):page::Base(
    app_info_page,
    Plugin::PageOpenParam(false, 200, Plugin::TransitionType_SlideFromBottom),
    Plugin::PageCloseParam(true)
),app(entry)
{
    root->FindChild(info_title_text)->SetString(app.title);
    root->FindChild(info_author_text)->SetString(app.author);
    root->FindChild(info_version_text)->SetString(app.version);

    RMutex::main_thread_mutex.Lock();
    
    if(app.screenshotURL.size() > 0)
    {
        Plugin::TemplateOpenParam tOpen;
        auto scrollBox = root->FindChild(screenshot_box);
        
        for(int i = 0; i < app.screenshotURL.size(); i++)
        {
            g_appPlugin->TemplateOpen(scrollBox, info_screenshot_button_template, tOpen);
            auto widget = scrollBox->GetChild(scrollBox->GetChildrenNum() - 1);
            widget->SetName(i);
            widget->AddEventCallback(ui::Button::CB_BTN_DECIDE, ScreenshotCB, &app.screenshotURL);
        }
    }
    else
    {
        transition::Do(0, root->FindChild(screenshot_plane), transition::Type_Reset, true, true);
        root->FindChild(description_plane)->SetSize(960, 444, 0, 0);
    }
    
    RMutex::main_thread_mutex.Unlock();

    new AsyncDescriptionLoader(app, root->FindChild(info_description_text));
}   

AppViewer::~AppViewer()
{

}

AppViewer::AsyncDescriptionLoader::AsyncDescriptionLoader(Source::Entry& entry, paf::ui::Widget *target, bool autoLoad)
{
    item = new Job(entry);
    item->target = target;
    item->workObj = this;

	target->AddEventListener(0x10000000, new TargetDeleteEventCallback(this));

    if(autoLoad)
        Load();
}

AppViewer::AsyncDescriptionLoader::~AsyncDescriptionLoader()
{
    Abort();
}

void AppViewer::AsyncDescriptionLoader::Load()
{
    SharedPtr<job::JobItem> itemParam(item);
    job::JobQueue::default_queue->Enqueue(itemParam);
}

void AppViewer::AsyncDescriptionLoader::Abort()
{
	if (item)
	{
		item->workObj = nullptr;
		item->Cancel();
		item = nullptr;
	}
}

void AppViewer::AsyncDescriptionLoader::Job::Run()
{
    int res = 0;
    
    paf::wstring desc;
    res = entry.pSource->GetDescription(entry, desc);
    
    if(IsCanceled())
        return;

    if(res < 0)
    {
        dialog::OpenError(g_appPlugin, res, g_appPlugin->GetString(msg_desc_error));
        target->SetString(g_appPlugin->GetString(msg_desc_error));
    }
    else 
        target->SetString(desc);
}

void AppViewer::AsyncDescriptionLoader::Job::Finish()
{
    if(workObj)
        workObj->item = nullptr;
}

void AppViewer::ScreenshotCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData)
{

}