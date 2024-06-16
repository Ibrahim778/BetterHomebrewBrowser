#ifndef _PSPHBB_H_
#define _PSPHBB_H_

#include <paf.h>

#include "source.h"
#include "bhbb_dl.h"

class PSPHBDB : public Source 
{
public:
    PSPHBDB();
    virtual ~PSPHBDB();

    int Parse();
    int DownloadIndex(bool forceRefresh);
    int GetDescription(Entry &entry, paf::wstring& out);
    int GetDownloadURL(Entry &entry, paf::string& out);
    int GetDataURL(Entry &entry, paf::string& out);
    wchar_t *GetName()
    {
        return L"PSP Homebrew Browser";
    }

    int CreateDownloadParam(Entry &entry, BGDLParam &param) override;

protected:
    enum Category 
    {
        GAME = 1,
        UTIL,
        EMU,
        PORT
    };
};

#endif