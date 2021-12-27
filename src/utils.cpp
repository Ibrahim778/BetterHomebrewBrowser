#include "utils.hpp"
#include "main.hpp"
#include "pagemgr.hpp"
#include "common.hpp"
#include "network.hpp"
#include "..\bhbb_dl\src\bhbb_dl.h"
#include "eventhandler.hpp"
#include <bgapputil.h>

static CURL *curl = SCE_NULL;
static SceInt32 currStok = 0;

typedef struct {
    Page *callingPage;
    void *data;
} CURLUserData;

SceUInt32 Utils::GetHashById(const char *id)
{
    Resource::Element searchReq;
    Resource::Element searchRes;
    
    searchReq.id = id;
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

bool Utils::isDirEmpty(const char *path)
{
    SceUID f = sceIoDopen(path);
    if(f < 0) return false;
    SceIoDirent d;
    bool r = sceIoDread(f, &d) < 0;
    sceIoDclose(f);

    return r;
}

UtilJob::UtilJob(JobCB task, void *callingPage, UtilQueue *caller, const char *name):Item(name) 
{ 
    this->task = task; 
    parentPage = callingPage; 
    this->caller = caller;
}

SceVoid UtilJob::Run()
{
    if(task != NULL) task(parentPage); 
}

SceVoid UtilJob::JobKiller(thread::JobQueue::Item *job)
{
    if(job != NULL)
        delete job;
}

SceVoid UtilJob::Finish()
{
    if(caller != NULL)
        caller->num--;
}

SceBool UtilQueue::WaitingTasks()
{
    return num > 1 || End;
}

UtilQueue::UtilQueue(void *callingPage, const char *name):JobQueue(name)
{
    End = SCE_FALSE;
    num = 0;
    this->parentPage = callingPage;
}

UtilQueue::~UtilQueue()
{
    Finish();
    Join();
}

SceVoid UtilQueue::AddTask(JobCB Task, const char *name)
{
    auto item = new UtilJob(Task, parentPage, this, name);

    CleanupHandler *req = new CleanupHandler();
    req->userData = item;
    req->refCount = 0;
    req->unk_08 = 1;
    req->cb = (CleanupHandler::CleanupCallback)UtilJob::JobKiller;

    ObjectWithCleanup itemParam;
    itemParam.object = item;
    itemParam.cleanup = req;

    Enqueue(&itemParam);

    num++;
}

int curlProgressCallback(void *userDat, double dltotal, double dlnow, double ultotal, double ulnow)
{
    if(userDat == NULL)
    {
        print("Error no userDat provided in curlProgressCallback()!\n");
        return 1; //End
    }

    CURLUserData *data = (CURLUserData *)userDat;

    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);

    if(data->callingPage->jobs->End)
        return 1; //End

    if(!data->callingPage->jobs->End && data->data != NULL)
        ((ProgressBar *)data->data)->SetProgress(dlnow / dltotal * 100.0, 0, 0);
    
    return 0; //Signal Continue
}

size_t curlWriteCB(void *datPtr, size_t chunkSize, size_t chunkNum, void *userDat)
{
    if(userDat == NULL)
    {
        print("Error no userDat provided in curlWriteCB()!\n");
        return 0;
    }

    CURLUserData *data = (CURLUserData *)userDat;

    if(data->callingPage->jobs->End)
        return 0;

    return sceIoWrite(*((int *)data->data), datPtr, chunkSize * chunkNum);
}

SceInt32 Utils::DownloadFile(const char *url, const char *dest, void *callingPage, ProgressBar *progressBar)
{
    if(curl == NULL) return -1;

    SceUID file = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if(file < 0) return file;

    curl_easy_setopt(curl, CURLOPT_URL, url);

    CURLUserData progressData;
    progressData.callingPage = (Page *)callingPage;
    progressData.data = progressBar;

    CURLUserData writeData;
    writeData.callingPage = (Page *)callingPage;
    writeData.data = &file;

    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progressData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeData);
    CURLcode ret = curl_easy_perform(curl);

    sceIoClose(file);
    if(ret != CURLE_OK) sceIoRemove(dest);
    return (int)ret;
}

