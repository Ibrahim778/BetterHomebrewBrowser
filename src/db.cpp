#include <paf.h>
#include <json.h>
#include <libsysmodule.h>
#include <vector>

#include "common.h"
#include "db.h"
#include "csv.h"
#include "print.h"
#include "json.hpp"
#include "utils.h"
#include "curl_file.h"

using namespace sce;
using namespace db;
using namespace paf;

#define SET_STRING(pafString, jsonString) { if(rootval[i][jsonString] != NULL) { pafString = rootval[i][jsonString].getString().c_str(); } } 
SceInt32 vitadb::Parse(db::List *outList, string &json)
{
    outList->Clear();

    int length = json.length();
    const char *jsonStr = json.data();

    PAFAllocator allocator;
    Json::InitParameter initParam((Json::MemAllocator *)&allocator, 0, 512);
    Json::Initializer init;


    init.initialize(&initParam);

    {
        Json::Value rootval;

        rootval.clear();
        

        SceInt32 ret = Json::Parser::parse(rootval, jsonStr, length);
        if(ret < 0)
        {
            print("Json::Parser::parse() -> 0x%X\n", ret);
            return ret;
        }
        
        int entryCount = rootval.count();
        

        for(int i = 0; i < entryCount; i++)
        {
            db::entryInfo currentEntry;

            if(rootval[i] == NULL) return -1;

            SET_STRING(currentEntry.titleID, "titleid");
            SET_STRING(currentEntry.id, "id");

            if(rootval[i]["icon"] != NULL)
            {
                currentEntry.iconURL.push_back(ccc::Sprintf("https://rinnegatamante.it/vitadb/icons/%s", rootval[i]["icon"].getString().c_str()));
                currentEntry.iconPath = ccc::Sprintf("%s/%s", db::info[VITADB].iconFolderPath, rootval[i]["icon"].getString().c_str());
            }

            SET_STRING(currentEntry.title, "name");
            currentEntry.downloadURL.push_back(paf::string(rootval[i]["url"].getString().c_str()));
            if(rootval[i]["data"] != NULL)
                currentEntry.dataURL.push_back(paf::string(rootval[i]["data"].getString().c_str()));

            SET_STRING(currentEntry.author, "author");
            currentEntry.dataPath = "ux0:data/";
            SET_STRING(currentEntry.description, "long_description");
            SET_STRING(currentEntry.version, "version");        

            auto screenshotStr = rootval[i]["screenshots"].getString().c_str();
            char *token = sce_paf_strtok((char *)screenshotStr, ";");
            
            while(token != SCE_NULL) 
            {
                char fileName[sce_paf_strlen(token) - 11];
                sce_paf_memset(fileName, 0, sizeof(fileName));
                sce_paf_strncpy(fileName, token + 12, sizeof(fileName));

                currentEntry.screenshotURL.push_back(ccc::Sprintf("https://rinnegatamante.it/vitadb/screenshots/%s", fileName));
                token = sce_paf_strtok(SCE_NULL, ";");
            }   

            if(rootval[i]["type"] != NULL)
                currentEntry.type = (int)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

            currentEntry.hash = Utils::GetHashById(currentEntry.id.data());

            outList->Add(currentEntry);
        }
    }
    return SCE_OK;
}

