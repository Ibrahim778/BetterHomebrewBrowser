#include <kernel.h>
#include <paf/stdc.h>
#include <psp2_compat/curl/curl.h>

#include "net.h"
#include "notifmgr.h"
#include "print.h"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"

CURL *curl = SCE_NULL;
extern SceBool running;

int curlProgressCallback(void *userDat, double dltotal, double dlnow, double ultotal, double ulnowm)
{
    if (NotifMgr_currDlCanceled)
		return 1; //Cancelled
	
    if(!running)
        return 1; //End

    char subtxt[64];
	sce_paf_memset(subtxt, 0, sizeof(subtxt));
	if(dltotal != 0.0)
        sce_paf_snprintf(subtxt, 64, "%d%% Done (%d KB / %d KB)", (int)(dlnow / dltotal * 100.0), (int)(dlnow / 1024.0), (int)(dltotal / 1024.0));
    else sce_paf_snprintf(subtxt, 64, "%d KB", dlnow);
    NotifMgr_UpdateProgressNotif(dlnow / dltotal * 100.0, subtxt, SCE_NULL);

    return 0; //Signal Continue
}

size_t curlWriteCB(void *datPtr, size_t chunkSize, size_t chunkNum, void *userDat)
{
    if(!running)
        return 0;

    if (NotifMgr_currDlCanceled)
		return 0; //Cancelled
    return sceIoWrite(*(int *)userDat, datPtr, chunkSize * chunkNum);
}

int dlFile(const char *url, const char *dest)
{
    sceIoRemove(dest);
    SceUID file = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if(file < 0)
    {
        print("Couldn't open file %s -> 0x%X\n", dest, file);
        return file;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    
    CURLcode ret = curl_easy_perform(curl);
    // if(ret != CURLE_OK && ret != 42)
    // {
    //     NotifMgr_SendNotif(curl_easy_strerror(ret));
    // }
    

    sceIoClose(file);
    return (int)ret;
}

void curlInit()
{
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
}