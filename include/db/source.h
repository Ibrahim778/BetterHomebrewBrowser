#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <paf.h>
#include <vector>

class Source
{
public:
    static const int CategoryAll = -1;

    enum ID 
    {
        CBPS_DB = 0,
        VITA_DB,
        VHB_DB,
    };

    struct Category 
    {
        Category(int _id, uint32_t sHash, uint32_t nHash):id(_id),singleHash(sHash),nameHash(nHash)
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

    Source(){}

    static Source *Create(ID id);

    virtual ~Source(){}

    virtual int Parse(){};
    virtual int DownloadIndex(bool forceRefresh = false){};
    virtual int GetDescription(Entry &entry, paf::wstring& out){};
    virtual int GetDownloadURL(Entry &entry, paf::string& out){};
    virtual int GetDataURL(Entry &entry, paf::string& out){};

    const Category& GetCategoryByID(int id);
    const Source::Category& GetCategoryByHash(uint32_t hash);
    List::SortFunc GetSortFunction(uint32_t hash);

    // List to parse to
    List *pList;

    // Info
    paf::string iconFolderPath;
    paf::string iconsURL;
    std::vector<Category> categories;
    std::vector<SortMode> sortModes; // first sort mode will be default!
    bool screenshotsSupported;
    uint8_t id;
};

#endif