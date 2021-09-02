#ifndef CONFIGMGR_HPP
#define CONFIGMGR_HPP

#include <kernel.h>
#include "main.hpp"

typedef struct
{
    short iconDownloadHourGap;
    DB_Type db;
    bool darkMode;
} userConfig;

void WriteConfig(userConfig *conf);
void WriteDefaultConfig();
void GetConfig(userConfig *confOut);

#endif