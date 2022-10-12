#include <kernel.h>
#include <audioout.h>
#include <ShellAudio.h>
#include <paf.h>
#include <power.h>
#include <libsysmodule.h>
#include <appmgr.h>
#include <message_dialog.h>
#include <taihen.h>

#include "utils.h"
#include "print.h"
#include "common.h"
#include "main.h"
#include "network.h"
#include "bhbb_dl.h"

using namespace paf;

SceUInt32 Utils::GetHashById(const char *id)
{
    rco::Element searchReq;
    rco::Element searchRes;
    
    searchReq.id = id;
    searchRes.hash = searchRes.GetHash(&searchReq.id);

    return searchRes.hash;
}

SceVoid Utils::GetStringFromID(const char *id, paf::string *out)
{
    rco::Element e;
    e.hash = Utils::GetHashById(id);
    
    wchar_t *wstr = mainPlugin->GetWString(&e);
    ccc::UTF16toUTF8((const wchar_t *)wstr, out);
}

SceVoid Utils::GetfStringFromID(const char *id, paf::string *out)
{
    paf::string *str = new paf::string;
    Utils::GetStringFromID(id, str);

    int slashNum = 0;
    int strlen = str->length();
    const char *strptr = str->data();
    for(int i = 0; i < strlen + 1 && strptr[i] != '\0'; i++)
        if(strptr[i] == '\\') slashNum++;

    int buffSize = (strlen + 1) - slashNum;
    char *buff = new char[buffSize];
    sce_paf_memset(buff, 0, buffSize);

    for(char *buffPtr = buff, *strPtr = (char *)strptr; *strPtr != '\0'; strPtr++, buffPtr++)
    {
        if(*strPtr == '\\')
        {
            switch(*(strPtr + sizeof(char)))
            {
                case 'n':
                    *buffPtr = '\n';
                    break;
                case 'a':
                    *buffPtr = '\a';
                    break;
                case 'b':
                    *buffPtr = '\b';
                    break;
                case 'e':
                    *buffPtr = '\e';
                    break;
                case 'f':
                    *buffPtr = '\f';
                    break;
                case 'r':
                    *buffPtr = '\r';
                    break;
                case 'v':
                    *buffPtr = '\v';
                    break;
                case '\\':
                    *buffPtr = '\\';
                    break;
                case '\'':
                    *buffPtr = '\'';
                    break;
                case '\"':
                    *buffPtr = '\"';
                    break;
                case '?':
                    *buffPtr = '\?';
                    break;
                case 't':
                    *buffPtr = '\t';
                    break;
            }
            strPtr++;
        }
        else *buffPtr = *strPtr;
    }

    *out = buff;

    delete str;
    delete[] buff;
}

SceInt32 Utils::SetWidgetLabel(paf::ui::Widget *widget, const char *text)
{
    paf::wstring wstr;
    ccc::UTF8toUTF16(text, &wstr);    
    return widget->SetLabel(&wstr);
}

SceInt32 Utils::SetWidgetLabel(paf::ui::Widget *widget, paf::string *text)
{
    paf::wstring wstr;

    ccc::UTF8toUTF16(text, &wstr);

    return widget->SetLabel(&wstr);
}

wchar_t *Utils::GetStringPFromID(const char *id)
{
    rco::Element e;
    e.hash = Utils::GetHashById(id);

    return mainPlugin->GetWString(&e);
}

void Utils::ToLowerCase(char *string)
{
    //Convert to lowerCase
    for(int i = 0; string[i] != '\0'; i++)
        if(string[i] > 64 && string[i] < 91) string[i] += 0x20;
}

void Utils::InitMusic()
{
    SceInt32 ret = -1;

    ret = sceMusicInternalAppInitialize(0);
    if(ret < 0) print("[AUDIO_INIT] Error! 0x%X", ret);

    SceMusicOpt optParams;
    sce_paf_memset(&optParams, 0, 0x10);

    optParams.flag = -1;

    ret = sceMusicInternalAppSetUri((char *)"pd0:data/systembgm/store.at9", &optParams);
    if(ret < 0) print("[CORE_OPEN] Error! 0x%X", ret);

    ret = sceMusicInternalAppSetVolume(SCE_AUDIO_VOLUME_0DB);
    if(ret < 0) print("[SET_VOL] Error! 0x%X", ret);

    ret = sceMusicInternalAppSetRepeatMode(SCE_MUSIC_REPEAT_ONE);
    if(ret < 0) print("[SET_REPEAT_MODE] Error! 0x%X", ret);

    ret = sceMusicInternalAppSetPlaybackCommand(SCE_MUSIC_EVENTID_DEFAULT, 0);
    if(ret < 0) print("[SEND_EVENT_PLAY] Error! 0x%X", ret);
}

