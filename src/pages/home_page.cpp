#include <paf.h>
#include <message_dialog.h>
#include <kernel.h>
#include <shellsvc.h>

#include "pages/page.hpp"
#include "pages/home_page.hpp"
#include "pages/text_page.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "common.hpp"
#include "db.hpp"
#include "settings.hpp"

SceUInt32 totalLoads = 0;

using namespace paf;

home::Page::Page():generic::Page::Page("home_page_template")
{
    parsedList = SCE_NULL;
    searchList = SCE_NULL;
    list = SCE_NULL;
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
    loadQueue = new thread::JobQueue("BHBB::home::Page:loadQueue", &opt);

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

home::Page::~Page()
{

}

SceVoid home::Page::SetMode(PageMode targetMode)
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

        if(list != parsedList)
        {
            list = parsedList;
            listSize = parsedListSize;
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

SceVoid home::Page::SearchCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
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

SceVoid home::Page::ForwardButtonCB(SceInt32 id, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    home::Page *page = (home::Page *)pUserData;
    totalLoads++;

    if(((totalLoads + 1) * Settings::GetInstance()->nLoad) <= db::GetNumByCategory(page->list, page->listSize, -1))
        g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    else
        g_forwardButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
    
    
    auto populateJob = new PopulateJob("BHBB::PopulateJob()");
    populateJob->callingPage = page;
    
    auto populatePtr = paf::shared_ptr<thread::JobQueue::Item>(populateJob);
    page->loadQueue->Enqueue(&populatePtr);

}

SceVoid home::Page::DeleteBody(void *_page)
{
    totalLoads--;

    Page *page = (Page *)_page;
    PageBody *body = (PageBody *)page->body;

    if(body->iconThread)
    {
        if(body->iconThread->IsAlive())
        {
            body->iconThread->Cancel();
            body->iconThread->Join();
        }
        delete body->iconThread;
    }

	paf::common::Utils::WidgetStateTransition(-100, body->widget, paf::ui::Widget::Animation_3D_SlideFromFront, SCE_TRUE, SCE_TRUE);
	if (body->prev != SCE_NULL)
	{
		body->prev->widget->PlayAnimationReverse(0.0f, paf::ui::Widget::Animation_3D_SlideToBack1);
		body->prev->widget->PlayAnimation(0.0f, paf::ui::Widget::Animation_Reset);
		if (body->prev->widget->animationStatus & 0x80)
			body->prev->widget->animationStatus &= ~0x80;

		if (body->prev->prev != SCE_NULL) {
			body->prev->prev->widget->PlayAnimation(0.0f, paf::ui::Widget::Animation_Reset);
			if (body->prev->prev->widget->animationStatus & 0x80)
				body->prev->prev->widget->animationStatus &= ~0x80;
		}
	}
	page->body = page->body->prev;

    if (page->body != NULL && page->body->prev != SCE_NULL)
    {
        generic::Page::SetBackButtonEvent((generic::Page::BackButtonEventCallback)home::Page::DeleteBody, _page);
        g_backButton->PlayAnimation(0, paf::ui::Widget::Animation_Reset);
    }
    else
    {
        generic::Page::SetBackButtonEvent(NULL, NULL); //Default
        g_backButton->PlayAnimationReverse(0, paf::ui::Widget::Animation_Reset);
    } 

    if((totalLoads * Settings::GetInstance()->nLoad) < db::GetNumByCategory(page->list, page->listSize, -1))
        g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    else
        g_forwardButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
}

SceVoid home::Page::OnRedisplay()
{
    if(((totalLoads + 1) * Settings::GetInstance()->nLoad) < db::GetNumByCategory(list, listSize, -1))
        g_forwardButton->PlayAnimation(0, ui::Widget::Animation_Reset);
    else
        g_forwardButton->PlayAnimationReverse(0, ui::Widget::Animation_Reset);
    
    if(body != NULL)
    {
        if(body->prev != NULL)
        {
            Page::SetBackButtonEvent(Page::DeleteBody, this);
            g_backButton->PlayAnimation(0, ui::Widget::Animation_Reset);
        }
        else
            Page::SetBackButtonEvent(NULL, NULL);
    }
}

home::Page::PageBody *home::Page::MakeNewBody()
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
		newBody->prev->widget->PlayAnimation(0, paf::ui::Widget::Animation_3D_SlideToBack1);
		if (newBody->prev->widget->animationStatus & 0x80)
			newBody->prev->widget->animationStatus &= ~0x80;

		if (newBody->prev->prev != SCE_NULL)
		{
			newBody->prev->prev->widget->PlayAnimationReverse(0, paf::ui::Widget::Animation_Reset);
			if (newBody->prev->prev->widget->animationStatus & 0x80)
				newBody->prev->prev->widget->animationStatus &= ~0x80;
		}

        generic::Page::SetBackButtonEvent((generic::Page::BackButtonEventCallback)home::Page::DeleteBody, this);
        g_backButton->PlayAnimation(0, ui::Widget::Animation_Reset);
        
    }

    newBody->widget->PlayAnimation(-50000, paf::ui::Widget::Animation_3D_SlideFromFront);
	if (newBody->widget->animationStatus & 0x80)
		newBody->widget->animationStatus &= ~0x80;

    newBody->startEntry = db::GetByCategoryIndex(list, listSize, (totalLoads) * Settings::GetInstance()->nLoad, category);

    return newBody;
}

