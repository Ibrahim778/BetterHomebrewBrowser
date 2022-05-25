#include <paf.h>

#include "common.hpp"
#include "db.hpp"
#include "csv.h"
#include "main.hpp"

using namespace db;

void vitadb::Parse(const char *json)
{
    
}

void cbpsdb::Parse(const char *csv)
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