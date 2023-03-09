#include <kernel.h>
#include <stdio.h>
#include <paf.h>
#include <shellsvc.h>
#include <libsysmodule.h>
#include <apputil.h>
#include <taihen.h>

#include "print.h"
#include "common.h"
#include "utils.h"
#include "network.h"
#include "downloader.h"
#include "settings.h"
#include "pages/text_page.h"
#include "dialog.h"
#include "pages/apps_page.h"
#include "pages/apps_info_page.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"

extern "C" {
    const char			sceUserMainThreadName[] = "BHBB_MAIN";
    const int			sceUserMainThreadPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    const unsigned int	sceUserMainThreadStackSize = SCE_KERNEL_THREAD_STACK_SIZE_DEFAULT_USER_MAIN;
    unsigned int        sceLibcHeapSize = 0x180000;

    SceUID _vshKernelSearchModuleByName(const char *name, SceUInt64 *unk);
}

using namespace paf;
using namespace Utils;

Plugin *g_appPlugin              = SCE_NULL;

graph::Surface *g_brokenTex       = SCE_NULL;
graph::Surface *g_transparentTex  = SCE_NULL;

Downloader *g_downloader        = SCE_NULL;
apps::Page *g_appsPage          = SCE_NULL;

job::JobQueue *g_mainQueue      = SCE_NULL;

void OnNetworkChecked()
{
    if(Network::GetCurrentStatus() == Network::Online)
    {
        g_appsPage->Load();
    }
    else 
    {
        string msgTemplate;
        str::GetfFromHash(msg_net_fix, &msgTemplate);
        string errorMsg;
        common::string_util::setf(errorMsg, msgTemplate.data(), Network::GetLastError());
        
        new text::Page(errorMsg.data());

        // generic::Page::SetBackButtonEvent(apps::Page::ErrorRetryCB, g_appsPage); TODO: FIX
    }
}

SceVoid onPluginReady(Plugin *plugin)
{
    if(plugin == SCE_NULL)
    {
        print("[MAIN_BHBB] Error Plugin load failed!\n");
        return;
    }

    g_appPlugin = plugin;
    
    rco::Element e; 
    e.hash = tex_missing_icon;
    g_appPlugin->GetTexture(&g_brokenTex, g_appPlugin, &e);
    g_brokenTex->AddRef(); //Prevent Deletion

	e.hash = Misc::GetHash("_common_texture_transparent");
	Plugin::GetTexture(&g_transparentTex, Plugin::Find("__system__common_resource"), &e);
    g_transparentTex->AddRef(); //Prevent Deletion

    sceShellUtilInitEvents(0);

    SceUInt64 unk = 0;
    SceUID itlsID = _vshKernelSearchModuleByName("itlsKernel", &unk);

    print("iTLS-Enso: 0x%X\n", itlsID);
    if(itlsID < 0)
    {
        string err;
        str::GetfFromHash(msg_no_itls, &err);
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
#if defined(SCE_PAF_TOOL_PRX) && defined(_DEBUG) && !defined(__INTELLISENSE__)
    SCE_PAF_AUTO_TEST_SET_EXTRA_TTY(sceIoOpen("tty0:", SCE_O_WRONLY, 0)); //This line will break things if using non devkit libpaf
#endif

    Misc::StartBGDL(); // Init bhbb_dl
    Misc::InitMusic(); // BG Music

	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

    new Module("app0:module/libcurl.suprx");
    
    Framework::InitParam fwParam;

    fwParam.LoadDefaultParams();
    fwParam.applicationMode = Framework::ApplicationMode::Mode_Application;
    
    fwParam.defaultSurfacePoolSize = 16 * 1024 * 1024;
    fwParam.textSurfaceCacheSize = 2621440; //2.5MB

    Framework *fw = new Framework(fwParam);

    fw->LoadCommonResourceSync();

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
#if defined(SCE_PAF_TOOL_PRX) && defined(_DEBUG)
    piParam.pluginFlags = Plugin::InitParam::PluginFlag_UseRcdDebug; //This line will break things if using non devkit libpaf
#endif
    piParam.pluginStartCB = onPluginReady;

    fw->LoadPluginAsync(piParam);

    fw->Run();

    return sceKernelExitProcess(0);
}