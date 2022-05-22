#ifndef DB_HPP
#define DB_HPP

#include <kernel.h>

namespace db
{
    namespace vitadb
    {
        void Parse(const char *json);
        void GetScreenshotURL(int id,  char *out, int maxBuffSize);
    };

    namespace cbpsdb
    {
        void Parse(const char *csv);
    };

    typedef enum // Should match the index in info[]
    {
        VITADB = 0,
        CBPSDB = 1
    } Id;

    typedef struct
    {
        void (*Parse)(const char *data);
        void (*GetScreenshotURL)(void *id,  char *out, int maxBuffSize);
        const char *name;
        const char *iconFolderPath;
        const char *iconsURL;
        const char *indexURL;
        bool ScreenshotsSuppourted;
        int id;
    } dbInfo;

    static const dbInfo info[] =
    {
        {   //Vita DB
            .Parse = vitadb::Parse, 
            .GetScreenshotURL = (void (*)(void *, char *, int))vitadb::GetScreenshotURL,
            .name = "Vita DB", 
            .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vitadb", 
            .iconsURL = "https://bhbb-wrapper.herokuapp.com/icon_zip", 
            .indexURL = "https://bhbb-wrapper.herokuapp.com/index.json",
            .ScreenshotsSuppourted = true,
            .id = VITADB //index in info[]
        },
        {   //CBPS DB
            .Parse = cbpsdb::Parse,
            .GetScreenshotURL = SCE_NULL,
            .name = "CBPS DB",
            .iconFolderPath = "ux0:data/betterHomebrewBrowser/icons/cbpsdb",
            .iconsURL = "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/raw/main/icons.zip?raw=true",
            .indexURL = "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv",
            .ScreenshotsSuppourted = false,
            .id = CBPSDB
        }
    };
};

#endif