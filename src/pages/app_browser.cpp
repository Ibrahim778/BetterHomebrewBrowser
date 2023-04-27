#include <paf.h>
#include <psp2_compat/curl/curl.h>
#include <shellsvc.h>

#include "pages/app_browser.h"
#include "pages/app_viewer.h"
#include "db/source.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"
#include "common.h"
#include "print.h"
#include "dialog.h"
#include "utils.h"

using namespace paf;
using namespace common;
using namespace thread;
using namespace math;

AppBrowser::AppBrowser(Source *_source):
    page::Base(
        app_page,
        Plugin::PageOpenParam(true),
        Plugin::PageCloseParam(true)
    ),
    pageJobs(
        "AppBrowser::pageJobs",
        &job::JobQueue::Option(
            SCE_KERNEL_DEFAULT_PRIORITY_USER + 10,
            SCE_KERNEL_256KiB,
            4,
            nullptr
        )
    ),
    mode((PageMode)-1),source(nullptr),category(Source::CategoryAll),loading(false)
{
    // Obtain widgets
    busyIndicator = (ui::BusyIndicator *)root->FindChild(busy);
    optionsButton = (ui::CornerButton *)root->FindChild(options_button);
    refreshButton = (ui::Button *)root->FindChild(refresh_button);
    listView = (ui::ListView *)root->FindChild(list_view);
    searchEnterButton = (ui::Button *)root->FindChild(search_enter_button);
    searchButton = (ui::Button *)root->FindChild(search_button);
    searchBackButton = (ui::Button *)root->FindChild(search_back_button);
    searchBox = (ui::TextBox *)root->FindChild(search_box);

    searchButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchCB, this);
    searchBackButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchCB, this);
    searchEnterButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchCB, this);
    searchBox->AddEventCallback(ui::TextBox::CB_TEXT_BOX_ENTER_PRESSED, SearchCB, this);
    refreshButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, RefreshCB, this);

    listView->SetKeycode(inputdevice::pad::Data::Keycode::PAD_L | inputdevice::pad::Data::Keycode::PAD_R);
    listView->AddEventCallback(ui::Handler::CB_KEYCODE_PRESS, QuickCategoryCB, this);

    // TODO: test
    // Maybe not needed now?
    // SceInt32 button = 0;
	// sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button); //Get enter button assignment (1 = CROSS, 0 = CIRCLE)

    searchBackButton->SetKeycode(inputdevice::pad::Data::Keycode::PAD_ESCAPE);

    SetMode(PageMode_Browse);

    SetSource(_source);

    // Setup list view
    listView->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);
    listView->InsertSegment(0, 1);
    listView->SetCellSizeDefault(0, v4(960, 100));

    listView->SetItemFactory(new EntryFactory(this));
}

AppBrowser::~AppBrowser()
{

}

bool AppBrowser::SetCategory(int id)
{
    if(category == id) return false;

    category = id;

    auto plane = root->FindChild(plane_category_buttons);
    for(int i = 0; i < plane->GetChildrenNum(); i++)
        plane->GetChild(i)->SetColor(1,1,1,0.4f);

    plane->FindChild(source->GetCategoryByID(id).nameHash)->SetColor(1,1,1,1);
    
    return true;
}

void AppBrowser::RefreshCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData)
{
    ((AppBrowser *)pUserData)->Load();
}

void AppBrowser::SearchCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData)
{
    AppBrowser *workPage = (AppBrowser *)pUserData;
    workPage->searchBox->EndEdit();

    switch(((ui::Widget *)widget)->GetName().GetIDHash())
    {
    case search_button:
        workPage->SetMode(PageMode_Search);
        break;

    case search_back_button:
        if(workPage->mode == PageMode_Search)
        {
            workPage->SetMode(PageMode_Browse);
            if(workPage->targetList != &workPage->appList)
            {
                workPage->ClearList();
                workPage->targetList = &workPage->appList;
                workPage->CreateList();
            }
        }
        break;

    case search_enter_button:
    case search_box:
    {
        wstring text16;
        string key;

        auto textBox = workPage->root->FindChild(search_box);
        textBox->GetString(text16);

        if(text16.empty())
            break;

        common::Utf16ToUtf8(text16, &key);
        
        Utils::Decapitalise(key.c_str());
        
        //Get rid of trailing space char
        for(int i = key.length() - 1; i > 0; i--)
        {
            if(key.data()[i] == ' ')
            {
                ((char *)key.data())[i] = 0;
            } else break;
        }

        // CLear the list we will store search results in
        workPage->searchList.Clear();

        int i = 0;
        for(Source::Entry &entry : workPage->appList.entries)
        {
            string titleID;
            common::Utf16ToUtf8(entry.titleID, &titleID);

            string title;
            common::Utf16ToUtf8(entry.title, &title);

            Utils::Decapitalise(title.c_str());
            Utils::Decapitalise(title.c_str());
            
            if(sce_paf_strstr(title.c_str(), key.c_str()) || sce_paf_strstr(titleID.c_str(), key.c_str()))
                workPage->searchList.Add(entry);
        }

        workPage->ClearList();
        workPage->targetList = &workPage->searchList;
        workPage->CreateList();
        break;
    }

    default:
        break;
    }
}

