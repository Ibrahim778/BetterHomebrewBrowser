#ifndef BHH_PARSER_CPP 
#define BHH_PARSER_CPP

#include <kernel.h>
#include <paf.h>
using namespace paf;

typedef enum
{
    VPK,
    DATA
} homebrewType;

typedef struct
{
    String id;
    String titleID;
    String title;
    String credits;
    String icon0;
    String download_url;
    String options;

    String icon0Local;

} homeBrewInfo;

struct node
{
    homeBrewInfo widget;
    node *next;
    widget::ImageButton *button;
};

class linked_list
{
public:
    node *head, *tail;
    linked_list();
    void printall();

    int num;

    void clear();
    homeBrewInfo *add_node();
    void remove_node(const char *tag);
    homeBrewInfo *get(const char *id);
    node *getByNum(int n);

};

void parseJson(const char *path);
void parseCSV(const char *path);

#endif