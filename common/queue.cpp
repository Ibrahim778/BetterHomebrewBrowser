#ifndef BHBB_DL_LIST_CPP 
#define BHBB_DL_LIST_CPP

#include <kernel.h>
#include <paf/stdc.h>

#define free sce_paf_free
#define malloc sce_paf_malloc

struct qnode
{
    LIST_TYPE data;
    qnode *next;
};

class Queue
{
public:
    qnode *head, *tail;
    Queue()
    {
        head = NULL;
        tail = NULL;
        num = 0;
    }

    void printall();

    int num;

    void clear()
    {
        if(head == NULL) return;

        qnode *temp;
        while(head != NULL)
        {
            temp = head;
            head = head->next;
            delete temp;
        }

        num = 0;
    }
    void enqueue(LIST_TYPE *data, int dataSize)
    {
        qnode *tmp = new qnode;
        sce_paf_memset(&tmp->data, 0, sizeof(tmp->data));
        sce_paf_memcpy(&tmp->data, data, dataSize);
        tmp->next = NULL;

        if (head == NULL)
        {
            head = tmp;
            tail = tmp;
        }
        else
        {
            tail->next = tmp;
            tail = tail->next;
        }
        num ++;
    }

    void dequeue()
    {
        sceClibPrintf("Dequeueing!\n");
        //Check head isn't NULL
        if (head == NULL)
            return;
        sceClibPrintf("DE: 1\n");
        
        qnode *nodeToDelete = head;
        sceClibPrintf("DE: 2\n");
        head = head->next;
        sceClibPrintf("DE: 3\n");
        free(nodeToDelete);
        sceClibPrintf("DE: 4\n");
        num --;
        sceClibPrintf("DE: 5\nDone!\n");
        return;
    }

    LIST_TYPE *Find(const char *name);
    void remove(const char *url);

};

#endif