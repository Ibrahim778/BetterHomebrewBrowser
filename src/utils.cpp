#include "utils.hpp"
#include "main.hpp"
#include "pagemgr.hpp"
#include "common.hpp"
#include "network.hpp"
#include "..\bhbb_dl\src\bhbb_dl.h"
#include "eventhandler.hpp"
#include <bgapputil.h>

CURL *BHBB::Utils::curl = SCE_NULL;
SceInt32 BHBB::Utils::currStok = 0;

SceUInt32 BHBB::Utils::GetHashById(const char *id)
{
    Resource::Element searchReq;
    Resource::Element searchRes;
    
    searchReq.id.Set(id);
    searchRes.hash = searchRes.GetHashById(&searchReq);

    return searchRes.hash;
}

Resource::Element BHBB::Utils::GetParamWithHash(SceUInt32 hash)
{
    Resource::Element search;
    search.hash = hash;

    return search;
}

Resource::Element BHBB::Utils::GetParamWithHashFromId(const char *id)
{
    Resource::Element search;

    search.hash = BHBB::Utils::GetHashById(id);

    return search;
}

Widget::Color BHBB::Utils::makeSceColor(float r, float g, float b, float a)
{
    Widget::Color col;
    col.r = r;
    col.g = g;
    col.b = b;
    col.a = a;
    return col;
}

Widget * BHBB::Utils::GetChildByHash(Widget *parent, SceUInt32 hash)
{
    Resource::Element search = GetParamWithHash(hash);
    return parent->GetChildByHash(&search, 0);
}

bool BHBB::Utils::isDirEmpty(const char *path)
{
    SceUID f = sceIoDopen(path);
    if(f < 0) return false;
    SceIoDirent d;
    bool r = sceIoDread(f, &d) < 0;
    sceIoDclose(f);

    return r;
}

SceVoid UtilThread::EntryFunction()
{
    if(Entry != NULL) Entry(ParentPage);
}

UtilThread::UtilThread(ThreadCB entry, const char *pName):Thread(SCE_KERNEL_COMMON_QUEUE_LOWEST_PRIORITY, SCE_KERNEL_16KiB, pName, NULL)
{
    Entry = entry;
    EndThread = SCE_FALSE;
    ParentPage = Page::GetCurrentPage();
}

SceVoid UtilThread::Kill()
{
    EndThread = SCE_TRUE;
    Join();
    EndThread = SCE_FALSE;
}

SceVoid UtilThread::Delete()
{
    Kill();
    delete this;
}

int curlProgressCallback(void *userDat, double dltotal, double dlnow, double ultotal, double ulnow)
{
    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
    if(Page::GetCurrentPage()->pageThread->EndThread)
        return 1; //End

    if(userDat != NULL && !Page::GetCurrentPage()->pageThread->EndThread)
        ((ProgressBar *)userDat)->SetProgress(dlnow / dltotal * 100.0, 0, 0);
    
    return 0; //Signal Continue
}

size_t curlWriteCB(void *datPtr, size_t chunkSize, size_t chunkNum, void *userDat)
{
    if(Page::GetCurrentPage()->pageThread->EndThread)
        return 0;

    return sceIoWrite(*(int *)userDat, datPtr, chunkSize * chunkNum);
}

SceInt32 BHBB::Utils::DownloadFile(const char *url, const char *dest, ProgressBar *progressBar)
{
    if(curl == NULL) return -1;

    SceUID file = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if(file < 0) return file;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progressBar);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    CURLcode ret = curl_easy_perform(curl);

    sceIoClose(file);
    if(ret != CURLE_OK) sceIoRemove(dest);
    return (int)ret;
}

SceBool BHBB::Utils::TestTexture(const char *path)
{
    SceBool ret = true;
    Misc::OpenResult res;
    SceInt32 err;
    Misc::OpenFile(&res, path, SCE_O_RDONLY, 0666, &err);
    print("%d\n", res.localFile->GetSize());
    if(err < 0) return SCE_FALSE;

    graphics::Texture tex;
    graphics::Texture::CreateFromFile(&tex, mainPlugin->memoryPool, &res);
    if(tex.texSurface == NULL) ret = false;

    Utils::DeleteTexture(&tex, false);

    delete res.localFile;
    sce_paf_free(res.unk_04);
    return ret;
}

SceInt32 BHBB::Utils::SetWidgetLabel(Widget *widget, const char *text)
{
    String str;
    str.Set(text);

    WString wstr;
    str.ToWString(&wstr);

    return widget->SetLabel(&wstr);
}

SceInt32 BHBB::Utils::SetWidgetLabel(Widget *widget, String *text)
{
    WString wstr;
    text->ToWString(&wstr);

    return widget->SetLabel(&wstr);
}

void BHBB::Utils::ResetStrtok()
{
    currStok = 0;
}

int BHBB::Utils::getStrtokNum(char splitter, char *str)
{
    int ret = 0; 
    for (int i = 0; i < sce_paf_strlen(str); i++)
    {
        if(str[i] == splitter)
            ret++;
    }

    return ret + 1;
}

