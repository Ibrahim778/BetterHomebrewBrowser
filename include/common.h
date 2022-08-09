#ifndef BHBB_COMMON_H
#define BHBB_COMMON_H

#include "downloader.h"

#define MUSIC_PATH "pd0:data/systembgm/store.at9"

extern paf::Plugin *mainPlugin;

extern paf::graph::Surface *BrokenTex;
extern paf::graph::Surface *TransparentTex;

extern int loadFlags;

extern paf::ui::CornerButton *g_backButton;
extern paf::ui::CornerButton *g_forwardButton;
extern paf::ui::BusyIndicator *g_busyIndicator;

extern Downloader *g_downloader;

extern wchar_t *g_versionInfo;


#endif