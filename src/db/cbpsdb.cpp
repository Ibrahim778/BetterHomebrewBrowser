#include <paf.h>

#include "db/cbpsdb.h"
#include "bhbb_locale.h"

using namespace paf;

#define CBPSDB_SAVE_PATH "savedata0:cbpsdb.csv"
#define CBPSDB_INDEX_URL "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv"

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
}

CBPSDB::~CBPSDB()
{
    
}

int CBPSDB::GetDownloadURL(Source::Entry& entry, paf::string& out)
{
    return 0;
}

int CBPSDB::GetDataURL(Source::Entry& entry, paf::string& out)
{
    return 0;
}

int CBPSDB::DownloadIndex(bool forceRefresh)
{
    return 0;
}

int CBPSDB::GetDescription(Source::Entry& entry, paf::wstring& out)
{
    return 0;
}

int CBPSDB::Parse()
{
    return 0;
}
