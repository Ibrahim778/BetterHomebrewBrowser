#include <paf.h>

#include "db/psphbb.h"
#include "bhbb_locale.h"
#include "utils.h"
#include "print.h"
#include "json.h"

#define PSPHBB_INDEX_URL "http://archive.org/download/all_20240613/all.json"
#define PSPHBB_SAVE_PATH "savedata0:psphbb.json"

using namespace paf;

PSPHBDB::PSPHBDB()
{
    iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/psphbb/";
    screenshotsSupported = true;
    categories = {
        {
            CategoryAll,
            db_category_all,
            db_category_all
        },
        {
            GAME,
            db_category_single_game,
            db_category_game
        },
        {
            UTIL,
            db_category_single_util,
            db_category_util
        },
        {
            EMU,
            db_category_single_emu,
            db_category_emu
        },
        {
            PORT,
            db_category_single_port,
            db_category_port
        }
    };

    sortModes = {
        {
            msg_sort_mostrecent,
            List::Sort_MostRecent
        },
        {
            msg_sort_oldfirst,
            List::Sort_OldestFirst
        },
        {
            msg_sort_alpha,
            List::Sort_Alphabetical       
        },
        {
            msg_sort_alpharev,
            List::Sort_AlphabeticalRev
        }
    };
    iconRatio = r67x37;

    Dir::CreateRecursive(iconFolderPath.c_str());
}

PSPHBDB::~PSPHBDB()
{

}

int PSPHBDB::Parse()
{
    pList->Clear();

    int ret = SCE_OK;
    auto file = paf::LocalFile::Open(PSPHBB_SAVE_PATH, SCE_O_RDONLY, 0, &ret);
    if(ret < 0)
    {
        print("[PSPHBDB::DownloadIndex] LocalFile::Open -> 0x%X\n", ret);
        return ret;
    }

    auto fileSize = file->GetFileSize();

    char *jstring = new char[fileSize + 1];
    if(jstring == nullptr)
    {
        print("[PSPHBDB::Parse] Failed to allocate read buffer");
        return 0xC0FFEE;
    }

    ret = file->Read(jstring, fileSize);
    if(ret < 0)
    {
        print("[PSPHBDB::Parse] Failed to read file");
        delete[] jstring;
        return 0xAAABBB;
    }

    DynamicJsonDocument jdoc(SCE_KERNEL_32KiB);

    DeserializationError err = deserializeJson(jdoc, jstring);    
    if(err != DeserializationError::Ok)
    {
        print("Error parsing JSON: %s\n", err.c_str());
        return err.code();
    } 

    size_t elemCount = jdoc.size();
    print("[PSPHBB::Parse] %zu elements\n", elemCount);
    for(size_t i = 0; i < elemCount; i++)
    {
        Source::Entry entry(this);
        
        entry.hash = IDParam(jdoc[i]["id"].as<const char *>()).GetIDHash();
        entry.lastUpdated.ParseSQLiteDateTime(jdoc[i]["date"].as<const char *>());
        common::Utf8ToUtf16(jdoc[i]["author"].as<const char *>(), &entry.author);
        common::Utf8ToUtf16(jdoc[i]["description"].as<const char *>(), &entry.description);
        common::Utf8ToUtf16(jdoc[i]["name"].as<const char *>(), &entry.title);
        common::Utf8ToUtf16(jdoc[i]["version"].as<const char *>(), &entry.version);
        
        entry.category = jdoc[i]["category"].as<int>();
        entry.downloadSize = jdoc[i]["size"].as<int>();

        entry.dataPath = "ux0:/pspemu/";

        if(jdoc[i]["icon"] != nullptr)
        {
            entry.iconURL.push_back(jdoc[i]["icon"].as<const char *>());
            entry.iconPath = common::FormatString("%s%x.png", iconFolderPath.c_str(), entry.hash);
        }

        if(jdoc[i]["download_url"] != nullptr)
            entry.downloadURL.push_back(jdoc[i]["download_url"].as<const char *>());
        
        if(jdoc[i]["screenshots"] != nullptr)
        {
            for(int x = 0; x < jdoc[i]["screenshots"].size(); x++)
                entry.screenshotURL.push_back(jdoc[i]["screenshots"][x].as<const char *>());
        }

        entry.dataSize = 0;
        
        pList->Add(entry);
    }

    delete[] jstring;

    return 0;
}

int PSPHBDB::DownloadIndex(bool forceRefresh)
{
    if(paf::LocalFile::Exists(PSPHBB_SAVE_PATH) && !forceRefresh) // No need to redownload
    {
        return 0;
    }

    int ret = Utils::DownloadFile(PSPHBB_INDEX_URL, PSPHBB_SAVE_PATH);

    print("[PSPHBDB::DownloadIndex] Utils::DownloadFile -> 0x%X\n", ret);

    return ret;
}

int PSPHBDB::GetDescription(Source::Entry &entry, paf::wstring& out)
{
    out = entry.description;
    return 0;
}

int PSPHBDB::GetDownloadURL(Source::Entry &entry, paf::string& out)
{
    if(entry.downloadURL.empty())
        return -1;
        
    out = entry.downloadURL[0];    
    return 0;
}

int PSPHBDB::CreateDownloadParam(Source::Entry &entry, BGDLParam& dlParam)
{
    dlParam.type = BGDLTarget_CompressedFile;
    sce_paf_strncpy(dlParam.path, entry.dataPath.c_str(), sizeof(dlParam.path));
    sce_paf_strncpy(dlParam.data_icon, entry.iconPath.c_str(), sizeof(dlParam.data_icon));
}

int PSPHBDB::GetDataURL(Source::Entry &entry, paf::string& out)
{
    print("[PSPHBDB::GetDataURL] This source does not support data files!");
    return 0;
}