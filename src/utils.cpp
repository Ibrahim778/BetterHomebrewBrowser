#include <kernel.h>
#include <bgapputil.h>
#include <audioout.h>
#include <ShellAudio.h>
#include <curl/curl.h>
#include <paf.h>
#include <power.h>
#include <libsysmodule.h>
#include <appmgr.h>
#include <message_dialog.h>

#include "utils.hpp"
#include "main.hpp"
#include "parser.hpp"
#include "common.hpp"
#include "network.hpp"
#include "..\bhbb_dl\src\bhbb_dl.h"

static CURL *curl = SCE_NULL;
static SceInt32 currStok = 0;

SceUInt32 Utils::GetHashById(const char *id)
{
    paf::Resource::Element searchReq;
    paf::Resource::Element searchRes;
    
    searchReq.id = id;
    searchRes.hash = searchRes.GetHashById(&searchReq);

    return searchRes.hash;
}

paf::Resource::Element Utils::GetParamWithHash(SceUInt32 hash)
{
    paf::Resource::Element search;
    search.hash = hash;

    return search;
}

paf::Resource::Element Utils::GetParamWithHashFromId(const char *id)
{
    paf::Resource::Element search;

    search.hash = Utils::GetHashById(id);

    return search;
}

paf::ui::Widget * Utils::GetChildByHash(paf::ui::Widget *parent, SceUInt32 hash)
{
    paf::Resource::Element search = GetParamWithHash(hash);
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

int curlProgressCallback(void *userDat, double dltotal, double dlnow, double ultotal, double ulnow)
{
    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);

    return 0; //Signal Continue
}

size_t curlWriteCB(void *datPtr, size_t chunkSize, size_t chunkNum, void *userDat)
{
    if(userDat == NULL)
    {
        print("Error no userDat provided in curlWriteCB()!\n");
        return 0;
    }


    return sceIoWrite(*(int *)userDat, datPtr, chunkSize * chunkNum);
}

SceInt32 Utils::DownloadFile(const char *url, const char *dest, void *callingPage, paf::ui::ProgressBar *progressBar)
{
    if(curl == NULL) return -1;

    SceUID file = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if(file < 0) return file;

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    CURLcode ret = curl_easy_perform(curl);

    sceIoClose(file);
    if(ret != CURLE_OK) sceIoRemove(dest);
    return (int)ret;
}

SceVoid Utils::GetStringFromID(const char *id, paf::string *out)
{
    paf::Resource::Element e = Utils::GetParamWithHashFromId(id);
    SceWChar16 *wstr = mainPlugin->GetString(&e);
    paf::string::WCharToNewString((const wchar_t *)wstr, out);
}

