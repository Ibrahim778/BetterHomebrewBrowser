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
    char *dbIconPath = NULL;

    switch (conf.db)
    {
    case CBPSDB:
        dbIconPath = CBPSDB_ICON_SAVE_PATH;
        break;
    
    case VITADB:
        dbIconPath = VITADB_ICON_SAVE_PATH;
        break;

    default:
        break;
    }

    if(!paf::io::Misc::Exists(dbIconPath))
        return 1;
    
    return Utils::isDirEmpty(dbIconPath);
}