#include <paf.h>
#include <json.h>
#include <libsysmodule.h>

#include "common.hpp"
#include "db.hpp"
#include "csv.h"
#include "main.hpp"
#include "json.hpp"
#include "utils.hpp"

using namespace sce;
using namespace db;

#define SET_STRING(pafString, jsonString) { if(rootval[i][jsonString] != NULL) { pafString = rootval[i][jsonString].getString().c_str(); } } 
void vitadb::Parse(db::entryInfo **outList, int *outListNum, const char *jsonStr, int length)
{
    PAFAllocator allocator;
    Json::InitParameter initParam((Json::MemAllocator *)&allocator, 0, 512);
    Json::Initializer init;
    db::entryInfo *entries = new db::entryInfo[length];

    init.initialize(&initParam);

    {
        Json::Value rootval;

        rootval.clear();
        

        SceInt32 ret = Json::Parser::parse(rootval, jsonStr, length);
        if(ret < 0)
            print("Json::Parser::parse() -> 0x%X\n", ret);
        
        int length = rootval.count();
        sce_paf_memset(entries, 0, length * sizeof(db::entryInfo));

        db::entryInfo *currentEntry = entries;
        for(int i = 0; i < length; i++, currentEntry = &entries[i])
        {
            if(rootval[i] == NULL) return;

            SET_STRING(currentEntry->titleID, "titleid");
            SET_STRING(currentEntry->id, "id");

            if(rootval[i]["icon"] != NULL)
            {
                currentEntry->icon0.Setf("https://rinnegatamante.it/vitadb/icons/%s", rootval[i]["icon"].getString().c_str());
                currentEntry->icon0Local.Setf(VITADB_ICON_SAVE_PATH "/%s", rootval[i]["icon"].getString().c_str());
            }

            SET_STRING(currentEntry->title, "name");

            SET_STRING(currentEntry->download_url, "url");
            SET_STRING(currentEntry->credits, "author");
            SET_STRING(currentEntry->options, "data");
            SET_STRING(currentEntry->description, "long_description");
            SET_STRING(currentEntry->version, "version");        

            if(rootval[i]["type"] != NULL)
                currentEntry->type = (int)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

            SET_STRING(currentEntry->size, "size");

            currentEntry->hash = Utils::GetHashById(currentEntry->id.data);
        }
    }

    *outListNum = length;
    *outList = entries;
}

void cbpsdb::Parse(db::entryInfo **outList, int *outListNum, const char *csv, int length)
{
    char *line;
    int num = 0;
    while(getLine(csv) != NULL) num++;

    db::entryInfo *entries = new db::entryInfo[num];
    sce_paf_memset(entries, 0, sizeof(db::entryInfo) * num);

    db::entryInfo *currentEntry = entries;
    for (int i = 0; i < num && (line = getLine(csv)) != NULL; i++, currentEntry = &entries[i])
    {
        char **parsed = parse_csv(line);

        if(parsed != NULL)
        {
            int dead = 0;
            for (int i = 0; i < 17 && !dead; i++)
            {
                dead = parsed[i] == NULL;
            }
            
            if(!dead && sce_paf_strncmp(parsed[14], "VPK", 3) == 0)
            {
                currentEntry->id = parsed[0];

                currentEntry->title = parsed[1];
          
                currentEntry->credits = parsed[2];
                if(parsed[3][0] == 'h')
                    currentEntry->icon0 = parsed[3];
                else currentEntry->icon0.Clear();
                currentEntry->download_url = parsed[5];
                currentEntry->options = parsed[13];
                currentEntry->description = parsed[0];
                currentEntry->type = -1;

                //In CBPS DB titleID is used for id, so if any contradict _n is added, this is done to get just the id
                char titleID[10];
                sce_paf_memset(titleID, 0, sizeof(titleID));
                sce_paf_strncpy(titleID, parsed[0], 10);

                currentEntry->titleID = titleID;
                currentEntry->icon0Local.Setf(CBPSDB_ICON_SAVE_PATH "/%s.png",  currentEntry->id.data);
                currentEntry->version = "";

                currentEntry->hash = Utils::GetHashById(currentEntry->id.data);
            }
        }

        free_csv_line(parsed);
    }

    *outListNum = num;
    *outList = entries;
}

int db::GetNumByCategory(db::entryInfo *list, int num, int category)
{
    if(category == -1) return num;

    int count = 0;

    db::entryInfo *currEntry = list;
    for(int i = 0; i < num; i++, currEntry++)
    {
        if(currEntry->type == category) count++;
    }

    return count;
}

db::entryInfo *db::GetByCategoryIndex(db::entryInfo *list, int listNum, int index, int category)
{

    db::entryInfo *currEntry = list;
    for(int i = 0; i < listNum && i < index; i++, currEntry++)
    {
        if(currEntry->type != category) i--;
    }

    return currEntry;
}