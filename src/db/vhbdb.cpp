#include <paf.h>

#include "db/vhbdb.h"
#include "bhbb_locale.h"

using namespace paf;

VHBDB::VHBDB()
{
    name = "VHB DB";
    iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/vhbdb/";
    indexURL = "https://github.com/vhbd/database/releases/download/latest/db_minify.json";
    indexPath = "ux0:temp/vhbdb.json";
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
}

VHBDB::~VHBDB()
{
    
}

int VHBDB::DownloadIndex()
{

}

int VHBDB::GetDownloadURL(Source::Entry& entry, string& out)
{

}

int VHBDB::GetDataURL(Source::Entry& entry, string& out)
{

}

int VHBDB::GetDescription(Source::Entry& entry, wstring& out)
{

}

int VHBDB::Parse()
{
    
}
