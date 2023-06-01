#ifndef _VHBDB_H_
#define _VHBDB_H_

#include <paf.h>

#include "source.h"

class VHBD : public Source
{
public:
    VHBD();
    virtual ~VHBD();

    int Parse();
    int DownloadIndex(bool forceRefresh);
    int GetDescription(Entry &entry, paf::wstring& out);
    int GetDownloadURL(Entry &entry, paf::string& out);
    int GetDataURL(Entry &entry, paf::string& out);

};

#endif