void AppBrowser::QuickCategoryCB(int eventID, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData)
{
    unsigned int detail = event->GetDetail();
    AppBrowser *workPage = (AppBrowser *)pUserData;

    if(workPage->source->categories.size() == 1) // This source doesn't support categories
        return;
    
    if(workPage->loading) // We're loading (prevent crash)
        return;

    if(workPage->mode != PageMode_Browse) // It *kinda* works without this? but we still need it
        return;

    int targetID = workPage->GetCategory();
    int i = 0;
    for(auto& cat : workPage->source->categories) // Get category index inside array
    {
        if(cat.id == targetID)
            break;
        i++;
    }

    if((detail & 0x20) == 0x20) // Right   
        i++;
    else if((detail & 0x1F) == 0x1F) // Left
        i--;
    
    if(i < 0)
        return;
    
    if(i > (workPage->source->categories.size() - 1))
        return;
    
    if(workPage->SetCategory(workPage->source->categories[i].id))
    {
        workPage->ClearList();
        workPage->CreateList();
    }
}

void AppBrowser::CategoryCB(int eventID, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData)
{
    AppBrowser *workPage = (AppBrowser *)pUserData;
    if(workPage->source->categories.size() == 1) // This db doesn't support categories
        return;
    
    if(workPage->loading)
        return;

    int id = workPage->source->GetCategoryByHash(((ui::Widget *)widget)->GetName().GetIDHash()).id;
    if(workPage->SetCategory(id))
    {
        // Redisplay
        workPage->ClearList();
        workPage->CreateList();
    }
}

void AppBrowser::SetSource(Source *tSource)
{
    if(source == tSource) return;

    // cleanup previous source (if valid)
    if(source)
        delete source;

    // update source
    source = tSource;

    source->pList = &appList;

    // Update category widgets

    bool mainThread = ThreadIDCache::Check(ThreadIDCache::Type_Main);
    if(!mainThread)
        RMutex::main_thread_mutex.Lock();

    // Delete old widgets
    ui::Plane *categoriesPlane = (ui::Plane *)root->FindChild(plane_category_buttons);
    for(unsigned short i = 0; i < categoriesPlane->GetChildrenNum(); i++)
        transition::DoReverse(0, categoriesPlane->GetChild(i), transition::Type_Reset, true, true);

    // Create new ones
    Plugin::TemplateOpenParam tOpen;
    float buttonSize = 844.0f / source->categories.size();

    int i = 0;
    for (const Source::Category& category : source->categories)
    {
        g_appPlugin->TemplateOpen(categoriesPlane, category_button_template, tOpen);
        ui::Widget *button = categoriesPlane->GetChild(categoriesPlane->GetChildrenNum() - 1);
        button->SetSize(buttonSize, 58, 0);
        button->SetPos(i * buttonSize, 1, 0);
        button->SetAnchor(ui::Widget::ANCHOR_LEFT, ui::Widget::ANCHOR_NONE, ui::Widget::ANCHOR_NONE, SCE_NULL);
        button->SetAlign(ui::Widget::ALIGN_LEFT, ui::Widget::ALIGN_NONE, ui::Widget::ALIGN_NONE, SCE_NULL);
        button->SetString(g_appPlugin->GetString(category.nameHash));
        button->SetColor(v4(1,1,1, this->category == category.id ? 1 : 0.4f));
        button->Show(transition::Type_Fadein1);
        button->SetName(category.nameHash);
        button->AddEventCallback(ui::Button::CB_BTN_DECIDE, AppBrowser::CategoryCB, this);
        i++;
    }

    if(!mainThread)
        RMutex::main_thread_mutex.Unlock();
}

void AppBrowser::SetMode(PageMode targetMode)
{
    if(mode == targetMode) return;

    ui::Widget *categoriesPlane = root->FindChild(plane_categories);
    ui::Widget *searchPlane = root->FindChild(plane_search);
    ui::Widget *searchBox = root->FindChild(search_box);

    switch(targetMode)
    {
    case PageMode_Browse:
        searchPlane->Hide(common::transition::Type_Fadein1);
        categoriesPlane->Show(common::transition::Type_Fadein1);
        break;

    case PageMode_Search:
        categoriesPlane->Hide(common::transition::Type_Fadein1);
        searchPlane->Show(common::transition::Type_Fadein1);
        break;
    }

    mode = targetMode;
}

void AppBrowser::EntryCB(int eventID, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData)
{
    auto workPage = (AppBrowser *)pUserData;

    new AppViewer(workPage->appList.Get(((ui::Widget *)widget)->GetName().GetIDHash()));
}

