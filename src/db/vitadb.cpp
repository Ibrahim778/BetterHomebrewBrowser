/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2025 Muhammad Ibrahim

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
#include <psp2_compat/curl/curl.h>
#include <paf_file_ext.h>

#include "db/vitadb.h"
#include "bhbb_locale.h"
#include "utils.h"
#include "print.h"
#include "json.h"

#define VITADB_INDEX_URL "https://www.rinnegatamante.eu/vitadb/list_hbs_json.php"

using namespace paf;
using namespace paf::common;

VitaDB::VitaDB()
{
    iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vitadb/";
    iconsURL = "https://vitadb.rinnegatamante.it/icons_zip.php";
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
            PORT,
            db_category_single_port,
            db_category_port
        },
        {
            EMU,
            db_category_single_emu,
            db_category_emu
        },
        {
            UTIL,
            db_category_single_util,
            db_category_util
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
        },
        {
            msg_sort_mostdownloaded,
            List::Sort_MostDownloaded
        },
        {
            msg_sort_leastdownloaded,
            List::Sort_LeastDownloaded
        }
    };

    paf::Dir::CreateRecursive(iconFolderPath.c_str());
    buff = nullptr;
    buffSize = 0;

}

VitaDB::~VitaDB()
{
    if(buff != nullptr)    
        sce_paf_free(buff);
}

size_t VitaDB::SaveCore(char *ptr, size_t size, size_t nmeb, VitaDB *workDB)
{
    unsigned int prevBuffSize = workDB->buffSize;

    workDB->buffSize += size * nmeb;
    workDB->buff = (char *)sce_paf_realloc(workDB->buff, workDB->buffSize + 1);

    sce_paf_memcpy(workDB->buff + prevBuffSize, ptr, size * nmeb);
    workDB->buff[workDB->buffSize] = '\0';

    return size * nmeb;
}

int VitaDB::DownloadIndex(bool forceRefresh)
{
    // Just read entire file with CURL (vitadb does not offer any method to see if something changed afaik)
    
    int ret = SCE_OK;
    
    // Cleanup previous download buffer (if any)
    if(buff != nullptr)
    {
        sce_paf_free(buff);
        buff = nullptr;
        buffSize = 0;
    }

    CURL *handle = curl_easy_init();
    if(!handle)
    {
        print("[VitaDB::DownloadIndex] Failed to create curl handle!\n");
        return -1;
    }
    
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 1L);
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, 
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
    curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    curl_easy_setopt(handle, CURLOPT_URL, VITADB_INDEX_URL);
    
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, SaveCore);

    curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, 524288);

    ret = curl_easy_perform(handle);
    
    print("[VitaDB::DownloadIndex] Download %s -> (0x%X) %s\n", VITADB_INDEX_URL, ret, curl_easy_strerror((CURLcode)ret));
    
    curl_easy_cleanup(handle);

    return ret;
}

int VitaDB::GetSCECompatibleURL(std::vector<paf::string> &urlList, paf::string &out)
{
    int ret = -1;
    for(auto& url : urlList)
    {
        // First use curl to get the redirected URL
        
        // Open file
        auto file = CurlFile::Open(url.c_str(), SCE_O_RDONLY, 0, &ret);

        print("[VitaDB::GetSCECompatibleURL] CurlFileOpen(%s) -> 0x%X\n", url.c_str(), ret);
        print("FileSize: %llu\n", file.get()->GetFileSize());
        if(ret != SCE_PAF_OK)
            continue; // Try next URL

        // Get URL
        char *redirectURL = nullptr;
        ret = curl_easy_getinfo(file.get()->curl, CURLINFO_EFFECTIVE_URL, &redirectURL);

        print("[VitaDB::GetSCECompatibleURL] getinfo(CURLINFO_EFFECTIVE_URL) -> 0x%X (%s)\n", ret, redirectURL);

        if(ret != CURLE_OK || redirectURL == nullptr)
            continue;

        paf::string httpURL;
        Utils::HttpsToHttp(redirectURL, httpURL); // Convert our URL from https to http

        print("[VitaDB::GetSCECompatibleURL] Utils::HttpsToHttp -> %s\n", httpURL.c_str());

        // Finally, check if it can be used with plain SCE download methods
        if(Utils::IsValidURLSCE(httpURL.c_str()))
        {
            // Great! We can use this to download
            out = httpURL;
            return SCE_PAF_OK;
        }
    }

    return -1;
}

