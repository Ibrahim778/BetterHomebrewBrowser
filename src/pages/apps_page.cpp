#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <shellsvc.h>
#include <vector>
#include <algorithm>

#include "pages/apps_page.h"
#include "pages/text_page.h"
#include "pages/apps_info_page.h"
#include "print.h"
#include "utils.h"
#include "settings.h"
#include "common.h"
#include "dialog.h"
#include "curl_file.h"
#include "cURLFile.h"
#include "bgdl.h"
#include "error_codes.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"

using namespace paf;
using namespace Utils;

apps::Page::Page():generic::Page(apps_page, Plugin::PageOpenParam(true), Plugin::PageCloseParam()),mode((PageMode)-1),category(-1)
{
    //Get required widgets
    busyIndicator = Widget::GetChild<ui::BusyIndicator>(root, busy);
    optionsButton = Widget::GetChild<ui::CornerButton>(root, options_button);
    refreshButton = Widget::GetChild<ui::Button>(root, refresh_button);
    listView = Widget::GetChild<ui::ListView>(root, list_view);
    searchEnterButton = Widget::GetChild<ui::Button>(root, search_enter_button);
    searchButton = Widget::GetChild<ui::Button>(root, search_button);
    searchBackButton = Widget::GetChild<ui::Button>(root, search_back_button);
    searchBox = Widget::GetChild<ui::TextBox>(root, search_box);

    //Setup button events
    searchButton->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(SearchCB, this));
    searchEnterButton->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(SearchCB, this));
    searchBackButton->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(SearchCB, this));
    
    SceInt32 button = 0;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button); //Get enter button assignment (1 = CROSS, 0 = CIRCLE)

    searchBackButton->SetDirectKey(button == 1 ? SCE_PAF_CTRL_CIRCLE : SCE_PAF_CTRL_CROSS);
    
    searchBox->RegisterEventCallback(0x1000000B, new SimpleEventCallback(SearchCB, this)); //Enter button on IME keyboard
    refreshButton->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(RefreshCB, this));
    optionsButton->RegisterEventCallback(ui::EventMain_Decide, new Settings::OpenCallback());

    Utils::GamePad::RegisterButtonUpCB(QuickCategoryCB, this);

    //Configure UI elements
    SetMode(PageMode_Browse);

    //Setup the list view
	listView->RegisterItemCallback(new ListViewCB(this));

	listView->SetConfigurationType(0, ui::ListView::ConfigurationType_Simple);
    listView->SetSegmentEnable(0, 1);
    listView->SetCellSize(0, &paf::Vector4(960.0f, 100.0f));
}

apps::Page::~Page()
{
    
}

ui::ListItem *apps::Page::ListViewCB::Create(apps::Page::ListViewCB::Param *param)
{
    rco::Element                searchParam;
    Plugin::TemplateOpenParam   tOpen;

    ui::ListItem    *item           = SCE_NULL;
    ui::ImageButton *button         = SCE_NULL;
    ui::Text        *authorText     = SCE_NULL;
    ui::Text        *versionText    = SCE_NULL;
    ui::Text        *categoryText   = SCE_NULL;
    ui::Text        *titleText      = SCE_NULL;
    db::List        *workList       = SCE_NULL;
    db::entryInfo   *workItem       = SCE_NULL;

    ui::Widget *targetRoot = param->parent;

    searchParam.hash = app_button_list_item_template;
    g_appPlugin->TemplateOpen(targetRoot, &searchParam, &tOpen);
    
    item = (ui::ListItem *)targetRoot->GetChild(targetRoot->childNum - 1);
    button = Widget::GetChild<ui::ImageButton>(item, app_button);
    authorText = Widget::GetChild<ui::Text>(button, text_list_item_author);
    versionText = Widget::GetChild<ui::Text>(button, text_list_item_version);
    categoryText = Widget::GetChild<ui::Text>(button, text_list_item_category);
    titleText = Widget::GetChild<ui::Text>(button, text_list_item_title);

    workList = workPage->GetTargetList();
    
    if(workPage->category == -1)
        workItem = &workList->entries[param->cellIndex];
    else 
        workItem = workList->GetCategory(workPage->category).entries[param->cellIndex];

    button->elem.hash = workItem->hash;
    
    Widget::SetLabel(titleText, workItem->title.data());
    Widget::SetLabel(authorText, workItem->author.data());
    Widget::SetLabel(versionText, workItem->version.data());

    if(db::info[Settings::GetInstance()->source].CategoriesSupported)
    {
        SceUInt64 categoryTextHash = db_category_unk;
        for(int i = 0; i < db::info[Settings::GetInstance()->source].categoryNum; i++)
        {
            if(db::info[Settings::GetInstance()->source].categories[i].id == workItem->type)
            {
                categoryTextHash = db::info[Settings::GetInstance()->source].categories[i].singleHash;
                break;
            }
        }
        categoryText->SetLabel(&wstring(str::GetPFromHash(categoryTextHash)));
    }
    else 
        categoryText->PlayEffectReverse(0, effect::EffectType_Reset);
    
    if(!workPage->appList.Get(workItem->hash).surface) // We need to ensure we're using appList. GetTargetList may also return searchList, whose icons are never cleaned up
    {
        new AsyncIconLoader(workItem->iconPath.c_str(), button, &workPage->appList.Get(workItem->hash).surface);
    }
    else
    {
        button->SetSurfaceBase(&workPage->appList.Get(workItem->hash).surface);
    }

    button->RegisterEventCallback(ui::EventMain_Decide, new apps::button::Callback(workPage));

    return item;
}

