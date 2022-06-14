#include <paf.h>
#include <message_dialog.h>
#include <kernel.h>
#include <shellsvc.h>

#include "pages/page.h"
#include "pages/apps_page.h"
#include "pages/text_page.h"
#include "print.h"
#include "utils.h"
#include "common.h"
#include "db.h"
#include "settings.h"
#include "dialog.h"

SceUInt32 totalLoads = 0;

using namespace paf;

apps::Page::Page():generic::Page::Page("home_page_template")
{
    list = &parsedList;
    body = SCE_NULL;
    loadQueue = SCE_NULL;

    ui::Widget::EventCallback *categoryCallback = new ui::Widget::EventCallback();
    categoryCallback->pUserData = this;
    categoryCallback->eventHandler = Page::CategoryButtonCB;

    Utils::GetChildByHash(root, Utils::GetHashById("game_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("all_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("emu_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("port_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, categoryCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("util_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, categoryCallback, 0);

    SetCategory(-1);
    
    ui::Widget::EventCallback *searchCallback = new ui::Widget::EventCallback();
    searchCallback->pUserData = this;
    searchCallback->eventHandler = Page::SearchCB;

    Utils::GetChildByHash(root, Utils::GetHashById("search_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, searchCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("search_enter_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, searchCallback, 0);
    Utils::GetChildByHash(root, Utils::GetHashById("search_back_button"))->RegisterEventCallback(ui::Widget::EventMain_Decide, searchCallback, 0);

    SetMode(PageMode_Browse);
 
    thread::JobQueue::Opt opt;
    opt.workerNum = 1;
    opt.workerOpt = NULL;
    opt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    opt.workerStackSize = SCE_KERNEL_16KiB;
    loadQueue = new thread::JobQueue("BHBB::apps::Page:loadQueue", &opt);

    g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    auto eventCallback = new ui::Widget::EventCallback();
    eventCallback->eventHandler = Page::ForwardButtonCB;
    eventCallback->pUserData = this;
    
    g_forwardButton->RegisterEventCallback(ui::Widget::EventMain_Decide, eventCallback, 0);

    auto optionsButton = Utils::GetChildByHash(root, Utils::GetHashById("options_button"));
    auto optionsCallback = new ui::Widget::EventCallback();
    optionsCallback->eventHandler = Settings::OpenCB;

    optionsButton->RegisterEventCallback(ui::Widget::EventMain_Decide, optionsCallback, 0);
}

apps::Page::~Page()
{

}

SceVoid apps::Page::DeleteBodyCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    ((Page *)pUserData)->DeleteBody();
}

SceVoid apps::Page::SetMode(PageMode targetMode)
{
    if(mode == targetMode) return;

    ui::Widget *categoriesPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_categories"));
    ui::Widget *searchPlane = Utils::GetChildByHash(root, Utils::GetHashById("plane_search"));
    ui::Widget *searchBox = Utils::GetChildByHash(root, Utils::GetHashById("search_box"));

    switch(targetMode)
    {
    case PageMode_Browse:
        searchPlane->PlayAnimationReverse(0, ui::Widget::Animation_Fadein1);
        categoriesPlane->PlayAnimation(0, ui::Widget::Animation_Fadein1);

        if(list != &parsedList)
        {
            list = &parsedList;
            Redisplay();
        }
        break;
    
    case PageMode_Search:
        categoriesPlane->PlayAnimationReverse(0, ui::Widget::Animation_Fadein1);
        searchPlane->PlayAnimation(0, ui::Widget::Animation_Fadein1);
        break;
    }

    mode = targetMode;
}

SceVoid apps::Page::SearchCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    Page *page = (Page *)pUserData;
    switch(self->hash)
    {
    case Hash_SearchButton:
        page->SetMode(PageMode_Search);
        break;

    case Hash_SearchBackButton:
        page->SetMode(PageMode_Browse);
        break;

    case Hash_SearchEnterButton:
        break;
    }
}

SceVoid apps::Page::ForwardButtonCB(SceInt32 id, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    apps::Page *page = (apps::Page *)pUserData;
    totalLoads++;

    if(((totalLoads + 1) * Settings::GetInstance()->nLoad) <= page->list->GetSize(page->category))
        g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    else
        g_forwardButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
    
    
    auto populateJob = new PopulateJob("BHBB::PopulateJob()");
    populateJob->callingPage = page;
    
    auto populatePtr = shared_ptr<thread::JobQueue::Item>(populateJob);
    page->loadQueue->Enqueue(&populatePtr);

}

SceVoid apps::Page::DeleteBody()
{
    totalLoads--;

    if(body->iconThread)
    {
        if(body->iconThread->IsAlive())
        {
            body->iconThread->Cancel();
            body->iconThread->Join();
        }
        delete body->iconThread;
    }

	common::Utils::WidgetStateTransition(-100, body->widget, ui::Widget::Animation_3D_SlideFromFront, SCE_TRUE, SCE_TRUE);
	if (body->prev != SCE_NULL)
	{
		body->prev->widget->PlayAnimationReverse(0.0f, ui::Widget::Animation_3D_SlideToBack1);
		body->prev->widget->PlayAnimation(0.0f, ui::Widget::Animation_Reset);
		if (body->prev->widget->animationStatus & 0x80)
			body->prev->widget->animationStatus &= ~0x80;

		if (body->prev->prev != SCE_NULL) {
			body->prev->prev->widget->PlayAnimation(0.0f, ui::Widget::Animation_Reset);
			if (body->prev->prev->widget->animationStatus & 0x80)
				body->prev->prev->widget->animationStatus &= ~0x80;
		}
	}
	body = body->prev;

    if (body != NULL && body->prev != SCE_NULL)
    {
        generic::Page::SetBackButtonEvent((generic::Page::BackButtonEventCallback)apps::Page::DeleteBodyCB, this);
        g_backButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    }
    else
    {
        generic::Page::SetBackButtonEvent(NULL, NULL); //Default
        g_backButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
    } 

    if((totalLoads * Settings::GetInstance()->nLoad) < list->GetSize(category))
        g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    else
        g_forwardButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
}

SceVoid apps::Page::IconDownloadDecideCB(Dialog::ButtonCode buttonResult, ScePVoid userDat)
{
    if(buttonResult == Dialog::ButtonCode::ButtonCode_Yes)
    {
        BGDLParam param;
        sce_paf_memset(&param, 0, sizeof(param));
        param.magic = (BHBB_DL_CFG_VER | BHBB_DL_MAGIC);
        param.type = 0;
        sce_paf_strncpy(param.path, db::info[Settings::GetInstance()->source].iconFolderPath, sizeof(param.path));

        string str;
        str.Setf("%s Icons", db::info[Settings::GetInstance()->source].name);
        g_downloader->EnqueueAsync(db::info[Settings::GetInstance()->source].iconsURL, str.data, &param);
    }
}

SceVoid apps::Page::OnRedisplay()
{
    if(((totalLoads + 1) * Settings::GetInstance()->nLoad) < list->GetSize(category))
        g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    else
        g_forwardButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
    
    if(body != NULL)
    {
        if(body->prev != NULL)
        {
            Page::SetBackButtonEvent(Page::DeleteBodyCB, this);
            g_backButton->PlayAnimation(0, ui::Widget::Animation_Reset);
        }
        else
            Page::SetBackButtonEvent(NULL, NULL);
    }
}

apps::Page::PageBody *apps::Page::MakeNewBody()
{
    PageBody *newBody = new PageBody;

    newBody->prev = body;
    body = newBody;

    Plugin::TemplateInitParam tInit;
    Resource::Element e = Utils::GetParamWithHashFromId("home_page_list_template");
    
    mainPlugin->TemplateOpen(root, &e, &tInit);

    newBody->widget = (ui::Plane *)root->GetChildByNum(root->childNum - 1);
	
    if (newBody->prev != NULL)
	{
		newBody->prev->widget->PlayAnimation(0, ui::Widget::Animation_3D_SlideToBack1);
		if (newBody->prev->widget->animationStatus & 0x80)
			newBody->prev->widget->animationStatus &= ~0x80;

		if (newBody->prev->prev != SCE_NULL)
		{
			newBody->prev->prev->widget->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
			if (newBody->prev->prev->widget->animationStatus & 0x80)
				newBody->prev->prev->widget->animationStatus &= ~0x80;
		}

        generic::Page::SetBackButtonEvent((generic::Page::BackButtonEventCallback)apps::Page::DeleteBodyCB, this);
        g_backButton->PlayAnimation(0, ui::Widget::Animation_Reset);
        
    }

    newBody->widget->PlayAnimation(-50000, ui::Widget::Animation_3D_SlideFromFront);
	if (newBody->widget->animationStatus & 0x80)
		newBody->widget->animationStatus &= ~0x80;

    newBody->startEntry = list->Get((totalLoads) * Settings::GetInstance()->nLoad, category);
    newBody->iconThread = SCE_NULL;

    return newBody;
}

SceBool apps::Page::SetCategory(int _category)
{
    if(_category == category) return SCE_FALSE;
    category = _category;

    ui::Widget::Color transparent, normal;
    transparent.r = 1;
    transparent.g = 1;
    transparent.b = 1;
    transparent.a = .4f;

    normal.r = 1;
    normal.g = 1;
    normal.b = 1;
    normal.a = 1;

    ui::Widget *allButton = Utils::GetChildByHash(root, Utils::GetHashById("all_button"));
    ui::Widget *gamesButton = Utils::GetChildByHash(root, Utils::GetHashById("game_button"));
    ui::Widget *emuButton = Utils::GetChildByHash(root, Utils::GetHashById("emu_button"));
    ui::Widget *portsButton = Utils::GetChildByHash(root, Utils::GetHashById("port_button"));
    ui::Widget *utilButton = Utils::GetChildByHash(root, Utils::GetHashById("util_button"));
    
    allButton->SetColor(&transparent);
    gamesButton->SetColor(&transparent);
    emuButton->SetColor(&transparent);
    portsButton->SetColor(&transparent);
    utilButton->SetColor(&transparent);

    switch(category)
    {
    case -1:
        allButton->SetColor(&normal);
        break;
    case db::Category::EMULATOR:
        emuButton->SetColor(&normal);
        break;
    case db::Category::GAME:
        gamesButton->SetColor(&normal);
        break;
    case db::Category::PORT:
        portsButton->SetColor(&normal);
        break;
    case db::Category::UTIL:
        utilButton->SetColor(&normal);
        break;
    }

    return SCE_TRUE;
}

SceVoid apps::Page::CategoryButtonCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(!db::info[Settings::GetInstance()->source].CategoriesSuppourted) return;
    apps::Page *page = (Page *)pUserData;
    int targetCategory = 0;
    switch (self->hash)
    {
    case Hash_All:
        targetCategory = -1;
        break;
    case Hash_Util:
        targetCategory = db::Category::UTIL;
        break;
    case Hash_Game:
        targetCategory = db::Category::GAME;
        break;
    case Hash_Emu:
        targetCategory = db::Category::EMULATOR;
        break;
    case Hash_Port:
        targetCategory = db::Category::PORT;
        break;
    }

    if(page->SetCategory(targetCategory)) // returns true if there's any change
        page->Redisplay();
}

SceVoid apps::Page::LoadJob::Finish()
{

}

SceVoid apps::Page::Load()
{
    while(body)
    {
        Page::DeleteBody();
    }

    totalLoads = 0;

    auto job = new LoadJob("BHBB::LoadJob()");
    job->callingPage = this;

    auto ptr = shared_ptr<thread::JobQueue::Item>(job);
    loadQueue->Enqueue(&ptr);
        
    auto populateJob = new PopulateJob("BHBB::PopulateJob()");
    populateJob->callingPage = this;

    auto populatePtr = shared_ptr<thread::JobQueue::Item>(populateJob);
    loadQueue->Enqueue(&populatePtr);
    
}

SceVoid apps::Page::PopulateJob::Run()
{
    if(((totalLoads + 1) * Settings::GetInstance()->nLoad) <= callingPage->list->GetSize(callingPage->category))
        g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    else
        g_forwardButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);

    g_busyIndicator->Start();
    
    Plugin::TemplateInitParam tInit;
    Resource::Element e = Utils::GetParamWithHashFromId("homebrew_button");

    thread::s_mainThreadMutex.Lock();

    PageBody *body = callingPage->MakeNewBody();

    auto listBox = Utils::GetChildByHash(body->widget, Utils::GetHashById("list_scroll_box"));

    db::entryInfo *currentEntry = body->startEntry;

    for(auto i = 0; i < Settings::GetInstance()->nLoad && callingPage->list->IsValidEntry(currentEntry); i++, currentEntry++)
    {
        if(currentEntry->type != callingPage->category && callingPage->category != -1)
        {
            i--;
            continue;
        }
        mainPlugin->TemplateOpen(listBox, &e, &tInit);
        ui::ImageButton *button = (ui::ImageButton *)listBox->GetChildByNum(listBox->childNum - 1);
        
        Utils::SetWidgetLabel(button, currentEntry->title);

        currentEntry->button = button;

        if(currentEntry->icon.data == &string::s_emptyString && currentEntry->icon_mirror.data == &string::s_emptyString)
            button->SetTextureBase(&BrokenTex);
        else if(currentEntry->tex != SCE_NULL)
            button->SetTextureBase(&currentEntry->tex);
    }

    thread::s_mainThreadMutex.Unlock();
    g_busyIndicator->Stop();

    body->iconThread = new IconLoadThread(SCE_KERNEL_LOWEST_PRIORITY_USER, SCE_KERNEL_16KiB, "BHBB::IconLoadThread");
    body->iconThread->startEntry = body->startEntry;
    body->iconThread->callingPage = callingPage;
    body->iconThread->Start();
}

SceVoid apps::Page::Redisplay()
{
    if(loadQueue->GetSize() > 0)
        return; //Wait for it to finish loading ffs

    while(body)
    {
        Page::DeleteBody();
    }

    totalLoads = 0;

    auto populateJob = new PopulateJob("BHBB::PopulateJob()");
    populateJob->callingPage = this;
    
    auto populatePtr = shared_ptr<thread::JobQueue::Item>(populateJob);
    loadQueue->Enqueue(&populatePtr);
}

SceVoid apps::Page::PopulateJob::Finish()
{
    
}

SceVoid apps::Page::IconLoadThread::EntryFunction()
{
    db::entryInfo *currentEntry = startEntry;
    for(int i = 0; i < Settings::GetInstance()->nLoad && callingPage->list->IsValidEntry(currentEntry) && !IsCanceled(); i++, currentEntry++)
    {
        if(currentEntry->type != callingPage->category && callingPage->category != -1)
        {
            i--;
            continue;
        }

        if(currentEntry->tex != NULL)
            continue;
        if(io::Misc::Exists(currentEntry->iconLocal.data))
        {
            if(Utils::CreateTextureFromFile(&currentEntry->tex, currentEntry->iconLocal.data))
            {
                currentEntry->button->SetTextureBase(&currentEntry->tex);
                if(callingPage->mode == PageMode_Search)
                {
                    for(db::entryInfo *parsedListEntry = callingPage->parsedList.Get(); callingPage->parsedList.IsValidEntry(parsedListEntry); currentEntry++)
                    {
                        if(parsedListEntry->hash == currentEntry->hash)
                        {
                            parsedListEntry->tex = currentEntry->tex;
                        }
                    }
                }
            }
            else
            {
                currentEntry->button->SetTextureBase(&BrokenTex);
                goto STREAM;
            } 
            
            continue;
        }

STREAM:
        shared_ptr<HttpFile> file;
        int error = SCE_OK;

        if(currentEntry->icon.data == &string::s_emptyString) goto FAILSAFE;
        
        HttpFile::Open(&file, currentEntry->icon.data, &error, SCE_O_RDONLY);
        if(error != SCE_OK)
            goto FAILSAFE;

        graphics::Surface::CreateFromFile(&currentEntry->tex, mainPlugin->memoryPool, (shared_ptr<LocalFile> *)&file);
        if(currentEntry->tex == SCE_NULL)
        {
            file.reset();
            goto FAILSAFE; 
        }
        
        currentEntry->button->SetTextureBase(&currentEntry->tex);
        
        continue;

    FAILSAFE:
        if(IsCanceled()) break;
        if(currentEntry->icon_mirror.data == &string::s_emptyString)
        {
            currentEntry->button->SetTextureBase(&BrokenTex);
            continue;
        }

        error = SCE_OK;
        HttpFile::Open(&file, currentEntry->icon_mirror.data, &error, SCE_O_RDONLY);
        if(error != SCE_OK)
        {
            currentEntry->button->SetTextureBase(&BrokenTex);
            continue;
        }

        graphics::Surface::CreateFromFile(&currentEntry->tex, mainPlugin->memoryPool, (shared_ptr<LocalFile> *)&file);
        if(currentEntry->tex == SCE_NULL)
        {
            currentEntry->button->SetTextureBase(&BrokenTex);
            continue; 
        }

        currentEntry->button->SetTextureBase(&currentEntry->tex);
    }
    sceKernelExitDeleteThread(0);
}

SceVoid apps::Page::ErrorRetryCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    Page::DeleteCurrentPage();
    ((Page *)pUserData)->Load();
    Page::SetBackButtonEvent(NULL, NULL);
}

