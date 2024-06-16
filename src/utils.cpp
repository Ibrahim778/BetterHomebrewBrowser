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

#include <audioout.h>
#include <paf.h>
#include <ShellAudio.h>
#include <taihen.h>
#include <appmgr.h>
#include <psp2_compat/curl/curl.h>

#include "utils.h"
#include "print.h"
#include "common.h"
#include "bhbb_dl.h"
#include "error_codes.h"

using namespace paf;
using namespace paf::common;

void Utils::HttpsToHttp(const char *src, paf::string& outURL)
{ 
    if(sce_paf_strncmp(src, "https", 5) != 0)
    {
        outURL = src;
        return;
    }
    
    outURL = common::FormatString("http%s", &src[5]);
}

bool Utils::IsValidURLSCE(const char *url)
{
    paf::HttpFile file;
    paf::HttpFile::OpenArg openArg;
    SceInt32 ret = SCE_OK;

    openArg.ParseUrl(url);
    openArg.SetOption(4000000, HttpFile::OpenArg::OptionType_ResolveTimeOut);
	openArg.SetOption(10000000, HttpFile::OpenArg::OptionType_ConnectTimeOut);

    ret = file.Open(&openArg);
    
    if(ret == SCE_HTTP_ERROR_SSL) ret = SCE_OK; // Temporary fix till I figure out wot is going on with this SSL stuff (SceDownload should report proper error anyways)

    if(ret == SCE_OK)
    {
        file.Close();
        return SCE_TRUE;
    }
    print("Open %s FAIL 0x%X\n", url, ret);
    return SCE_FALSE;
}

void Utils::InitMusic()
{
    SceInt32 ret = -1;

    ret = sceMusicInternalAppInitialize(0);
    print("[sceMusicInternalAppInitialize] 0x%X\n", ret);

    SceMusicOpt optParams;
    sce_paf_memset(&optParams, 0, 0x10);

    optParams.flag = -1;

    ret = sceMusicInternalAppSetUri((char *)"pd0:data/systembgm/store.at9", &optParams);
    print("[sceMusicInternalAppSetUri] 0x%X\n", ret);

    ret = sceMusicInternalAppSetVolume(SCE_AUDIO_VOLUME_0DB);
    print("[sceMusicInternalAppSetVolume] 0x%X\n", ret);

    ret = sceMusicInternalAppSetRepeatMode(SCE_MUSIC_REPEAT_ONE);
    print("[sceMusicInternalAppSetRepeatMode] 0x%X\n", ret);

    ret = sceMusicInternalAppSetPlaybackCommand(SCE_MUSIC_EVENTID_DEFAULT, 0);
    print("[sceMusicInternalAppSetPlaybackCommand] 0x%X\n", ret);
}

size_t SaveCore(char *ptr, size_t size, size_t nmeb, SharedPtr<LocalFile> *file)
{
    return file->get()->Write(ptr, size * nmeb);
}

int Utils::DownloadFile(const char *url, const char *dest)
{
    int ret = SCE_OK;
    SharedPtr<LocalFile> file = LocalFile::Open(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &ret);
    if(ret != SCE_OK)
    {
        print("[Utils::DownloadFile] Failed to open %s for writing -> 0x%X\n", dest, ret);
        return ret;
    }
    
    CURL *handle = curl_easy_init();
    if(!handle)
    {
        print("[Utils::DownloadFile] Failed to create curl handle!\n");
        return -1;
    }
    
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 1L);
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
    curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);

#ifdef _DEBUG
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
#endif

    curl_easy_setopt(handle, CURLOPT_URL, url);

    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, SaveCore);

    ret = curl_easy_perform(handle);

    curl_easy_cleanup(handle);

    if(ret != CURLE_OK)
    {
        file.release();
        LocalFile::RemoveFile(dest);
    }

    print("[Utils::DownloadFile] (%s -> %s) result > 0x%X\n", url, dest, ret);
    return ret;
}

void Utils::Decapitalise(char *string)
{
    //Convert to lowerCase
    for(int i = 0; string[i] != '\0'; i++)
        if(string[i] > 64 && string[i] < 91) string[i] += 0x20;
}

void Utils::StartBGDL()
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
        
        common::SharedPtr<LocalFile> openResult = LocalFile::Open("ux0:data/bgdlid", SCE_O_RDONLY, 0, NULL);
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
    common::SharedPtr<LocalFile> openResult = LocalFile::Open("ux0:data/bgdlid", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, NULL);
    openResult.get()->Write(&moduleID, sizeof(SceUID));
#endif
}