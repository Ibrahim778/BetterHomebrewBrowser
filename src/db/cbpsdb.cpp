/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2023 Muhammad Ibrahim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/

#include <paf.h>
#include <paf_file_ext.h>
#include <common_gui_dialog.h>
#include <vector>
#include <list>

#include "db/cbpsdb.h"
#include "bhbb_locale.h"
#include "print.h"
#include "common.h"
#include "dialog.h"
#include "json.h"
#include "utils.h"
extern "C"
{
    #include "csv.h"
}

using namespace paf;

#define CBPSDB_SAVE_PATH "savedata0:/cbpsdb.csv"
#define CBPSDB_COMMIT_SHA "savedata0:/cbpsdb_commit.sha"

#define CBPSDB_INDEX_URL "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv"
#define CBPSDB_COMMIT_URL "https://api.github.com/repos/KuromeSan/cbps-db/commits?path=cbpsdb.csv&since=2022-12-09T00:36:21Z"

CBPSDB::CBPSDB()
{
    iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/cbpsdb/";
    iconsURL = "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/raw/main/icons.zip?raw=true";
    screenshotsSupported = true;
    categories = {
        Source::Category(
            CategoryAll,
            db_category_all,
            db_category_all
        )
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

    paf::Dir::CreateRecursive(iconFolderPath.c_str());
}

CBPSDB::~CBPSDB()
{
    
}

int CBPSDB::GetDownloadURL(Source::Entry& entry, paf::string& out)
{
    return GetSCECompatibleURL(entry.downloadURL, out);
}

int CBPSDB::GetDataURL(Source::Entry& entry, paf::string& out)
{
    return GetSCECompatibleURL(entry.dataURL, out);
}

int CBPSDB::DownloadIndex(bool forceRefresh)
{
    // Simply query github api for commits on the csv file in repo, if new commit, then redownload and save commit.

    print("Checking for updates...\n");
    auto diagText = sce::CommonGuiDialog::Dialog::GetWidget(dialog::Current(), sce::CommonGuiDialog::REGISTER_ID_TEXT_MESSAGE_1); // Update dialog
    diagText->SetString(g_appPlugin->GetString(msg_check_update));
    
    char buff[4096 + 1]; //Leave 1 char for '\0'
    DynamicJsonDocument jdoc(SCE_KERNEL_32KiB);
    int ret = 0;
    DeserializationError err;
    common::SharedPtr<LocalFile> prevDate;
    common::SharedPtr<CurlFile> file;
    paf::string resp;
    paf::string commit_sha;

    // Query github API to check for commits
    file = CurlFile::Open(CBPSDB_COMMIT_URL, SCE_O_RDONLY, 0, &ret); // GetFileSize returns wrong size :(((
    if(ret < 0)
        return ret;

    size_t bytesRead;
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

    size_t assetNum = jdoc.size();
    
    if(assetNum == 0)
        return -1;
    
    // The file's latest commit
    commit_sha = jdoc[0]["sha"].as<const char *>();

    print("Latest commit sha: %s\n", commit_sha.c_str());

    // Some checks
    if(!LocalFile::Exists(CBPSDB_SAVE_PATH) || !LocalFile::Exists(CBPSDB_COMMIT_SHA) || forceRefresh)
        goto redownload;

    // Now we compare the date we just got from the previously saved date    
    prevDate = LocalFile::Open(CBPSDB_COMMIT_SHA, SCE_O_RDONLY, 0, &ret);
    
    if(ret != SCE_PAF_OK) // Really weird IO error? Just ignore
        goto redownload;

    sce_paf_memset(buff, 0, sizeof(buff));
    bytesRead = prevDate.get()->Read(buff, prevDate.get()->GetFileSize());

    print("Saved sha: %s Server sha: %s\n", buff, commit_sha.c_str());

    if(sce_paf_strncmp(buff, commit_sha.c_str(), bytesRead) == 0) // Server date and saved date are the same
        return 0; // Do not redownload!

redownload:
    
    diagText->SetString(g_appPlugin->GetString(msg_wait)); // Update dialog
    
    ret = Utils::DownloadFile(CBPSDB_INDEX_URL, CBPSDB_SAVE_PATH); // Download new index

    // Reopen file for writing and save our new download date

    prevDate.reset();
    
    prevDate = LocalFile::Open(CBPSDB_COMMIT_SHA, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &ret);
    
    if(ret == SCE_PAF_OK) // File opened successfully
        prevDate.get()->Write(commit_sha.c_str(), commit_sha.size() + 1);

    return ret;
}

int CBPSDB::GetDescription(Source::Entry& entry, paf::wstring& out)
{
    int ret = SCE_OK;
    paf::string resp;
    char buff[4096 + 1]; // Leave 1 char for null terminator
    
    auto file = CurlFile::Open(entry.description.c_str(), &ret, SCE_O_RDONLY);
    if(ret != SCE_OK)
        return ret;

    size_t bytesRead;
    do 
    {
        sce_paf_memset(buff, 0, sizeof(buff));
        bytesRead = file.get()->Read(buff, 4096);    
    
        resp += buff;
    } while(bytesRead > 0);

    common::Utf8ToUtf16(resp, &out);

    return 0;
}

int CBPSDB::GetSCECompatibleURL(std::vector<paf::string> &list, paf::string &out)
{
    for(auto &url : list)
    {
        paf::string httpURL;
        Utils::HttpsToHttp(url.c_str(), httpURL);
        print("[CBPSDB::GetSCECompatibleURL] Trying URL: %s...", url.c_str());

        if(Utils::IsValidURLSCE(httpURL.c_str()))
        {
            print("OK\n");
            out = url;
            return SCE_PAF_OK;
        }
        print("FAIL\n");
    }
    print("[CBPSDB::GetSCECompatibleURL] No compatible URL found! FAIL\n");
    return -1;
}

int CBPSDB::Parse()
{
    // Thanks stack overflow
    auto ListAt = [](paf::list<paf::string>& _list, int _i) -> paf::string&
    {
        auto it = _list.begin();
        for(int i=0; i<_i; i++)
        {
            ++it;
        }
        return *it;
    };

    pList->Clear();
    
    auto fh = sce_paf_fopen(CBPSDB_SAVE_PATH, "r");
    if(!fh)
        return sce_paf_ferror(fh);
    
    std::vector<paf::string> dataLines;

    int ret = SCE_OK;
    int done = 0;
    while(!done)
    {
        char *line = fread_line(fh, 1024, &done, &ret);
        if(sce_paf_strstr(line, ",DATA,") != nullptr)
            dataLines.push_back(line);   
        sce_paf_free(line);
    }
    print("Parsed %lu data files\n", dataLines.size());
    sce_paf_fseek(fh, 0, SEEK_SET);
    
    ret = SCE_OK;
    done = 0;
    while(!done || ret != SCE_OK)
    {
        char *line = fread_line(fh, 1024, &done, &ret);
        if(!line || ret != SCE_OK || done)
        {
            print("[Warn] failed to read line\n");
            continue;
        }

        if(sce_paf_strstr(line, ",VPK,") == nullptr)
        {
            print("skipping non-vpk\n");
            sce_paf_free(line);
            continue;
        }  

        paf::token_list_t tokens;
        common::string_util::tokenize(tokens, line, 0, ',');
        
        Source::Entry entry(this);
        
        entry.hash = IDParam(ListAt(tokens, 0).c_str()).GetIDHash();
        
        common::Utf8ToUtf16(paf::string(ListAt(tokens, 0).c_str(), 9), &entry.titleID);

        common::Utf8ToUtf16(ListAt(tokens, 1), &entry.title);
        common::Utf8ToUtf16(ListAt(tokens, 2), &entry.author);

        if(ListAt(tokens, 3).c_str()[0] == 'h')
            entry.iconURL.push_back(ListAt(tokens, 3));
        
        if(ListAt(tokens, 4).c_str()[0] == 'h')
            entry.iconURL.push_back(ListAt(tokens, 4));

        entry.downloadURL.push_back(ListAt(tokens, 5));

        common::Utf8ToUtf16(ListAt(tokens, 7).c_str(), &entry.description);
        
        entry.iconPath = common::FormatString("%s%x.png", iconFolderPath.c_str(), entry.hash);

        if( !(ListAt(tokens, 15) == "None") ) // Entry has a data file
        {
            for(auto &datLine : dataLines)
            {
                if(sce_paf_strstr(datLine.c_str(), ListAt(tokens, 15).c_str()) == nullptr)
                    continue;
                
                paf::token_list_t parsedData;   
                common::string_util::tokenize(parsedData, datLine.c_str(), 0, ',');
            
                entry.dataURL.push_back(ListAt(parsedData, 5));
                entry.dataPath = ListAt(parsedData, 13);
                
                break;
            }
        }

        SceDateTime dtime;
        rtc::Tick tick = sce_paf_strtoull(ListAt(tokens, 11).c_str(), nullptr, 10);
        sceRtcSetTime64_t(&dtime, tick);
        sceRtcGetTick(&dtime, (SceRtcTick *)&tick);

        entry.lastUpdated.ConvertRtcTickToDateTime(&tick);
        
        entry.dataSize = 0;
        entry.downloadSize = 0;

        pList->Add(entry);
        
        sce_paf_free(line);
    }

    sce_paf_fclose(fh);

    return ret;
}
