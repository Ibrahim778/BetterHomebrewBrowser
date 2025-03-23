/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2024 Muhammad Ibrahim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/

#include <paf.h>
#include <libsysmodule.h>
#include <taihen.h>

#include "settings.h"
#include "common.h"
#include "utils.h"
#include "print.h"
#include "pages/text_page.h"
#include "bhbb_plugin.h"
#include "bhbb_settings.h"
#include "pages/page.h"
#include "pages/app_browser.h"

#define WIDE2(x) L##x
#define WIDE(x) WIDE2(x)

#define VERSION         "1.44"
#define BGDL_VERSION    "3.2"

using namespace paf;
using namespace sce;

static Settings *currentSettingsInstance = SCE_NULL;
sce::AppSettings *Settings::appSettings = SCE_NULL;

static wchar_t *s_verinfo = NULL;

static wchar_t *s_credits = L"@GrapheneCt\n"
                            "  ScePaf reversing\n"
                            "  NetStream (reference)\n"
                            "  SceDownload + SceIPMI reversing\n"
                            "  Help, and CXML compiler\n\n"
                            "@CreepNT\n"
                            "  SceDownload reversing\n\n"
                            "@CBPS\n"
                            "  Original SceDownload PoC\n\n"
                            "@Princess-of-sleeping\n"
                            "  Help with debugging and development\n"
                            "  SceLsdb notification reversing\n\n"
                            "@SKGleba & TheFl0w\n"
                            "  bgvpk and Download Enabler\n\n"
                            "Beta Testers\n"
                            "  @SomeonPC\n"
                            "  @ShadowKnight1620\n"
                            "  @Koutsie\n"
                            "  @SparklingPheonix\n"
                            "  @gl33ntwine\n\n"
                            "Translators\n"
                            "  @isage - RU\n"
                            "  @S1ngyy - DE\n"
                            "  @CreepNT - FR\n"
                            "  @Princess-of-sleeping - JA\n"
                            "  @maniek6666 - PL\n"
                            "  @michael-moon-lee - ZH\n"
                            "  @Sfresneda - ES\n"
                            "  @gilgilgan - KO\n" 
                            "  @Koutsie - FI\n";
                            "  @ganona - IT\n";
                            "  @iveurne - PT-BR";

Settings::Settings()
{
	if (currentSettingsInstance != SCE_NULL)
	{
		print("Error another settings instance exists! ABORT!\n");
		sceKernelExitProcess(0);
	}

	int ret = 0;
    size_t fileSize = 0;
    const char *mimeType = SCE_NULL;
	
    Plugin::InitParam pInit;
	AppSettings::InitParam sInit;

	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BXCE);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_INI_FILE_PROCESSOR);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_COMMON_GUI_DIALOG);

	pInit.name = "app_settings_plugin";
	pInit.resource_file = "vs0:vsh/common/app_settings_plugin.rco";
	pInit.caller_name = "__main__";

	pInit.set_param_func = AppSettings::PluginSetParamCB;
	pInit.init_func = AppSettings::PluginInitCB;
	pInit.start_func = AppSettings::PluginStartCB;
	pInit.stop_func = AppSettings::PluginStopCB;
	pInit.exit_func = AppSettings::PluginExitCB;
	pInit.module_file = "vs0:vsh/common/app_settings.suprx";
	pInit.draw_priority = 0x96;

	Plugin::LoadSync(pInit);
    
	sInit.xml_file = g_appPlugin->GetResource()->GetFile(file_bhbb_settings, &fileSize, &mimeType);

	sInit.alloc_cb = sce_paf_malloc;
	sInit.free_cb = sce_paf_free;
	sInit.realloc_cb = sce_paf_realloc;
	sInit.safemem_offset = 0;
	sInit.safemem_size = 0x400;

	AppSettings::GetInstance(sInit, &appSettings);

	ret = -1;
	appSettings->GetInt("settings_version", &ret, 0);
	if (ret != d_settingsVersion) //Need to setup default values
	{
        ret = appSettings->Initialize();

        appSettings->SetInt("settings_version", d_settingsVersion);
        appSettings->SetInt("source", d_source);
	}

    //Get values
    appSettings->GetInt("source", (int *)&source, d_source);

    paf::wstring *verinfo = new paf::wstring();

#ifdef _DEBUG
	*verinfo = L"DEBUG ";
