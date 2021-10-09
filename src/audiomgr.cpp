#include "audiomgr.hpp"
#include "main.hpp"
#include "configmgr.hpp"

extern userConfig conf;

void initMusic()
{
    SceInt32 ret = -1;

    ret = sceMusicInternalAppInitialize(0);
    if(ret < 0) LOG_ERROR("AUDIO_INIT", ret);

    SceMusicOpt optParams;
    sceClibMemset(&optParams, 0, 0x10);

    optParams.flag = -1;

    ret = sceMusicInternalAppSetUri((char *)MUSIC_PATH, &optParams);
    if(ret < 0) LOG_ERROR("CORE_OPEN", ret);

    ret = sceMusicInternalAppSetVolume(SCE_AUDIO_VOLUME_0DB);
    if(ret < 0) LOG_ERROR("SET_VOL", ret);

    ret = sceMusicInternalAppSetRepeatMode(SCE_MUSIC_REPEAT_ONE);
    if(ret < 0) LOG_ERROR("SET_REPEAT_MODE", ret);

    ret = sceMusicInternalAppSetPlaybackCommand(SCE_MUSIC_EVENTID_DEFAULT, 0);
    if(ret < 0) LOG_ERROR("SEND_EVENT_PLAY", ret);
}