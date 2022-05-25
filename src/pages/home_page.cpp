#include <paf.h>
#include <message_dialog.h>
#include <kernel.h>
#include <shellsvc.h>

#include "pages/page.hpp"
#include "pages/home_page.hpp"
#include "pages/text_page.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "parser.hpp"
#include "common.hpp"
#include "db.hpp"
#include "parser.hpp"

db::Id currentDB = db::Id::CBPSDB;

using namespace paf;

home::Page::Page():generic::Page::Page("home_page_template")
{
    loadThread = SCE_NULL;
    iconLoadThread = SCE_NULL;

    thread::JobQueue::Opt opt;
    opt.workerNum = 1;
    opt.workerOpt = NULL;
    opt.workerPriority = SCE_KERNEL_HIGHEST_PRIORITY_USER + 20;
    opt.workerStackSize = SCE_KERNEL_16KiB;
    populateQueue = new thread::JobQueue("BHBB::home::Page::populateQueue", &opt);
}

SceVoid home::Page::Populate()
{
    auto job = new PopulateJob("BHBB::home::Page::PopulateJob");
    job->callingPage = this;

    shared_ptr<thread::JobQueue::Item> item = shared_ptr<thread::JobQueue::Item>(job);
    populateQueue->Enqueue(&item);
}

home::Page::~Page()
{

}

SceVoid home::Page::Load()
{
    if(loadThread)
    {
        if(loadThread->IsAlive())
        {
            loadThread->Cancel();
            loadThread->Join();
        }
        delete loadThread;
        loadThread = SCE_NULL;
    }
    if(iconLoadThread)
    {
        print("Icon thread is alive!\n");
        iconLoadThread->Cancel();
        iconLoadThread->Join();
        
        delete iconLoadThread;
        iconLoadThread = SCE_NULL;
    }
    list.Clear(false);
    
    loadThread = new LoadThread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_8KiB, "BHBB::LoadThread");
    loadThread->callingPage = this;
    loadThread->Start();
}

SceVoid home::Page::IconLoadThread::EntryFunction()
{
    parser::HomebrewList::node *node = list.head;
    int i = 0;
    while(node != NULL && !IsCanceled() && i < 100)
    {
        print("IL: node %p button %p tex %p url %s\n", node, node->button, node->tex, node->info.icon0.data);
        if(node->tex != NULL)
        {
            node->button->SetTextureBase(&node->tex);
            continue;
        }

        bool tried = false;
    ASSIGN:
        if(paf::io::Misc::Exists(node->info.icon0Local.data))
        {
            if(Utils::CreateTextureFromFile(&node->tex, node->info.icon0Local.data))
                node->button->SetTextureBase(&node->tex);
            else 
                node->button->SetTextureBase(&BrokenTex);
        }
        else if(node->info.icon0.data != &string::s_emptyString)
        {

            shared_ptr<paf::HttpFile> httpFile;
            SceInt32 error = SCE_OK;

            HttpFile::Open(&httpFile, node->info.icon0.data, &error, SCE_O_RDONLY);    
            
            if(error == SCE_OK)
            {
                SceInt32 bytesRead = 0;
                io::File *file = new io::File();
                file->Open(node->info.icon0Local.data, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);

                do
                {
                    char buff[256];
                    sce_paf_memset(buff, 0, sizeof(buff));
                    bytesRead = httpFile->Read(buff, 256);
                    file->Write(buff, bytesRead);
                } while (bytesRead > 0);

                file->Close();
                delete file;
                httpFile->Close();
                
                tried = true;
                goto ASSIGN;
            }
            else 
            {
                print("HttpFile::Open(%s) -> 0x%X\n", node->info.icon0.data);
                node->button->SetTextureBase(&BrokenTex);
            }
        }
        node = node->next;
        i++;
    }

    print("Done!\n");
    sceKernelExitDeleteThread(0);
}

