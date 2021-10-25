#include "configmgr.hpp"
#include "utils.hpp"
#include "common.hpp"

void WriteConfig(userConfig *conf)
{
    SceUID file = sceIoOpen(CONFIG_SAVE_PATH, SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0666);
    sceIoWrite(file, conf, sizeof(userConfig));
    sceIoClose(file);
}

void WriteDefaultConfig()
{
    userConfig conf;
    conf.db = userDbDefault;
    conf.CBPSDBSettings.iconDownloadHourGap = userDownloadIconGapDefault;
    conf.vitaDBSettings.iconDownloadHourGap = userDownloadIconGapDefault;

    SceUID file = sceIoOpen(CONFIG_SAVE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    sceIoWrite(file, &conf, sizeof(conf));
    sceIoClose(file);
}

void GetConfig(userConfig *confOut)
{
    if(!paf::io::Misc::Exists(CONFIG_SAVE_PATH))
    {
        WriteDefaultConfig();
        GetConfig(confOut);
    }
    else
    {
        SceUID file = sceIoOpen(CONFIG_SAVE_PATH, SCE_O_RDONLY, 0);
        sceIoRead(file, confOut, sizeof(userConfig));
        sceIoClose(file);
    }
}
