#include <paf.h>
#include <paf_file_ext.h>

#include "db/vhbd.h"
#include "bhbb_locale.h"
#include "print.h"
#include "json.h"
#include "dialog.h"
#include "common.h"
#include "utils.h"

using namespace paf;

#define VHBD_SAVE_PATH "savedata0:vhbdb.json"
#define VHBD_TIME_PATH "savedata0:time_vhbdb"
#define VHBD_RELEASE_URL "https://api.github.com/repos/vhbd/database/releases/tags/latest"

VHBD::VHBD()
{
    iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vhbd/";
    screenshotsSupported = false;
    categories = {
        Source::Category(
            CategoryAll,
            db_category_all,
            db_category_all
        ),
        Source::Category(
            0,
            db_category_single_app,
            db_category_app
        ),
        Source::Category(
            1,
            db_category_single_game,
            db_category_game
        ),
        Source::Category(
            2,
            db_category_single_emu,
            db_category_emu
        ),
    };

    sortModes = {
        Source::SortMode(
            msg_sort_mostrecent,
            List::Sort_MostRecent
        ),
        Source::SortMode(
            msg_sort_oldfirst,
            List::Sort_OldestFirst
        ),
        Source::SortMode(
            msg_sort_alpha,
            List::Sort_Alphabetical       
        ),
        Source::SortMode(
            msg_sort_alpharev,
            List::Sort_AlphabeticalRev
        )
    };
    
    Dir::CreateRecursive(iconFolderPath.c_str());
}

VHBD::~VHBD()
{
    
}

int VHBD::DownloadIndex(bool forceRefresh)
{
    print("Checking for updates...\n");
    
    auto diagText = sce::CommonGuiDialog::Dialog::GetWidget(dialog::Current(), sce::CommonGuiDialog::REGISTER_ID_TEXT_MESSAGE_1); 
    
    diagText->SetString(g_appPlugin->GetString(msg_check_update));

    DynamicJsonDocument jdoc(SCE_KERNEL_128KiB);
    
    int ret = 0;
    DeserializationError err;
    
    auto file = CurlFile::Open(VHBD_RELEASE_URL, SCE_O_RDONLY, 0, &ret); // GetFileSize returns wrong size :(((
    if(ret < 0)
        return ret;

    paf::string resp;
    size_t bytesRead;
    char buff[4096 + 1]; //Leave 1 char for '\0'
    do
    {
        sce_paf_memset(buff, 0, sizeof(buff));

        bytesRead = file.get()->Read(buff, 4096);
        
        resp += buff;
    } while(bytesRead > 0);

    err = deserializeJson(jdoc, resp.c_str());

    if(err != DeserializationError::Ok)
    {
        print("Error parsing JSON: %s\n", err.c_str());
        return err.code();
    }   

    paf::string update_time;
    paf::string download_url;
    size_t asset_num = jdoc["assets"].size();
    for(size_t i = 0; i < asset_num; i++)
    {
        if(sce_paf_strncmp(jdoc["assets"][i]["name"].as<const char *>(), "db_minify.json", 8) == 0)
        {
            update_time = jdoc["assets"][i]["updated_at"].as<const char *>();
            download_url = jdoc["assets"][i]["browser_download_url"].as<const char *>();
            goto asset_found;
        }
    }

    return -1;

asset_found:
    common::SharedPtr<LocalFile> prev_time;
    
    if(!LocalFile::Exists(VHBD_SAVE_PATH) | !LocalFile::Exists(VHBD_TIME_PATH) | forceRefresh)
        goto redownload;
    
    prev_time = LocalFile::Open(VHBD_TIME_PATH, SCE_O_RDONLY, 0, &ret);
    
    if(ret != 0)
        goto redownload;

    sce_paf_memset(buff, 0, sizeof(buff));
    bytesRead = prev_time.get()->Read(buff, prev_time.get()->GetFileSize());

    print("buff: %s\nupdate_time: %s\n", buff, update_time.c_str());    
    if(sce_paf_strncmp(buff, update_time.c_str(), bytesRead) == 0)
        return 0; // Do not redownload!
    
redownload:
    diagText->SetString(g_appPlugin->GetString(msg_wait)); // Update dialog
    ret = Utils::DownloadFile(download_url.c_str(), VHBD_SAVE_PATH); // Download new index
    
    // Reset file handler & save new download time
    prev_time.reset(); 
    
    prev_time = LocalFile::Open(VHBD_TIME_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &ret);
#ifdef _DEBUG
    if(ret < 0)
    {
        print("[Warn VHBD::DownloadIndex] failed to open %s for writing -> 0x%X!\n", VHBD_TIME_PATH, ret);
    }
#endif // _DEBUG
    prev_time.get()->Write(update_time.c_str(), update_time.length() + 1);
    return ret;
}

