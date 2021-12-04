#ifndef BHH_PARSER_HPP 
#define BHH_PARSER_HPP

#include <kernel.h>
#include <paf.h>
using namespace paf;

typedef enum
{
    VPK,
    DATA
} homebrewType;

typedef enum
{
    GAME = 1,
    PORT = 2,
    EMULATOR = 5,
    UTIL = 4
} Category;

typedef struct
{
    paf::String id;
    paf::String titleID;
    paf::WString wstrtitle;
    paf::String title;
    paf::String credits;
    paf::String icon0;
    paf::String download_url;
    paf::String options;
    paf::String icon0Local;
    paf::String description;
    paf::String screenshot_url;
    paf::String version;
    paf::String size;
    Category type;

} homeBrewInfo;

struct node
{
    homeBrewInfo info;
    node *next;
    widget::ImageButton *button;
    paf::graphics::Texture *tex;
};

class LinkedList
{
public:
    node *head, *tail;
    LinkedList();
    void PrintAll();

    int num;

    int GetNumByCategory(int category);

    void LinkedList::AddFromPointer(node *p);
    void Clear(bool deleteTex = false);
    homeBrewInfo *AddNode();
    void RemoveNode(const char *tag);
    homeBrewInfo *Get(const char *id);
    node *GetByIndex(int n);
    node *GetByCategoryIndex(int n, int category);
    void CopyTo(LinkedList *list);

    node *Find(const char *name);

};

void parseJson(const char *path);
void parseCSV(const char *path);

#endif