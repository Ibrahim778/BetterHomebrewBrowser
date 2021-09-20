#include "audiomgr.hpp"
#include "main.hpp"
#include "configmgr.hpp"

extern userConfig conf;

void initMusic()
{
    SceInt32 ret = -1;

    ret = sceCustomMusicServiceInitialize(0);
    if(ret < 0) LOG_ERROR("AUDIO_INIT", ret);

    SceMusicCoreCustomOpt optParams;
    sceClibMemset(&optParams, 0, 0x10);

    optParams.flag = -1;

    ret = sceCustomMusicCoreBgmOpen((char *)MUSIC_PATH, &optParams);
    if(ret < 0) LOG_ERROR("CORE_OPEN", ret);

    ret = sceCustomMusicCoreBgmSetAudioVolume(MAX_VOL);
    if(ret < 0) LOG_ERROR("SET_VOL", ret);

    ret = sceCustomMusicCoreBgmSetParam2(1);
    if(ret < 0) LOG_ERROR("SET_PARAM_2", ret);
}

void updateMusic()
{
    SceInt32 ret = -1;
    if(conf.enableMusic)
    {
        ret = sceCustomMusicCoreSendEvent(SCE_MUSICCORE_EVENTID_DEFAULT, 0);
        if(ret < 0) LOG_ERROR("SEND_EVENT_PLAY", ret);
    }
    else
    {
        ret = sceCustomMusicCoreSendEvent(SCE_MUSICCORE_EVENTID_STOP, 0);
        if(ret < 0) LOG_ERROR("SEND_EVENT_STOP", ret);
    }
    
}