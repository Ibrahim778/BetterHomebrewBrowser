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
    int DownloadIndex(bool forceRefresh);
    int GetDescription(Entry &entry, paf::wstring& out);
    int GetDownloadURL(Entry &entry, paf::string& out);
    int GetDataURL(Entry &entry, paf::string& out);

protected:
    static int GetSCECompatibleURL(std::vector<paf::string> &urlList, paf::string &out);
    static size_t SaveCore(char *ptr, size_t size, size_t nmeb, VitaDB *workDB);

    size_t buffSize;
    char *buff;
};

#endif