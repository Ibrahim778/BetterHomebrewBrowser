#include <paf.h>
#include <json.h>
#include <libsysmodule.h>
#include <vector>

#include "common.h"
#include "db.h"
#include "csv.h"
#include "print.h"
#include "json.h"
#include "utils.h"
#include "curl_file.h"
#include "bhbb_locale.h"

using namespace sce;
using namespace db;
using namespace paf;
using namespace Utils;

#define SET_STRING(pafString, jsonString) pafString = rootval[i][jsonString].getString().c_str()
SceInt32 vitadb::Parse(db::List *outList, const char *jsonPath)
{
    outList->Clear();

    //Read the json string
    SceInt32 ret = SCE_OK;
    SceOff fileSize = 0;
    char *jsonData = SCE_NULL;

    auto jsonFile = LocalFile::Open(jsonPath, SCE_O_RDONLY, 0, &ret);
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

    print("Read 0x%X bytes\n", ret);

    DynamicJsonDocument jdoc(SCE_KERNEL_1MiB);
    DeserializationError err = deserializeJson(jdoc, jstring);
    if(err != DeserializationError::Ok)
    {
        print("Error parsing JSON: %s\n", err.c_str());
        delete[] jstring;
        return err.code();
    }   

    size_t elemNum = jdoc.size();
    for(unsigned int i = 0; i < elemNum; i++)
    {
        db::entryInfo &info = outList->ReturnNew();
        info.title = jdoc[i]["name"].as<const char *>();
        info.id = jdoc[i]["id"].as<const char *>();
        if(jdoc[i]["icon"] != NULL)
        {
            common::string_util::setf(info.iconPath, "%s/%s", db::info[VITADB].iconFolderPath, jdoc[i]["icon"].as<const char *>());
            
            string url;
            common::string_util::setf(url, "https://rinnegatamante.it/vitadb/icons/%s", jdoc[i]["icon"].as<const char *>());
            info.iconURL.push_back(url);
        }

        info.titleID = jdoc[i]["titleid"].as<const char *>();
        info.author = jdoc[i]["author"].as<const char *>();
        info.dataPath = "ux0:data/";
        info.description = jdoc[i]["long_description"].as<const char *>();
        info.version = jdoc[i]["version"].as<const char *>();

        // common::string_util::tokenize(info.screenshotURL, jdoc[i]["screenshots"].as<const char *>(), sce_paf_strlen(jdoc[i]["screenshots"].as<const char *>()), ';');
        // print("ssize = %d\n", info.screenshotURL.size());
        info.type = jdoc[i]["type"].as<int>();
        info.hash = jdoc[i]["hash"].as<int>();

        // outList->Add(info);
    }

    delete[] jstring;

    return SCE_OK;
}

