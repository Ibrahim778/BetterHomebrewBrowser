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

    typedef enum
    {
        GAME = 1,
        PORT = 2,
        EMULATOR = 5,
        UTIL = 4
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
        void Parse(db::List *outList, paf::string& jsonStr);
        SceInt32 GetDescription(db::entryInfo &entry, paf::string &out);
    };

    namespace cbpsdb
    {
        void Parse(db::List *outList, paf::string& csv);
        SceInt32 GetDescription(db::entryInfo &entry, paf::string &out);
    };

    namespace vhbdb
    {
        void Parse(db::List *outList, paf::string& jsonStr);
        SceInt32 GetDescription(db::entryInfo &entry, paf::string &out);
    }

    typedef enum // Should match the index in info[]
    {
        CBPSDB = 0,
        VITADB = 1,
        VHBDB = 2,
    } Id;

    typedef struct
    {
        void (*Parse)(db::List *outList, paf::string& data);
        SceInt32 (*GetDescription)(db::entryInfo& entry, paf::string& out);
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
            .GetDescription = cbpsdb::GetDescription,
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
            .GetDescription = vitadb::GetDescription,
            .name = "Vita DB", 
            .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vitadb", 
            .iconsURL = "https://bhbb-wrapper.herokuapp.com/icon_zip", 
            .indexURL = "https://rinnegatamante.it/vitadb/list_hbs_json.php",
            .ScreenshotsSuppourted = true,
            .CategoriesSuppourted = true,
            .id = VITADB //index in info[]
        },
        // {   //Vita Homebrew DB
        //     .Parse = vhbdb::Parse,
        //     .GetDescription = vhbdb::GetDescription,
        //     .name = "VHB DB",
        //     .iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vhbdb",
        //     .iconsURL = SCE_NULL,
        //     .indexURL = "https://github.com/vhbd/database/releases/download/latest/db_minify.json",
        //     .ScreenshotsSuppourted = false,
        //     .CategoriesSuppourted = false,
        //     .id = VHBDB
        // }
    };
};

#endif