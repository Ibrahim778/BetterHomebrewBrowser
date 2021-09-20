#ifndef BHH_DL_LIST_CPP 
#define BHH_DL_LIST_CPP

#include <kernel.h>
#include "bhbb_dl.h"

#define LIST_TYPE bhbbPacket

struct node
{
    LIST_TYPE packet;
    node *next;
};

class Queue
{
public:
    node *head, *tail;
    Queue();
    void printall();

    int num;

    void clear();
    void enqueue(LIST_TYPE *data, int dataSize);
    void dequeue();
    LIST_TYPE *Find(const char *name);
    void remove(const char *url);

};

#endif