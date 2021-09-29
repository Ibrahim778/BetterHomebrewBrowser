#ifndef MAIN_HPP
#define MAIN_HPP

#include <kernel.h>
#include <appmgr.h>
#include <stdio.h>
#include <curl/curl.h>


#ifdef _DEBUG
#define print sceClibPrintf
#define LOG_ERROR(prefix, error_code) sceClibPrintf("[%s] Got Error: 0x%X\n", prefix, error_code);
#else
#define print (void)NULL;
#define LOG_ERROR(prefix, error_code) (void)NULL;
#endif

void onReady();

typedef enum
{
    PLUGIN,
    APP
} AppType;

typedef enum
{
    CBPSDB,
    VITADB
} DB_Type;

void PrintFreeMem(ScePVoid);

#define BUTTON_CB(name) void name(Widget *self, SceInt32 eventID, void *userDat)

#endif