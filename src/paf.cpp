#include <appmgr.h>
#include <kernel.h>
#include <paf.h>

#include "paf.hpp"
#include "parser.hpp"
#include "common.hpp"
#include "main.hpp"
#include "utils.hpp"

paf::Plugin *mainPlugin = SCE_NULL;

paf::graphics::Surface *BrokenTex = SCE_NULL;
paf::graphics::Surface *TransparentTex = SCE_NULL;

void getDefaultWidgets()
{
    paf::Resource::Element e = Utils::GetParamWithHashFromId(ICON_MISSING_TEX_ID);
    mainPlugin->LoadTexture(&BrokenTex, mainPlugin, &e);

	e.hash = Utils::GetHashById("_common_texture_transparent");
	paf::Plugin::LoadTexture(&TransparentTex, paf::Plugin::Find("__system__common_resource"), &e);
}

SceVoid onPluginReady(paf::Plugin *plugin)
{
    if(plugin == NULL)
    {
        print("[MAIN_BHBB] Error Plugin load failed!\n");
        return;
    }

    mainPlugin = plugin;

    getDefaultWidgets();

    OnReady();
}

void initPlugin()
{
    paf::Framework::InitParam fwParam;
    fwParam.LoadDefaultParams();
    fwParam.applicationMode = paf::Framework::ApplicationMode::Mode_Application;
    
    fwParam.defaultSurfacePoolSize = 5 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_ICONS) fwParam.defaultSurfacePoolSize += 16 * 1024 * 1024;
    if(loadFlags & LOAD_FLAGS_SCREENSHOTS) fwParam.defaultSurfacePoolSize += 5 * 1024 * 1024;
    fwParam.textSurfaceCacheSize = 2621440; //2.5MB

    paf::Framework *fw = new paf::Framework(&fwParam);

    fw->LoadCommonResource();

    SceAppUtilInitParam init;
    SceAppUtilBootParam boot;

    //Can use sce_paf_... because paf is now loaded
    sce_paf_memset(&init, 0, sizeof(SceAppUtilInitParam));
    sce_paf_memset(&boot, 0, sizeof(SceAppUtilBootParam));
    
    sceAppUtilInit(&init, &boot);

    paf::Framework::PluginInitParam piParam;

    piParam.pluginName = PLUGIN_NAME;
    piParam.resourcePath = RESOURCE_PATH;
    piParam.scopeName = "__main__";

    piParam.pluginStartCB = onPluginReady;

    fw->LoadPluginAsync(&piParam);

    fw->EnterRenderingLoop();
}