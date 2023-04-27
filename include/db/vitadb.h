#ifndef _VITADB_H_
#define _VITADB_H_

#include <paf.h>

#include "source.h"

class VitaDB : public Source
{
public:
    VitaDB();
    virtual ~VitaDB();

    int Parse();
    int DownloadIndex();
    int GetDescription(Entry &entry, paf::wstring& out);
    int GetDownloadURL(Entry &entry, paf::string& out);
    int GetDataURL(Entry &entry, paf::string& out);
};

#endif