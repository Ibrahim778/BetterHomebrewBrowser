#include <kernel.h>
#include <stdio.h>
#include <paf.h>
#include <shellsvc.h>
#include <libsysmodule.h>

#include "print.h"
#include "main.h"
#include "paf.h"
#include "common.h"
#include "utils.h"
#include "network.h"
#include "downloader.h"
#include "settings.h"
#include "pages/apps_page.h"
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

apps::Page *g_appsPage = SCE_NULL;

Plugin *mainPlugin = SCE_NULL;

graphics::Surface *BrokenTex = SCE_NULL;
graphics::Surface *TransparentTex = SCE_NULL;

Downloader *g_downloader = SCE_NULL;

SceWChar16 *g_versionInfo = SCE_NULL;

void OnNetworkChecked()
{
    if(Network::GetCurrentStatus() == Network::Online)
    {
        g_appsPage->Load();
    }
    else 
    {
        string msgTemplate;
        Utils::GetfStringFromID("msg_net_fix", &msgTemplate);

        string errorMsg;
        errorMsg.Setf(msgTemplate.data, Network::GetLastError());

        new text::Page(errorMsg.data);

        generic::Page::SetBackButtonEvent(apps::Page::ErrorRetryCB, g_appsPage);
    }
}

SceVoid onPluginReady(Plugin *plugin)
{
    if(plugin == NULL)
    {
        print("[MAIN_BHBB] Error Plugin load failed!\n");
        return;
    }

    mainPlugin = plugin;

    Resource::Element e = Utils::GetParamWithHashFromId("tex_missing_icon");
    mainPlugin->LoadTexture(&BrokenTex, mainPlugin, &e);

	e.hash = Utils::GetHashById("_common_texture_transparent");
	Plugin::LoadTexture(&TransparentTex, Plugin::Find("__system__common_resource"), &e);

    //Thanks Graphene
    auto infoString = new wstring;

#ifdef _DEBUG
    *infoString = L"Type: Private Beta\n";
#else
    *infoString = L"Type: Public Release\n";
#endif

    *infoString += L"Date: " WIDE(__DATE__) L"\n";
    *infoString += L"Version: 1.0\nBGDL Version: 2.0";

    print("%ls\n", infoString->data);

    g_versionInfo = (SceWChar16 *)infoString->data;

    sceShellUtilInitEvents(0);
    
    generic::Page::Setup();

    new Settings();
    g_appsPage = new apps::Page();

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

    Framework *fw = new Framework(&fwParam);

    fw->LoadCommonResource();

    SceAppUtilInitParam init;
    SceAppUtilBootParam boot;

    //Can use sce_paf_... because paf is now loaded
    sce_paf_memset(&init, 0, sizeof(SceAppUtilInitParam));
    sce_paf_memset(&boot, 0, sizeof(SceAppUtilBootParam));
    
    sceAppUtilInit(&init, &boot);

    Framework::PluginInitParam piParam;

    piParam.pluginName = "bhbb_plugin";
    piParam.resourcePath = "app0:resource/bhbb_plugin.rco";
    piParam.scopeName = "__main__";

    piParam.pluginStartCB = onPluginReady;

    fw->LoadPluginAsync(&piParam);

    fw->EnterRenderingLoop();

    return sceKernelExitProcess(0);
}