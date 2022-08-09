#include <kernel.h>
#include <stdio.h>
#include <paf.h>
#include <shellsvc.h>
#include <libsysmodule.h>
#include <apputil.h>

#include "print.h"
#include "main.h"
#include "paf.h"
#include "common.h"
#include "utils.h"
#include "network.h"
#include "downloader.h"
#include "settings.h"
#include "pages/text_page.h"
#include "dialog.h"

#define WIDE2(x) L##x
#define WIDE(x) WIDE2(x)

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

using namespace paf;

int loadFlags = 0;

ui::Widget *g_mainPage;
ui::Widget *g_errorPage;

Plugin *mainPlugin = SCE_NULL;

graph::Surface *BrokenTex = SCE_NULL;
graph::Surface *TransparentTex = SCE_NULL;

Downloader *g_downloader = SCE_NULL;

wchar_t *g_versionInfo = SCE_NULL;

void OnNetworkChecked()
{
    if(Network::GetCurrentStatus() == Network::Online)
    {
        
    }
    else 
    {
        string msgTemplate;
        Utils::GetfStringFromID("msg_net_fix", &msgTemplate);

        string errorMsg = ccc::Sprintf(msgTemplate.data(), Network::GetLastError());

        new text::Page(errorMsg.data());

        //generic::Page::SetBackButtonEvent(apps::Page::ErrorRetryCB, g_appsPage);
    }
}

SceVoid onPluginReady(Plugin *plugin)
{
    if(plugin == SCE_NULL)
    {
        print("[MAIN_BHBB] Error Plugin load failed!\n");
        return;
    }

    mainPlugin = plugin;

    rco::Element e = Utils::GetParamWithHashFromId("tex_missing_icon");
    mainPlugin->GetTexture(&BrokenTex, mainPlugin, &e);

	e.hash = Utils::GetHashById("_common_texture_transparent");
	Plugin::GetTexture(&TransparentTex, Plugin::Find("__system__common_resource"), &e);

    //Thanks Graphene
    auto infoString = new wstring;

#ifdef _DEBUG
    *infoString = L"Type: Private Beta\n";
#else
    *infoString = L"Type: Public Release\n";
#endif

    *infoString += L"Date: " WIDE(__DATE__) L"\n";
    *infoString += L"Version: 1.0\nBGDL Version: 2.0";

    print("%ls\n", infoString->data());

    g_versionInfo = (wchar_t *)infoString->data();

    sceShellUtilInitEvents(0);
    generic::Page::Setup();
    /*
    auto page = new generic::Page("blank_page_template");
    
    rco::Element e2;
    e.hash = Utils::GetHashById("byslidebar");
    e2.hash = Utils::GetHashById("_common_default_style_slidebar");

    ui::SlideBar *bar = (ui::SlideBar *)mainPlugin->CreateWidgetWithStyle(page->root, "slidebar", &e, &e2);
    ((int (*)(void *, int, int))(*(int *)(*((int *)(bar)) + 0x188)))(bar, 100, 0);

    Utils::SetWidgetSize(bar, 300, 50);
    Utils::SetWidgetPosition(bar, 0, 0);
    Utils::SetWidgetColor(bar, 1,1,1,1);
*/

    new Settings();
    new text::Page("Hello World!");
    Settings::GetInstance()->Open();
    Network::Init();

    g_downloader = new Downloader();
    
    Network::Check(OnNetworkChecked);

    sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);
}

int main()
{
    Utils::StartBGDL();
    Utils::InitMusic();
    Utils::SetMemoryInfo();

    Framework::InitParam fwParam;
    fwParam.LoadDefaultParams();
    fwParam.applicationMode = Framework::ApplicationMode::Mode_Application;
    
    fwParam.defaultSurfacePoolSize = 5 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_ICONS) fwParam.defaultSurfacePoolSize += 16 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_SCREENSHOTS) fwParam.defaultSurfacePoolSize += 5 * 1024 * 1024;
    fwParam.textSurfaceCacheSize = 2621440; //2.5MB

    Framework *fw = new Framework(fwParam);

    fw->LoadCommonResource();

    SceAppUtilInitParam init;
    SceAppUtilBootParam boot;

    //Can use sce_paf_... because paf is preloaded
    sce_paf_memset(&init, 0, sizeof(SceAppUtilInitParam));
    sce_paf_memset(&boot, 0, sizeof(SceAppUtilBootParam));
    
    sceAppUtilInit(&init, &boot);

    Plugin::InitParam piParam;

    piParam.pluginName = "bhbb_plugin";
    piParam.resourcePath = "app0:resource/bhbb_plugin.rco";
    piParam.scopeName = "__main__";

    piParam.pluginStartCB = onPluginReady;

    fw->LoadPluginAsync(&piParam);

    fw->EnterRenderingLoop();

    return sceKernelExitProcess(0);
}