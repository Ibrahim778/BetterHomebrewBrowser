#ifndef BHBB_COMMON_H
#define BHBB_COMMON_H

#include "downloader.h"
#include "pages/apps_page.h"

#define MUSIC_PATH "pd0:data/systembgm/store.at9"

extern paf::Plugin *g_appPlugin;

extern paf::graph::Surface *g_brokenTex;
extern paf::graph::Surface *g_transparentTex;

extern int loadFlags;

extern Downloader *g_downloader;
extern apps::Page *g_appsPage;

extern job::JobQueue *g_mainQueue;

#endif