SceVoid Utils::GetfStringFromID(const char *id, paf::string *out)
{
    paf::string *str = new paf::string;
    Utils::GetStringFromID(id, str);

    int slashNum = 0;
    for(int i = 0; i < str->length + 1 && str->data[i] != '\0'; i++)
        if(str->data[i] == '\\') slashNum++;

    int buffSize = (str->length + 1) - slashNum;
    char *buff = new char[buffSize];
    sce_paf_memset(buff, 0, buffSize);

    for(char *buffPtr = buff, *strPtr = str->data; *strPtr != '\0'; strPtr++, buffPtr++)
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
                case '\?':
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

SceInt32 Utils::SetWidgetLabel(paf::ui::Widget *widget, const char *text)
{
    paf::wstring wstr;
    paf::wstring::CharToNewWString(text, &wstr);    
    return widget->SetLabel(&wstr);
}

SceInt32 Utils::SetWidgetLabel(paf::ui::Widget *widget, paf::string *text)
{
    paf::wstring wstr;
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

void Utils::NetTerm()
{
    if(curl != NULL)
        curl_easy_cleanup(curl);
}

void Utils::ToLowerCase(char *string)
{
    //Convert to lowerCase
    for(int i = 0; string[i] != '\0'; i++)
        if(string[i] > 64 && string[i] < 91) string[i] += 0x20;
}

bool Utils::stringContains(char *h, char *n)
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

void Utils::InitMusic()
{
    SceInt32 ret = -1;

    ret = sceMusicInternalAppInitialize(0);
    if(ret < 0) LOG_ERROR("AUDIO_INIT", ret);

    SceMusicOpt optParams;
    sce_paf_memset(&optParams, 0, 0x10);

    optParams.flag = -1;

    ret = sceMusicInternalAppSetUri((char *)MUSIC_PATH, &optParams);
    if(ret < 0) LOG_ERROR("CORE_OPEN", ret);

    ret = sceMusicInternalAppSetVolume(SCE_AUDIO_VOLUME_0DB);
    if(ret < 0) LOG_ERROR("SET_VOL", ret);

    ret = sceMusicInternalAppSetRepeatMode(SCE_MUSIC_REPEAT_ONE);
    if(ret < 0) LOG_ERROR("SET_REPEAT_MODE", ret);

    ret = sceMusicInternalAppSetPlaybackCommand(SCE_MUSIC_EVENTID_DEFAULT, 0);
    if(ret < 0) LOG_ERROR("SEND_EVENT_PLAY", ret);
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

SceInt32 Utils::SetWidgetSize(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    SceFVector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetSize(&v);
}

SceInt32 Utils::SetWidgetPosition(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    SceFVector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetPosition(&v);
}

SceInt32 Utils::SetWidgetColor(paf::ui::Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a)
{
    paf::ui::Widget::Color col;
    col.r = r;
    col.g = g;
    col.b = b;
    col.a = a;

    return widget->SetColor(&col);
}

SceBool Utils::CreateTextureFromFile(paf::graphics::Surface **tex, const char *file)
{
    if(tex == NULL) return SCE_FALSE;

    paf::shared_ptr<paf::LocalFile> openResult;
    SceInt32 err;
    paf::LocalFile::Open(&openResult, file, SCE_O_RDONLY, 0666, &err);

    if(err < 0)
        return SCE_FALSE;

    paf::graphics::Surface::CreateFromFile(tex, mainPlugin->memoryPool, &openResult);

    return *tex != NULL;
}

SceVoid Utils::DeleteTexture(paf::graphics::Surface **tex, bool deletePointer)
{
    if(tex != NULL)
    {
        if(*tex == TransparentTex) return;
        if(*tex != NULL)
        {
            paf::graphics::Surface *s = *tex;
            *tex = SCE_NULL;
            delete s;
        }
        
    }
}

SceVoid Utils::DeleteWidget(paf::ui::Widget *w)
{
    paf::common::Utils::WidgetStateTransition(0, w, paf::ui::Widget::Animation_Reset, SCE_TRUE, SCE_TRUE);
}

void Utils::MsgDialog::MessagePopup(const char *message, SceMsgDialogButtonType buttonType)
{
    if(sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) return;

    SceMsgDialogParam               msgParam;
    SceMsgDialogUserMessageParam    userParam;

    sceMsgDialogParamInit(&msgParam);
    msgParam.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
    msgParam.userMsgParam = &userParam;

    sce_paf_memset(&userParam, 0, sizeof(userParam));
    msgParam.userMsgParam->msg = (const SceChar8 *)message;
    msgParam.userMsgParam->buttonType = buttonType;
    
    sceMsgDialogInit(&msgParam);

    while(sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
        paf::thread::Sleep(100);
    
    sceMsgDialogTerm();
}

void Utils::MsgDialog::MessagePopupFromID(const char *messageID, SceMsgDialogButtonType buttonType)
{
    if(sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) return;

    SceMsgDialogParam               msgParam;
    SceMsgDialogUserMessageParam    userParam;

    sceMsgDialogParamInit(&msgParam);
    msgParam.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
    msgParam.sysMsgParam = SCE_NULL;
    
    sce_paf_memset(&userParam, 0, sizeof(userParam));

    msgParam.userMsgParam = &userParam;

    paf::string message;
    Utils::GetfStringFromID(messageID, &message);

    msgParam.userMsgParam->msg = (const SceChar8 *)message.data;
    msgParam.userMsgParam->buttonType = buttonType;
    
    sceMsgDialogInit(&msgParam);

    while(sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
        paf::thread::Sleep(100);

    
    sceMsgDialogTerm();
}

void Utils::MsgDialog::SystemMessage(SceMsgDialogSystemMessageType type)
{
    if(sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) return;

    SceMsgDialogParam                 msgParam;
    SceMsgDialogSystemMessageParam    sysParam;

    sceMsgDialogParamInit(&msgParam);
    msgParam.mode = SCE_MSG_DIALOG_MODE_SYSTEM_MSG;
    msgParam.sysMsgParam = &sysParam;

    sce_paf_memset(&sysParam, 0, sizeof(sysParam));
    sysParam.sysMsgType = type;
    
    sceMsgDialogInit(&msgParam);
}

void Utils::MsgDialog::EndMessage()
{
    while(sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_RUNNING)
        paf::thread::Sleep(100);
    
    sceMsgDialogClose();

    while(sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
        paf::thread::Sleep(100);
    
    sceMsgDialogTerm();
}

#ifdef _DEBUG

SceVoid Utils::PrintAllChildren(paf::ui::Widget *widget, int offset)
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