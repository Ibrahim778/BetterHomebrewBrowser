#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <paf.h>

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
            sceClibPrintf("Adding elem\n");
            iconURL.push_back("Hey");
            sceClibPrintf("Done\n");
        };

        Source *pSource;

        uint32_t hash;
        int category;
        
        paf::wstring titleID;
        paf::wstring title;
        paf::wstring author;
        
        paf::vector<paf::string> iconURL;
        paf::string iconPath;

        paf::vector<paf::string> dataURL;
        paf::string dataPath;

        paf::wstring description;
        paf::vector<paf::string> downloadURL;

        paf::vector<paf::string> screenshotURL;
        
        paf::wstring version;
    };

    class List
    {
    public:
        struct CategorisedEntries
        {
            CategorisedEntries(int _category):category(_category){}

            int category;
            paf::vector<Source::Entry *> entries;
        };

        List();
        virtual ~List();

        void Categorise(Source *source);
        void Add(Source::Entry &entry);
        void Clear();

        size_t GetSize(int categoryID = -1);
        Source::Entry& Get(uint32_t hash);
        CategorisedEntries& GetCategory(int categoryID);
        
        paf::vector<Source::Entry> entries;
        paf::vector<CategorisedEntries> categoriedEntries;
    };

    Source(){}
    virtual ~Source(){}

    virtual int Parse(){};
    virtual int DownloadIndex(){};
    virtual int GetDescription(Entry &entry, paf::wstring& out){};
    virtual int GetDownloadURL(Entry &entry, paf::string& out){};
    virtual int GetDataURL(Entry &entry, paf::string& out){};

    const Category& GetCategoryByID(int id);
    const Source::Category& GetCategoryByHash(uint32_t hash);

    // List to parse to
    List *pList;

    // Info
    paf::string name;
    paf::string iconFolderPath;
    paf::string iconsURL;
    paf::string indexURL;
    paf::string indexPath;
    paf::vector<Category> categories;
    bool screenshotsSupported;
    uint8_t id;
};

#endif