apps::Page::AsyncIconLoader::TargetDeleteEventCallback::~TargetDeleteEventCallback()
{
	delete workObj;
}

apps::Page::AsyncIconLoader::AsyncIconLoader(const char *path, ui::Widget *target, graph::Surface **surf, SceBool autoLoad)
{
	item = new Job("AsyncNetworkSurfaceLoader");
	item->target = target;
	item->fPath = path;
	item->loadedSurface = surf;
	item->workObj = this;

	target->RegisterEventCallback(0x10000000, new TargetDeleteEventCallback(this));

	if (autoLoad)
	{
		SharedPtr<job::JobItem> itemParam(item);
		g_mainQueue->Enqueue(itemParam);
	}
}

apps::Page::AsyncIconLoader::~AsyncIconLoader()
{
	if (item)
	{
		item->workObj = SCE_NULL;
		item->Cancel();
		item = SCE_NULL;
	}
}

SceVoid apps::Page::AsyncIconLoader::Load()
{
	SharedPtr<job::JobItem> itemParam(item);
	g_mainQueue->Enqueue(itemParam);
}

SceVoid apps::Page::AsyncIconLoader::Abort()
{
	if (item)
	{
		item->workObj = SCE_NULL;
		item->Cancel();
		item = SCE_NULL;
	}
}

SceVoid apps::Page::AsyncIconLoader::Job::Finish()
{
	if (workObj)
		workObj->item = SCE_NULL;
}

SceBool apps::Page::AsyncIconLoader::Job::DownloadCancelCheck(ScePVoid pUserData)
{
    return ((Job *)pUserData)->IsCanceled();
}

SceVoid apps::Page::AsyncIconLoader::Job::Run()
{
	graph::Surface *tex = SCE_NULL;
	SceInt32 res = -1;
	LocalFile::OpenArg oarg;
	oarg.filename = fPath.c_str();
    oarg.mode = 0;
    oarg.flag = SCE_O_RDONLY;
    
	LocalFile *file = new LocalFile();
	res = file->Open(&oarg);
	if (res != 0)
	{
        delete file;
        return;
	}

	SharedPtr<LocalFile> fres(file);

	if (IsCanceled())
	{
		fres->Close();
		fres.reset();
		return;
	}

	graph::Surface::Create(&tex, g_appPlugin->memoryPool, (SharedPtr<File>*)&fres);
	fres.reset();
	if (tex == SCE_NULL)
	{   
        print("Texture load failed\n");
        if(!IsCanceled()) target->SetSurfaceBase(&g_brokenTex);
		return;
	}

	if (IsCanceled())
	{
		tex->Release();
		return;
	}

	thread::s_mainThreadMutex.Lock();
	target->SetSurfaceBase(&tex);
	thread::s_mainThreadMutex.Unlock();

	if (loadedSurface)
	{
		*loadedSurface = tex;
	}
	else
	{
		tex->Release();
	}
}

SceVoid apps::Page::RefreshCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    Time::SetPreviousDLTime(Settings::GetInstance()->source, 0); //Force redownload
    ((apps::Page *)pUserData)->Load();
}

