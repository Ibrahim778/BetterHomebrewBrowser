/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2023 Muhammad Ibrahim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/

#include <paf.h>
#include <shellsvc.h>

#include "pages/app_browser.h"
#include "pages/app_viewer.h"
#include "db/source.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"
#include "bhbb_settings.h"
#include "common.h"
#include "print.h"
#include "dialog.h"
#include "utils.h"
#include "settings.h"
#include "downloader.h"

#include "pages/text_page.h"

using namespace paf;
using namespace common;
using namespace thread;
using namespace math;

AppBrowser::AppBrowser(paf::common::SharedPtr<Source> _source):
    page::Base(
        app_page,
        Plugin::PageOpenParam(true),
        Plugin::PageCloseParam(true)
    ),
    mode((PageMode)-1),source(nullptr),category(Source::CategoryAll),loading(false),sortMode(0)
{
    // Obtain widgets
    busyIndicator = (ui::BusyIndicator *)root->FindChild(busy);
    optionsButton = (ui::CornerButton *)root->FindChild(options_button);
    refreshButton = (ui::Button *)root->FindChild(refresh_button);
    listView = (ui::ListView *)root->FindChild(app_list_view);
    searchEnterButton = (ui::Button *)root->FindChild(search_enter_button);
    searchButton = (ui::Button *)root->FindChild(search_button);
    searchBackButton = (ui::Button *)root->FindChild(search_back_button);
    searchBox = (ui::TextBox *)root->FindChild(search_box);
    listHeader = (ui::Plane *)root->FindChild(plane_header);

    searchButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchCB, this);
    searchBackButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchCB, this);
    searchEnterButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, SearchCB, this);
    searchBox->AddEventCallback(ui::TextBox::CB_TEXT_BOX_ENTER_PRESSED, SearchCB, this);
    refreshButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, RefreshCB, this);

    // Shoulder button category CB
    listView->SetKeycode(inputdevice::pad::Data::Keycode::PAD_L | inputdevice::pad::Data::Keycode::PAD_R);
    listView->AddEventCallback(ui::Handler::CB_KEYCODE_PRESS, QuickCategoryCB, this);

    texPool = new TexPool(g_appPlugin);
 
    // Search back button Keycode callback
    // TODO: test (Maybe not needed now?)
    // SceInt32 button = 0;
	// sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button); //Get enter button assignment (1 = CROSS, 0 = CIRCLE)

    searchBackButton->SetKeycode(inputdevice::pad::Data::Keycode::PAD_ESCAPE);

    SetMode(PageMode_Browse);

    SetSource(_source);

    // Setup list view
    listView->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);
    listView->InsertSegment(0, 1);
    listView->SetCellSizeDefault(0, { 960, 100 });
    
    listView->SetSegmentHeader(0, listHeader, false, false);
    listView->SetItemFactory(new EntryFactory(this));
    listHeader->FindChild(button_header_sort)->AddEventCallback(ui::Button::CB_BTN_DECIDE, SortButtonCB, this);

    // setup our lovely little settings callbacks :kms:
    root->FindChild(options_button)->AddEventCallback(ui::Button::CB_BTN_DECIDE, Settings::OpenCB);
    root->AddEventCallback(Settings::SettingsEvent, SettingsCB, this);

}

AppBrowser::~AppBrowser()
{
    texPool->DestroyAsync();
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

void AppBrowser::SettingsCB(int id, paf::ui::Handler *handler, paf::ui::Event *event, void *pUserData)
{
    auto workPage = (AppBrowser *)pUserData;
    auto eventType = event->GetValue(0);
    auto elemHash = event->GetValue(1);
    auto valHash = event->GetValue(2);
    auto iVal = event->GetValue(3);

    if(eventType != Settings::SettingsEvent_ValueChange)
        return;

    switch(elemHash)
    {
    case list_source:
        Settings::GetInstance()->Close();
        workPage->ClearList();
        workPage->SetSource(Source::Create((Source::ID)iVal));
        workPage->Load();
        break;
    
    case button_dl_dev:
        print("Downloading test app...\n");
        BGDLParam param;
        param.magic = (BHBB_DL_CFG_VER | BHBB_DL_MAGIC);
        param.type = BGDLTarget_App;

        Downloader::GetCurrentInstance()->Enqueue(g_appPlugin, "http://github.com/Ibrahim778/SelfLauncher/releases/download/V1.1/SelfLauncher.vpk", "Self Launcher", "ux0:app/VITASHELL/sce_sys/icon0.png", &param);
        break;

    case button_donations:
        Settings::GetInstance()->Close();
        new page::Base(page_donations, Plugin::PageOpenParam(true), Plugin::PageCloseParam(true));
        break;
    default:
        break;
    }
}

void AppBrowser::RefreshCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData)
{
    ((AppBrowser *)pUserData)->Load(true);
}

