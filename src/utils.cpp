#include "utils.hpp"
#include "main.hpp"
#include "pagemgr.hpp"
#include "common.hpp"
#include "network.hpp"

extern Page *currPage;

CURL *Utils::curl = SCE_NULL;

SceUInt32 Utils::GetHashById(const char *id)
{
    Resource::Element searchReq;
    Resource::Element searchRes;
    
    searchReq.id.Set(id);
    searchRes.hash = searchRes.GetHashById(&searchReq);

    return searchRes.hash;
}

Resource::Element Utils::GetParamWithHash(SceUInt32 hash)
{
    Resource::Element search;
    search.hash = hash;

    return search;
}

Resource::Element Utils::GetParamWithHashFromId(const char *id)
{
    Resource::Element search;

    search.hash = Utils::GetHashById(id);

    return search;
}

Widget::Color Utils::makeSceColor(float r, float g, float b, float a)
{
    Widget::Color col;
    col.r = r;
    col.g = g;
    col.b = b;
    col.a = a;
    return col;
}

Widget * Utils::GetChildByHash(Widget *parent, SceUInt32 hash)
{
    Resource::Element search = GetParamWithHash(hash);
    return parent->GetChildByHash(&search, 0);
}

bool checkFileExist(const char *path)
{
    SceIoStat s;
    return sceIoGetstat(path, &s) >= 0;
}

SceVoid UtilThread::EntryFunction()
{
    if(Entry != NULL) Entry();
}

int curlProgressCallback(void *userDat, double dltotal, double dlnow, double ultotal, double ulnow)
{
    if(currPage->pageThread->EndThread)
        return 1; //End

    if(userDat != NULL && !currPage->pageThread->EndThread)
        ((ProgressBar *)userDat)->SetProgress(dlnow / dltotal * 100.0, 0, 0);
    
    return 0; //Signal Continue
}

size_t curlWriteCB(void *datPtr, size_t chunkSize, size_t chunkNum, void *userDat)
{
    if(currPage->pageThread->EndThread)
        return 0;

    return sceIoWrite(*(int *)userDat, datPtr, chunkSize * chunkNum);
}

SceInt32 Utils::DownloadFile(const char *url, const char *dest, ProgressBar *progressBar)
{

    if(curl == NULL) return -1;

    SceUID file = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if(file < 0) return file;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progressBar);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

    CURLcode ret = curl_easy_perform(curl);

    sceIoClose(file);
    return (int)ret;
}

SceInt32 Utils::SetWidgetLabel(Widget *widget, const char *text)
{
    String str;
    str.Set(text);

    WString wstr;
    str.ToWString(&wstr);

    return widget->SetLabel(&wstr);
}

void Utils::MakeDataDirs()
{
    if(!checkFileExist(DATA_PATH))
        sceIoMkdir(DATA_PATH, 0777);
    if(!checkFileExist(ICON_SAVE_PATH))
        sceIoMkdir(ICON_SAVE_PATH, 0777);
}

void Utils::OverClock()
{
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
}

void Utils::UnderClock()
{
    scePowerSetArmClockFrequency(333);
    scePowerSetBusClockFrequency(111);
    scePowerSetGpuClockFrequency(111);
    scePowerSetGpuXbarClockFrequency(111);
}

void Utils::NetInit()
{
    netInit();
    httpInit();
    loadPsp2CompatModule();

    curl = curl_easy_init();

    printf("Curl = 0x%X\n", curl);
    if(curl != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCB);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curlProgressCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    }
}