SceVoid home::Page::PopulateJob::Run()
{
    bool isMainThread = thread::IsMainThread();
    if(!isMainThread)
        thread::s_mainThreadMutex.Lock();

    auto prevBody = Utils::GetChildByHash(callingPage->root, Utils::GetHashById("list_plane"));
    if(prevBody)
    {
        parser::HomebrewList::node *n = list.head;
        for(int i = 0; i < list.num && i < 100 && n != NULL; i++, n = n->next)
        {
            n->button->SetTextureBase(&TransparentTex);
            Utils::DeleteTexture(&n->tex);
        }
        common::Utils::WidgetStateTransition(0, prevBody, ui::Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
    } 

    if(!isMainThread)
        thread::s_mainThreadMutex.Unlock();

    db::info[currentDB].Parse(callingPage->dbIndex.data);

    parser::HomebrewList::node *node = list.head;
    
    Plugin::TemplateInitParam tInit;
    Resource::Element e;

    e.hash = Utils::GetHashById("home_page_list_template");

    if(!isMainThread)
        thread::s_mainThreadMutex.Lock();

    mainPlugin->TemplateOpen(callingPage->root, &e, &tInit);
    
    e.hash = Utils::GetHashById("homebrew_button");
    auto listBox = Utils::GetChildByHash(callingPage->root, Utils::GetHashById("list_scroll_box"));
    int i = 0;
    while(node != NULL && i < 100)
    {
        mainPlugin->TemplateOpen(listBox, &e, &tInit);
        
        ui::ImageButton *button = (ui::ImageButton *)listBox->GetChildByNum(listBox->childNum - 1);
        button->SetLabel(&node->info.wstrtitle);
        
        node->button = button;
        if(node->info.icon0.data == &string::s_emptyString)
            button->SetTextureBase(&BrokenTex);
        
        node = node->next;
        i++;
    }

    if(db::info[currentDB].ScreenshotsSuppourted)
    {
        //Impliment recent view
    }
    else common::Utils::WidgetStateTransition(0, Utils::GetChildByHash(callingPage->root, Utils::GetHashById("plane_top")), ui::Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);

    if(!isMainThread)
        thread::s_mainThreadMutex.Unlock();

    callingPage->LoadIcons();
}

SceVoid home::Page::PopulateJob::Finish()
{

}

SceVoid home::Page::LoadIcons()
{
    if(iconLoadThread)
    {
        if(iconLoadThread->IsAlive())
        {
            iconLoadThread->Cancel();
            iconLoadThread->Join();
        }
        delete iconLoadThread;
    }

    iconLoadThread = new IconLoadThread(SCE_KERNEL_LOWEST_PRIORITY_USER, SCE_KERNEL_16KiB, "BHBB::IconLoadThread");
    iconLoadThread->Start();
}

home::Page::PopulateJob::~PopulateJob()
{

}

SceVoid home::Page::LoadThread::EntryFunction()
{    
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    HttpFile        httpFile;
    HttpFile::Param httpParam;

    Utils::MsgDialog::SystemMessage(SCE_MSG_DIALOG_SYSMSG_TYPE_WAIT_SMALL);

	httpParam.SetUrl(db::info[currentDB].indexURL);
	httpParam.SetOpt(4000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT);
	httpParam.SetOpt(10000000, HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT);

    SceInt32 ret = httpFile.Open(&httpParam);
    if(ret == SCE_OK)
    {
        callingPage->dbIndex = "";

        int bytesRead;
        do
        {
            char buff[257];
            sce_paf_memset(buff, 0, sizeof(buff));

            bytesRead = httpFile.Read(buff, 256);

            callingPage->dbIndex += buff;
        } while(bytesRead > 0 && !IsCanceled());

        httpFile.Close();

        Utils::MsgDialog::EndMessage();
        
        if(io::Misc::Exists(db::info[currentDB].iconFolderPath) || IsCanceled()) //Skip icon downloading
            goto LOAD_PAGE;

        SceMsgDialogButtonId buttonId = Utils::MsgDialog::MessagePopupFromID("msg_icon_pack_missing", SCE_MSG_DIALOG_BUTTON_TYPE_YESNO);

        if(buttonId == SCE_MSG_DIALOG_BUTTON_ID_YES)
        {
            sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
            Utils::MsgDialog::InitProgress("Downloading");
            
            httpParam.SetUrl(db::info[currentDB].iconsURL);

            if(httpFile.Open(&httpParam) == SCE_OK)    
            {
                
                SceUInt64 dlSize = httpFile.GetSize();
                SceUInt64 totalRead = 0;

                char *buff = new char[dlSize];

                int bytesRead = 0;
                do
                {
                    bytesRead = httpFile.Read(&buff[totalRead], 1024);

                    totalRead += bytesRead;
                    Utils::MsgDialog::UpdateProgress(((float)totalRead / (float)dlSize) * 100.0f);
                } while(bytesRead > 0);
              

                httpFile.Close();
                
                Utils::MsgDialog::EndMessage();

                Utils::MsgDialog::InitProgress("Extracting");

                Utils::ExtractZipFromMemory((uint8_t *)buff, dlSize, db::info[currentDB].iconFolderPath, true);

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

LOAD_PAGE:
        if(!IsCanceled());
            callingPage->Populate();
    }
    else
    {
        Utils::MsgDialog::EndMessage();
        print("ret = 0x%X\n", ret);
    }
    
    if(ret != SCE_OK)
    {
        Utils::MsgDialog::MessagePopupFromID("msg_error_index");
    }
    
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    sceKernelExitDeleteThread(0);
}