SceVoid apps::Page::SetMode(PageMode targetMode)
{
    if(mode == targetMode) return;
    
    ui::Widget *categoriesPlane = Widget::GetChild(root, plane_categories);
    ui::Widget *searchPlane = Widget::GetChild(root, plane_search);
    ui::Widget *searchBox = Widget::GetChild(root, search_box);

    if(categoriesPlane->animationStatus & 0x80)
        categoriesPlane->animationStatus &= ~0x80;

    if(searchPlane->animationStatus & 0x80)
        searchPlane->animationStatus &= ~0x80;
        
    switch(targetMode)
    {
    case PageMode_Browse:
        searchPlane->PlayEffectReverse(0, effect::EffectType_Fadein1);
        categoriesPlane->PlayEffect(0, effect::EffectType_Fadein1);
        break;
    
    case PageMode_Search:
        categoriesPlane->PlayEffectReverse(0, effect::EffectType_Fadein1);
        searchPlane->PlayEffect(0, effect::EffectType_Fadein1);
        break;
    }

    mode = targetMode;
}

SceVoid apps::Page::SearchCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    Page *page = (Page *)pUserData;
    page->searchBox->Hide();
    switch(self->elem.hash)
    {
    case search_button:
        page->SetMode(PageMode_Search);
        break;

    case search_back_button:
        if(page->mode != PageMode_Search) return;
        page->SetMode(PageMode_Browse);
        if(page->GetTargetList() != &page->appList)
        {
            page->SetTargetList(&page->appList);
            page->ClearList();
            page->DisplayList();
        } // TODO: FIXED
        break;

    case search_box:
    case search_enter_button:
    {
        paf::string key8;
        paf::wstring key16;
        
        ui::Widget *textBox = Widget::GetChild(page->root, search_box);
        textBox->GetLabel(&key16);
        
        if(key16.length() == 0)
            break;

        common::Utf16ToUtf8(key16, &key8);
        
        str::ToLowerCase((char *)key8.data());
        
        //Get rid of trailing space char
        for(int i = key8.length() - 1; i > 0; i--)
        {
            if(key8.data()[i] == ' ')
            {
                ((char *)key8.data())[i] = 0; //Shouldn't really do this with a paf::string but w/e
            } else break;
        }

        page->searchList.Clear();

        int i = 0;
        auto end = page->appList.entries.end();
        for(auto entry = page->appList.entries.begin(); entry != end; entry++)
        {
            string titleID = entry->titleID.data();
            string title = entry->title.data();

            str::ToLowerCase((char *)titleID.data());
            str::ToLowerCase((char *)title.data());
            if(sce_paf_strstr(title.data(), key8.data()) || sce_paf_strstr(titleID.data(), key8.data()))
            {
                page->searchList.Add(*entry);
            }
        }

        page->SetTargetList(&page->searchList);
        page->SetCategory(db::CategoryAll);
        page->ClearList();
        page->DisplayList(); //TODO: FIXED
        
        break;
    }
    default:
        print("Unknown hash (0x%X)! corrupted widget?\n", self->elem.hash);
        break;
    }
}

SceInt32 apps::Page::GetCategory()
{
    return category;
}

void apps::Page::QuickCategoryCB(input::GamePad::GamePadData *data, ScePVoid pUserData)
{
    Page *page = (Page *)pUserData;
    if(generic::Page::GetCurrentPage() != pUserData)
        return;
        
    //Find current index in category array according to id
    int target = page->GetCategory();
    int i = 0;

    for(const db::Category &cat : db::info[Settings::GetInstance()->source].categories)
    {
        if(cat.id == target)
            break;
        i++;
    }

    if(data->buttons & (SCE_PAF_CTRL_R1 | SCE_PAF_CTRL_R))    
        i++;
    else if(data->buttons & (SCE_PAF_CTRL_L1 | SCE_PAF_CTRL_L))
        i--;

    if(i < 0) 
        return;    

    if(i > (db::info[Settings::GetInstance()->source].categoryNum - 1))
        return;

    if(page->SetCategory(db::info[Settings::GetInstance()->source].categories[i].id))
        page->Redisplay();
}

SceVoid apps::Page::IconZipJob::Run()
{
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    Dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, str::GetPFromHash(msg_wait));

    string titleTemplate;
    str::GetFromHash(icons_dl_name, &titleTemplate);
    
    string title;
    common::string_util::setf(title, titleTemplate.data(), db::info[Settings::GetInstance()->source].name);
    
    EnqueueCBGDLTask(title.data(), db::info[Settings::GetInstance()->source].iconsURL, db::info[Settings::GetInstance()->source].iconFolderPath);

    Dialog::Close();
    Dialog::OpenOk(g_appPlugin, NULL, str::GetPFromHash(msg_download_queued));
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
}

SceVoid apps::Page::HideKeyboard()
{
    searchBox->Hide();
}