void AppBrowser::SortButtonListCB(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData)
{
    auto workPage = (AppBrowser *)pUserData;
    auto workWidget = (ui::Widget *)self;

    dialog::Close();

    workPage->busyIndicator->Start();
    workPage->ClearList();
    workPage->Sort(workWidget->GetName().GetIDHash());
    workPage->targetList->Categorise(workPage->source.get()); // cba to sort individual categories so just re-categorise instead!
    workPage->CreateList();
    workPage->busyIndicator->Stop();
}

void AppBrowser::SortButtonCB(int id, paf::ui::Handler *widget, paf::ui::Event *event, void *pUserData)
{
    auto workPage = (AppBrowser *)pUserData;

    // scroll_view my old friend :)
    auto scrollView = dialog::OpenScrollView(g_appPlugin, g_appPlugin->GetString(msg_sort_header));
    auto box = (ui::Box *)scrollView->FindChild("dialog_view_box");

    Plugin::TemplateOpenParam openParam;
    for(auto& i : workPage->source->sortModes)
    {
        g_appPlugin->TemplateOpen(box, template_image_button_list, openParam);
        auto button = box->GetChild(box->GetChildrenNum() - 1);
        
        button->SetString(g_appPlugin->GetString(i.hash));
        button->SetName(i.hash);
        button->AddEventCallback(ui::Button::CB_BTN_DECIDE, SortButtonListCB, pUserData);
        if(i.hash == workPage->sortMode)
            button->SetTexture(Plugin::Find("__system__common_resource")->GetTexture("_common_texture_check_mark"));
    }
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
                workPage->Sort();
                workPage->targetList->Categorise(workPage->source.get());
                workPage->CreateList();
            }
        }
        break;

    case search_enter_button:
    case search_box:
    {
        paf::wstring text16;
        paf::string key;

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

        // Search and store results
        for(const Source::Entry &entry : workPage->appList.entries)
        {
            paf::string titleID;
            common::Utf16ToUtf8(entry.titleID, &titleID);

            paf::string title;
            common::Utf16ToUtf8(entry.title, &title);

            paf::string author;
            common::Utf16ToUtf8(entry.author, &author);

            Utils::Decapitalise(title.c_str());
            Utils::Decapitalise(titleID.c_str());
            Utils::Decapitalise(author.c_str());
            
            if(sce_paf_strstr(title.c_str(), key.c_str()) || sce_paf_strstr(titleID.c_str(), key.c_str()) || sce_paf_strstr(author.c_str(), key.c_str()))
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

    if((detail & 0x1000000) == 0) // Triggers (We only pass bumpers)
        return;

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

void AppBrowser::UpdateListHeader()
{
    String str;
    str.SetFormattedString(g_appPlugin->GetString(msg_list_header), listView->GetCellNum(0));
    listHeader->FindChild(text_header)->SetString(str.GetWString());
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

void AppBrowser::SetSource(paf::common::SharedPtr<Source> tSource)
{
    if(source.get() == tSource.get()) return;

    // update source
    source = tSource;

    source->pList = &appList; // Reset list (probably done later on too, but just to be sure)

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
    float buttonSize = categoriesPlane->GetSize(0)->extract_x() / source->categories.size();

    int i = 0;
    for (const Source::CategoryInfo& category : source->categories)
    {
        g_appPlugin->TemplateOpen(categoriesPlane, category_button_template, tOpen);
        ui::Widget *button = categoriesPlane->GetChild(categoriesPlane->GetChildrenNum() - 1);
        button->SetSize(buttonSize, 58, 0);
        button->SetPos(i * buttonSize, 1, 0);
        button->SetAnchor(ui::Widget::ANCHOR_LEFT, ui::Widget::ANCHOR_NONE, ui::Widget::ANCHOR_NONE, SCE_NULL);
        button->SetAlign(ui::Widget::ALIGN_LEFT, ui::Widget::ALIGN_NONE, ui::Widget::ALIGN_NONE, SCE_NULL);
        button->SetString(g_appPlugin->GetString(category.nameHash));
        button->SetColor({ 1, 1, 1, this->category == category.id ? 1 : 0.4f });
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
    auto workWidget = (ui::Widget *)widget;
    
    workPage->searchBox->EndEdit();
    new AppViewer(workPage->appList.Get(workWidget->GetName().GetIDHash()), workPage->texPool);
}

void AppBrowser::EntryFactory::TextureCB(bool success, paf::ui::Widget *target, Source::Entry *workItem, TexPool *workPool)
{
    auto plane = (ui::Plane *)target->FindChild(plane_list_item_icon);
    if(!success)
    {
        auto brokenSurf = g_appPlugin->GetTexture(tex_missing_icon);
        workPool->Add(target->GetName().GetIDHash(), brokenSurf, true);
    }

    
    plane->SetColor(1,1,1,1);
    plane->SetTexture(workPool->Get(target->GetName().GetIDHash()));
    plane->Show(transition::Type_Fadein2);

}

void AppBrowser::EntryFactory::TexPoolAddCbFun(int id, paf::ui::Handler *self, paf::ui::Event *event, void *pUserData)
{
    auto workPool = (TexPool *)pUserData;
    auto targetWidget = (ui::Widget *)self;
    auto icon = (ui::Plane *)targetWidget->FindChild(plane_list_item_icon);

    if(event->GetValue(0) == targetWidget->GetName().GetIDHash())
    {
        icon->SetColor(1,1,1,1);
        icon->SetTexture(workPool->Get(targetWidget->GetName().GetIDHash()));
        icon->Show(transition::Type_Fadein2);
    }
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
    ui::Plane       *iconPlane      = nullptr;
    Source::List    *workList       = nullptr;
    Source::Entry   *workItem       = nullptr;

    ui::Widget *targetRoot = param.parent;
    int category = workPage->GetCategory();

    g_appPlugin->TemplateOpen(targetRoot, workPage->source->iconRatio == Source::r1x1 ? app_button_list_item_1x1_template : app_button_list_item_67x37_template, tOpen);

    item = (ui::ListItem *)targetRoot->GetChild(targetRoot->GetChildrenNum() - 1);
    button = (ui::ImageButton *)item->FindChild(app_button);
    authorText = (ui::Text *)button->FindChild(text_list_item_author);
    versionText = (ui::Text *)button->FindChild(text_list_item_version);
    categoryText = (ui::Text *)button->FindChild(text_list_item_category);
    titleText = (ui::Text *)button->FindChild(text_list_item_title);
    iconPlane = (ui::Plane *)button->FindChild(plane_list_item_icon);

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
    button->AddEventCallback(ui::Handler::CB_STATE_READY_CACHEIMAGE, TexPoolAddCbFun, workPage->texPool);

    thread::RMutex::main_thread_mutex.Lock();

    if(workPage->texPool->Exist(workItem->hash))
    {
        iconPlane->SetColor(1,1,1,1);
        iconPlane->SetTexture(workPage->texPool->Get(workItem->hash));
        iconPlane->Show(transition::Type_Fadein2);
    }
    else
        workPage->texPool->AddAsync(workItem, param.list_view, button->GetName().GetIDHash());
    
    thread::RMutex::main_thread_mutex.Unlock();

    return item;
}

void AppBrowser::SortSizeAdjustCB(int id, ui::Handler *self, ui::Event *event, void *pUserData)
{
    auto button = (ui::Button *)self;

    auto width = button->GetDrawObj(ui::Button::OBJ_LABEL)->GetSize().extract_x() + 40.0f;
    button->SetSize({ width, button->GetSize(0)->extract_y(), 0, 0 });

    auto sortPlane = button->GetParent();
    auto sortText = sortPlane->FindChild(text_label_sort);

    auto planeWidth = sortText->GetSize(0)->extract_x() + width + 25.0f;
    sortPlane->SetSize({ planeWidth, sortPlane->GetSize(0)->extract_y() });

    self->DeleteEventCallback(ui::Handler::CB_STATE_READY, SortSizeAdjustCB, pUserData);
}

void AppBrowser::Sort(uint32_t hash)
{
    int workHash = hash == -1 ? sortMode : hash; // hash is by default -1, so re-sort with the prev sort mode if the function was called without arguments

    auto button = listHeader->FindChild(button_header_sort);
    button->SetString(g_appPlugin->GetString(workHash));

    // When PAF updates the string, we will be notified and adjust the size of the button
    button->AddEventCallback(ui::Handler::CB_STATE_READY, SortSizeAdjustCB, nullptr); 

    targetList->Sort(source->GetSortFunction(workHash));
    
    sortMode = workHash;
}

void AppBrowser::Load(bool forceRefresh)
{
    if(!loading && source.get() != nullptr)
    {
        auto item = new LoadJob(this);
        item->forceRefresh = forceRefresh;
        auto itemParam = SharedPtr<job::JobItem>(item);
        job::JobQueue::default_queue->Enqueue(itemParam);
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
        workPage->refreshButton->Enable(0);
        workPage->optionsButton->Show(transition::Type_Reset);
        Cancel();
        delete this;
        return;
    }

    workPage->targetList = &workPage->appList;
    print("[AppBrowser::ParseThread] Assigned list\n");

    if(workPage->source->sortModes.size() > 0)
    {
        workPage->Sort(workPage->source->sortModes.begin()->hash); // Sort with the first sort mode in list (it is the default)
        print("[AppBrowser::ParseThread] Sorted entries\n");
    }
    
    if(workPage->source->categories.size() > 1)
    {
        workPage->targetList->Categorise(workPage->source.get());
        print("[AppBrowser::ParseThread] Categorised entries\n");
    }

    RMutex::main_thread_mutex.Lock();

    // Add items to list view
    workPage->CreateList();
    print("[AppBrowser::ParseThread] Created list\n");

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

    // Clear all previous textures
    pPage->texPool->SetAlive(false);
    pPage->texPool->AddAsyncWaitComplete();
    pPage->texPool->RemoveAll();
    pPage->texPool->SetAlive(true);

    pPage->refreshButton->Disable(0);
    pPage->optionsButton->Hide(transition::Type_Reset);

    pPage->appList.Clear();
    pPage->searchList.Clear();

    pPage->SetCategory(Source::CategoryAll);

    RMutex::main_thread_mutex.Unlock();

    int ret = SCE_OK;

    dialog::OpenPleaseWait(g_appPlugin, nullptr, g_appPlugin->GetString(msg_wait));

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    ret = pPage->source->DownloadIndex(forceRefresh);

    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    dialog::Close();

    if(ret != SCE_OK)
    {
        print("[AppBrowser::LoadJob] source->DownloadIndex FAIL! 0x%X\n", ret);
        dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_error_index));
        pPage->refreshButton->Enable(0);
        pPage->optionsButton->Show(transition::Type_Reset);
        Cancel();
        return;
    }

    print("[AppBrowser::LoadJob] Run(end)\n");
}

void AppBrowser::LoadJob::Finish()
{
    if(IsCanceled())
    {
        pPage->loading = false;
        return;
    }

    auto pThread = new AppBrowser::ParseThread(pPage);
    pThread->Start();
}

bool AppBrowser::TexPool::Add(Source::Entry *workItem, bool allowReplace, int *res)
{
    if(!alive)
    {
        return false;
    }

    if(!allowReplace)
    {
        if(Exist(workItem->hash))
            return false;
    }

    if(!LocalFile::Exists(workItem->iconPath.c_str()))
    {
        int ret = -1;
        for(auto &url : workItem->iconURL)
            if((ret = Utils::DownloadFile(url.c_str(), workItem->iconPath.c_str())) == 0)
                break;
                
        if(ret != 0) // All downloads failed (lol)
        {
            Remove(workItem->hash);
            if(res) *res = ret;
            return false;
        }
    }

    return AddLocal(workItem->hash, workItem->iconPath.c_str());
}

void AppBrowser::TexPool::ListIconJob::Run()
{
    thread::RMutex::main_thread_mutex.Lock();

    auto child = workList->FindChild(targetHash);

    thread::RMutex::main_thread_mutex.Unlock();
    
    if(child == nullptr)
        return;

    bool result = workObj->Add(workEntry);
    thread::RMutex::main_thread_mutex.Lock();

    if(workObj && workObj->cbPlugin && workList->FindChild(targetHash) != nullptr)
    {
        AppBrowser::EntryFactory::TextureCB(result, workList->FindChild(targetHash), workEntry, workObj);
    }
    thread::RMutex::main_thread_mutex.Unlock();
}

bool AppBrowser::TexPool::AddAsync(Source::Entry *workItem, ui::Widget *workList, uint32_t targetHash, bool allowReplace)
{
    if (!alive)
	{
		return false;
	}

	if (!allowReplace)
	{
		if (Exist(workItem->hash))
		{
			return false;
		}
	}
    
    ListIconJob *job = new ListIconJob(workItem, workList, targetHash);
    job->workObj = this;
    common::SharedPtr<job::JobItem> itemParam(job);
    addAsyncQueue->Enqueue(itemParam);
    return true;
}