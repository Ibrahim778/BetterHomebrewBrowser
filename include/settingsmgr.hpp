#ifndef SETTINGS_MGR_HPP
#define SETTINGS_MGR_HPP

#include <paf.h>
#include <kernel.h>
#include <app_settings.h>

class Settings 
{
private:
    sce::AppSettings *instance;
    
public:
    void Init();
};

#endif