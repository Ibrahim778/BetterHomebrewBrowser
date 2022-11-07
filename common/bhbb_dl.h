#ifndef BHBB_DL_H
#define BHBB_DL_H

#define BHBB_DL_MAGIC 'BG'
#define BHBB_DL_CFG_VER 2
#define BHBB_DL_PIPE_NAME "BGVPK::RxPipe"

typedef struct cBGDLTItem {
    char name[0x50];
    char url[0x100];
    char dest[0x200];
} cBGDLItem;

typedef enum 
{
    CustomPath = 0,
    App = 1
} BGDLTarget;

typedef struct BGDLParam
{
    uint16_t magic;     // bhbb_dl magic | cfg version
    uint8_t type;       // 0 - custom path, 1 - app
    char path[256];     // This is used when type is set to 0 (custom path)
} BGDLParam;

#endif