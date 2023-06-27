#include <paf.h>
#include <algorithm>

#include "db/source.h"
#include "print.h"

#include "db/vitadb.h"
#include "db/vhbd.h"
#include "db/cbpsdb.h"

Source::List::List()
{

}

Source::List::~List()
{
    Clear();
}

paf::common::SharedPtr<Source> Source::Create(Source::ID id)
{
    switch(id)
    {
    case VITA_DB:
        return paf::common::SharedPtr<Source>(new VitaDB());
    case VHB_DB:
        return paf::common::SharedPtr<Source>(new VHBD());
    case CBPS_DB:
        return paf::common::SharedPtr<Source>(new CBPSDB());
    }
}

const Source::Category& Source::GetCategoryByID(int id)
{
    for(const Source::Category& cat : categories)
        if(cat.id == id)
            return cat;
        
    const Source::Category notFound(0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF);
    return notFound;
}

const Source::Category& Source::GetCategoryByHash(uint32_t hash)
{
    for(const Source::Category& cat : categories)
        if(cat.nameHash == hash)
            return cat;
        
    const Source::Category notFound(0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF);
    return notFound;
}

void Source::List::Clear()
{
    categoriedEntries.clear();
    entries.clear();
}

size_t Source::List::GetSize(int category)
{
    if(category == Source::CategoryAll)
        return entries.size();
    
    for(auto& cat : categoriedEntries)
        if(cat.category == category)
            return cat.entries.size();
    
    return -1;
}

Source::Entry& Source::List::Get(uint32_t hash)
{
    for(Source::Entry& entry : entries)
        if(entry.hash == hash) return entry;
}

Source::List::CategorisedEntries& Source::List::GetCategory(int category)
{
    auto errList = List::CategorisedEntries(0xDEADBEEF);
    if(category == Source::CategoryAll)
    {
        print("[Error] Please use the entries property in order to acces an element without a specific category!\n");
        return errList; 
    }

    for(CategorisedEntries &catList : categoriedEntries)
    {
        if(catList.category == category)
            return catList;
    }
    return errList;
}

Source::List::SortFunc Source::GetSortFunction(uint32_t hash)
{
    for(auto& i : sortModes)
    {
        if(i.hash == hash)
            return i.func;
    }
    return List::Sort_None;
}

void Source::List::Categorise(Source *source)
{
    if(!categoriedEntries.empty())
        categoriedEntries.clear();

    for (Source::Category &category : source->categories)
    {
        if(category.id == Source::CategoryAll)
            continue;
        
        CategorisedEntries entryList = CategorisedEntries(category.id);

        for(Source::Entry& entry : entries)
            if(entry.category == category.id)
                entryList.entries.push_back(&entry);
        
        categoriedEntries.push_back(entryList);
    }
}

bool Source::List::Sort_MostRecent(Source::Entry &a, Source::Entry &b)
{
    return b.lastUpdated < a.lastUpdated;
}

bool Source::List::Sort_OldestFirst(Source::Entry &a, Source::Entry &b)
{
    return a.lastUpdated < b.lastUpdated;
}

bool Source::List::Sort_MostDownloaded(Source::Entry &a, Source::Entry &b)
{
    return b.downloadNum < a.downloadNum;
}

bool Source::List::Sort_LeastDownloaded(Source::Entry &a, Source::Entry &b)
{
    return a.downloadNum < b.downloadNum;
}

bool Source::List::Sort_Alphabetical(Source::Entry &a, Source::Entry &b)
{
    return sce_paf_wcscasecmp(a.title.c_str(), b.title.c_str()) < 0;
}

bool Source::List::Sort_AlphabeticalRev(Source::Entry &a, Source::Entry &b)
{
    return sce_paf_wcscasecmp(b.title.c_str(), a.title.c_str()) < 0;
}

void Source::List::Sort(SortFunc func)
{
    std::sort(entries.begin(), entries.end(), func);
}