int VHBD::GetDownloadURL(Source::Entry& entry, paf::string& out)
{
    return 0;
}

int VHBD::GetDataURL(Source::Entry& entry, paf::string& out)
{
    return 0;
}

int VHBD::GetDescription(Source::Entry& entry, paf::wstring& out)
{
    out = entry.description;
    return 0;
}

int VHBD::Parse()
{
    pList->Clear();

    int ret = 0;
    off_t fileSize = 0;
    char *jstring = nullptr;

    auto jsonFile = LocalFile::Open(VHBD_SAVE_PATH, SCE_O_RDONLY, 0, &ret);
    if(ret != SCE_OK)
        return ret;

    fileSize = jsonFile.get()->GetFileSize();
    
    jstring = new char[fileSize + 1];
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
    
    auto homebrew = jdoc["homebrew"];

    size_t elemCount = homebrew.size();
    for(size_t i = 0; i < elemCount; i++)
    {
        Source::Entry entry(this);
        entry.hash = IDParam(common::FormatString("%s%s", homebrew[i]["name"].as<const char *>(), homebrew[i]["title_id"].as<const char *>())).GetIDHash();
        
        if(homebrew[i]["icons"] != nullptr)
        {
            for(unsigned int x = 0; x < homebrew[i]["icons"].size(); x++)
                entry.iconURL.push_back(homebrew[i]["icons"][x].as<const char *>());

            entry.iconPath = common::FormatString("%s%x.png", iconFolderPath.c_str(), entry.hash);
        }

        common::Utf8ToUtf16(homebrew[i]["title_id"].as<const char *>(), &entry.titleID);
        common::Utf8ToUtf16(homebrew[i]["description"].as<const char *>(), &entry.description);
        common::Utf8ToUtf16(homebrew[i]["version"].as<const char *>(), &entry.version);
        common::Utf8ToUtf16(homebrew[i]["name"].as<const char *>(), &entry.title);

        common::Utf8ToUtf16(homebrew[i]["authors"][0]["name"].as<const char *>(), &entry.author);
        for(unsigned int x = 1; x < homebrew[i]["authors"].size(); x++)
        {
            paf::wstring a;
            common::Utf8ToUtf16(homebrew[i]["authors"][x]["name"].as<const char *>(), &a);
            entry.author += L" & ";
            entry.author += a;
        }

        entry.dataPath = homebrew[i]["data_path"].as<const char *>(); 

        const char *type = homebrew[i]["type"].as<const char *>();
        if(sce_paf_strncmp(type, "app", 3) == 0)
        {
            entry.category = 0;
        } 
        else if(sce_paf_strncmp(type, "game", 4) == 0)
        {
            entry.category = 1;
        }
        else if(sce_paf_strncmp(type, "emulator", 8) == 0)
        {
            entry.category = 2;
        }

        entry.lastUpdated.ParseSQLiteDateTime(homebrew[i]["updated_at"].as<const char *>());
        
        pList->Add(entry);
    }

    delete[] jstring;

    return 0;
}
