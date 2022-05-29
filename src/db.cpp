#include <paf.h>
#include <json.h>
#include <libsysmodule.h>

#include "common.hpp"
#include "db.hpp"
#include "csv.h"
#include "main.hpp"
#include "json.hpp"

using namespace sce;
using namespace db;

#define SET_STRING(pafString, jsonString) { if(rootval[i][jsonString] != NULL) { pafString = rootval[i][jsonString].getString().c_str(); } } 
void vitadb::Parse(const char *jsonStr, int length)
{
    list.Clear();

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
        
        int length = rootval.count();
        for(int i = 0; i < length; i++)
        {
            if(rootval[i] == NULL) return;
            parser::HomebrewList::homeBrewInfo *info = list.AddNode();

            SET_STRING(info->titleID, "titleid");
            SET_STRING(info->id, "id");
            
            if(rootval[i]["icon"] != NULL)
                info->icon0.Setf("https://rinnegatamante.it/vitadb/icons/%s", rootval[i]["icon"].getString().c_str());
            
            if(rootval[i]["icon"] != NULL)
                info->icon0Local.Setf(VITADB_ICON_SAVE_PATH "/%s", rootval[i]["icon"].getString().c_str());

            SET_STRING(info->title, "name");
            info->title.ToWString(&info->wstrtitle);

            SET_STRING(info->download_url, "url");
            SET_STRING(info->credits, "author");
            SET_STRING(info->options, "data");
            SET_STRING(info->description, "long_description");
            SET_STRING(info->screenshot_url, "screenshots");
            SET_STRING(info->version, "version");        

            if(rootval[i]["type"] != NULL)
                info->type = (parser::HomebrewList::Category)sce_paf_strtoul(rootval[i]["type"].getString().c_str(), NULL, 10);

            SET_STRING(info->size, "size");
        }
    }
}

void cbpsdb::Parse(const char *csv, int length)
{
    list.Clear(true);

    char *line;
    while ((line = getLine(csv)) != NULL)
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
                parser::HomebrewList::homeBrewInfo *info = list.AddNode();

                info->id = parsed[0];

                info->title = parsed[1];

                info->title.ToWString(&info->wstrtitle);           
                info->credits = parsed[2];
                if(parsed[3][0] == 'h')
                    info->icon0 = parsed[3];
                else info->icon0.Clear();
                info->download_url = parsed[5];
                info->options = parsed[13];
                info->description = parsed[0];

                //In CBPS DB titleID is used for id, so if any contradict _n is added, this is done to get just the id
                char titleID[10];
                sce_paf_memset(titleID, 0, sizeof(titleID));
                sce_paf_strncpy(titleID, parsed[0], 10);

                info->titleID = titleID;
                info->icon0Local.Setf(CBPSDB_ICON_SAVE_PATH "/%s.png",  info->id.data);
                info->version = "";
            }
        }

        free_csv_line(parsed);
    }
}