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

using namespace sce;
using namespace db;
using namespace paf;

#define SET_STRING(pafString, jsonString) { if(rootval[i][jsonString] != NULL) { pafString = rootval[i][jsonString].getString().c_str(); } } 
void vitadb::Parse(db::List *outList, string &json)
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
            print("Json::Parser::parse() -> 0x%X\n", ret);
        
        int entryCount = rootval.count();
        

        for(int i = 0; i < entryCount; i++)
        {
            db::entryInfo currentEntry;

            if(rootval[i] == NULL) return;

            SET_STRING(currentEntry.titleID, "titleid");
            SET_STRING(currentEntry.id, "id");

            if(rootval[i]["icon"] != NULL)
            {
                currentEntry.icon = ccc::Sprintf("https://bhbb-wrapper.herokuapp.com/icon?id=%s", rootval[i]["icon"].getString().c_str());
                currentEntry.icon_mirror = currentEntry.icon;
                currentEntry.iconLocal = ccc::Sprintf("%s/%s", db::info[VITADB].iconFolderPath, rootval[i]["icon"].getString().c_str());
            }

            SET_STRING(currentEntry.title, "name");

            SET_STRING(currentEntry.download_url, "url");
            SET_STRING(currentEntry.credits, "author");
            SET_STRING(currentEntry.dataURL, "data");
            currentEntry.dataPath = "ux0:data/";
            SET_STRING(currentEntry.description, "long_description");
            SET_STRING(currentEntry.version, "version");        

            if(rootval[i]["type"] != NULL)
                currentEntry.type = (int)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

            SET_STRING(currentEntry.size, "size");

            currentEntry.hash = SCE_NULL;
            currentEntry.hash = Utils::GetHashById(currentEntry.id.data());

            outList->Add(currentEntry);
        }
    }
}

void cbpsdb::Parse(db::List *outList, string &csvStr)
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
        currentEntry.credits = parsed[2];
        if(parsed[3][0] == 'h')
            currentEntry.icon = parsed[3];
        else currentEntry.icon.clear();
        if(parsed[4][0] == 'h')
            currentEntry.icon = parsed[4];
        else currentEntry.icon_mirror.clear();
        currentEntry.download_url = parsed[5];
        currentEntry.type = -1;

        currentEntry.hash = Utils::GetHashById(currentEntry.id.data());
        currentEntry.size = "0";
        
        currentEntry.iconLocal = ccc::Sprintf("%s/%s.png", db::info[CBPSDB].iconFolderPath, currentEntry.id.data());
        
        currentEntry.dataPath = "None";
        currentEntry.dataURL = "None";

        if(sce_paf_strncmp(parsed[15], "None", 4) != 0)
        {
            for(int i = 0; i < dataEntryNum; i++)
            {
                if(sce_paf_strstr(dataLines[i].data(), parsed[15]) != NULL)
                {
                    char **parsedData = parse_csv(dataLines[i].data());
                    currentEntry.dataURL = parsedData[5];
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
    print("Clearing vector...\n");
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