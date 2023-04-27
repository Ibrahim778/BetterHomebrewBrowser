#ifndef _VHBDB_H_
#define _VHBDB_H_

#include <paf.h>

#include "source.h"

class VHBDB : public Source
{
public:
    VHBDB();
    virtual ~VHBDB();

    int Parse();
    int DownloadIndex();
    int GetDescription(Entry &entry, paf::wstring& out);
    int GetDownloadURL(Entry &entry, paf::string& out);
    int GetDataURL(Entry &entry, paf::string& out);
};

#endif