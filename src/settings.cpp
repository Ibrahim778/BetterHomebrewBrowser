#include <kernel.h>
#include <libsysmodule.h>
#include <taihen.h>

#include "settings.h"
#include "common.h"
#include "utils.h"
#include "print.h"
#include "db.h"
#include "pages/text_page.h"

using namespace paf;
using namespace sce;

static Settings *currentSettingsInstance = SCE_NULL;
sce::AppSettings *Settings::appSettings = SCE_NULL;

Settings::Settings()
{
	if (currentSettingsInstance != SCE_NULL)
	{
		print("Error another settings instance exists! ABORT!\n");
		sceKernelExitProcess(0);
	}

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

	pInit.pluginCreateCB = AppSettings::PluginCreateCB;
	pInit.pluginInitCB = AppSettings::PluginInitCB;
	pInit.pluginStartCB = AppSettings::PluginStartCB;
	pInit.pluginStopCB = AppSettings::PluginStopCB;
	pInit.pluginExitCB = AppSettings::PluginExitCB;
	pInit.pluginPath = "vs0:vsh/common/app_settings.suprx";
	pInit.unk_58 = 0x96;

	Framework::GetInstance()->LoadPlugin(&pInit);
    
	sInit.xmlFile = mainPlugin->resource->GetFile(Utils::GetHashById("file_bhbb_settings"), &fileSize, &mimeType);

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
        appSettings->SetInt("source", d_source);
        appSettings->SetInt("nLoad", d_nLoad);
	}

    //Get values
    appSettings->GetInt("source", (int *)&source, d_source);
    appSettings->GetInt("nLoad", &nLoad, d_nLoad);

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
    ui::Widget *root = plugin->GetPageByHash(&e);
    if(!root) return;
    e.hash = 0x8211F03F;
    ui::Widget *exitButton = root->GetChild(&e, 0);
    if(!exitButton) return;
    exitButton->SendEvent(ui::EventMain::EventMain_Decide, 0);
}

SceVoid Settings::Open()
{
    // g_appsPage->root->SetAlpha(0.39f);
    // g_appsPage->root->PlayEffectReverse(10, effect::EffectType_Fadein1);

	AppSettings::InterfaceCallbacks ifCb;

	ifCb.listChangeCb = CBListChange;
	ifCb.listForwardChangeCb = CBListForwardChange;
	ifCb.listBackChangeCb = CBListBackChange;
	ifCb.isVisibleCb = CBIsVisible;
	ifCb.elemInitCb = CBElemInit;
	ifCb.elemAddCb = CBElemAdd;
	ifCb.valueChangeCb = CBValueChange;
	ifCb.valueChangeCb2 = CBValueChange2;
	ifCb.termCb = CBTerm;
	ifCb.getStringCb = CBGetString;
	ifCb.getTexCb = CBGetTex;

	Plugin *appSetPlug = paf::Plugin::Find("app_settings_plugin");
	AppSettings::Interface *appSetIf = (sce::AppSettings::Interface *)appSetPlug->GetInterface(1);

	appSetIf->Show(&ifCb);
}

SceVoid Settings::OpenCB(SceInt32, ui::Widget *, SceInt32, ScePVoid)
{
    Settings::GetInstance()->Open();
}

SceVoid Settings::CBListChange(const char *elementId)
{

}

SceVoid Settings::CBListForwardChange(const char *elementId)
{

}

SceVoid Settings::CBListBackChange(const char *elementId)
{

}

SceInt32 Settings::CBIsVisible(const char *elementId, SceBool *pIsVisible)
{
	*pIsVisible = SCE_TRUE;

	return SCE_OK;
}

SceInt32 Settings::CBElemInit(const char *elementId)
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
	SceUInt32 elementHash = Utils::GetHashById(elementId);
	SceInt64 value = sce_paf_strtol(newValue, NULL, 10);
	print("Element with id: %s (0x%X) called CBValueChange! newValue = %s\n", elementId, elementHash, newValue);

	switch (elementHash)
	{
    
    case Hash_nLoad:
        GetInstance()->nLoad = value;
        // g_appsPage->Redisplay();
        break;

    case Hash_Source:
        GetInstance()->source = (db::Id)value;
    
    case Hash_Refresh:
        // g_appsPage->SetCategory(-1);
        // g_appsPage->Load();
        GetInstance()->Close();
        break;

	default:
		break;
	}

	return ret;
}

SceInt32 Settings::CBValueChange2(const char *elementId, const char *newValue)
{
	return SCE_OK;
}

SceVoid Settings::CBTerm()
{    
    // g_appsPage->root->PlayEffect(-100, effect::EffectType_Fadein1);
    // g_appsPage->root->SetAlpha(1.0f);
}

wchar_t *Settings::CBGetString(const char *elementId)
{
    if(sce_paf_strncmp(elementId, "msg_version_info", 16) == 0)
        return g_versionInfo;

    rco::Element searchParam;
    searchParam.hash = Utils::GetHashById(elementId);

    return mainPlugin->GetWString(&searchParam);
}

SceInt32 Settings::CBGetTex(graph::Surface **tex, const char *elementId)
{
	return SCE_OK;
}

