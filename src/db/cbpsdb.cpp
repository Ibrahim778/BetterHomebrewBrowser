#include <paf.h>

#include "db/cbpsdb.h"
#include "bhbb_locale.h"

using namespace paf;

CBPSDB::CBPSDB()
{
    name = "CBPS DB";
    iconFolderPath = "ux0:/data/betterHomebrewBrowser/icons/cbpsdb/";
    iconsURL = "https://github.com/Ibrahim778/CBPS-DB-Icon-Downloader/raw/main/icons.zip?raw=true";
    indexURL = "https://raw.githubusercontent.com/KuromeSan/cbps-db/master/cbpsdb.csv";
    indexPath = "ux0:temp/cbpsdb.csv";
    screenshotsSupported = true;
    categories = {
        Source::Category(
            CategoryAll,
            db_category_all,
            db_category_all
        )
    };
}

CBPSDB::~CBPSDB()
{
    
}

int CBPSDB::GetDownloadURL(Source::Entry& entry, string& out)
{

}

int CBPSDB::GetDataURL(Source::Entry& entry, string& out)
{

}

int CBPSDB::DownloadIndex()
{

}

int CBPSDB::GetDescription(Source::Entry& entry, wstring& out)
{

}

int CBPSDB::Parse()
{
    
}
