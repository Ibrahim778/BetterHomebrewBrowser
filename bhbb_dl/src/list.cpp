#include "list.hpp"
#include "main.hpp"
#include "notifmgr.hpp"
#include <paf/stdc.h>

#define free sce_paf_free
#define malloc sce_paf_malloc

Queue::Queue()
{
    head = NULL;
    tail = NULL;
    num = 0;
}

void Queue::printall()
{
#ifdef _DEBUG
    node *current = head;
    while (current != NULL)
    {
        print("%s, ", current->packet.url);
        current = current->next;
    }
#endif
}

LIST_TYPE *Queue::Find(const char *name)
{
    node *n = head;
    while(n != NULL)
    {
        if(sce_paf_strcmp(name, n->packet.name) == 0) return &n->packet;
        n = n->next;
    }
    return NULL;
}

void Queue::remove(const char *url)
{
    //Check head isn't NULL
    if (head == NULL)
        return;

    //First, handle the case where we free the head
    if (sce_paf_strcmp(head->packet.url, url) == 0)
    {
        node *nodeToDelete = head;
        head = head->next;
        free(nodeToDelete);
        return;
    }

    //Bail out if the head is the only node
    if (head->next == NULL)
        return;

    //Else, try to locate node we're asked to remove
    node **pCurrentNodeNext = &head; //This points to the current node's `next` field (or to pHead)
    while (1)
    {
        if (sce_paf_strcmp((*pCurrentNodeNext)->packet.url, url) == 0) //pCurrentNodeNext points to the pointer that points to the node we need to delete
            break;

        //If the next node's next is NULL, we reached the end of the list. Bail out.
        if ((*pCurrentNodeNext)->next == NULL)
            return;

        pCurrentNodeNext = &(*pCurrentNodeNext)->next;
    }
    node *nodeToDelete = *pCurrentNodeNext;
    *pCurrentNodeNext = (*pCurrentNodeNext)->next;
    free(nodeToDelete);

    num --;
}

void Queue::enqueue(LIST_TYPE *data, int dataSize)
{
    if(num > 1)
    {
        char txt[64];
        sce_paf_memset(txt, 0, sizeof(txt));
        sce_paf_snprintf(txt, sizeof(txt), "Added %s to queue", data->name);
        NotifMgr::SendNotif(txt);
    }
    node *tmp = new node;
    sce_paf_memset(&tmp->packet, 0, sizeof(tmp->packet));
    sce_paf_memcpy(&tmp->packet, data, dataSize);
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

void Queue::clear()
{
    if(head == NULL) return;

    node *temp;
    while(head != NULL)
    {
        temp = head;
        head = head->next;
        delete temp;
    }

    num = 0;
}

void Queue::dequeue()
{
    //Check head isn't NULL
    if (head == NULL)
        return;
    
    node *nodeToDelete = head;
    head = head->next;
    free(nodeToDelete);
    num --;
    return;
}
