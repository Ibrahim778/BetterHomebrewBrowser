#include <kernel.h>
#include <audioout.h>
#include <ShellAudio.h>
#include <paf.h>
#include <power.h>
#include <libsysmodule.h>
#include <appmgr.h>
#include <message_dialog.h>
#include <libdeflt.h>

#include "utils.hpp"
#include "main.hpp"
#include "parser.hpp"
#include "common.hpp"
#include "network.hpp"
#include "..\bhbb_dl\src\bhbb_dl.h"

using namespace paf;

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
    {
        print("Utils::CreateTextureFromFile() -> 0x%X\n", err);
        return SCE_FALSE;
    }
    

    paf::graphics::Surface::CreateFromFile(tex, mainPlugin->memoryPool, &openResult);

    if(*tex == NULL) print("Utils::CreateTexureFromFile() Create texture failed!\n");
    return *tex != NULL;
}

SceVoid Utils::DeleteTexture(paf::graphics::Surface **tex)
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

int Utils::MsgDialog::MessagePopup(const char *message, SceMsgDialogButtonType buttonType)
{
    if(sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) return;

    SceMsgDialogParam               msgParam;
    SceMsgDialogUserMessageParam    userParam;
    SceMsgDialogResult              result;

    sceMsgDialogParamInit(&msgParam);
    msgParam.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
    msgParam.userMsgParam = &userParam;

    sce_paf_memset(&userParam, 0, sizeof(userParam));
    msgParam.userMsgParam->msg = (const SceChar8 *)message;
    msgParam.userMsgParam->buttonType = buttonType;
    
    sceMsgDialogInit(&msgParam);

    while(sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
        paf::thread::Sleep(100);

    sce_paf_memset(&result, 0, sizeof(result));
    sceMsgDialogGetResult(&result);
    sceMsgDialogTerm();

    return result.buttonId;
}

int Utils::MsgDialog::MessagePopupFromID(const char *messageID, SceMsgDialogButtonType buttonType)
{
    if(sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) return;

    SceMsgDialogParam               msgParam;
    SceMsgDialogUserMessageParam    userParam;
    SceMsgDialogResult           result;

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

    sce_paf_memset(&result, 0, sizeof(result));
    sceMsgDialogGetResult(&result);
    sceMsgDialogTerm();

    return result.buttonId;
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

void Utils::MsgDialog::InitProgress(const char *text)
{
    if(sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING) return;

    SceMsgDialogParam               msgParam;
    SceMsgDialogProgressBarParam    progParam;

    sceMsgDialogParamInit(&msgParam);
    msgParam.mode = SCE_MSG_DIALOG_MODE_PROGRESS_BAR;
    msgParam.progBarParam = &progParam;

    sce_paf_memset(&progParam, 0, sizeof(progParam));
    msgParam.progBarParam->msg = (const SceChar8 *)text;
    msgParam.progBarParam->barType = SCE_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE;

    sceMsgDialogInit(&msgParam);

    sceMsgDialogTerm();
}

void Utils::MsgDialog::UpdateProgress(float progress)
{
    if(sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_RUNNING) return;
    
    sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, progress);
}

void Utils::MsgDialog::SetProgressMsg(const char *txt)
{
    sceMsgDialogProgressBarSetMsg(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, (const SceChar8 *)txt);
}

paf::ui::Widget *Utils::TemplateOpen(paf::ui::Widget *root, SceInt32 hash)
{
    paf::Resource::Element e;
    paf::Plugin::TemplateInitParam tInit;

    e.hash = hash;

    SceBool isMainThread = paf::thread::IsMainThread();

    if (!isMainThread)
            paf::thread::s_mainThreadMutex.Lock();

    mainPlugin->TemplateOpen(root, &e, &tInit);
    if(!isMainThread)
        paf::thread::s_mainThreadMutex.Unlock();

    return root->GetChildByNum(root->childNum - 1);
}

void Utils::ExtractZipFromMemory(uint8_t *buff, size_t archiveSize, const char *outDir, bool logProgress)
{   
    if(!io::Misc::Exists(outDir)) io::Misc::CreateRecursive(outDir, 0666);

    const uint8_t *pCurrIn = buff, *pZipEnd = buff + archiveSize;

    int totalCount = 0;
    
    void *pData;
    while(sceZipGetInfo(pCurrIn, NULL, NULL, &pData) == SCE_OK)
    {
        totalCount++;
        pCurrIn = (const uint8_t *)((uintptr_t)pData + ((SceZipHeaderPK0304 *)pCurrIn)->compsize);
    }

    if(totalCount == 0) io::Misc::RemoveRecursive(outDir);

    pCurrIn = buff;
    pZipEnd = buff + archiveSize;
    int processedEntries = 0;
    while(pCurrIn < pZipEnd)
    {
        const void *pData;
        char *fileName = SCE_NULL; //File names in headers aren't null terminated, we will extract and terminate it later

        unsigned int crc;

        SceInt32 res = SCE_OK;

        SceZipHeaderPK0304 *header = (SceZipHeaderPK0304 *)pCurrIn;
        
        res = sceZipGetInfo(pCurrIn, NULL, &crc, &pData);
        if(res < 0)
        {
            print("Error sceZipGetInfo() -> 0x%X\n", res);
            break;
        }

        fileName = new char[header->fnamelen + 1];

        sce_paf_strncpy(fileName, header->filename, header->fnamelen);
        fileName[header->fnamelen] = 0;

        string outPath;
        outPath = outDir;
        outPath += "/";
        outPath += fileName;

        delete[] fileName;

        if(header->cm == 0) //Not compressed
        {
            io::File *file = new io::File();
            SceInt32 res = file->Open(outPath.data, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
            if(res == SCE_OK)
            {
                int bytesWritten = file->Write((void *)pData, header->uncompsize);
                if(bytesWritten < 0)
                    print("Only wrote %d bytes out of %d!\n", bytesWritten, header->uncompsize);
                file->Close();
            }
            else print("Unable to open file %s for writing! (0x%X)\n", outPath.data, res);
            delete file;
        }
        else if(header->cm == 8) //Compressed
        {
            char *outBuff = new char[header->uncompsize];
            sce_paf_memset(outBuff, 0, header->uncompsize);

            res = sceDeflateDecompress(outBuff, header->uncompsize, pData, NULL);
            
            if(res < 0)
                print("sceDeflateDecompress() -> 0x%X\n", res);
            else if (res != header->uncompsize)
                print("Only %d out of %d bytes were extracted!\n", res, header->uncompsize);
            else if(crc == sceGzipCrc32(0, (const unsigned char *)outBuff, res)) //Extraction successful
            {
                io::File *file = new io::File();

                SceInt32 res = file->Open(outPath.data, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
                if(res == SCE_OK)
                {
                    int bytesWritten = file->Write(outBuff, header->uncompsize);
                    if(bytesWritten != header->uncompsize)
                        print("Only able to write %d out of %d bytes!\n", bytesWritten);
                    file->Close();
                }
                else print("Unable to open file %s for writing! (0x%X)\n", outPath.data, res);
                delete file;
            }

            delete[] outBuff;
        }
        else 
            print("Cannot extract (%s)\n", fileName);

        print("Extractor: %s\n", outPath.data);

        pCurrIn = (const uint8_t *)((uintptr_t)pData + header->compsize);
        processedEntries ++;

        if(logProgress) Utils::MsgDialog::UpdateProgress(((float)processedEntries / (float)totalCount) * 100.0f);
    }
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