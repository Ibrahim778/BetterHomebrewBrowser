#include <kernel.h>
#include <paf.h>
#include <shellsvc.h>
#include <libsysmodule.h>
#include <apputil.h>
#include <taihen.h>
#include <net.h>
#include <libperf.h>
#include <libnetctl.h>
#include <vshbridge.h>

#include "print.h"
#include "utils.h"
#include "common.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"
#include "pages/app_browser.h"
#include "pages/textf_page.h"
#include "db/vitadb.h"

#define NET_HEAP_SIZE  (2 * 1024 * 1024)
#define HTTP_HEAP_SIZE (2 * 1024 * 1024)
#define SSL_HEAP_SIZE  (2 * 1024 * 1024)

extern "C" {
    const char			sceUserMainThreadName[] = "BHBB_MAIN";
    const int			sceUserMainThreadPriority = SCE_KERNEL_DEFAULT_PRIORITY_USER;
    const unsigned int	sceUserMainThreadStackSize = SCE_KERNEL_THREAD_STACK_SIZE_DEFAULT_USER_MAIN;

    SceUID _vshKernelSearchModuleByName(const char *name, SceUInt64 *unk);
    int curl_global_memmanager_set_np(void *(*allocate)(size_t), void (*deallocate)(void *), void *(*reallocate)(void *, size_t));
}

using namespace paf;

Plugin *g_appPlugin = SCE_NULL;

intrusive_ptr<graph::Surface> g_brokenTex;
intrusive_ptr<graph::Surface> g_transparentTex;

SceVoid PluginStart(Plugin *plugin)
{
    if(plugin == SCE_NULL)
    {
        print("[bhbb_plugin] Error Plugin load failed!\n");
        return;
    }

    g_appPlugin = plugin;
    print("[bhbb_plugin] Create success! %p\n", plugin);

    SceUInt64 buff = 0;
    SceUID itlsID = _vshKernelSearchModuleByName("itlsKernel", &buff);

    print("iTLS-Enso: 0x%X\n", itlsID);
    if(itlsID < 0)
    {
        new page::TextfPage(msg_no_itls);
        return;
    }

    IDParam param(tex_missing_icon);
    
    g_brokenTex = g_appPlugin->GetTexture(param);
    g_brokenTex->AddRef(); //Prevent Deletion

    param = "_common_texture_transparent";

	g_transparentTex = Plugin::Find("__system__common_resource")->GetTexture(param);
    g_transparentTex->AddRef(); //Prevent Deletion

    sceShellUtilInitEvents(0);
    
    job::JobQueue::Option opt;
    opt.workerNum = 1;
    opt.workerStackSize = SCE_KERNEL_256KiB;
    
    job::JobQueue::Init(&opt);

    SceNetInitParam netInitParam;
	netInitParam.memory = sce_paf_malloc(NET_HEAP_SIZE);
	netInitParam.size = NET_HEAP_SIZE;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);

	sceNetCtlInit();

    /* HTTPS */
	sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
	sceHttpInit(HTTP_HEAP_SIZE);
	sceSslInit(SSL_HEAP_SIZE);
    
    auto page = new AppBrowser(new VitaDB());
    page->Load();
}

int main()
{
#if defined(SCE_PAF_TOOL_PRX) && defined(_DEBUG) && !defined(__INTELLISENSE__)
    //This line will break things if using non devkit libpaf
    SCE_PAF_AUTO_TEST_SET_EXTRA_TTY(sceIoOpen("tty0:", SCE_O_WRONLY, 0)); 
#endif

    Utils::StartBGDL(); // Init bhbb_dl
    Utils::InitMusic(); // BG Music

	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_COMMON_GUI_DIALOG);

    new Module("app0:module/libcurl.suprx", "libcurl");
    curl_global_memmanager_set_np(sce_paf_malloc, sce_paf_free, sce_paf_realloc);

    Framework::InitParam fwParam;

    Framework::SampleInit(&fwParam);
    fwParam.mode = Framework::Mode_Application;
    
    fwParam.surface_pool_size = 16 * 1024 * 1024;
    fwParam.text_surface_pool_size = 2621440; //2.5MB

    Framework *fw = new Framework(fwParam);

    fw->LoadCommonResourceSync();

    SceAppUtilInitParam init;
    SceAppUtilBootParam boot;

    //Can use sce_paf_... because paf is preloaded
    sce_paf_memset(&init, 0, sizeof(SceAppUtilInitParam));
    sce_paf_memset(&boot, 0, sizeof(SceAppUtilBootParam));
    
    sceAppUtilInit(&init, &boot);

    Plugin::InitParam piParam;

    piParam.name = "bhbb_plugin";
    piParam.resource_file = "app0:resource/bhbb_plugin.rco";
    piParam.caller_name = "__main__";

#if defined(SCE_PAF_TOOL_PRX) && defined(_DEBUG)
    //This line will break things if using non devkit libpaf
    // piParam.option = Plugin::InitParam::PluginFlag_UseRcdDebug;
#endif

    piParam.start_func = PluginStart;

    Plugin::LoadAsync(piParam);

    // if(vshSblAimgrIsTool())
    //     sceRazorCpuStartCapture();
    
    fw->Run();

    return sceKernelExitProcess(0);
}