#else
	*verinfo = L"RELEASE ";
#endif
	*verinfo += WIDE(__DATE__);
	*verinfo += L"\nVersion " VERSION "\nBGDL Version " BGDL_VERSION;
	s_verinfo = (wchar_t *)verinfo->c_str();

    print("%ls\n", s_verinfo);

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
    auto plugin = paf::Plugin::Find("app_settings_plugin");

    ui::Widget *root = plugin->PageRoot(0xF6C9D4C0);
    if(!root) 
        return;

    auto exitButton = (ui::Button *)root->FindChild(0x8211F03F);
    if(!exitButton) 
        return;

	ui::Event ev(ui::EV_COMMAND, ui::Event::MODE_DISPATCH, ui::Event::ROUTE_FORWARD, 0, 0, 0, 0, 0);
    
    exitButton->DoEvent(ui::Button::CB_BTN_DECIDE, &ev); // I miss the good ol' days when you could pass nullptr as the second arg :(
}

SceVoid Settings::Open()
{
    page::Base::GetCurrentPage()->root->SetActivate(false);
    
	AppSettings::InterfaceCallbacks ifCb;

	ifCb.onStartPageTransitionCb = CBOnStartPageTransition;
	ifCb.onPageActivateCb = CBOnPageActivate;
	ifCb.onPageDeactivateCb = CBOnPageDeactivate;
	ifCb.onCheckVisible = CBOnCheckVisible;
	ifCb.onPreCreateCb = CBOnPreCreate;
	ifCb.onPostCreateCb = CBOnPostCreate;
	ifCb.onPressCb = CBOnPress;
	ifCb.onPressCb2 = CBOnPress2;
	ifCb.onTermCb = CBOnTerm;
	ifCb.onGetStringCb = CBOnGetString;
	ifCb.onGetSurfaceCb = CBOnGetSurface;

	Plugin *appSetPlug = paf::Plugin::Find("app_settings_plugin");
	AppSettings::Interface *appSetIf = (sce::AppSettings::Interface *)appSetPlug->GetInterface(1);
    
	appSetIf->Show(&ifCb);
}

void Settings::OpenCB(int, ui::Handler *, ui::Event*, void*)
{
    Settings::GetInstance()->Open();
}

void Settings::CBOnStartPageTransition(const char *elementId, int32_t type)
{

}

void Settings::CBOnPageActivate(const char *elementId, int32_t type)
{

}

void Settings::CBOnPageDeactivate(const char *elementId, int32_t type)
{

}

int32_t Settings::CBOnCheckVisible(const char *elementId, int *pIsVisible)
{
	*pIsVisible = true;

	return SCE_OK;
}

int32_t Settings::CBOnPreCreate(const char *elementId, sce::AppSettings::Element *element)
{
	return SCE_OK;
}

int32_t Settings::CBOnPostCreate(const char *elementId, paf::ui::Widget *widget)
{
	return SCE_OK;
}

int32_t Settings::CBOnPress(const char *elementId, const char *newValue)
{
	int32_t ret = SCE_OK;
	IDParam elem(elementId);
	IDParam val(newValue);

	event::BroadcastGlobalEvent(g_appPlugin, SettingsEvent, SettingsEvent_ValueChange, elem.GetIDHash(), val.GetIDHash(), sce_paf_atoi(newValue));

	return ret;
}

int32_t Settings::CBOnPress2(const char *elementId, const char *newValue)
{
	return SCE_OK;
}

void Settings::CBOnTerm(int32_t result)
{
    page::Base::GetCurrentPage()->root->SetActivate(true);
}

wchar_t *Settings::CBOnGetString(const char *elementId)
{
	wchar_t *res = g_appPlugin->GetString(elementId);

	if (res[0] == 0)
	{
		if (!sce_paf_strcmp(elementId, "msg_verinfo"))
		{
			return s_verinfo;
		}
        if(!sce_paf_strcmp(elementId, "msg_credinfo"))
        {
            return s_credits;
        }
        if(!sce_paf_strncmp(elementId, "db_name", 7))
        {
            auto id = (Source::ID)sce_paf_strtoul(sce_paf_strrchr(elementId, '_') + 1, nullptr, 10);
            return Source::Create(id)->GetName();
        }
	}

	return res;
}

int32_t Settings::CBOnGetSurface(graph::Surface **surf, const char *elementId)
{
	return SCE_OK;
}