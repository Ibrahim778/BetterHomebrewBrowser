#ifndef CONFIGMGR_HPP
#define CONFIGMGR_HPP

#include <kernel.h>
#include "main.hpp"

typedef struct
{
    DB_Type db;
    short iconDownloadHourGap;
    bool enableMusic;
    bool enableScreenshots;
    bool enableIcons;
} userConfig;

void WriteConfig(userConfig *conf);
void WriteDefaultConfig();
void GetConfig(userConfig *confOut);

#endif