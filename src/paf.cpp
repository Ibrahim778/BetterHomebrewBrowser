#include "paf_internal.hpp"
#include "common.hpp"
#include "paf.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "eventhandler.hpp"
#include "audiomgr.hpp"
#include "pagemgr.hpp"
#include "configmgr.hpp"

SceUID main_thread = SCE_UID_INVALID_UID;

Plugin *mainPlugin = SCE_NULL;
Plane *mainRoot = SCE_NULL;

Widget *mainScene = SCE_NULL;

CornerButton *mainBackButton = SCE_NULL;
CornerButton *settingsButton = SCE_NULL;

CornerButton *forwardButton = SCE_NULL;

Allocator *fwAllocator = SCE_NULL;

graphics::Texture *BrokenTex = SCE_NULL;

void initPaf()
{
    print("Init paf\n");
    SceInt32 res = -1, load_res;

    ScePafInit initParam;
    SceSysmoduleOpt opt;

    initParam.global_heap_size = 5 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_ICONS)
        initParam.global_heap_size += 5 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_SCREENSHOTS)
        initParam.global_heap_size += 2 * 1024 * 1024;

    initParam.a2 = 0x0000EA60;
    initParam.a3 = 0x00040000;

    initParam.cdlg_mode = SCE_FALSE;

    initParam.heap_opt_param1 = 0;
    initParam.heap_opt_param2 = 0;

    //Specify that we will pass some arguments
    opt.flags = 0;
    opt.result = &load_res;

    print("Set Vars\n");
    print("Loading....");
    res = _sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(initParam), &initParam, &opt);
    print("Done!\n");

    if(res < 0 || load_res < 0)
    {
        print("Error!\n");
        LOG_ERROR("INIT_PAF", res);
        LOG_ERROR("INIT_PAF", load_res);
    }

    print("Done initPaf()\n");
}

void getDefaultWidgets()
{
    print("Getting defaults..\n");
    Plugin::SceneInitParam sinit;
    Resource::Element search = Utils::GetParamWithHashFromId(MAIN_PAGE_ID);
    mainScene = mainPlugin->CreateScene(&search, &sinit);

    if(mainScene == NULL) sceKernelExitProcess(0);
    print("mainScene = 0x%X\n");
    mainRoot = (Plane *)Utils::GetChildByHash(mainScene, Utils::GetHashById(MAIN_PLANE_ID));
    mainBackButton = (CornerButton *)Utils::GetChildByHash(mainScene, Utils::GetHashById(BACK_BUTTON_ID));

    mainBackButtonEvent = new BackButtonEventHandler();
    mainForwardButtonEvent = new ForwardButtonEventHandler();

    forwardButton = (CornerButton *)Utils::GetChildByHash(mainScene, Utils::GetHashById(FORWARD_BUTTON_ID));
    settingsButton = (CornerButton *)Utils::GetChildByHash(mainScene, Utils::GetHashById(SETTINGS_BUTTON_ID));

    PopupMgr::InitDialog();

    BrokenTex = new graphics::Texture();
    Resource::Element e = Utils::GetParamWithHashFromId(ICON_MISSING_TEX_ID);
    mainPlugin->LoadTexture(BrokenTex, mainPlugin, &e);

}

SceVoid onPluginReady(Plugin *plugin)
{
    if(plugin == NULL)
    {
        print("[MAIN_BHBB] Error Plugin load failed!\n");
        return;
    }

    mainPlugin = plugin;

    getDefaultWidgets();

    onReady();
}

void initPlugin()
{
    print("Init Plugin\n");
    Framework::InitParam fwParam;
    fwParam.LoadDefaultParams();
    fwParam.applicationMode = Framework::ApplicationMode::Mode_Application;
    
    fwParam.defaultSurfacePoolSize = 5 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_ICONS) fwParam.defaultSurfacePoolSize += 6 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_SCREENSHOTS) fwParam.defaultSurfacePoolSize += 5 * 1024 * 1024;
    fwParam.textSurfaceCacheSize = 2621440; //2.5MB

    print("Set Vars\n");
    print("Making fw\n");

    Framework * fw = new Framework(&fwParam);
    print("Loading Common Resource!\n");

    fw->LoadCommonResource();
    print("Done!\n");

    fwAllocator = fw->defaultAllocator;
    SceAppUtilInitParam init;
    SceAppUtilBootParam boot;

    //Can use sce_paf_... because paf is now loaded
    sce_paf_memset(&init, 0, sizeof(SceAppUtilInitParam));
    sce_paf_memset(&boot, 0, sizeof(SceAppUtilBootParam));
    
    print("Set Vars\n");
    sceAppUtilInit(&init, &boot);

    print("Done!\n");

    main_thread = sceKernelGetThreadId();
    print("main_thread = 0x%X\n", main_thread);

    Framework::PluginInitParam piParam;
    print("Made Init Param\n");

    piParam.pluginName = PLUGIN_NAME;
    piParam.resourcePath = RESOURCE_PATH;
    piParam.scopeName = "__main__";

    piParam.pluginStartCB = onPluginReady;
    print("Set Vars\n");
    print("Loading Plugin!\n");
    fw->LoadPluginAsync(&piParam);
    print("Done");
    print("Entering Rendering Loop\n");
    fw->EnterRenderingLoop();
    print("Done InitPlugin()!\n");
}