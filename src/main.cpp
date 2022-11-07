#include <kernel.h>
#include <stdio.h>
#include <paf.h>
#include <shellsvc.h>
#include <libsysmodule.h>
#include <apputil.h>
#include <taihen.h>

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
#include "pages/apps_page.h"

#define WIDE2(x) L##x
#define WIDE(x) WIDE2(x)

extern "C" {

    SCE_USER_MODULE_LIST("app0:module/libScePafPreload.suprx");

    extern const char			sceUserMainThreadName[] = "BHBB_MAIN";
    extern const int			sceUserMainThreadPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    extern const unsigned int	sceUserMainThreadStackSize = SCE_KERNEL_THREAD_STACK_SIZE_DEFAULT_USER_MAIN;
    extern unsigned int         sceLibcHeapSize = 0x180000;

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

    SceUID _vshKernelSearchModuleByName(const char *name, SceUInt64 *unk);
}

using namespace paf;

Plugin *mainPlugin = SCE_NULL;

graph::Surface *BrokenTex = SCE_NULL;
graph::Surface *TransparentTex = SCE_NULL;

Downloader *g_downloader = SCE_NULL;
apps::Page *g_appsPage = SCE_NULL;

wchar_t *g_versionInfo = SCE_NULL;

job::JobQueue *g_mainQueue = SCE_NULL;

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

        string errorMsg = ccc::Sprintf(msgTemplate.data(), Network::GetLastError());

        new text::Page(errorMsg.data());

        generic::Page::SetBackButtonEvent(apps::Page::ErrorRetryCB, g_appsPage);
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

    rco::Element e; 
    e.hash = Utils::GetHashById("tex_missing_icon");
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
    *infoString += L"Version: 1.0\nBGDL Version: 2.0\ncBGDL Version: 1.0";

    print("%ls\n", infoString->data());

    g_versionInfo = (wchar_t *)infoString->data();

    sceShellUtilInitEvents(0);
    generic::Page::Setup();

    SceUInt64 unk = 0;
    SceUID itlsID = _vshKernelSearchModuleByName("itlsKernel", &unk);

    print("iTLS-Enso: 0x%X\n", itlsID);
    if(itlsID < 0)
    {
        string err;
        Utils::GetfStringFromID("msg_no_itls", &err);
        new text::Page(err.data());
        return;
    }

    new Settings();
    
    Network::Init();
    
    job::JobQueue::Option mainOpt;
    mainOpt.workerNum = 1;
    mainOpt.workerOpt = SCE_NULL;
    mainOpt.workerPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER + 5;
    mainOpt.workerStackSize = SCE_KERNEL_16KiB;

    g_mainQueue = new job::JobQueue("BHBB::MainQueue", &mainOpt);
    g_downloader = new Downloader();
    g_appsPage = new apps::Page();

    Network::Check(OnNetworkChecked);

    sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);
}

int main()
{
#ifdef _DEBUG
    SCE_PAF_AUTO_TEST_SET_EXTRA_TTY(sceIoOpen("tty0:", SCE_O_WRONLY, 0));
#endif

    Utils::StartBGDL();
    Utils::InitMusic();

    Framework::InitParam fwParam;
    fwParam.LoadDefaultParams();
    fwParam.applicationMode = Framework::ApplicationMode::Mode_Application;
    
    fwParam.defaultSurfacePoolSize = 16 * 1024 * 1024;
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
#ifdef _DEBUG
    piParam.pluginFlags = Plugin::InitParam::PluginFlag_UseRcdDebug;
#endif
    piParam.pluginStartCB = onPluginReady;

    fw->LoadPluginAsync(&piParam);

    fw->EnterRenderingLoop();

    return sceKernelExitProcess(0);
}