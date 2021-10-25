#include "timemgr.hpp"
#include "main.hpp"
#include "common.hpp"
#include "configmgr.hpp"
#include "utils.hpp"

#define ABS(num) (num) < 0 ? -((num)) : (num)

void saveCurrentTime()
{  
    SceDateTime time;
    sceRtcGetCurrentClockUtc(&time);

    SceUID file = sceIoOpen(conf.db == CBPSDB ? CBPSDB_TIME_SAVE_PATH : VITADB_TIME_SAVE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    sceIoWrite(file, &time, sizeof(time));

    sceIoClose(file);
}

bool checkDownloadIcons()
{
    if((conf.db == CBPSDB ? conf.CBPSDBSettings.iconDownloadHourGap : conf.vitaDBSettings.iconDownloadHourGap) == -1)
        return 0;
    
    if(!paf::io::Misc::Exists(conf.db == CBPSDB ? CBPSDB_TIME_SAVE_PATH : VITADB_TIME_SAVE_PATH))
        return 1;
    
    SceDateTime prevTime;

    SceUID file = sceIoOpen(conf.db == CBPSDB ? CBPSDB_TIME_SAVE_PATH : VITADB_TIME_SAVE_PATH, SCE_O_RDONLY, 0666);
    sceIoRead(file, &prevTime, sizeof(prevTime));

    sceIoClose(file);

    SceDateTime currTime;

    sceRtcGetCurrentClockUtc(&currTime);

    //Get time difference in hours between 2 SceDateTime
    short hourGap =  ( ABS(currTime.year - prevTime.year) * 8760) + ( ABS(currTime.month - prevTime.month) * 730 ) + ABS(currTime.hour - prevTime.hour);
    return hourGap >= (conf.db == CBPSDB ? conf.CBPSDBSettings.iconDownloadHourGap : conf.vitaDBSettings.iconDownloadHourGap);
}