int VitaDB::GetDownloadURL(Source::Entry& entry, paf::string& out)
{
    return GetSCECompatibleURL(entry.downloadURL, out);
}

int VitaDB::GetDataURL(Source::Entry& entry, paf::string& out)
{
    return GetSCECompatibleURL(entry.dataURL, out);
}

int VitaDB::GetDescription(Source::Entry& entry, paf::wstring& out)
{
    out = entry.description;
    return 0;
}

int VitaDB::Parse()
{
    if(!buff)
    {
        print("[VitaDB::Parse] Error buff == nullptr\n");
        return -1;
    }

    pList->Clear();

    int ret = 0;
    
    char *jstring = buff;

    DynamicJsonDocument jdoc(SCE_KERNEL_1MiB);
    DeserializationError err = deserializeJson(jdoc, jstring);
    if(err != DeserializationError::Ok)
    {
        print("Error parsing JSON: %s\n", err.c_str());
        return err.code();
    }   
    
    size_t elemCount = jdoc.size();
    for(size_t i = 0; i < elemCount; i++)
    {
        Source::Entry entry(this);

        entry.hash = IDParam(jdoc[i]["id"].as<const char *>()).GetIDHash();
        
        if(jdoc[i]["icon"] != nullptr)
        {
            entry.iconPath = common::FormatString("%s%s", iconFolderPath.c_str(), jdoc[i]["icon"].as<const char *>());
            entry.iconURL.push_back(common::FormatString("https://www.rinnegatamante.eu/vitadb/icons/%s", jdoc[i]["icon"].as<const char *>()));
        }

        // Would love to use common::string_util::tokenize, but paf::list is wonky (crashes :()
        char *token = sce_paf_strtok((char *)jdoc[i]["screenshots"].as<const char *>(), ";");
        
        while(token != nullptr) 
        {
            paf::string url = "https://www.rinnegatamante.eu/vitadb/";
            url += token;
            entry.screenshotURL.push_back(url);
            token = sce_paf_strtok(nullptr, ";");
        }

        common::Utf8ToUtf16(jdoc[i]["titleid"].as<const char *>(), &entry.titleID);
        common::Utf8ToUtf16(jdoc[i]["author"].as<const char *>(), &entry.author);
        common::Utf8ToUtf16(jdoc[i]["long_description"].as<const char *>(), &entry.description);
        common::Utf8ToUtf16(jdoc[i]["version"].as<const char *>(), &entry.version);
        common::Utf8ToUtf16(jdoc[i]["name"].as<const char *>(), &entry.title);

        entry.dataPath = "ux0:data/"; // VitaDB has a constant data path

        entry.category = jdoc[i]["type"].as<int>();
        
        entry.lastUpdated.ParseSQLiteDateTime(jdoc[i]["date"].as<const char *>());

        entry.downloadNum = jdoc[i]["downloads"].as<unsigned int>();

        entry.downloadURL.push_back(jdoc[i]["url"].as<const char *>());
        if(jdoc[i]["data_size"].as<int>() != 0)
            entry.dataURL.push_back(jdoc[i]["data"].as<const char *>());

        entry.downloadSize = sce_paf_strtoull(jdoc[i]["size"].as<const char *>(), nullptr, 10);
        entry.dataSize = sce_paf_strtoull(jdoc[i]["data_size"].as<const char *>(), nullptr, 10);

        pList->Add(entry);
    }

    return 0;
}
