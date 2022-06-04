#ifndef DB_HPP
#define DB_HPP

#include <kernel.h>

namespace db
{
    typedef enum
    {
        GAME = 1,
        PORT = 2,
        EMULATOR = 5,
        UTIL = 4
    } Category;

    typedef struct
    {
        paf::string id;
        paf::string titleID;
        paf::string title;
        paf::string credits;
        paf::string icon0;
        paf::string icon0Local;
        paf::string download_url;
        paf::string options;
        paf::string description;
        
        paf::string *screenshot_urls;
        int screenshot_url_num;

        paf::string version;
        paf::string size;

        paf::ui::ImageButton *button;
        paf::graphics::Surface *tex;

        int type;

        SceUInt64 hash;

    } entryInfo;

    namespace vitadb
    {
        void Parse(db::entryInfo **outList, int *outListNum, const char *json, int length);
    };

    namespace cbpsdb
    {
        void Parse(db::entryInfo **outList, int *outListNum, const char *csv, int length);
    };

    typedef enum // Should match the index in info[]
    {
        CBPSDB = 0,
        VITADB = 1
    } Id;

    typedef struct
    {
        void (*Parse)(db::entryInfo **outList, int *outListNum, const char *data, int length);
        const char *name;
        const char *iconFolderPath;
        const char *iconsURL;
        const char *indexURL;
        bool ScreenshotsSuppourted;
        bool CategoriesSuppourted;
        int id;
    } dbInfo;

    int GetNumByCategory(db::entryInfo *list, int listNum, int category);
    db::entryInfo *GetByCategoryIndex(db::entryInfo *list, int listNum, int index, int category);

    static const dbInfo info[] =
    {
        {   //CBPS DB
            .Parse = cbpsdb::Parse,
            .name = "CBPS DB",
            .iconFolderPath = "ux0:data/betterHomebrewBrowser/icons/cbpsdb",
            .iconsURL = "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/raw/main/icons.zip?raw=true",
            .indexURL = "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv",
            .ScreenshotsSuppourted = false,
            .CategoriesSuppourted = false,
            .id = CBPSDB
        },
        {   //Vita DB
            .Parse = vitadb::Parse, 
            .name = "Vita DB", 
            .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vitadb", 
            .iconsURL = "https://bhbb-wrapper.herokuapp.com/icon_zip", 
            .indexURL = "https://bhbb-wrapper.herokuapp.com/index.json",
            .ScreenshotsSuppourted = true,
            .CategoriesSuppourted = true,
            .id = VITADB //index in info[]
        }
    };
};

#endif