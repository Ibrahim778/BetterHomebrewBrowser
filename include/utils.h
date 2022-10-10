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
    paf::ui::Widget *GetChildByHash(paf::ui::Widget *parent, SceUInt32 hash);
    SceInt32 DownloadFile(const char *url, const char *destination, void *callingPage, paf::ui::ProgressBar *progressBar = NULL);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, const char *text);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, paf::string *text);
    SceVoid DeleteTexture(paf::graph::Surface **tex);
    paf::ui::Widget *CreateWidget(const char *id, const char *type, const char *style, paf::ui::Widget *parent);
    SceVoid StartBGDL();

#ifdef _DEBUG
    SceVoid PrintAllChildren(paf::ui::Widget *widget, int offset = 0);
#endif

};
#endif