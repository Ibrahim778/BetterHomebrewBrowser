#ifndef BHBB_UTILS_H
#define BHBB_UTILS_H

#include <paf.h>

namespace Utils
{
    void ToLowerCase(char *string);
    void InitMusic();
    void SetMemoryInfo();
    SceVoid GetStringFromID(const char *id, paf::string *out);
    SceVoid GetfStringFromID(const char *id, paf::string *out);
    wchar_t *GetStringPFromID(const char *id);
    SceUInt32 GetHashById(const char *id);    
    SceInt32 PlayEffect(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType animId, paf::ui::EventCallback::EventHandler animCB = (paf::ui::EventCallback::EventHandler)SCE_NULL, ScePVoid pUserData = SCE_NULL);
    SceInt32 PlayEffectReverse(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType animId, paf::ui::EventCallback::EventHandler animCB = (paf::ui::EventCallback::EventHandler)SCE_NULL, ScePVoid pUserData = SCE_NULL);
    paf::rco::Element GetParamWithHashFromId(const char *id);
    paf::rco::Element GetParamWithHash(SceUInt32 hash);
    paf::ui::Widget *GetChildByHash(paf::ui::Widget *parent, SceUInt32 hash);
    SceInt32 DownloadFile(const char *url, const char *destination, void *callingPage, paf::ui::ProgressBar *progressBar = NULL);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, const char *text);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, paf::string *text);
    SceInt32 SetWidgetPosition(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0, SceFloat w = 0);
    SceInt32 SetWidgetSize(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0.0f, SceFloat w = 0.0f);
    SceInt32 SetWidgetColor(paf::ui::Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a);
    SceVoid DeleteTexture(paf::graph::Surface **tex);
    SceBool CreateTextureFromFile(paf::graph::Surface **tex, const char *file);
    SceVoid ExtractZipFromMemory(SceUInt8 *buff, SceSize bufferSize, const char *outDir);
    SceVoid StartBGDL();

#ifdef _DEBUG
    SceVoid PrintAllChildren(paf::ui::Widget *widget, int offset = 0);
#endif

};
#endif