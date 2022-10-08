#include <kernel.h>
#include <audioout.h>
#include <ShellAudio.h>
#include <paf.h>
#include <power.h>
#include <libsysmodule.h>
#include <appmgr.h>
#include <message_dialog.h>
#include <libdeflt.h>
#include <taihen.h>

#include "utils.h"
#include "print.h"
#include "common.h"
#include "main.h"
#include "network.h"
#include "bhbb_dl.h"

using namespace paf;

static SceInt32 currStok = 0;

SceUInt32 Utils::GetHashById(const char *id)
{
    rco::Element searchReq;
    rco::Element searchRes;
    
    searchReq.id = id;
    searchRes.hash = searchRes.GetHash(&searchReq.id);

    return searchRes.hash;
}

rco::Element Utils::GetParamWithHash(SceUInt32 hash)
{
    rco::Element search;
    search.hash = hash;

    return search;
}

rco::Element Utils::GetParamWithHashFromId(const char *id)
{
    rco::Element search;

    search.hash = Utils::GetHashById(id);

    return search;
}

paf::ui::Widget *Utils::GetChildByHash(paf::ui::Widget *parent, SceUInt32 hash)
{
    rco::Element search = GetParamWithHash(hash);
    return parent->GetChild(&search, 0);
}

SceVoid Utils::GetStringFromID(const char *id, paf::string *out)
{
    rco::Element e = Utils::GetParamWithHashFromId(id);
    
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

SceInt32 Utils::SetWidgetSize(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    paf::Vector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetSize((const Vector4 *)&v);
}

SceInt32 Utils::SetWidgetPosition(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z, SceFloat w)
{
    paf::Vector4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return widget->SetPosition((const Vector4 *)&v);
}

SceInt32 Utils::SetWidgetColor(paf::ui::Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a)
{
    paf::Rgba col;
    col.r = r;
    col.g = g;
    col.b = b;
    col.a = a;
    
    return widget->SetColor((const paf::Rgba *)&col);
}

SceBool Utils::CreateTextureFromFile(paf::graph::Surface **tex, const char *file)
{
    if(tex == NULL) return SCE_FALSE;
    
    SceInt32 err;
    SharedPtr<LocalFile> ptr = paf::LocalFile::Open(file, SCE_O_RDONLY, 0666, &err);

    if(err < 0)
    {
        print("Utils::CreateTextureFromFile() -> 0x%X\n", err);
        return SCE_FALSE;
    }
    

    paf::graph::Surface::Create(tex, mainPlugin->memoryPool, (SharedPtr<File> *)&ptr);

    if(*tex == NULL) print("Utils::CreateTexureFromFile() Create texture failed!\n");
    ptr.get()->Close();
    return *tex != NULL;
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

    if(LocalFile::Exists("ux0:data/bgdlid"))
    {
        SceUID id;
        
        SharedPtr<LocalFile> openResult = LocalFile::Open("ux0:data/bgdlid", SCE_O_RDONLY, 0, NULL);
        openResult.get()->Read(&id, sizeof(SceUID));
        taiStopUnloadModuleForPid(sceShellID, id, 0, NULL, 0, NULL, NULL);
    }

    res = taiLoadStartModuleForPid(sceShellID, "ux0:app/BHBB00001/module/bhbb_dl.suprx", 0, NULL, 0);
    if(res < 0)
    {
        print("Unable to start BGDL! (Already Running?) 0x%X\n", res);
        return;
    }

    moduleID = res;
    
    print("BGDL started with ID 0x%X\n", moduleID);

    SharedPtr<LocalFile> openResult = LocalFile::Open("ux0:data/bgdlid", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, NULL);
    openResult.get()->Write(&moduleID, sizeof(SceUID));

}

ui::Widget *Utils::CreateWidget(const char *id, const char *type, const char *style, ui::Widget *parent)
{
    rco::Element styleInfo = Utils::GetParamWithHashFromId(style);
    rco::Element widgetInfo = Utils::GetParamWithHashFromId(id);

    return mainPlugin->CreateWidgetWithStyle(parent, type, &widgetInfo, &styleInfo);
}

SceVoid Utils::ExtractZipFromMemory(SceUInt8 *buff, SceSize archiveSize, const char *outDir)
{   
    print("[FATAL ERROR] Called Utils::ExtractZipFromMemory! Function has been removed ABORT!\n");
    *(int *)0 = 0;

    // if(!LocalFile::Exists(outDir)) Dir::CreateRecursive(outDir, 0666);

    // const uint8_t *pCurrIn = buff, *pZipEnd = buff + archiveSize;

    // int totalCount = 0;
    
    // void *pData;
    // while(sceZipGetInfo(pCurrIn, NULL, NULL, &pData) == SCE_OK)
    // {
    //     totalCount++;
    //     pCurrIn = (const uint8_t *)((uintptr_t)pData + ((SceZipHeaderPK0304 *)pCurrIn)->compsize);
    // }

    // if(totalCount == 0) Dir::RemoveRecursive(outDir);

    // pCurrIn = buff;
    // pZipEnd = buff + archiveSize;
    // int processedEntries = 0;
    // while(pCurrIn < pZipEnd)
    // {
    //     const void *pData;
    //     char *fileName = SCE_NULL; //File names in headers aren't null terminated, we will extract and terminate it later

    //     unsigned int crc;

    //     SceInt32 res = SCE_OK;

    //     SceZipHeaderPK0304 *header = (SceZipHeaderPK0304 *)pCurrIn;
        
    //     res = sceZipGetInfo(pCurrIn, NULL, &crc, &pData);
    //     if(res < 0)
    //     {
    //         print("Error sceZipGetInfo() -> 0x%X\n", res);
    //         break;
    //     }

    //     fileName = new char[header->fnamelen + 1];

    //     sce_paf_strncpy(fileName, header->filename, header->fnamelen);
    //     fileName[header->fnamelen] = 0;

    //     string outPath;
    //     outPath = outDir;
    //     outPath += "/";
    //     outPath += fileName;

    //     delete[] fileName;

    //     if(header->cm == 0) //Not compressed
    //     {
    //         io::File *file = new io::File();
    //         SceInt32 res = file->Open(outPath.data, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    //         if(res == SCE_OK)
    //         {
    //             int bytesWritten = file->Write((void *)pData, header->uncompsize);
    //             if(bytesWritten < 0)
    //                 print("Only wrote %d bytes out of %d!\n", bytesWritten, header->uncompsize);
    //             file->Close();
    //         }
    //         else print("Unable to open file %s for writing! (0x%X)\n", outPath.data, res);
    //         delete file;
    //     }
    //     else if(header->cm == 8) //Compressed
    //     {
    //         char *outBuff = new char[header->uncompsize];
    //         sce_paf_memset(outBuff, 0, header->uncompsize);

    //         res = sceDeflateDecompress(outBuff, header->uncompsize, pData, NULL);
            
    //         if(res < 0)
    //         {
    //            print("sceDeflateDecompress() -> 0x%X\n", res);
    //         }
    //         else if (res != header->uncompsize)
    //         {
    //             print("Only %d out of %d bytes were extracted!\n", res, header->uncompsize);
    //         }
    //         else if(crc == sceGzipCrc32(0, (const unsigned char *)outBuff, res)) //Extraction successful
    //         {
    //             io::File *file = new io::File();

    //             SceInt32 res = file->Open(outPath.data, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    //             if(res == SCE_OK)
    //             {
    //                 int bytesWritten = file->Write(outBuff, header->uncompsize);
    //                 if(bytesWritten != header->uncompsize)
    //                     print("Only able to write %d out of %d bytes!\n", bytesWritten);
    //                 file->Close();
    //             }
    //             else print("Unable to open file %s for writing! (0x%X)\n", outPath.data, res);
    //             delete file;
    //         }

    //         delete[] outBuff;
    //     }
    //     else 
    //         print("Cannot extract (%s)\n", fileName);

    //     print("Extractor: %s\n", outPath.data);

    //     pCurrIn = (const uint8_t *)((uintptr_t)pData + header->compsize);
    //     processedEntries ++;
    // }
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