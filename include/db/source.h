/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2024 Muhammad Ibrahim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/

#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <paf.h>
#include <vector>

#include "bhbb_dl.h"

class Source
{
public:
    static const int CategoryAll = -1;

    enum ID 
    {
        CBPS_DB = 0,
        VITA_DB,
        VHB_DB,
        PSPHBB,
    };

    struct CategoryInfo 
    {
        CategoryInfo(int _id, uint32_t sHash, uint32_t nHash):id(_id),singleHash(sHash),nameHash(nHash)
        {

        }

        int id;
        uint32_t singleHash;
        uint32_t nameHash;
    };

    class Entry
    {
    public:
        Entry(Source *source):hash(0xDEADBEEF),pSource(source)
        {
            
        };

        Source *pSource;

        uint32_t hash;
        int category;
        
        paf::wstring titleID;
        paf::wstring title;
        paf::wstring author;
        
        std::vector<paf::string> iconURL;
        paf::string iconPath;

        std::vector<paf::string> dataURL;
        paf::string dataPath;

        paf::wstring description;
        std::vector<paf::string> downloadURL;

        std::vector<paf::string> screenshotURL;
        
        paf::wstring version;

        paf::datetime::DateTime lastUpdated;
        unsigned int downloadNum;

        size_t downloadSize;
        size_t dataSize;
    };

    class List
    {
    public:
        struct CategorisedEntries
        {
            CategorisedEntries(int _category):category(_category){}

            int category;
            std::vector<Source::Entry *> entries;
        };

        typedef bool (*SortFunc)(Source::Entry&a, Source::Entry& b);

        List();
        virtual ~List();

        void Categorise(Source *source);
        void Clear();
        void Sort(SortFunc func);

        void Add(Source::Entry &entry)
        {
            entries.push_back(entry);
        }

        void Add(const Source::Entry &entry)
        {
            entries.push_back(entry);
        }

        size_t GetSize(int categoryID = -1);
        Source::Entry& Get(uint32_t hash);
        CategorisedEntries& GetCategory(int categoryID);
        
        std::vector<Source::Entry> entries;
    
        static bool Sort_MostDownloaded(Source::Entry &a,Source::Entry &b);
        static bool Sort_LeastDownloaded(Source::Entry &a,Source::Entry &b);
        static bool Sort_MostRecent(Source::Entry &a,Source::Entry &b);
        static bool Sort_OldestFirst(Source::Entry &a,Source::Entry &b);
        static bool Sort_Alphabetical(Source::Entry &a,Source::Entry &b);
        static bool Sort_AlphabeticalRev(Source::Entry &a,Source::Entry &b);
        static bool Sort_None(Source::Entry &a,Source::Entry &b)
        {
            return false;
        }

    private:
        std::vector<CategorisedEntries> categoriedEntries;
    };

    struct SortMode // Could use a map for this maybe?
    {
        SortMode(uint32_t _hash, List::SortFunc _func):hash(_hash),func(_func)
        {

        }

        uint32_t hash; // label hash
        List::SortFunc func;
    };

    enum IconAspectRatio 
    {
        r1x1,
        r67x37 // PSPHBB uses this for icons (wtf?)
    };

    Source();

    static paf::common::SharedPtr<Source> Create(ID id);

    virtual ~Source(){}; 

    virtual int Parse() = 0;
    virtual int DownloadIndex(bool forceRefresh = false) = 0;
    virtual int GetDescription(Entry &entry, paf::wstring& out) = 0;
    virtual int GetDownloadURL(Entry &entry, paf::string& out) = 0;
    virtual int GetDataURL(Entry &entry, paf::string& out) = 0;
    virtual wchar_t *GetName() = 0;
    
    // These functions HAVE defaults and sources are not expected to override them unless they have special download logic
    virtual int CreateDownloadParam(Entry& entry, BGDLParam& param);
    virtual int CreateDataDownloadParam(Entry& entry, BGDLParam& param);

    const CategoryInfo& GetCategoryByID(int id);
    const Source::CategoryInfo& GetCategoryByHash(uint32_t hash);
    List::SortFunc GetSortFunction(uint32_t hash);

    // List to parse to
    List *pList;

    // Info
    IconAspectRatio iconRatio;
    paf::string iconFolderPath;
    paf::string iconsURL;
    std::vector<CategoryInfo> categories;
    std::vector<SortMode> sortModes; // first sort mode will be default!
    bool screenshotsSupported;
    uint8_t id;
};

#endif