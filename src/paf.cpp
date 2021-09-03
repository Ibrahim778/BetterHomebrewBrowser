#include "paf_internal.hpp"
#include "common.hpp"
#include "paf.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "eventhandler.hpp"
#include "audiomgr.hpp"
#include "pagemgr.hpp"

SceUID main_thread = SCE_UID_INVALID_UID;

Plugin *mainPlugin = SCE_NULL;
Plane *mainRoot = SCE_NULL;

Widget *mainScene = SCE_NULL;

CornerButton *mainBackButton = SCE_NULL;
CornerButton *settingsButton = SCE_NULL;
Allocator *fwAllocator = SCE_NULL;

graphics::Texture *BrokenTex = SCE_NULL;

void initPaf()
{
    SceInt32 res = -1, load_res;

    ScePafInit initParam;
    SceSysmoduleOpt opt;

    initParam.global_heap_size = 0x1600000;

	initParam.a2 = 0x0000EA60;
	initParam.a3 = 0x00040000;

    initParam.use_gxm = SCE_FALSE;

    initParam.heap_opt_param1 = 0;
    initParam.heap_opt_param2 = 0;

    //Specify that we will pass some arguments
    opt.flags = 0;
    opt.result = &load_res;

    res = _sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(initParam), &initParam, &opt);

    if(res < 0 || load_res < 0)
    {
        LOG_ERROR("INIT_PAF", res);
        LOG_ERROR("INIT_PAF", load_res);
    }

}

void getDefaultWidgets()
{
    Plugin::SceneInitParam sinit;
    Resource::Element search = Utils::GetParamWithHashFromId(SELECTION_PAGE_ID);
    mainScene = mainPlugin->CreateScene(&search, &sinit);

    mainRoot = (Plane *)Utils::GetChildByHash(mainScene, Utils::GetHashById(MAIN_PLANE_ID));
    mainBackButton = (CornerButton *)Utils::GetChildByHash(mainScene, Utils::GetHashById(BACK_BUTTON_ID));

    settingsButton = (CornerButton *)Utils::GetChildByHash(mainScene, Utils::GetHashById(SETTINGS_BUTTON_ID));

    PopupMgr::initDialog();

    BrokenTex = new graphics::Texture();
    Resource::Element e = Utils::GetParamWithHashFromId(ICON_MISSING_TEX_ID);
    mainPlugin->LoadTexture(BrokenTex, mainPlugin, &e);

}

SceVoid onPluginReady(Plugin *plugin)
{
    if(plugin == NULL)
    {
        printf("[MAIN_BHBB] Error Plugin load failed!\n");
        return;
    }

    mainPlugin = plugin;

    getDefaultWidgets();

    onReady();
}

void initPlugin()
{
    Framework::InitParam fwParam;
    fwParam.LoadDefaultParams();
    fwParam.applicationMode = Framework::Mode_ApplicationDefault;
    
    fwParam.defaultSurfacePoolSize = 17 * 1024 * 1024;
    fwParam.textSurfaceCacheSize = 8 * 1024 * 1024;

    Framework * fw = new Framework(&fwParam);

    fw->LoadCommonResourceAsync();
    fwAllocator = fw->defaultAllocator;
	SceAppUtilInitParam init;
	SceAppUtilBootParam boot;
    //Can use sce_paf_... because paf is now loaded
	sce_paf_memset(&init, 0, sizeof(SceAppUtilInitParam));
	sce_paf_memset(&boot, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init, &boot);

    main_thread = sceKernelGetThreadId();

    Framework::PluginInitParam piParam;

    piParam.pluginName.Set(PLUGIN_NAME);
    piParam.resourcePath.Set(RESOURCE_PATH);
    piParam.scopeName.Set("__main__");

    piParam.loadCB3 = onPluginReady;

    fw->LoadPluginAsync(&piParam);

    fw->EnterRenderingLoop();
}

