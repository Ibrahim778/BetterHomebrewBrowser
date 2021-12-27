#ifndef MAIN_HPP
#define MAIN_HPP

#include <kernel.h>
#include <appmgr.h>
#include <stdio.h>
#include <curl/curl.h>

#ifdef _DEBUG
#define print sceClibPrintf
#define LOG_ERROR(prefix, error_code) print("[%s] Got Error: 0x%X\n", prefix, error_code);
#else
#define print (void)NULL;
#define LOG_ERROR(prefix, error_code) (void)NULL;
#endif

#define LOAD_FLAGS_ALL 0xFFFFFFFF
#define LOAD_FLAGS_ICONS 1
#define LOAD_FLAGS_SCREENSHOTS 2

#define DOWNLOAD_ICONS_PER_TIME 5

void onReady();

typedef enum
{
    PLUGIN,
    APP,
    THEME
} AppType;

typedef enum
{
    CBPSDB,
    VITADB
} DB_Type;

void PrintFreeMem(ScePVoid);

#define BUTTON_CB(name) void name(Widget *self, SceInt32 eventID, void *userDat)
#define PAGE_CB(name) void name(void *callingPage)
#define THREAD(name) void name(void *callingPage)

#define APPS_PER_PAGE 80

#endif