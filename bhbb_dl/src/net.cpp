#include <kernel.h>
#include <libsysmodule.h>
#include <net.h>
#include <libnetctl.h>
#include <stdio.h>
#include <libhttp.h>
#include <libssl.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "main.hpp"
#include "notifmgr.hpp"

extern SceBool Running;

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"

CURL *curl = SCE_NULL;

#define MODULE_PATH "vs0:data/external/webcore/ScePsp2Compat.suprx"
#define NET_INIT_SIZE 1024 * 1024 * 2

SceUID moduleID;

void loadPsp2CompatModule()
{
    //curl needs compat
    if(moduleID <= 0)
        moduleID = sceKernelLoadStartModule(MODULE_PATH, 0, NULL, 0, NULL, NULL);
}

void netInit() 
{
	int ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	SceNetInitParam netInitParam;
	netInitParam.memory = malloc(NET_INIT_SIZE);
	netInitParam.size = NET_INIT_SIZE;
	netInitParam.flags = 0;
	ret = sceNetInit(&netInitParam);
	ret = sceNetCtlInit();
}

void netTerm() 
{
	sceNetCtlTerm();
	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
}

void httpInit() 
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
	sceHttpInit(NET_INIT_SIZE);
	sceSslInit(NET_INIT_SIZE);
}

void httpTerm() 
{
	sceSslTerm();
	sceHttpTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
}

int curlProgressCallback(void *userDat, double dltotal, double dlnow, double ultotal, double ulnowm)
{
    if (NotifMgr::currDlCanceled)
		return 1; //Cancelled
	
    if(!Running)
        return 1; //End


    char subtxt[64] = {0};
    snprintf(subtxt, 64, "%d%% Done", (int)(dlnow / dltotal * 100.0));
    NotifMgr::UpdateProgressNotif(dlnow / dltotal * 100.0, subtxt);

    
    return 0; //Signal Continue
}

size_t curlWriteCB(void *datPtr, size_t chunkSize, size_t chunkNum, void *userDat)
{
    if(!Running)
        return 0;

    if (NotifMgr::currDlCanceled)
		return 0; //Cancelled

    return sceIoWrite(*(int *)userDat, datPtr, chunkSize * chunkNum);
}

void dlFile(const char *url, const char *dest)
{
    SceUID file = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    CURLcode ret = curl_easy_perform(curl);
    if(ret != CURLE_OK && ret != 42)
    {
        NotifMgr::SendNotif(curl_easy_strerror(ret));
    }

    sceIoClose(file);
}

void curlInit()
{
    netInit();
    httpInit();
    loadPsp2CompatModule();

    curl = curl_easy_init();

    if(curl != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCB);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curlProgressCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    }

}

void curlEnd()
{
    curl_easy_cleanup(curl);
    httpTerm();
    netTerm();
}