SceVoid apps::Page::IconDownloadDecideCB(Dialog::ButtonCode buttonResult, ScePVoid userDat)
{
    if(buttonResult == Dialog::ButtonCode::ButtonCode_Yes)
    {
        SharedPtr<job::JobItem> ptr = paf::common::SharedPtr<job::JobItem>(new IconZipJob("BHBB::IconZipJob"));
        job::s_defaultJobQueue->Enqueue(ptr);
    }
}

SceVoid apps::Page::ErrorRetryCB(Dialog::ButtonCode buttonResult, ScePVoid pUserData)
{
    ((Page *)pUserData)->Load();
}

SceVoid apps::Page::Load()
{
    auto jobPtr = SharedPtr<job::JobItem>(new LoadJob("BHBB::apps::Page::LoadJob", this));
    g_mainQueue->Enqueue(jobPtr);
}

SceBool apps::Page::SetCategory(int _category)
{
    if(_category == category || locked) return SCE_FALSE;
    category = _category;

    // Update the buttons
    auto plane = Widget::GetChild(root, plane_category_buttons);
    for(int i = 0; i < plane->childNum; i++)
        plane->GetChild(i)->SetColor(&paf::Rgba(1,1,1,0.4f));

    for(int i = 0; i < db::info[Settings::GetInstance()->source].categoryNum; i++)
    {
        if(db::info[Settings::GetInstance()->source].categories[i].id == category)
        {
            auto button = Widget::GetChild(root, db::info[Settings::GetInstance()->source].categories[i].nameHash);
            if(button) button->SetColor(&paf::Rgba(1,1,1,1));
            break;
        }
    }

    return SCE_TRUE;
}

SceVoid apps::Page::CategoryCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(!db::info[Settings::GetInstance()->source].CategoriesSupported) return;
    Page *page = (Page *)pUserData;

    //Loop through and find category ID from button hash (same as category hash)
    for(int i = 0; i < db::info[Settings::GetInstance()->source].categoryNum; i++)
    {
        if(db::info[Settings::GetInstance()->source].categories[i].nameHash == self->elem.hash)
        {
            if(page->SetCategory(db::info[Settings::GetInstance()->source].categories[i].id)) // returns true if there's any change
                page->Redisplay();
            break;
        }
    }
}

SceVoid apps::Page::SetCategories(const std::vector<db::Category>& categoryList)
{
    //Delete all previous buttons
    auto plane = Widget::GetChild(root, plane_category_buttons);
    for(int i = 0; i < plane->childNum; i++)
        effect::Play(0, plane->GetChild(i), effect::EffectType_Reset, SCE_TRUE, SCE_TRUE);
    
    Plugin::TemplateOpenParam tOpen;
    rco::Element e;
    SceFloat buttonSize = 844.0f / categoryList.size(); // (960 - 56 - 56 = 844)
    
    e.hash = category_button_template;

    int i = 0;
    for(const db::Category& category : categoryList)
    {
        g_appPlugin->TemplateOpen(plane, &e, &tOpen);
        auto button = plane->GetChild(plane->childNum - 1);
        button->SetSize(&paf::Vector4(buttonSize, 58));
        button->SetPosition(&paf::Vector4(i * buttonSize, 1));
        button->SetAnchor(ui::Anchor_Left, ui::Anchor_None, ui::Anchor_None, ui::Anchor_None);
        button->SetAlign(ui::Align_Left, ui::Align_None, ui::Align_None, ui::Align_None);
        button->SetLabel(&paf::wstring(str::GetPFromHash(category.nameHash)));
        button->SetColor(this->category == category.id ? &paf::Rgba(1,1,1,1) : &paf::Rgba(1,1,1,0.4f));
        button->PlayEffect(0, effect::EffectType_Fadein1);
        button->elem.hash = category.nameHash;
        button->RegisterEventCallback(ui::EventMain_Decide, new SimpleEventCallback(apps::Page::CategoryCB, this), SCE_FALSE); //TODO: FIXED
        i++;
    }
}

SceVoid apps::Page::LockCategory()
{
    locked = true;
}

SceVoid apps::Page::ReleaseCategory()
{
    locked = false;
}

SceVoid apps::Page::Redisplay()
{
    ClearList();
    DisplayList();
}

SceVoid apps::Page::ClearList()
{
    if(listView->GetCellNum(0) > 0)
        listView->RemoveItem(0, 0, listView->GetCellNum(0)); //Delete all items 
}

SceVoid apps::Page::DisplayList()
{
    listView->AddItem(0, 0, targetList->GetSize(category));
}