ui::ListItem *AppBrowser::EntryFactory::Create(ui::listview::ItemFactory::CreateParam& param)
{
    Plugin::TemplateOpenParam   tOpen;

    ui::ListItem    *item           = nullptr;
    ui::ImageButton *button         = nullptr;
    ui::Text        *authorText     = nullptr;
    ui::Text        *versionText    = nullptr;
    ui::Text        *categoryText   = nullptr;
    ui::Text        *titleText      = nullptr;
    Source::List    *workList       = nullptr;
    Source::Entry   *workItem       = nullptr;

    ui::Widget *targetRoot = param.parent;
    int category = workPage->GetCategory();

    g_appPlugin->TemplateOpen(targetRoot, app_button_list_item_template, tOpen);

    item = (ui::ListItem *)targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
    button = (ui::ImageButton *)item->FindChild(app_button);
    authorText = (ui::Text *)button->FindChild(text_list_item_author);
    versionText = (ui::Text *)button->FindChild(text_list_item_version);
    categoryText = (ui::Text *)button->FindChild(text_list_item_category);
    titleText = (ui::Text *)button->FindChild(text_list_item_title);

    workList = workPage->targetList;

    if(category == -1)
        workItem = &workList->entries[param.cell_index];
    else
        workItem = workList->GetCategory(category).entries[param.cell_index];

    button->SetName(workItem->hash);

    titleText->SetString(workItem->title);
    authorText->SetString(workItem->author);

    titleText->SetString(workItem->title);
    authorText->SetString(workItem->author);
    versionText->SetString(workItem->version);

    if(workPage->source->categories.size() > 1) // This db has category suppourt
        categoryText->SetString(g_appPlugin->GetString(workPage->source->GetCategoryByID(workItem->category).singleHash));
    else
        categoryText->Hide(transition::Type_Reset);

    button->AddEventCallback(ui::Button::CB_BTN_DECIDE, AppBrowser::EntryCB, workPage);

    // TODO: assign icons
    return item;
}

void AppBrowser::Load()
{
    if(!loading && source)
    {
        auto j = SharedPtr<job::JobItem>(new LoadJob(this));
        job::JobQueue::default_queue->Enqueue(j);
        loading = true;
    }
}

void AppBrowser::ParseThread::EntryFunction()
{
    print("[AppBrowser::ParseThread] EntryFunction(start)\n");

    RMutex::main_thread_mutex.Lock();
    workPage->busyIndicator->Start();
    RMutex::main_thread_mutex.Unlock();

    int ret = workPage->source->Parse();

    RMutex::main_thread_mutex.Lock();
    workPage->busyIndicator->Stop();
    RMutex::main_thread_mutex.Unlock();

    if(ret != SCE_OK)
    {
        print("[AppBrowser::ParseThread] source->Parse FAIL! 0x%X\n", ret);
        dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_error_parse));
        Cancel();
        delete this;
        return;
    }

    workPage->appList.Categorise(workPage->source);

    workPage->targetList = &workPage->appList;

    RMutex::main_thread_mutex.Lock();

    // Add items to list view
    workPage->CreateList();

    workPage->refreshButton->Enable(0);
    workPage->optionsButton->Show(transition::Type_Reset);

    RMutex::main_thread_mutex.Unlock();

    print("[AppBrowser::ParseThread] EntryFunction(stop)\n");
    delete this;
}

AppBrowser::ParseThread::~ParseThread()
{
    print("[AppBrowser::ParseThread] ~ParseThread(run)\n");
    workPage->loading = false;
    print("[AppBrowser::ParseThread] ~ParseThread(end)\n");
}

void AppBrowser::LoadJob::Run()
{
    print("[AppBrowser::LoadJob] Run(begin)\n");

    RMutex::main_thread_mutex.Lock();

    pPage->ClearList();    

    pPage->pageJobs.CancelAllItems();

    pPage->refreshButton->Disable(0);
    pPage->optionsButton->Hide(transition::Type_Reset);

    pPage->SetCategory(Source::CategoryAll);

    RMutex::main_thread_mutex.Unlock();

    int ret = SCE_OK;

    dialog::OpenPleaseWait(g_appPlugin, nullptr, g_appPlugin->GetString(msg_wait));

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    ret = pPage->source->DownloadIndex();

    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    dialog::Close();

    if(ret != SCE_OK)
    {
        print("[AppBrowser::LoadJob] source->DownloadIndex FAIL! 0x%X\n", ret);
        dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_error_index));
        Cancel();
        return;
    }

    print("[AppBrowser::LoadJob] Run(end)\n");
}

void AppBrowser::LoadJob::Finish()
{
    if(IsCanceled())
        return;

    auto pThread = new AppBrowser::ParseThread(pPage);
    pThread->Start();
}