#include <paf.h>
#include <psp2_compat/curl/curl.h>

#include "db/vitadb.h"
#include "bhbb_locale.h"
#include "utils.h"
#include "print.h"
#include "json.h"

using namespace paf;
using namespace paf::common;

VitaDB::VitaDB()
{
    name = "Vita DB";
    iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vitadb/";
    iconsURL = "https://vitadb.rinnegatamante.it/icons_zip.php";
    indexURL = "https://rinnegatamante.it/vitadb/list_hbs_json.php";
    indexPath = "ux0:temp/vitadb.json";
    screenshotsSupported = true;
    categories = {
        Source::Category(
            CategoryAll,
            db_category_all,
            db_category_all
        ),
        Source::Category(
            1,
            db_category_single_game,
            db_category_game
        ),
        Source::Category(
            2,
            db_category_single_port,
            db_category_port
        ),
        Source::Category(
            5,
            db_category_single_emu,
            db_category_emu
        ),
        Source::Category(
            4,
            db_category_single_util,
            db_category_util
        )
    };
}

VitaDB::~VitaDB()
{
    
}

int VitaDB::DownloadIndex()
{
    // Just save file with CURL (vitadb does not offer any method to see if something changed afaik)
    return Utils::DownloadFile(indexURL.c_str(), indexPath.c_str());
}

int VitaDB::GetDownloadURL(Source::Entry& entry, paf::string& out)
{
    out = entry.downloadURL.back();
    return 0;
}

int VitaDB::GetDataURL(Source::Entry& entry, paf::string& out)
{
    out = entry.dataURL.back();
    return 0;
}

int VitaDB::GetDescription(Source::Entry& entry, paf::wstring& out)
{
    out = entry.description;
    return 0;
}

int VitaDB::Parse()
{
    pList->Clear();

    int ret = 0;
    off_t fileSize = 0;
    char *jsonData = nullptr;

    auto jsonFile = LocalFile::Open(indexPath.c_str(), SCE_O_RDONLY, 0, &ret);
    if(ret != SCE_OK)
        return ret;

    fileSize = jsonFile.get()->GetFileSize();
    
    char *jstring = new char[fileSize + 1];
    sce_paf_memset(jstring, 0, fileSize + 1);
    
    ret = jsonFile.get()->Read(jstring, fileSize);

    if(ret < 0)
    {
        delete[] jstring;
        return ret;
    }

    DynamicJsonDocument jdoc(SCE_KERNEL_1MiB);
    DeserializationError err = deserializeJson(jdoc, jstring);
    if(err != DeserializationError::Ok)
    {
        print("Error parsing JSON: %s\n", err.c_str());
        delete[] jstring;
        return err.code();
    }   
    
    size_t elemCount = jdoc.size();
    for(size_t i = 0; i < elemCount; i++)
    {
        Source::Entry entry(this);

        entry.hash = IDParam(jdoc[i]["id"].as<const char *>()).GetIDHash();
        
        if(jdoc[i]["icon"] != nullptr)
        {
            entry.iconPath = common::FormatString("%s/%s", iconFolderPath.c_str(), jdoc[i]["icon"].as<const char *>());
            // common::FormatString("https://rinnegatamante.it/vitadb/icons/%s", jdoc[i]["icon"].as<const char *>())
            entry.iconURL.push_back("Hello?");
        }

        common::Utf8ToUtf16(jdoc[i]["titleid"].as<const char *>(), &entry.titleID);
        common::Utf8ToUtf16(jdoc[i]["author"].as<const char *>(), &entry.author);
        common::Utf8ToUtf16(jdoc[i]["long_description"].as<const char *>(), &entry.description);
        common::Utf8ToUtf16(jdoc[i]["version"].as<const char *>(), &entry.version);
        common::Utf8ToUtf16(jdoc[i]["name"].as<const char *>(), &entry.title);

        entry.dataPath = "ux0:data/"; // VitaDB has a constant data path

        entry.category = jdoc[i]["type"].as<int>();

        pList->Add(entry);
    }

    delete[] jstring;

    return 0;
}