SceBool Utils::TestTexture(const char *path)
{
    /*
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
    */
}

SceInt32 Utils::SetWidgetLabel(Widget *widget, const char *text)
{
    WString wstr;
    WString::CharToNewWString(text, &wstr);    
    return widget->SetLabel(&wstr);
}

SceInt32 Utils::SetWidgetLabel(Widget *widget, String *text)
{
    WString wstr;
    text->ToWString(&wstr);

    return widget->SetLabel(&wstr);
}

void Utils::ResetStrtok()
{
    currStok = 0;
}

int Utils::getStrtokNum(char splitter, char *str)
{
    int ret = 0; 
    for (int i = 0; i < sce_paf_strlen(str); i++)
    {
        if(str[i] == splitter)
            ret++;
    }

    return ret + 1;
}

char *Utils::strtok(char splitter, char *str)
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

void Utils::MakeDataDirs()
{
    
    if(!paf::io::Misc::Exists(DATA_PATH))
    	paf::io::Misc::Mkdir(DATA_PATH, 0666);
    if(!paf::io::Misc::Exists(ICON_SAVE_PATH))
		paf::io::Misc::Mkdir(ICON_SAVE_PATH, 0666);
    if(!paf::io::Misc::Exists(SCREENSHOT_SAVE_PATH))
		paf::io::Misc::Mkdir(SCREENSHOT_SAVE_PATH, 0666);
    
}

void Utils::StartBGDL()
{
    SceUID pipe = sceKernelOpenMsgPipe(BHBB_DL_PIPE_NAME);
    if(pipe > 0) return;

    sceSysmoduleLoadModule(SCE_SYSMODULE_BG_APP_UTIL);
    sceBgAppUtilStartBgApp(0);
}

void Utils::NetInit()
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

void Utils::ToLowerCase(char *string)
{
    //Convert to lowerCase
    for(int i = 0; string[i] != '\0'; i++)
        if(string[i] > 64 && string[i] < 91) string[i] += 0x20;
}

bool Utils::StringContains(char *h, char *n)
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

SceInt32 Utils::SetWidgetSize(Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    SceFVector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetSize(&v);
}

SceInt32 Utils::SetWidgetPosition(Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    SceFVector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetPosition(&v);
}

SceInt32 Utils::SetWidgetColor(Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a)
{
    Widget::Color col;
    col.r = r;
    col.g = g;
    col.b = b;
    col.a = a;

    return widget->SetColor(&col);
}

SceInt32 Utils::AssignButtonHandler(Widget *button, ECallback onPress, void *userDat, int id)
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

SceBool Utils::CreateTextureFromFile(graphics::Texture *tex, const char *file)
{
    if(tex == NULL) return SCE_FALSE;

    ObjectWithCleanup openResult;
    SceInt32 err;
    LocalFile::Open(&openResult, file, SCE_O_RDONLY, 0666, &err);

    if(err < 0)
        return SCE_FALSE;

    graphics::Texture::CreateFromFile(tex, mainPlugin->memoryPool, &openResult);

    openResult.cleanup->cb(openResult.object);
    delete openResult.cleanup;

    return tex->texSurface != NULL;
}

SceVoid Utils::DeleteTexture(graphics::Texture *tex, bool deletePointer)
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

SceVoid Utils::DeleteWidget(Widget *w)
{
    common::Utils::WidgetStateTransition(0, w, Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
}

#ifdef _DEBUG

SceVoid Utils::PrintAllChildren(Widget *widget, int offset)
{
    for (int i = 0; i < widget->childNum; i++)
    {
        for (int i = 0; i < offset; i++) print("-");
        print(" %d 0x%X\n", i, widget->GetChildByNum(i)->hash);
        Utils::PrintAllChildren(widget->GetChildByNum(i), offset + 4);
    }
    if(widget->childNum == 0) print("No Children to Print!\n");
}

#endif