SceBool home::Page::SetCategory(int _category)
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

SceVoid home::Page::CategoryButtonCB(SceInt32 eventID, ui::Widget *self, SceInt32 unk, ScePVoid pUserData)
{
    if(!db::info[Settings::GetInstance()->source].CategoriesSuppourted) return;
    home::Page *page = (Page *)pUserData;
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

SceVoid home::Page::LoadJob::Finish()
{

}

SceVoid home::Page::Load()
{
    while(body)
    {
        Page::DeleteBody(this);
    }

    totalLoads = 0;

    auto job = new LoadJob("BHBB::LoadJob()");
    job->callingPage = this;

    auto ptr = paf::shared_ptr<thread::JobQueue::Item>(job);
    loadQueue->Enqueue(&ptr);
        
    auto populateJob = new PopulateJob("BHBB::PopulateJob()");
    populateJob->callingPage = this;

    auto populatePtr = paf::shared_ptr<thread::JobQueue::Item>(populateJob);
    loadQueue->Enqueue(&populatePtr);
    
}

SceVoid home::Page::PopulateJob::Run()
{
    if(((totalLoads + 1) * Settings::GetInstance()->nLoad) <= db::GetNumByCategory(callingPage->list, callingPage->listSize, callingPage->category))
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
    for(auto i = 0; i < Settings::GetInstance()->nLoad && ((totalLoads * Settings::GetInstance()->nLoad) + i) < callingPage->listSize; i++, currentEntry = &currentEntry[i])
    {
        if(currentEntry->type != callingPage->category && callingPage->category != -1)
        {
            i--;
            continue;
        }
        mainPlugin->TemplateOpen(listBox, &e, &tInit);
        ui::ImageButton *button = (ui::ImageButton *)listBox->GetChildByNum(listBox->childNum - 1);
        
        //Utils::SetWidgetLabel(button, &currentEntry->title);

        currentEntry->button = button;
        if(currentEntry->icon0.data == &string::s_emptyString)
            button->SetTextureBase(&BrokenTex);
    
    }

    thread::s_mainThreadMutex.Unlock();
    g_busyIndicator->Stop();

    body->iconThread = new IconLoadThread(SCE_KERNEL_LOWEST_PRIORITY_USER, SCE_KERNEL_16KiB, "BHBB::IconLoadThread");
    body->iconThread->startEntry = body->startEntry;
    body->iconThread->callingPage = callingPage;
    body->iconThread->Start();
}

SceVoid home::Page::Redisplay()
{
    while(body)
    {
        Page::DeleteBody(this);
    }
    totalLoads = 0;

    auto populateJob = new PopulateJob("BHBB::PopulateJob()");
    populateJob->callingPage = this;
    
    auto populatePtr = paf::shared_ptr<thread::JobQueue::Item>(populateJob);
    loadQueue->Enqueue(&populatePtr);
}

SceVoid home::Page::PopulateJob::Finish()
{
    
}

SceVoid home::Page::IconLoadThread::EntryFunction()
{
    /*
    HttpFile        httpFile;
    HttpFile::Param httpParam;

    httpParam.SetOpt(4000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT);
	httpParam.SetOpt(10000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT);
*/    
    db::entryInfo *currentEntry = startEntry;
    for(int i = 0; i < Settings::GetInstance()->nLoad && ((Settings::GetInstance()->nLoad * (totalLoads - 1)) + i) < callingPage->listSize && !IsCanceled(); i++, currentEntry++)
    {
        if(currentEntry->type != callingPage->category && callingPage->category != -1)
        {
            i--;
            continue;
        }
        if(currentEntry->tex != NULL)
        {
            currentEntry->button->SetTextureBase(&currentEntry->tex);
            continue;
        }

        bool tried = false;
    ASSIGN:
        if(paf::io::Misc::Exists(currentEntry->icon0Local.data))
        {
            if(Utils::CreateTextureFromFile(&currentEntry->tex, currentEntry->icon0Local.data))
            {
                currentEntry->button->SetTextureBase(&currentEntry->tex);
                if(callingPage->mode == PageMode_Search)
                {
                    for(int x = 0; x < callingPage->listSize; x++)
                    {
                        if(callingPage->list[x].hash == currentEntry->hash)
                        {
                            callingPage->list[x].tex = currentEntry->tex;
                        }
                    }
                }
            }
            else 
                currentEntry->button->SetTextureBase(&BrokenTex);
        }
        else if(currentEntry->icon0.data != &string::s_emptyString && !tried)
        {
        /*    httpParam.SetUrl(currentEntry->icon0.data);

            SceInt32 res = SCE_OK;
            if((res = httpFile.Open(&httpParam)) == SCE_OK)
            {
                SceInt32 bytesRead = 0;
                io::File *file = new io::File();
                file->Open(currentEntry->icon0Local.data, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);

                do
                {
                    char buff[256];
                    sce_paf_memset(buff, 0, sizeof(buff));
                    bytesRead = httpFile.Read(buff, 256);
                    file->Write(buff, bytesRead);
                } while (bytesRead > 0);

                file->Close();
                delete file;
                httpFile.Close();
                
                tried = true;
                goto ASSIGN;
            }
            else 
            {
                print("HttpFile::Open(%s) -> 0x%X\n", currentEntry->icon0.data, res);
                currentEntry->button->SetTextureBase(&BrokenTex);
            }
        */
        }
    }

    sceKernelExitDeleteThread(0);
}

SceVoid home::Page::LoadJob::Run()
{    
    HttpFile        httpFile;
    HttpFile::Param httpParam;

    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    Utils::MsgDialog::SystemMessage(SCE_MSG_DIALOG_SYSMSG_TYPE_WAIT_SMALL);

	httpParam.SetUrl(db::info[Settings::GetInstance()->source].indexURL);
	httpParam.SetOpt(4000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT);
	httpParam.SetOpt(10000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT);

    SceInt32 ret = httpFile.Open(&httpParam);
    if(ret == SCE_OK)
    {
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

        Utils::MsgDialog::EndMessage();
        
        if(io::Misc::Exists(db::info[Settings::GetInstance()->source].iconFolderPath)) //Skip icon downloading
            goto LOAD_PAGE;

        SceMsgDialogButtonId buttonId = Utils::MsgDialog::MessagePopupFromID("msg_icon_pack_missing", SCE_MSG_DIALOG_BUTTON_TYPE_YESNO);

        if(buttonId == SCE_MSG_DIALOG_BUTTON_ID_YES)
        {
            sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
            Utils::MsgDialog::InitProgress("Downloading");
            
            httpParam.SetUrl(db::info[Settings::GetInstance()->source].iconsURL);

            if(httpFile.Open(&httpParam) == SCE_OK)    
            {
                
                SceUInt64 dlSize = httpFile.GetSize();
                SceUInt64 totalRead = 0;

                char *buff = new char[dlSize];
                
                int bytesRead = 0;
                do
                {
                    bytesRead = httpFile.Read(&buff[totalRead], 1024);
                    if(bytesRead < 0)
                        print("Error reading from server: 0x%X\n", bytesRead);

                    totalRead += bytesRead;
                    Utils::MsgDialog::UpdateProgress(((float)totalRead / (float)dlSize) * 100.0f);
                } while(bytesRead > 0);
              

                httpFile.Close();
                
                Utils::MsgDialog::EndMessage();

                Utils::MsgDialog::InitProgress("Extracting");

                Utils::ExtractZipFromMemory((uint8_t *)buff, dlSize, db::info[Settings::GetInstance()->source].iconFolderPath, true);

                delete[] buff;
                Utils::MsgDialog::EndMessage();

            }
            else
            {
                Utils::MsgDialog::EndMessage();
                Utils::MsgDialog::MessagePopup("Network Error Occurred!\n");
            }

            sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
        }
    }
    else
    {
        Utils::MsgDialog::EndMessage();
        Utils::MsgDialog::MessagePopupFromID("msg_error_index");
        print("ret = 0x%X\n", ret);
    }

LOAD_PAGE:
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
    
    g_busyIndicator->Start();

    // Delete list.

    db::info[Settings::GetInstance()->source].Parse(&callingPage->parsedList, &callingPage->parsedListSize, callingPage->dbIndex.data, callingPage->dbIndex.length);
    callingPage->list = callingPage->parsedList;
    callingPage->listSize = callingPage->parsedListSize;

    g_busyIndicator->Stop();
}