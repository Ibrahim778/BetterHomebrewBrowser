#ifndef BHBB_DL_LIST_CPP 
#define BHBB_DL_LIST_CPP

#include <kernel.h>
#include <paf/stdc.h>
#include "bhbb_dl.h"

#define free sce_paf_free
#define malloc sce_paf_malloc

struct qnode
{
    bhbbPacket data;
    qnode *next;
};

class Queue
{
public:
    qnode *head, *tail;
    Queue();

    void printall();

    int num;

    void clear();

    void enqueue(bhbbPacket *data, int dataSize);

    void dequeue();

    bhbbPacket *Find(const char *name);
    void remove(const char *url);

};

#endif