SceInt32 cbpsdb::Parse(db::List *outList, const char *csvPath)
{
    //Parse one time to get data files and add to an array, parse entire db to add files with data.
    outList->Clear();
    
    FILE *file = sce_paf_fopen(csvPath, "rb");
    if(file == NULL) return -2;

    int dataCount = 0;

    int done = 0;
    int err = 0;
    sce_paf_free(fread_csv_line(file, 1024, &done, &err)); //Skip the first line

    while(done != 1) //Count data files
    {
        char *line = fread_csv_line(file, 1024, &done, &err);
        if(err != 0)
        {
            print("[Error] fread_csv_line -> %d\n", err);
            sce_paf_free(line);
            return -4;
        }

        if(!line)
        {
            sce_paf_free(line);
            return -3;
        }
        
        if(sce_paf_strstr(line, ",DATA,") != NULL)
            dataCount++;
        
        sce_paf_free(line);
    }

    //Reset the file
    done = 0;
    err = 0;
    sce_paf_fseek(file, 0, SEEK_SET);

    string *dataLines = new string[dataCount];
    
    sce_paf_free(fread_csv_line(file, 1024, &done, &err)); //Skip the first line

    for(int i = 0; i < dataCount; i++)
    {
        char *line = fread_csv_line(file, 1024, &done, &err);
        if(err != 0)
        {
            print("[Error] fread_csv_line -> %d\n", err);
            sce_paf_free(line);
            return -4;
        }

        if(!line)
        {
            sce_paf_free(line);
            return -3;
        }
        
        if(sce_paf_strstr(line, ",DATA,") == NULL)
        {
            sce_paf_free(line);
            i--;
            continue;
        }

        dataLines[i] = line;
    }

    done = 0;
    err = 0;
    sce_paf_fseek(file, 0, SEEK_SET);
    sce_paf_free(fread_csv_line(file, 1024, &done, &err)); //Skip the first line
    int num = 0;

    // print("Going to read lines...\n");
    while(done != 1)
    {
        db::entryInfo currentEntry;

        char *line = fread_csv_line(file, 1024, &done, &err);
        // print("Read line: %s\n", line);
        if(err != 0)
        {
            print("[Error] fread_csv_line -> %d\n", err);
            sce_paf_free(line);
            return -4;
        }

        if(sce_paf_strstr(line, ",VPK,") == NULL)
        {
            sce_paf_free(line);
            continue;
        }

        char **parsed = parse_csv(line);
        // print("parsed csv: %p\n", parsed);
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

        currentEntry.hash = Misc::GetHash(currentEntry.id.data());
        currentEntry.description = parsed[7];

        common::string_util::setf(currentEntry.iconPath, "%s%s.png", db::info[CBPSDB].iconFolderPath, currentEntry.id.data());
        
        currentEntry.dataPath.clear();

        if(parsed[15] && sce_paf_strncmp(parsed[15], "None", 4) != 0)
        {
            for(int i = 0; i < dataCount; i++)
            {
                if(sce_paf_strstr(dataLines[i].data(), parsed[15]) != NULL)
                {
                    char **parsedData = parse_csv(dataLines[i].data());
                    currentEntry.dataURL.push_back(string(parsedData[5]));
                    currentEntry.dataPath = parsedData[13];
                    free_csv_line(parsedData);
                    break;
                }
            }
        }

        outList->Add(currentEntry);

        free_csv_line(parsed);
        // print("freed csv\n");
        free(line);
        // print("freed line\n");
        num++;
    }
    delete[] dataLines;

    return SCE_OK;
}

SceInt32 vhbdb::Parse(db::List *outList, const char *jsonPath)
{
//     print("Clearing...\n");
    outList->Clear();
//     print("Cleared\n");

//     PAFAllocator allocator;
//     Json::InitParameter initParam((Json::MemAllocator *)&allocator, 0, 1024);
//     Json::Initializer init;

//     init.initialize(&initParam);

//     {
//         Json::Value rootVal;

//         rootVal.clear();
        
//         print("Json Parser::parse\n");
//         SceInt32 ret = Json::Parser::parse(rootVal, jsonPath);
//         print("Done!\n");
//         if(ret < 0)
//         {
//             print("Json::Parser::parse() -> 0x%X\n", ret);
//             return ret;
//         }
        
//         int entryCount = rootVal["homebrew"].count();
//         auto rootval = rootVal["homebrew"];

//         for(int i = 0; i < entryCount; i++)
//         {
//             db::entryInfo currentEntry;

//             if(rootval[i] == NULL) return -1;

//             SET_STRING(currentEntry.titleID, "title_id");
//             SET_STRING(currentEntry.title, "name");

//             common::string_util::setf(currentEntry.id, "%X%X", Misc::GetHash(currentEntry.titleID.data()), Misc::GetHash(currentEntry.title.data()));

//             // if(rootval[i]["icons"] != NULL)
//             //     for(int x = 0; x < rootval[i]["icons"].count(); x++)
//             //         currentEntry.iconURL.push_back(paf::string(rootval[i]["icons"][x].getString().c_str()));
            
//             common::string_util::setf(currentEntry.iconPath, "%s%s.png", db::info[VHBDB].iconFolderPath, currentEntry.id.data());

//             // if(rootval[i]["downloads"] != NULL)
//             //     for(int x = 0; x < rootval[i]["downloads"].count(); x++)
//             //         currentEntry.downloadURL.push_back(paf::string(rootval[i]["downloads"][x].getString().c_str()));

//             // if(rootval[i]["data"] != NULL)
//             //     currentEntry.dataURL.push_back(paf::string(rootval[i]["data"].getString().c_str()));

//             currentEntry.author = "";
//             if(rootval[i]["authors"] != NULL)
//                 for(int x = 0; x < rootval[i]["authors"].count(); x++)
//                 {
//                     currentEntry.author += rootval[i]["authors"][x]["name"].getString().c_str();
//                     if(x != (rootval[i]["authors"].count() - 1)) currentEntry.author += " & ";
//                 }
            

//             SET_STRING(currentEntry.dataPath, "data_path");
//             SET_STRING(currentEntry.description, "description");
//             SET_STRING(currentEntry.version, "version"); 
  

//             // if(rootval[i]["type"] != NULL)
//             //     currentEntry.type = (int)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

//             if(rootval[i]["type"] != NULL)
//             {
//                 const char *typeName = rootval[i]["type"].getString().c_str();
//                 if(sce_paf_strncmp(typeName, "app", 3) == 0)
//                 {
//                     currentEntry.type = 0;
//                 }
//                 else if(sce_paf_strncmp(typeName, "emulator", 8) == 0)
//                 {
//                     currentEntry.type = 2;
//                 }
//                 else if(sce_paf_strncmp(typeName, "game", 4) == 0)
//                 {
//                     currentEntry.type = 1;
//                 }
//             }

//             currentEntry.hash = Misc::GetHash(currentEntry.id.data());

//             outList->Add(currentEntry);
//         }
//     }
    return SCE_OK;
}

