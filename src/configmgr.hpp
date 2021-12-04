#ifndef CONFIGMGR_HPP
#define CONFIGMGR_HPP

#include <kernel.h>
#include "main.hpp"

typedef struct
{   
    short downloadIconsDuringLoad;
} DBSettings;

typedef struct
{
    DB_Type db;
    DBSettings vitaDBSettings;
    DBSettings CBPSDBSettings;
} userConfig;

void WriteConfig(userConfig *conf);
void WriteDefaultConfig();
void GetConfig(userConfig *confOut);

#endif