SceVoid apps::Page::SetTargetList(db::List *list)
{
    targetList = list;
}

db::List *apps::Page::GetTargetList()
{
    return targetList;
}

SceVoid apps::Page::LoadJob::Run()
{
    SceInt32 ret = SCE_OK;

    print("apps::Page::LoadJob START!\n");

    Settings::GetInstance()->Close();

    thread::s_mainThreadMutex.Lock();
    callingPage->ClearList(); // Hide all widgets from the list

    callingPage->SetCategory(db::CategoryAll); // Reset the category
    // Set the categories according to db
    callingPage->SetCategories(std::vector<db::Category>(db::info[Settings::GetInstance()->source].categories, db::info[Settings::GetInstance()->source].categories + db::info[Settings::GetInstance()->source].categoryNum));
    // Prevent user from switching
    callingPage->LockCategory(); 
    // Delete any old surfaces
    for(db::entryInfo &entry : callingPage->appList.entries)
        if(entry.surface) entry.surface->Release();
    
    callingPage->refreshButton->Disable(0);
    callingPage->optionsButton->PlayEffectReverse(0, effect::EffectType_Reset);
    thread::s_mainThreadMutex.Unlock();
    
    //Get the previous time we downloaded from the last db in ticks
    rtc::Tick nextTick, prevTick = Time::GetPreviousDLTime(Settings::GetInstance()->source);    
    rtc::TickAddHours(&nextTick, &prevTick, Settings::GetInstance()->downloadInterval);

    rtc::Tick currentTick;
    rtc::GetCurrentTick(&currentTick);

    if(nextTick < currentTick) //The download time has passed (or was never set in the first place)
    {
        sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

        Dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, str::GetPFromHash(msg_wait));
        ret = cURLFile::SaveFile(db::info[Settings::GetInstance()->source].indexURL, db::info[Settings::GetInstance()->source].indexPath); 
        Dialog::Close();

        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);            

        if(ret != SCE_OK)
        {
            Dialog::OpenError(g_appPlugin, ret, str::GetPFromHash(msg_error_index), ErrorRetryCB, callingPage);
            return;
        }

        paf::rtc::Tick dlTick;
        paf::rtc::GetCurrentTick(&dlTick);
        Time::SetPreviousDLTime(Settings::GetInstance()->source, dlTick);
    }

    if(!LocalFile::Exists(db::info[Settings::GetInstance()->source].iconFolderPath))
    {
        Dir::CreateRecursive(db::info[Settings::GetInstance()->source].iconFolderPath); 
        if(db::info[Settings::GetInstance()->source].iconsURL != SCE_NULL)
        {
            string dialogText;
            
            common::String str(dialogText);
            str::GetfFromHash(msg_icon_pack_missing, &dialogText);
            
            wstring wstrText = str.GetWString();

            Dialog::OpenYesNo(g_appPlugin, NULL, (wchar_t *)wstrText.data(), IconDownloadDecideCB);
        }
    }

    thread::s_mainThreadMutex.Lock();
    callingPage->busyIndicator->Start(); //TODO: FIXED
    thread::s_mainThreadMutex.Unlock();

    callingPage->SetTargetList(&callingPage->appList);
    print("Parsing...");
    // Parse the DB
    ret = db::info[Settings::GetInstance()->source].Parse(&callingPage->appList, db::info[Settings::GetInstance()->source].indexPath);
    if(ret != SCE_OK) // Parsing failed
    {   
        Dialog::Close();
        Dialog::OpenError(g_appPlugin, ret, str::GetPFromHash(msg_error_parse), ErrorRetryCB, callingPage);
        return;   
    } print("Done!\n");
    // Sort the list into categories
    callingPage->appList.Categorise(Settings::GetInstance()->source);

    thread::s_mainThreadMutex.Lock();
    callingPage->busyIndicator->Stop();

    callingPage->DisplayList();

    callingPage->ReleaseCategory();

    callingPage->refreshButton->Enable(0);
    callingPage->optionsButton->PlayEffect(0, effect::EffectType_Reset);
    thread::s_mainThreadMutex.Unlock();
    print("apps::Page::LoadJob END!\n");
}

SceVoid apps::button::Callback::OnGet(SceInt32 eventId, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(self->elem.hash == 0x0 || !pUserData)
        return;
    
    apps::Page *page = (apps::Page *)pUserData;
    page->HideKeyboard();
    new apps::info::Page(page->GetTargetList()->Get(self->elem.hash));
}