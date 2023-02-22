#include <kernel.h>
#include <libsysmodule.h>
#include <taihen.h>

#include "settings.h"
#include "common.h"
#include "utils.h"
#include "print.h"
#include "db.h"
#include "pages/text_page.h"
#include "bhbb_plugin.h"
#include "bhbb_settings.h"

#define WIDE2(x) L##x
#define WIDE(x) WIDE2(x)

using namespace paf;
using namespace sce;
using namespace Utils;

static Settings *currentSettingsInstance = SCE_NULL;
sce::AppSettings *Settings::appSettings = SCE_NULL;
static wchar_t *s_versionInfo = SCE_NULL;

Settings::Settings()
{
	if (currentSettingsInstance != SCE_NULL)
	{
		print("Error another settings instance exists! ABORT!\n");
		sceKernelExitProcess(0);
	}


    //Thanks Graphene
    auto infoString = new wstring;

    #ifdef _DEBUG
        *infoString = L"Private Beta\n";
    #else
        *infoString = L"Public Release\n";
    #endif

    *infoString += WIDE(__DATE__) L"\n";
    *infoString += L"Version: 1.1\nBGDL Version: 2.1\ncBGDL Version: 1.1";

    print("%ls\n", infoString->data());

    s_versionInfo = (wchar_t *)infoString->data();

	SceInt32 ret = 0;
    SceSize fileSize = 0;
    const char *mimeType = SCE_NULL;
	Plugin::InitParam pInit;
	AppSettings::InitParam sInit;

	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BXCE);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_INI_FILE_PROCESSOR);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_COMMON_GUI_DIALOG);

	pInit.pluginName = "app_settings_plugin";
	pInit.resourcePath = "vs0:vsh/common/app_settings_plugin.rco";
	pInit.scopeName = "__main__";

	pInit.pluginSetParamCB = AppSettings::PluginCreateCB;
	pInit.pluginInitCB = AppSettings::PluginInitCB;
	pInit.pluginStartCB = AppSettings::PluginStartCB;
	pInit.pluginStopCB = AppSettings::PluginStopCB;
	pInit.pluginExitCB = AppSettings::PluginExitCB;
	pInit.pluginPath = "vs0:vsh/common/app_settings.suprx";
	pInit.unk_58 = 0x96;

	Plugin::LoadSync(pInit);
    
	sInit.xmlFile = g_appPlugin->resource->GetFile(file_bhbb_settings, &fileSize, &mimeType);

	sInit.allocCB = sce_paf_malloc;
	sInit.freeCB = sce_paf_free;
	sInit.reallocCB = sce_paf_realloc;
	sInit.safeMemoryOffset = 0;
	sInit.safeMemorySize = 0x400;
    
	AppSettings::GetInstance(&sInit, &appSettings);


	ret = -1;
	appSettings->GetInt("settings_version", &ret, 0);
	if (ret != d_settingsVersion) //Need to setup default values
	{
        ret = appSettings->Initialize();

        appSettings->SetInt("settings_version", d_settingsVersion);
        appSettings->SetInt("downloadInterval", d_downloadInterval);
        appSettings->SetInt("source", db::VHBDB);
	}
    
    //Get values
    appSettings->GetInt("source", (int *)&source, d_source);
    appSettings->GetInt("downloadInterval", &downloadInterval, d_downloadInterval);

	currentSettingsInstance = this;
}

Settings::~Settings()
{
	print("Not allowed! ABORT!\n");
	sceKernelExitProcess(0);
}

Settings *Settings::GetInstance()
{
	return currentSettingsInstance;
}

sce::AppSettings *Settings::GetAppSettings()
{
    return appSettings;
}

