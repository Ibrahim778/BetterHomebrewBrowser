#ifndef BHBB_COMMON_H
#define BHBB_COMMON_H

#include "pages/apps_page.h"
#include "downloader.h"

#define MUSIC_PATH "pd0:data/systembgm/store.at9"

extern paf::Plugin *mainPlugin;

extern paf::graphics::Surface *BrokenTex;
extern paf::graphics::Surface *TransparentTex;

extern int loadFlags;

extern paf::ui::CornerButton *g_backButton;
extern paf::ui::CornerButton *g_forwardButton;
extern paf::ui::BusyIndicator *g_busyIndicator;

extern apps::Page *g_appsPage;

extern Downloader *g_downloader;

extern SceWChar16 *g_versionInfo;


#endif