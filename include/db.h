#ifndef DB_H
#define DB_H

#include <kernel.h>
#include <vector>

namespace db
{
    typedef struct entryInfo //pls go away
    {
        paf::string id;
        paf::string titleID;
        paf::string title;
        paf::string author;
        std::vector<paf::string> iconURL;
        paf::string iconPath;
        std::vector<paf::string> downloadURL;
        paf::string description;
        
        std::vector<paf::string> dataURL;
        paf::string dataPath;
        
        std::vector<paf::string> screenshotURL;
        
        paf::string version;

        paf::ui::ImageButton *button;

        int type;

        SceUInt64 hash;

        entryInfo():hash(0xDEADBEEF){}
    } entryInfo;

    typedef struct 
    {
        int id;
        const char *nameID;
    } Category;

    class List
    {
    public:
        List();
        ~List();

        //void Init(int size);
        void Clear();
        void Add(db::entryInfo &entry);

        size_t GetSize(int category = -1);
        entryInfo &Get(int index);
        std::vector<db::entryInfo>::iterator Get(int index, int category);
        entryInfo &Get(SceUInt64 hash);
        
        std::vector<db::entryInfo> entries;
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

    typedef enum // Should match the index in info[]
    {
        CBPSDB = 0,
        VITADB = 1,
        VHBDB = 2,
    } Id;

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
            .iconFolderPath = "ux0:data/betterHomebrewBrowser/icons/cbpsdb",
            .iconsURL = "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/raw/main/icons.zip?raw=true",
            .indexURL = "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv",
            .indexPath = "ux0:temp/cbpsdb.csv",
            .ScreenshotsSupported = false,
            .CategoriesSupported = false,
            .categoryNum = 1,
            .categories = {
                {
                    .id = -1,
                    .nameID = "db_category_all"
                }
            },
            .id = CBPSDB
        },
        {   //Vita DB
            .Parse = vitadb::Parse, 
            .GetDescription = vitadb::GetDescription,
            .GetDownloadUrl = vitadb::GetDownloadUrl,
            .GetDataUrl = vitadb::GetDataUrl,
            .name = "Vita DB", 
            .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vitadb", 
            .iconsURL = "https://vitadb.rinnegatamante.it/icons_zip.php", 
            .indexURL = "https://rinnegatamante.it/vitadb/list_hbs_json.php",
            .indexPath = "ux0:temp/vitadb.json",
            .ScreenshotsSupported = true,
            .CategoriesSupported = true,
            .categoryNum = 5,
            .categories = {
                {
                    .id = -1, 
                    .nameID = "db_category_all"
                },
                {
                    .id = 1,
                    .nameID = "db_category_game"
                },
                {
                    .id = 2,
                    .nameID = "db_category_port"
                },
                {
                    .id = 5,
                    .nameID = "db_category_emu"
                },
                {
                    .id = 4,
                    .nameID = "db_category_util"
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
            .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vhbdb",
            .iconsURL = SCE_NULL,
            .indexURL = "https://github.com/vhbd/database/releases/download/latest/db_minify.json",
            .indexPath = "ux0:temp/vhbdb.json",
            .ScreenshotsSupported = false,
            .CategoriesSupported = true,
            .categoryNum = 4,
            .categories = {
                {
                    .id = -1,
                    .nameID = "db_category_all"
                },
                {
                    .id = 0,
                    .nameID = "db_category_app"
                },
                {
                    .id = 1,
                    .nameID = "db_category_game"
                },
                {
                    .id = 2,
                    .nameID = "db_category_emu"
                }
            },
            .id = VHBDB
        }
    };
};

#endif