SceInt32 cbpsdb::GetDataUrl(db::entryInfo& entry, paf::string& out)
{
    auto end = entry.dataURL.end();
    for(auto i = entry.dataURL.begin(); i != end; i++)
    {
        SceInt32 err = SCE_OK;
        
        paf::string buff = i->data();
        
        Net::HttpsToHttp(buff);
        
        if(Net::IsValidURLSCE(buff.data()))
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
        
        Net::HttpsToHttp(buff);
        
        if(Net::IsValidURLSCE(buff.data()))
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
        Net::HttpsToHttp(buff);
        print("Got URL: %s\n", buff);
        if(Net::IsValidURLSCE(buff.data()))
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
        Net::HttpsToHttp(buff);
        print("Got URL: %s\n", buff);
        if(Net::IsValidURLSCE(buff.data()))
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
        
        Net::HttpsToHttp(buff);
        
        if(Net::IsValidURLSCE(buff.data()))
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
        Net::HttpsToHttp(buff);
        print("Got URL: %s\n", buff);
        if(Net::IsValidURLSCE(buff.data()))
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
    if(sce_paf_strncmp(entry.description.data(), "None", 4) == 0)
    {
        str::GetFromHash(msg_no_desc, &out);
        return ret;
    }    

    print("Opening: %s\n", entry.description.data());
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

void db::List::Categorise(db::Id source)
{
    for(int i = 0; i < db::info[source].categoryNum; i++)
    {
        if(db::info[source].categories[i].id == db::CategoryAll) // Skip the all category
            continue; 
        
        CategorisedList catList = CategorisedList(db::info[source].categories[i].id);
        
        for (db::entryInfo &en : entries)
        {
            if(en.type == db::info[source].categories[i].id)
                catList.entries.push_back(&en);
        }

        categories.push_back(catList);
    }
}

db::List::CategorisedList &db::List::GetCategory(int category)
{
    auto catList = db::List::CategorisedList(0xFFFFFFFF);
    if(category == db::CategoryAll)
    {
        print("[Error] Please use the entries property in order to acces an element without a specific category!\n");
        return catList; 
    }

    for(CategorisedList &catList : categories)
    {
        if(catList.category == category)
            return catList;
    }
    return catList;
}

void db::List::Add(db::entryInfo &entry)
{
    entries.push_back(entry);
}

db::entryInfo &db::List::ReturnNew()
{
    db::entryInfo i;
    entries.push_back(i);
    return entries.back();
}

void db::List::Clear()
{
    entries.clear();
}

db::entryInfo &db::List::Get(SceUInt64 hash)
{   
    for(db::entryInfo& entry : entries)
        if(entry.hash == hash) return entry;
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