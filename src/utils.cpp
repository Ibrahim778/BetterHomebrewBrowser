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
#include "network.h"
#include "bhbb_dl.h"
#include "error_codes.h"
#include "curl_file.h"

using namespace paf;
using namespace Utils;

SceUInt32 Misc::GetHash(const char *id)
{
    rco::Element searchReq;
    searchReq.id = id;
    
    return searchReq.GetHash(&searchReq.id);
}

SceVoid Net::HttpsToHttp(paf::string& url)
{ 
    if(sce_paf_strncmp("https", url.data(), 5) != 0)
        return;
    
    int strLen = url.length();
    
    char* buff = new char[strLen]; //We don't add +1 bcs we will remove the 's' character anyways
    sce_paf_memset(buff, 0, strLen);

    sce_paf_snprintf(buff, strLen, "http%s", &url.data()[5]);

    url = buff;
    delete[] buff;
}

SceVoid str::GetFromID(const char *id, paf::string *out)
{
    rco::Element e;
    e.hash = Misc::GetHash(id);
    
    wchar_t *wstr = g_appPlugin->GetWString(&e);
    
    common::Utf16ToUtf8(wstr, out);
}

SceVoid str::GetFromHash(SceUInt64 id, paf::string *out)
{
    rco::Element e;
    e.hash = id;
    
    wchar_t *wstr = g_appPlugin->GetWString(&e);
    
    common::Utf16ToUtf8(wstr, out);
}

SceBool Net::IsValidURLSCE(const char *url)
{
    paf::HttpFile file;
    paf::HttpFile::OpenArg openArg;
    SceInt32 ret = SCE_OK;

    openArg.SetUrl(url);

    openArg.SetOpt(4000000, HttpFile::OpenArg::Opt_ResolveTimeOut);
	openArg.SetOpt(10000000, HttpFile::OpenArg::Opt_ConnectTimeOut);

    ret = file.Open(&openArg);
    if(ret == SCE_OK)
    {
        file.Close();
        return SCE_TRUE;
    }

    return SCE_FALSE;
}

SceVoid str::GetfFromID(const char *id, paf::string *out)
{
    paf::string *str = new paf::string;
    str::GetFromID(id, str);

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

SceVoid str::GetfFromHash(SceUInt64 id, paf::string *out)
{
    paf::string *str = new paf::string;
    str::GetFromHash(id, str);

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

SceInt32 Widget::SetLabel(paf::ui::Widget *widget, const char *text)
{
    wstring str16;
    common::Utf8ToUtf16(text, &str16);

    return widget->SetLabel(&str16);
}

SceInt32 Widget::SetLabel(paf::ui::Widget *widget, paf::string &text)
{
    paf::wstring str16;
    common::Utf8ToUtf16(text, &str16);
    return widget->SetLabel(&str16);
}

wchar_t *str::GetPFromID(const char *id)
{
    rco::Element e;
    e.hash = Misc::GetHash(id);

    return g_appPlugin->GetWString(&e);
}

wchar_t *str::GetPFromHash(SceUInt64 hash)
{
    rco::Element e;
    e.hash = hash;

    return g_appPlugin->GetWString(&e);
}

void str::ToLowerCase(char *string)
{
    //Convert to lowerCase
    for(int i = 0; string[i] != '\0'; i++)
        if(string[i] > 64 && string[i] < 91) string[i] += 0x20;
}

void Misc::InitMusic()
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

SceVoid Widget::DeleteTexture(paf::graph::Surface **tex)
{
    if(tex != NULL)
    {
        if(*tex == g_transparentTex) return;
        if(*tex != NULL)
        {
            (*tex)->UnsafeRelease();
            delete *tex;
            *tex = SCE_NULL;
        }
    }
}

SceVoid Misc::StartBGDL()
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
        print("Unloading....\n");
        taiStopUnloadModuleForPid(sceShellID, id, 0, NULL, 0, NULL, NULL);
        print("Done!\n");
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

SceInt32 Widget::PlayEffect(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType type, paf::ui::EventCallback::EventHandler animCB, ScePVoid pUserData)
{
    widget->PlayEffect(param, type, animCB, pUserData);
    if(widget->animationStatus & 0x80)
        widget->animationStatus &= ~0x80;
}

SceInt32 Widget::PlayEffectReverse(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType type, paf::ui::EventCallback::EventHandler animCB, ScePVoid pUserData)
{
    widget->PlayEffectReverse(param, type, animCB, pUserData);
    if(widget->animationStatus & 0x80)
        widget->animationStatus &= ~0x80;
}

SceVoid GamePad::CB(paf::input::GamePad::GamePadData *pData)
{
    //(~pData->buttons & buttons) = Removed bits
    if((~pData->buttons & buttons) != 0)
    {
        input::GamePad::GamePadData dat;
        sce_paf_memcpy(&dat, pData, sizeof(dat));
        dat.buttons = (~pData->buttons & buttons);
        for(GamePadCB& c : cbList)
            c.func(&dat, c.pUserData);
    }

    buttons = pData->buttons;
}

SceVoid GamePad::RegisterButtonUpCB(Utils::GamePad::CallbackFunction cb, ScePVoid pUserData)
{
    if(cbList.size() == 0)
        paf::input::GamePad::RegisterCallback(CB);

    cbList.push_back(GamePadCB(cb, pUserData));
}

SceVoid Time::SavePreviousDLTime()
{
    SceInt32 err = SCE_OK;
    auto file = LocalFile::Open(SavePath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &err);
    if(err != SCE_OK)
    {
        print("Error opening %s -> 0x%X\n", err);
        return;
    }

    file.get()->Write(Time::previousDownloadTime, sizeof(Time::previousDownloadTime));
}

paf::rtc::Tick Time::GetPreviousDLTime(db::Id source)
{
    Time::LoadPreviousDLTime();
    return Time::previousDownloadTime[source];
}

SceVoid Time::SetPreviousDLTime(db::Id source, paf::rtc::Tick time)
{
    Time::previousDownloadTime[source] = time;
    Time::SavePreviousDLTime();
}

SceVoid Time::LoadPreviousDLTime()
{
    sce_paf_memset(Time::previousDownloadTime, 0, sizeof(Time::previousDownloadTime));

    SceInt32 err = SCE_OK;
    auto file = LocalFile::Open(SavePath, SCE_O_RDONLY, 0, &err);
    if(err != SCE_OK)
    {
    #ifdef _DEBUG
        if(err == SCE_PAF_ERROR_ERRNO_ENOENT)
            print("[Info] Previous time does not exist, aborting\n");
        else
            print("[Error] Reading %s -> 0x%X\n", SavePath, err);
    #endif // _DEBUG
        return;
    }

    file.get()->Read(Time::previousDownloadTime, sizeof(Time::previousDownloadTime));
}   

SceVoid Time::ResetPreviousDLTime()
{
    sce_paf_memset(Time::previousDownloadTime, 0, sizeof(Time::previousDownloadTime));

    LocalFile::RemoveFile(Time::SavePath);
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