SceInt32 cbpsdb::Parse(db::List *outList, string &csvStr)
{
    //Parse one time to get data files and add to an array, parse entire db to add files with data.
    
    outList->Clear();

    const char *csv = csvStr.data();
    int length = csvStr.length();

    char *line = (char *)csv - 6;
    int dataEntryNum = 0;
    while((line = sce_paf_strstr(line + 6, ",DATA,")) != NULL) dataEntryNum++;

    string *dataLines = new string[dataEntryNum];

    int currentEntriesAdded = 0;
    int offset = 0; 
    while((line = getLine(csv, &offset)) != NULL && currentEntriesAdded < dataEntryNum)
    {
        if(sce_paf_strstr(line, ",DATA,") == NULL) continue;

        dataLines[currentEntriesAdded] = line;

        free(line);
        currentEntriesAdded++;
    }

    line = (char *)csv - 5;
    int entryNum = 0;
    while((line = sce_paf_strstr(line + 5, ",VPK,")) != NULL) entryNum++;

    currentEntriesAdded = 0;
    offset = 0;
        
    while((line = getLine(csv, &offset)) != NULL && currentEntriesAdded < entryNum)
    {
        db::entryInfo currentEntry;

        if(sce_paf_strstr(line, ",VPK,") == NULL) continue;

        char **parsed = parse_csv(line);

        currentEntry.id = parsed[0];
        currentEntry.title = parsed[1];
        
        char titleIDBuff[10];
        sce_paf_memset(titleIDBuff, 0, sizeof(titleIDBuff));
        sce_paf_strncpy(titleIDBuff, parsed[0], 9);

        currentEntry.titleID = titleIDBuff;
        currentEntry.author = parsed[2];
        
        if(parsed[3][0] == 'h')
            currentEntry.iconURL.push_back(paf::string(parsed[3]));
        
        if(parsed[4][0] == 'h')
            currentEntry.iconURL.push_back(parsed[4]);
        
        currentEntry.downloadURL.push_back(paf::string(parsed[5]));
        currentEntry.type = -1;

        currentEntry.hash = Utils::GetHashById(currentEntry.id.data());
        currentEntry.description = parsed[7];

        currentEntry.iconPath = ccc::Sprintf("%s/%s.png", db::info[CBPSDB].iconFolderPath, currentEntry.id.data());
        
        currentEntry.dataPath.clear();

        if(sce_paf_strncmp(parsed[15], "None", 4) != 0)
        {
            for(int i = 0; i < dataEntryNum; i++)
            {
                if(sce_paf_strstr(dataLines[i].data(), parsed[15]) != NULL)
                {
                    char **parsedData = parse_csv(dataLines[i].data());
                    currentEntry.dataURL.push_back(paf::string(parsedData[5]));
                    currentEntry.dataPath = parsedData[13];
                    break;
                }
            }
        }

        outList->Add(currentEntry);

        free(line);
        currentEntriesAdded++;
    }

    delete[] dataLines;
    return SCE_OK;
}

SceInt32 vhbdb::Parse(db::List *outList, string &json)
{
    outList->Clear();

    int length = json.length();
    const char *jsonStr = json.data();

    PAFAllocator allocator;
    Json::InitParameter initParam((Json::MemAllocator *)&allocator, 0, 512);
    Json::Initializer init;

    init.initialize(&initParam);

    {
        Json::Value rootVal;

        rootVal.clear();
        

        SceInt32 ret = Json::Parser::parse(rootVal, jsonStr, length);
        if(ret < 0)
        {
            print("Json::Parser::parse() -> 0x%X\n", ret);
            return ret;
        }
        
        int entryCount = rootVal["homebrew"].count();
        auto rootval = rootVal["homebrew"];

        for(int i = 0; i < entryCount; i++)
        {
            db::entryInfo currentEntry;

            if(rootval[i] == NULL) return -1;

            SET_STRING(currentEntry.titleID, "title_id");
            SET_STRING(currentEntry.title, "name");

            currentEntry.id = ccc::Sprintf("%X%X", Utils::GetHashById(currentEntry.titleID.data()), Utils::GetHashById(currentEntry.title.data()));

            if(rootval[i]["icons"] != NULL)
                for(int x = 0; x < rootval[i]["icons"].count(); x++)
                    currentEntry.iconURL.push_back(paf::string(rootval[i]["icons"][x].getString().c_str()));
            
            currentEntry.iconPath = ccc::Sprintf("%s/%s.png", db::info[VHBDB].iconFolderPath, currentEntry.id.data());

            if(rootval[i]["downloads"] != NULL)
                for(int x = 0; x < rootval[i]["downloads"].count(); x++)
                    currentEntry.downloadURL.push_back(paf::string(rootval[i]["downloads"][x].getString().c_str()));

            if(rootval[i]["data"] != NULL)
                currentEntry.dataURL.push_back(paf::string(rootval[i]["data"].getString().c_str()));

            currentEntry.author = "";
            if(rootval[i]["authors"] != NULL)
                for(int x = 0; x < rootval[i]["authors"].count(); x++)
                {
                    currentEntry.author += rootval[i]["authors"][x]["name"].getString().c_str();
                    if(x != (rootval[i]["authors"].count() - 1)) currentEntry.author += " & ";
                }
            

            SET_STRING(currentEntry.dataPath, "data_path")
            SET_STRING(currentEntry.description, "description");
            SET_STRING(currentEntry.version, "version"); 
  

            // if(rootval[i]["type"] != NULL)
            //     currentEntry.type = (int)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

            if(rootval[i]["type"] != NULL)
            {
                const char *typeName = rootval[i]["type"].getString().c_str();
                if(sce_paf_strncmp(typeName, "app", 3) == 0)
                {
                    currentEntry.type = 0;
                }
                else if(sce_paf_strncmp(typeName, "emulator", 8) == 0)
                {
                    currentEntry.type = 2;
                }
                else if(sce_paf_strncmp(typeName, "game", 4) == 0)
                {
                    currentEntry.type = 1;
                }
            }

            currentEntry.hash = Utils::GetHashById(currentEntry.id.data());

            outList->Add(currentEntry);
        }
    }
    return SCE_OK;
}