SceVoid apps::Page::LoadJob::Run()
{    
    HttpFile        httpFile;
    HttpFile::Param httpParam;

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    Dialog::OpenPleaseWait(mainPlugin, SCE_NULL, Utils::GetStringPFromID("msg_wait"));
    Settings::GetInstance()->Close();
	httpParam.SetUrl(db::info[Settings::GetInstance()->source].indexURL);
	httpParam.SetOpt(4000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT);
	httpParam.SetOpt(10000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT);

    SceInt32 ret = httpFile.Open(&httpParam);
    if(ret != SCE_OK)
    {
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

        Dialog::Close();
        Dialog::WaitEnd();

        string msgTemplate;
        Utils::GetfStringFromID("msg_net_fix", &msgTemplate);

        string errorMsg;
        errorMsg.Setf(msgTemplate.data, ret);
        
        new text::Page(errorMsg.data);
        
        Dialog::OpenError(mainPlugin, ret, Utils::GetStringPFromID("msg_error_index"));
        
        Page::SetBackButtonEvent(Page::ErrorRetryCB, callingPage);
        return;
    }

    callingPage->dbIndex = "";

    int bytesRead;
    do
    {
        char buff[257]; //Leave 1 char for '\0'
        sce_paf_memset(buff, 0, sizeof(buff));

        bytesRead = httpFile.Read(buff, 256);

        callingPage->dbIndex += buff;
    } while(bytesRead > 0);

    httpFile.Close();

    Dialog::Close();

    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    if(!io::Misc::Exists(db::info[Settings::GetInstance()->source].iconFolderPath))
    {
        string dialogText;
        wstring wstrText;
        Utils::GetfStringFromID("msg_icon_pack_missing", &dialogText);
        dialogText.ToWString(&wstrText);

        Dialog::OpenYesNo(mainPlugin, NULL, wstrText.data, IconDownloadDecideCB);
    }
    
    g_busyIndicator->Start();

    db::info[Settings::GetInstance()->source].Parse(&callingPage->parsedList, callingPage->dbIndex);

}