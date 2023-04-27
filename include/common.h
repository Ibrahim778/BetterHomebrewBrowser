#ifndef BHBB_COMMON_H
#define BHBB_COMMON_H

#include <paf.h>


#define MUSIC_PATH "pd0:data/systembgm/store.at9"

extern paf::Plugin *g_appPlugin;

extern paf::intrusive_ptr<paf::graph::Surface> g_brokenTex;
extern paf::intrusive_ptr<paf::graph::Surface> g_transparentTex;

extern int loadFlags;

extern paf::job::JobQueue *g_mainQueue;

#endif