SceInt32 cbpsdb::GetDataUrl(db::entryInfo& entry, paf::string& out)
{
    auto end = entry.dataURL.end();
    for(auto i = entry.dataURL.begin(); i != end; i++)
    {
        SceInt32 err = SCE_OK;
        
        paf::string buff = i->data();
        
        Utils::HttpsToHttp(buff);
        
        if(Utils::IsValidURLSCE(buff.data()))
        {
            out = buff;
            return SCE_OK;
        }
    }
    return -1;
}

SceInt32 cbpsdb::GetDownloadUrl(db::entryInfo& entry, paf::string& out)
{
    auto end = entry.downloadURL.end();
    for(auto i = entry.downloadURL.begin(); i != end; i++)
    {
        SceInt32 err = SCE_OK;
        
        paf::string buff = i->data();
        
        Utils::HttpsToHttp(buff);
        
        if(Utils::IsValidURLSCE(buff.data()))
        {
            out = buff;
            return SCE_OK;
        }
    }
    return -1;
}


SceInt32 vitadb::GetDataUrl(db::entryInfo& entry, paf::string& out)
{
    auto end = entry.dataURL.end();
    for(auto i = entry.dataURL.begin(); i != end; i++)
    {
        //First, use curl to get the redirected URL
        SceInt32 err = SCE_OK;
        auto file = CurlFile::Open(i->data(), SCE_O_RDONLY, 0, &err, false, true);

        if(err != SCE_OK)
        {
            print("Error opening CurlFile: 0x%X\n", err);
            continue;
        }
        char *url = SCE_NULL;
        err = file.get()->GetEffectiveURL(&url);
        
        if(err != SCE_OK || url == SCE_NULL)
        {
            print("Error getting effectiveURL: 0x%X 0x%X\n", url, err);
            continue;
        }
        paf::string buff = url;
        Utils::HttpsToHttp(buff);
        print("Got URL: %s\n", buff);
        if(Utils::IsValidURLSCE(buff.data()))
        {
            out = buff;
            return SCE_OK;
        }
    }

    return -1;
}

SceInt32 vitadb::GetDownloadUrl(db::entryInfo& entry, paf::string& out)
{
    auto end = entry.downloadURL.end();
    for(auto i = entry.downloadURL.begin(); i != end; i++)
    {
        //First, use curl to get the redirected URL
        SceInt32 err = SCE_OK;
        auto file = CurlFile::Open(i->data(), SCE_O_RDONLY, 0, &err, false, true);

        if(err != SCE_OK)
        {
            print("Error opening CurlFile: 0x%X\n", err);
            continue;
        }
        char *url = SCE_NULL;
        err = file.get()->GetEffectiveURL(&url);
        
        if(err != SCE_OK || url == SCE_NULL)
        {
            print("Error getting effectiveURL: 0x%X 0x%X\n", url, err);
            continue;
        }
        paf::string buff = url;
        Utils::HttpsToHttp(buff);
        print("Got URL: %s\n", buff);
        if(Utils::IsValidURLSCE(buff.data()))
        {
            out = buff;
            return SCE_OK;
        }
    }
    return -1;
}

