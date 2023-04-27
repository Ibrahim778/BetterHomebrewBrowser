#include <paf.h>

#include "db/source.h"
#include "print.h"

Source::List::List()
{

}

Source::List::~List()
{
    Clear();
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

void Source::List::Add(Source::Entry& entry)
{
    entries.push_back(entry);
}

size_t Source::List::GetSize(int category)
{
    if(category == Source::CategoryAll)
        return entries.size();
    
    size_t out = 0;
    for(Source::Entry& entry : entries)
        if(entry.category == category) out++;
    
    return out;
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

void Source::List::Categorise(Source *source)
{
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