SceVoid Settings::Close()
{
    rco::Element e;
    e.hash = 0xF6C9D4C0;

    auto plugin = paf::Plugin::Find("app_settings_plugin");
    if(!plugin) return;
    ui::Widget *root = plugin->GetPageByHash(&e);
    if(!root) return;
    e.hash = 0x8211F03F;
    ui::Widget *exitButton = root->GetChild(&e, 0);
    if(!exitButton) return;
    exitButton->SendEvent(ui::EventMain::EventMain_Decide, 0);
}

SceVoid Settings::Open()
{
    ui::Widget::SetControlFlags(g_appsPage->root, 0);
    g_appsPage->root->PlayEffectReverse(10, effect::EffectType_Fadein1);

	AppSettings::InterfaceCallbacks ifCb;

	ifCb.onStartPageTransitionCb = CBListChange;
	ifCb.onPageActivateCb = CBListForwardChange;
	ifCb.onPageDeactivateCb = CBListBackChange;
	ifCb.onCheckVisible = CBIsVisible;
	ifCb.onPostCreateCb = CBElemAdd;
	ifCb.onPreCreateCb = CBElemInit;
	ifCb.onPressCb = CBValueChange;
	ifCb.onPressCb2 = CBValueChange2;
	ifCb.onTermCb = CBTerm;
	ifCb.onGetStringCb = CBGetString;
	ifCb.onGetSurfaceCb = CBGetTex;

	Plugin *appSetPlug = paf::Plugin::Find("app_settings_plugin");
	AppSettings::Interface *appSetIf = (sce::AppSettings::Interface *)appSetPlug->GetInterface(1);

	appSetIf->Show(&ifCb);
}

Settings::OpenCallback::OpenCallback()
{
    eventHandler = OnGet;
}

SceVoid Settings::OpenCallback::OnGet(SceInt32, ui::Widget *, SceInt32, ScePVoid)
{
    Settings::GetInstance()->Open();
}

SceVoid Settings::CBListChange(const char *elementId, SceInt32 type)
{

}

SceVoid Settings::CBListForwardChange(const char *elementId, SceInt32 type)
{

}

SceVoid Settings::CBListBackChange(const char *elementId, SceInt32 type)
{

}

SceInt32 Settings::CBIsVisible(const char *elementId, SceBool *pIsVisible)
{
	*pIsVisible = SCE_TRUE;

	return SCE_OK;
}

SceInt32 Settings::CBElemInit(const char *elementId, AppSettings::Element *element)
{
	return SCE_OK;
}

SceInt32 Settings::CBElemAdd(const char *elementId, paf::ui::Widget *widget)
{
	return SCE_OK;
}

SceInt32 Settings::CBValueChange(const char *elementId, const char *newValue)
{
	SceInt32 ret = SCE_OK;
	SceUInt32 elementHash = Misc::GetHash(elementId);
	SceInt64 value = sce_paf_strtol(newValue, NULL, 10);

	switch (elementHash)
	{   
    case list_source:
        GetInstance()->source = (db::Id)value;
        g_appsPage->Load();
        break;
    
    case list_downloadInterval:
        GetInstance()->downloadInterval = value;
        break;

	default:
	    print("Element with id: %s (0x%X) called CBValueChange! newValue = %s\n", elementId, elementHash, newValue);
		break;
	}

	return ret;
}

SceInt32 Settings::CBValueChange2(const char *elementId, const char *newValue)
{
	return SCE_OK;
}

SceVoid Settings::CBTerm(SceInt32 result)
{    
    g_appsPage->root->PlayEffect(-100, effect::EffectType_Fadein1);
    ui::Widget::SetControlFlags(g_appsPage->root, 1);
}

wchar_t *Settings::CBGetString(const char *elementId)
{
    if(sce_paf_strncmp(elementId, "msg_version_info", 16) == 0)
        return s_versionInfo;

    rco::Element searchParam;
    searchParam.hash = Misc::GetHash(elementId);

    return g_appPlugin->GetWString(&searchParam);
}

SceInt32 Settings::CBGetTex(graph::Surface **tex, const char *elementId)
{
	return SCE_OK;
}

