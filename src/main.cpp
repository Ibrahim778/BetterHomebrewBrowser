#include <kernel.h>
#include <stdio.h>
#include <curl/curl.h>
#include <paf.h>
#include <shellsvc.h>
#include <message_dialog.h>
#include <libhttp.h>

#include "main.hpp"
#include "paf.hpp"
#include "parser.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "network.hpp"
#include "downloader.hpp"
#include "pages/page.hpp"
#include "pages/home_page.hpp"
#include "pages/text_page.hpp"

extern "C" {

    SCE_USER_MODULE_LIST("app0:module/libScePafPreload.suprx");

    extern const char			sceUserMainThreadName[] = "BHBB_MAIN";
    extern const int			sceUserMainThreadPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    extern const unsigned int	sceUserMainThreadStackSize = SCE_KERNEL_THREAD_STACK_SIZE_DEFAULT_USER_MAIN;
    extern const unsigned int   sceLibcHeapSize = 0x180000;

    void __cxa_set_dso_handle_main(void *dso)
    {

    }

    int _sceLdTlsRegisterModuleInfo()
    {
        return 0;
    }

    int __aeabi_unwind_cpp_pr0()
    {
        return 9;
    }

    int __aeabi_unwind_cpp_pr1()
    {
        return 9;
    }

    int __at_quick_exit()
    {
        return 0;
    }
}

int loadFlags = 0;

paf::ui::Widget *g_mainPage;
paf::ui::Widget *g_errorPage;

home::Page *g_homePage = SCE_NULL;

int main()
{
    //Utils::StartBGDL();
    Utils::InitMusic();
    Utils::SetMemoryInfo();

	initPlugin();

    return sceKernelExitProcess(0);
}

void OnNetworkReady()
{
    if(Network::GetCurrentStatus() == Network::Online)
    {
        g_homePage->Load();   
    }
    else 
    {
        Utils::MsgDialog::MessagePopupFromID("msg_error_net");

        paf::string errorMsg;
        Utils::GetfStringFromID("msg_net_fix", &errorMsg);

        new text::Page(errorMsg.data);

        g_backButton->PlayAnimationReverse(0, paf::ui::Widget::Animation_Reset);
    }
}

void OnReady()
{
    sceShellUtilInitEvents(0);
    
    generic::Page::Init();
    g_homePage = new home::Page();


    Network::Init();
    Network::Check(OnNetworkReady);
}