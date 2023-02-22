#ifndef DB_H
#define DB_H

#include <kernel.h>
#include <vector>
#include <paf.h>

#include "bhbb_locale.h"

namespace db
{
    typedef struct entryInfo //pls go away
    {
        paf::string id;
        paf::string titleID;
        paf::string title;
        paf::string author;
        paf::vector<paf::string> iconURL;
        paf::string iconPath;
        paf::vector<paf::string> downloadURL;
        paf::string description;
        
        paf::vector<paf::string> dataURL;
        paf::string dataPath;
        
        paf::vector<paf::string> screenshotURL;
        
        paf::string version;

        paf::graph::Surface *surface;

        int type;

        SceUInt64 hash;

        entryInfo():hash(0xDEADBEEF),surface(SCE_NULL){}
    } entryInfo;

    typedef struct 
    {
        int id;
        SceUInt64 nameHash; //Example: Apps
        SceUInt64 singleHash; //Example: App
    } Category;

    typedef enum // Should match the index in info[]
    {
        CBPSDB = 0,
        VITADB = 1,
        VHBDB = 2,
    } Id;
    
    class List
    {
    public:
        struct CategorisedList
        {
            CategorisedList(int _category):category(_category) {}

            int category;
            paf::vector<db::entryInfo *> entries;    
        };

        List();
        ~List();

        void Clear();
        void Add(db::entryInfo &entry);
        void Categorise(db::Id source);

        size_t GetSize(int category = -1);
        entryInfo &Get(SceUInt64 hash);
        CategorisedList &GetCategory(int category);
        
        paf::vector<db::entryInfo> entries;
        paf::vector<CategorisedList> categories;
    };

    namespace vitadb
    {
        SceInt32 Parse(db::List *outList, const char *jsonPath);
        SceInt32 GetDescription(db::entryInfo &entry, paf::string &out);
        SceInt32 GetDownloadUrl(db::entryInfo &entry, paf::string &out);
        SceInt32 GetDataUrl(db::entryInfo &entry, paf::string &out);
    };

    namespace cbpsdb
    {
        SceInt32 Parse(db::List *outList, const char *csvPath);
        SceInt32 GetDescription(db::entryInfo &entry, paf::string &out);
        SceInt32 GetDownloadUrl(db::entryInfo &entry, paf::string &out);
        SceInt32 GetDataUrl(db::entryInfo &entry, paf::string &out);
    };

    namespace vhbdb
    {
        SceInt32 Parse(db::List *outList, const char *jsonPath);
        SceInt32 GetDescription(db::entryInfo &entry, paf::string &out);
        SceInt32 GetDownloadUrl(db::entryInfo &entry, paf::string &out);
        SceInt32 GetDataUrl(db::entryInfo &entry, paf::string &out);
    }

    typedef struct
    {
        SceInt32 (*Parse)(db::List *outList, const char *path);
        SceInt32 (*GetDescription)(db::entryInfo& entry, paf::string& out);
        SceInt32 (*GetDownloadUrl)(db::entryInfo& entry, paf::string& out);
        SceInt32 (*GetDataUrl)(db::entryInfo& entry, paf::string& out);
        const char *name;
        const char *iconFolderPath;
        const char *iconsURL;
        const char *indexURL;
        const char *indexPath;
        bool ScreenshotsSupported;
        bool CategoriesSupported;
        const int categoryNum;
        Category categories[5];
        const int id;
    } dbInfo;

    static const int CategoryAll = -1;

    static const dbInfo info[] =
    {
        {   //CBPS DB
            .Parse = cbpsdb::Parse,
            .GetDescription = cbpsdb::GetDescription,
            .GetDownloadUrl = cbpsdb::GetDownloadUrl,
            .GetDataUrl = cbpsdb::GetDataUrl,
            .name = "CBPS DB",
            .iconFolderPath = "ux0:data/betterHomebrewBrowser/icons/cbpsdb/",
            .iconsURL = "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/raw/main/icons.zip?raw=true",
            .indexURL = "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv",
            .indexPath = "ux0:temp/cbpsdb.csv",
            .ScreenshotsSupported = false,
            .CategoriesSupported = false,
            .categoryNum = 1,
            .categories = {
                {
                    .id = CategoryAll,
                    .nameHash = db_category_all,
                    .singleHash = db_category_all
                },
            },
            .id = CBPSDB
        },
        {   //Vita DB
            .Parse = vitadb::Parse, 
            .GetDescription = vitadb::GetDescription,
            .GetDownloadUrl = vitadb::GetDownloadUrl,
            .GetDataUrl = vitadb::GetDataUrl,
            .name = "Vita DB", 
            .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vitadb/", 
            .iconsURL = "https://vitadb.rinnegatamante.it/icons_zip.php", 
            .indexURL = "https://rinnegatamante.it/vitadb/list_hbs_json.php",
            .indexPath = "ux0:temp/vitadb.json",
            .ScreenshotsSupported = true,
            .CategoriesSupported = true,
            .categoryNum = 5,
            .categories = {
                {
                    .id = CategoryAll, 
                    .nameHash = db_category_all,
                    .singleHash = db_category_all
                },
                {
                    .id = 1,
                    .nameHash = db_category_game,
                    .singleHash = db_category_single_game
                },
                {
                    .id = 2,
                    .nameHash = db_category_port,
                    .singleHash = db_category_single_port
                },
                {
                    .id = 5,
                    .nameHash = db_category_emu,
                    .singleHash = db_category_single_emu
                },
                {
                    .id = 4,
                    .nameHash = db_category_util,
                    .singleHash = db_category_single_util
                }
            },
            .id = VITADB //index in info[]
        },
        {   //Vita Homebrew DB
            .Parse = vhbdb::Parse,
            .GetDescription = vhbdb::GetDescription,
            .GetDownloadUrl = vhbdb::GetDownloadUrl,
            .GetDataUrl = vhbdb::GetDataUrl,
            .name = "VHB DB",
            .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vhbdb/",
            .iconsURL = SCE_NULL,
            .indexURL = "https://github.com/vhbd/database/releases/download/latest/db_minify.json",
            .indexPath = "ux0:temp/vhbdb.json",
            .ScreenshotsSupported = false,
            .CategoriesSupported = true,
            .categoryNum = 4,
            .categories = {
                {
                    .id = CategoryAll,
                    .nameHash = db_category_all,
                    .singleHash = db_category_all
                },
                {
                    .id = 0,
                    .nameHash = db_category_app,
                    .singleHash = db_category_single_app
                },
                {
                    .id = 1,
                    .nameHash = db_category_game,
                    .singleHash = db_category_single_game
                },
                {
                    .id = 2,
                    .nameHash = db_category_emu,
                    .singleHash = db_category_single_emu
                }
            },
            .id = VHBDB
        }
    };
};

#endif