void Utils::SetMemoryInfo()
{
    SceAppMgrBudgetInfo info;
    sce_paf_memset(&info, 0, sizeof(SceAppMgrBudgetInfo));
    info.size = sizeof(SceAppMgrBudgetInfo);

    sceAppMgrGetBudgetInfo(&info);
    if(info.budgetMain >= 0x2800000) loadFlags |= LOAD_FLAGS_ICONS;
    if(info.budgetMain >= 0x3200000) loadFlags |= LOAD_FLAGS_SCREENSHOTS;
}

SceVoid Utils::DeleteTexture(paf::graph::Surface **tex)
{
    if(tex != NULL)
    {
        if(*tex == TransparentTex) return;
        if(*tex != NULL)
        {
            //(*tex)->Release();
            delete *tex;
            *tex = SCE_NULL;
        }
    }
}

SceVoid Utils::StartBGDL()
{
    SceInt32 res = SCE_OK;
    SceUID moduleID = SCE_UID_INVALID_UID;
    SceUID sceShellID = SCE_UID_INVALID_UID;

    res = sceAppMgrGetIdByName(&sceShellID, "NPXS19999");
    if(res != SCE_OK)
    {
        print("Unable to get SceShell ID! 0x%X\n", res);
        return;
    }
#ifdef _DEBUG
    if(LocalFile::Exists("ux0:data/bgdlid"))
    {
        SceUID id;
        
        SharedPtr<LocalFile> openResult = LocalFile::Open("ux0:data/bgdlid", SCE_O_RDONLY, 0, NULL);
        openResult.get()->Read(&id, sizeof(SceUID));
        taiStopUnloadModuleForPid(sceShellID, id, 0, NULL, 0, NULL, NULL);
    }
#endif
    res = taiLoadStartModuleForPid(sceShellID, "ux0:app/BHBB00001/module/bhbb_dl.suprx", 0, NULL, 0);
    if(res < 0)
    {
        print("Unable to start BGDL! (Already Running?) 0x%X\n", res);
        return;
    }

    moduleID = res;
    
    print("BGDL started with ID 0x%X\n", moduleID);
#ifdef _DEBUG
    SharedPtr<LocalFile> openResult = LocalFile::Open("ux0:data/bgdlid", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, NULL);
    openResult.get()->Write(&moduleID, sizeof(SceUID));
#endif
}

ui::Widget *Utils::CreateWidget(const char *id, const char *type, const char *style, ui::Widget *parent)
{
    rco::Element styleInfo;
    styleInfo.hash = Utils::GetHashById(style);
    rco::Element widgetInfo;
    widgetInfo.hash = Utils::GetHashById(id);

    return mainPlugin->CreateWidgetWithStyle(parent, type, &widgetInfo, &styleInfo);
}

SceInt32 Utils::PlayEffect(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType type, paf::ui::EventCallback::EventHandler animCB, ScePVoid pUserData)
{
    widget->PlayEffect(param, type, animCB, pUserData);
    if(widget->animationStatus & 0x80)
        widget->animationStatus &= ~0x80;
}

SceInt32 Utils::PlayEffectReverse(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType type, paf::ui::EventCallback::EventHandler animCB, ScePVoid pUserData)
{
    widget->PlayEffectReverse(param, type, animCB, pUserData);
    if(widget->animationStatus & 0x80)
        widget->animationStatus &= ~0x80;
}
#ifdef _DEBUG

SceVoid Utils::PrintAllChildren(paf::ui::Widget *widget, int offset)
{
    for (int i = 0; i < widget->childNum; i++)
    {
        for (int i = 0; i < offset; i++) print(" ");
        wstring wstr;
        widget->GetChild(i)->GetLabel(&wstr);
        print(" %d 0x%X (%s, \"%ls\")\n", i, widget->GetChild(i)->elem.hash, widget->GetChild(i)->name(), wstr.data());
        Utils::PrintAllChildren(widget->GetChild(i), offset + 4);
    }
}

#endif