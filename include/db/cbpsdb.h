#ifndef _CPBSDB_H_
#define _CPBSDB_H_

#include <paf.h>

#include "source.h"

class CBPSDB : public Source
{
public:

    CBPSDB();
    virtual ~CBPSDB();

    int Parse();
    int DownloadIndex();
    int GetDescription(Entry &entry, paf::wstring& out);
    int GetDownloadURL(Entry &entry, paf::string& out);
    int GetDataURL(Entry &entry, paf::string& out);
};

#endif