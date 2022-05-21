#ifndef BHH_PARSER_HPP 
#define BHH_PARSER_HPP

#include <paf.h>

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
    paf::string id;
    paf::string titleID;
    paf::wstring wstrtitle;
    paf::string title;
    paf::string credits;
    paf::string icon0;
    paf::string download_url;
    paf::string options;
    paf::string icon0Local;
    paf::string description;
    paf::string screenshot_url;
    paf::string version;
    paf::string size;
    Category type;

} homeBrewInfo;

struct node
{
    homeBrewInfo info;
    node *next;

    paf::ui::ImageButton *button;
    paf::graphics::Surface *tex;
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