#ifndef DB_HPP
#define DB_HPP

#include <kernel.h>

#include "parser.hpp"

namespace db
{
    namespace vitadb
    {
        void Parse(parser::HomebrewList& list, const char *json, int length);
    };

    namespace cbpsdb
    {
        void Parse(parser::HomebrewList& list, const char *csv, int length);
    };

    typedef enum // Should match the index in info[]
    {
        CBPSDB = 0,
        VITADB = 1
    } Id;

    typedef struct
    {
        void (*Parse)(parser::HomebrewList& list, const char *data, int length);
        void (*GetScreenshotURL)(parser::HomebrewList::node *node, paf::string *out);
        const char *name;
        const char *iconFolderPath;
        const char *iconsURL;
        const char *indexURL;
        bool ScreenshotsSuppourted;
        bool CategoriesSuppourted;
        int id;
    } dbInfo;

    static const dbInfo info[] =
    {
        {   //CBPS DB
            .Parse = cbpsdb::Parse,
            .GetScreenshotURL = SCE_NULL,
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