#include "configmgr.hpp"
#include "utils.hpp"
#include "common.hpp"

short userDownloadIconGapDefault = 6;
DB_Type userDbDefault = VITADB;

void WriteConfig(userConfig *conf)
{
    SceUID file = sceIoOpen(CONFIG_SAVE_PATH, SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777);
    sceIoWrite(file, conf, sizeof(userConfig));
    sceIoClose(file);
}

void WriteDefaultConfig()
{
    userConfig conf;
    conf.enableScreenshots = false;
    conf.enableIcons = true;
    conf.enableMusic = true;
    conf.db = userDbDefault;
    conf.iconDownloadHourGap = userDownloadIconGapDefault;

    SceUID file = sceIoOpen(CONFIG_SAVE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    sceIoWrite(file, &conf, sizeof(conf));
    sceIoClose(file);
}

void GetConfig(userConfig *confOut)
{
    if(!checkFileExist(CONFIG_SAVE_PATH))
    {
        confOut->db = userDbDefault;
        confOut->iconDownloadHourGap = userDownloadIconGapDefault;
        confOut->enableMusic = false;

        WriteDefaultConfig();
    }
    else
    {
        SceUID file = sceIoOpen(CONFIG_SAVE_PATH, SCE_FIOS_O_RDONLY, 0777);
        sceIoRead(file, confOut, sizeof(userConfig));
        sceIoClose(file);
    }
}