SceInt32 vhbdb::GetDownloadUrl(db::entryInfo &entry, paf::string& out)
{
    auto end = entry.downloadURL.end();
    for(auto i = entry.downloadURL.begin(); i != end; i++)
    {
        SceInt32 err = SCE_OK;
        
        paf::string buff = i->data();
        
        Utils::HttpsToHttp(buff);
        
        if(Utils::IsValidURLSCE(buff.data()))
        {
            out = buff;
            return SCE_OK;
        }
    }
    return -1;
}

SceInt32 vhbdb::GetDataUrl(db::entryInfo& entry, paf::string& out)
{
    auto end = entry.dataURL.end();
    for(auto i = entry.dataURL.begin(); i != end; i++)
    {
        //First, use curl to get the redirected URL
        SceInt32 err = SCE_OK;
        auto file = CurlFile::Open(i->data(), SCE_O_RDONLY, 0, &err, false, true);

        if(err != SCE_OK)
        {
            print("Error opening CurlFile: 0x%X\n", err);
            continue;
        }
        char *url = SCE_NULL;
        err = file.get()->GetEffectiveURL(&url);
        
        if(err != SCE_OK || url == SCE_NULL)
        {
            print("Error getting effectiveURL: 0x%X 0x%X\n", url, err);
            continue;
        }
        paf::string buff = url;
        Utils::HttpsToHttp(buff);
        print("Got URL: %s\n", buff);
        if(Utils::IsValidURLSCE(buff.data()))
        {
            out = buff;
            return SCE_OK;
        }
    }

    return -1;
}


SceInt32 vitadb::GetDescription(db::entryInfo& entry, paf::string& out) 
{
    out = entry.description;
    return SCE_OK;
}

SceInt32 cbpsdb::GetDescription(db::entryInfo& entry, paf::string& out)
{   
    SceInt32 ret = SCE_OK;
    
    auto cFile = CurlFile::Open(entry.description.data(), &ret, SCE_O_RDONLY);
    if(ret < 0)
        return ret;

    auto fileSize = cFile->GetFileSize(); 
    char *buff = new char[fileSize + 1];
    sce_paf_memset(buff, 0, fileSize + 1);

    ret = cFile->Read(buff, fileSize);
    
    if(ret < 0)
        return ret;

    out = buff;
    delete[] buff;
    
    return SCE_OK;
}

SceInt32 vhbdb::GetDescription(db::entryInfo& entry, paf::string& out)
{
    out = entry.description;
    return SCE_OK;
}

db::List::~List()
{
    Clear();
}

db::List::List()
{

}

void db::List::Add(db::entryInfo &entry)
{
    entries.push_back(entry);
}

void db::List::Clear()
{
    entries.clear();
}

db::entryInfo &db::List::Get(SceUInt64 hash)
{   
    auto end = entries.end();
    for(auto entry = entries.begin(); entry != end; entry++)
        if(entry->hash == hash) return *entry;
}

db::entryInfo &db::List::Get(int index)
{
    return entries[index];
}

std::vector<db::entryInfo>::iterator db::List::Get(int index, int category)
{
    print("Get, index: %d\n", index);

    size_t i = 0, size = entries.size();
    auto end = entries.end();
    std::vector<db::entryInfo>::iterator entry;
    for(entry = entries.begin(); i < index && entry != end; i++, entry++)
        if(entry->type != category) i--;

    return entry;
}

size_t db::List::GetSize(int category)
{
    size_t size = entries.size();
    if(category == -1) return size;

    size_t out = 0;
    for(int i = 0; i < size; i++)
        if(entries[i].type == category) out++;

    return out;
}