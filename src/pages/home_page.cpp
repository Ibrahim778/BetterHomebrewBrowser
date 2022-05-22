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

home::Page::Page():generic::Page::Page("home_page_template")
{
    loadThread = SCE_NULL;

    paf::thread::JobQueue::Opt opt;
    opt.workerNum = 1;
    opt.workerOpt = NULL;
    opt.workerPriority = SCE_KERNEL_HIGHEST_PRIORITY_USER + 20;
    opt.workerStackSize = SCE_KERNEL_16KiB;
    
    populateQueue = new paf::thread::JobQueue("BHBB::home::Page::populateQueue", &opt);
}

SceVoid home::Page::Populate()
{
    auto job = new PopulateJob("BHBB::home::Page::PopulateJob");
    job->callingPage = this;

    paf::shared_ptr<paf::thread::JobQueue::Item> item = paf::shared_ptr<paf::thread::JobQueue::Item>(job);
    populateQueue->Enqueue(&item);
}

home::Page::~Page()
{

}

SceVoid home::Page::Load()
{
    if(loadThread) return;
    loadThread = new LoadThread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_4KiB, "BHBB::LoadThread");
    loadThread->callingPage = this;
    loadThread->Start();
}

SceVoid home::Page::PopulateJob::Run()
{
    paf::Plugin::TemplateInitParam tInit;
    paf::Resource::Element e;
    e.hash = Utils::GetHashById("homebrew_button");

    auto scrollBox = Utils::GetChildByHash(callingPage->root, Utils::GetHashById("list_scroll_box"));

    db::info[db::Id::CBPSDB].Parse(callingPage->dbIndex.data); //This doesn't touch any widgets, it works OK
    
    parser::HomebrewList::node *node = list.head;
    while(node != NULL)
    {
        mainPlugin->TemplateOpen(scrollBox, &e, &tInit);
        auto button = scrollBox->GetChildByNum(scrollBox->childNum - 1);
        button->SetLabel(&node->info.wstrtitle);

        node = node->next;
        paf::thread::Sleep(10);
    }
}

SceVoid home::Page::PopulateJob::Finish()
{
    
}

home::Page::PopulateJob::~PopulateJob()
{

}

paf::ui::ImageButton *home::Page::AddOption(paf::wstring *title)
{
    
}

SceVoid home::Page::LoadThread::EntryFunction()
{    
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    paf::HttpFile                   httpFile;
    paf::HttpFile::Param            httpParam;

    Utils::MsgDialog::SystemMessage(SCE_MSG_DIALOG_SYSMSG_TYPE_WAIT_SMALL);

	httpParam.SetUrl(db::info[db::Id::CBPSDB].indexURL);
	httpParam.SetOpt(4000000, paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT);
	httpParam.SetOpt(10000000, paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT);
    httpParam.userAgent = "PS Vita";
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
        } while(bytesRead > 0);

        httpFile.Close();

        callingPage->Populate();

    }
    else print("ret = 0x%X\n", ret);

    Utils::MsgDialog::EndMessage();
    
    if(ret != SCE_OK)
    {
        Utils::MsgDialog::MessagePopupFromID("msg_error_index");
    }
    
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

    sceKernelExitDeleteThread(0);
}