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
    paf::Resource::Element GetParamWithHashFromId(const char *id);
    paf::Resource::Element GetParamWithHash(SceUInt32 hash);
    paf::ui::Widget *GetChildByHash(paf::ui::Widget *parent, SceUInt32 hash);
    SceInt32 DownloadFile(const char *url, const char *destination, void *callingPage, paf::ui::ProgressBar *progressBar = NULL);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, const char *text);
    SceInt32 SetWidgetLabel(paf::ui::Widget *widget, paf::string& text);
    SceInt32 SetWidgetPosition(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0, SceFloat w = 0);
    SceInt32 SetWidgetSize(paf::ui::Widget *widget, SceFloat x, SceFloat y, SceFloat z = 0.0f, SceFloat w = 0.0f);
    SceInt32 SetWidgetColor(paf::ui::Widget *widget, SceFloat r, SceFloat g, SceFloat b, SceFloat a);
    SceVoid DeleteTexture(paf::graphics::Surface **tex);
    SceBool CreateTextureFromFile(paf::graphics::Surface **tex, const char *file);
    SceVoid ExtractZipFromMemory(SceUInt8 *buff, SceSize bufferSize, const char *outDir);
    SceVoid StartBGDL();

#ifdef _DEBUG
    SceVoid PrintAllChildren(paf::ui::Widget *widget, int offset = 0);
#endif

};
#endif