char *BHBB::Utils::strtok(char splitter, char *str)
{
    int len = sce_paf_strlen(str);
    for (int i = currStok; i <= len; i++)
    {
        if(str[i] == splitter || str[i] == '\0')
        {
            char *s = (char *)sce_paf_malloc((i - currStok) + 1);
            sce_paf_memset(s, 0, (i - currStok) + 1 );

            sce_paf_memcpy(s, str + sizeof(char) * currStok, (i - currStok));
            currStok = i + 1; 
            return s;
        }
    }

    return NULL;
}

void BHBB::Utils::MakeDataDirs()
{
    
    if(!paf::io::Misc::Exists(DATA_PATH))
    	paf::io::Misc::Mkdir(DATA_PATH, 0666);
    if(!paf::io::Misc::Exists(ICON_SAVE_PATH))
		paf::io::Misc::Mkdir(ICON_SAVE_PATH, 0666);
    if(!paf::io::Misc::Exists(SCREENSHOT_SAVE_PATH))
		paf::io::Misc::Mkdir(SCREENSHOT_SAVE_PATH, 0666);
    
}

void BHBB::Utils::StartBGDL()
{
    SceUID pipe = sceKernelOpenMsgPipe(BHBB_DL_PIPE_NAME);
    if(pipe > 0) return;

    sceSysmoduleLoadModule(SCE_SYSMODULE_BG_APP_UTIL);
    sceBgAppUtilStartBgApp(0);
}

void BHBB::Utils::NetInit()
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

void BHBB::Utils::ToLowerCase(char *string)
{
    //Convert to lowerCase
    for(int i = 0; string[i] != '\0'; i++)
        if(string[i] > 64 && string[i] < 91) string[i] += 0x20;
}

bool BHBB::Utils::StringContains(char *h, char *n)
{
    int needleLen = sce_paf_strlen(n);
    for(int currMatchLen = 0; *h != '\0'; h++)
    {
        if(*h == n[currMatchLen]) currMatchLen++;
        else currMatchLen = 0;
        if(currMatchLen == needleLen) return true;
    }

    return false;
}
SceInt32 BHBB::Utils::SetWidgetSize(Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    SceFVector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetSize(&v);
}

SceInt32 BHBB::Utils::SetWidgetPosition(Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    SceFVector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetPosition(&v);
}

SceInt32 BHBB::Utils::SetWidgetColor(Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a)
{
    Widget::Color col;
    col.r = r;
    col.g = g;
    col.b = b;
    col.a = a;

    return widget->SetFilterColor(&col);
}

SceInt32 BHBB::Utils::AssignButtonHandler(Widget *button, ECallback onPress, void *userDat, int id)
{
    if(button == NULL) return -1;
    EventHandler *eh = new EventHandler();
    eventcb callback;
    callback.Callback = onPress;
    callback.dat = userDat;

    eh->pUserData = sce_paf_malloc(sizeof(callback));
    if(eh->pUserData == NULL)
    {
        print("Error Allocating Memory for button userData");
        new TextPage("Error Allocating Memory for button userData", "Assign Handler Error");
        delete eh;
        return;
    }
    sce_paf_memcpy(eh->pUserData, &callback, sizeof(callback));

    return button->RegisterEventCallback(id, eh, 0);
}

SceBool BHBB::Utils::CreateTextureFromFile(graphics::Texture *tex, const char *file)
{
    if(tex == NULL) return SCE_FALSE;

    Misc::OpenResult openResult;
    SceInt32 err;
    Misc::OpenFile(&openResult, file, SCE_O_RDONLY, 0666, &err);

    if(err < 0)
        return SCE_FALSE;

    graphics::Texture::CreateFromFile(tex, mainPlugin->memoryPool, &openResult);

    delete openResult.localFile;
    sce_paf_free(openResult.unk_04);

    return tex->texSurface != NULL;
}

SceVoid BHBB::Utils::DeleteTexture(graphics::Texture *tex, bool deletePointer)
{
    if(tex == transparentTex) return;
    if(tex != NULL)
    {
        if(tex->texSurface != NULL)
        {
            graphics::Surface *s = tex->texSurface;
            tex->texSurface = SCE_NULL;
            delete s;
        }
        if(deletePointer)
            delete tex;
    }
}

SceVoid BHBB::Utils::DeleteWidget(Widget *w)
{
    common::Utils::WidgetStateTransition(0, w, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
}

#ifdef _DEBUG

SceVoid BHBB::Utils::PrintAllChildren(Widget *widget, int offset)
{
    for (int i = 0; i < widget->childNum; i++)
    {
        for (int i = 0; i < offset; i++) print("-");
        print(" %d 0x%X\n", i, widget->GetChildByNum(i)->hash);
        BHBB::Utils::PrintAllChildren(widget->GetChildByNum(i), offset + 4);
    }
    if(widget->childNum == 0) print("No Children to Print!\n");
}

#endif