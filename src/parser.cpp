#include <stdio.h>
#include <paf.h>
#include <json.h>
#include <libsysmodule.h>

#include "parser.hpp"
#include "csv.h"
#include "common.hpp"
#include "main.hpp"
#include "utils.hpp"

using namespace parser;

HomebrewList::HomebrewList()
{
    head = NULL;
    tail = NULL;
    num = 0;
}

void HomebrewList::PrintAll()
{
#ifdef _DEBUG
    node *current = head;
    while (current != NULL)
    {
        print("%s\n", current->info.title.data);
        current = current->next;
    }
#endif
}

void HomebrewList::AddFromPointer(node *p)
{
    node *tmp = new node;
    sce_paf_memset(tmp, 0, sizeof(node));
    sce_paf_memcpy(&tmp->info, &p->info, sizeof(node::info));


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

HomebrewList::homeBrewInfo *HomebrewList::AddNode()
{
    node *tmp = new node;
    sce_paf_memset(tmp, 0, sizeof(node));

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
    return &tmp->info;
}

void HomebrewList::Clear(bool deleteTex)
{
    if(head == NULL) return;

    node *temp = head;
    while(temp != NULL)
    {
        node *next = temp->next;
        if(deleteTex)
            Utils::DeleteTexture(&temp->tex);
        delete temp;
        temp = next;
    }

    num = 0;

    head = NULL;
    tail = NULL;
}

int HomebrewList::GetNumByCategory(int cat)
{
    if(cat == -1) return num;

    int i = 0;
    node *n = head;
    while (n != NULL)
    {
        if(n->info.type == cat) i++;
        n = n->next;
    }

    return i;
}

void HomebrewList::CopyTo(HomebrewList *list)
{
    if(list == NULL) return;
    node *n = head;
    for(int i = 0; i < num && n != NULL, n = n->next; i++)
    {
        list->AddFromPointer(n);
    }
}

HomebrewList::node *HomebrewList::GetByIndex(int n)
{
    node *node = head;

    for (int i = 0; i < n && node != NULL; i++)
        node = node->next;

    return node;
}

HomebrewList::node *HomebrewList::GetByCategoryIndex(int n, int category)
{
    node *node = head;

    for (int i = 0; i < n && node != NULL; i++)
    {
        if(node->info.type != category && category != -1)
        {
            i--;
        }
        node = node->next;
    }
    return node;
}

HomebrewList::homeBrewInfo *HomebrewList::Get(const char *id)
{
    node *curr = head;
    while (curr != NULL)
    {
        if(sce_paf_strcmp(curr->info.id.data, id))
            return &curr->info;
        
        curr = curr->next;
    }
    return NULL;
}

HomebrewList::node *HomebrewList::Find(const char *name)
{
    node *curr = head;
    int nLen = sce_paf_strlen(name);
    while(curr != NULL)
    {
        if(sce_paf_strncmp(curr->info.title.data, name, nLen) == 0)
            return curr;
        
        curr = curr->next;
    }
    return NULL;
}

//Credit to CreepNT for this
void HomebrewList::RemoveNode(const char *tag)
{
    //Check head isn't NULL
    if (head == NULL)
        return;

    //First, handle the case where we free the head
    if (sce_paf_strcmp(head->info.id.data, tag) == 0)
    {
        node *nodeToDelete = head;
        head = head->next;
        sce_paf_free(nodeToDelete);
        return;
    }

    //Bail out if the head is the only node
    if (head->next == NULL)
        return;

    //Else, try to locate node we're asked to remove
    node **pCurrentNodeNext = &head; //This points to the current node's `next` field (or to pHead)
    while (1)
    {
        if (sce_paf_strcmp((*pCurrentNodeNext)->info.id.data, tag) == 0) //pCurrentNodeNext points to the pointer that points to the node we need to delete
            break;

        //If the next node's next is NULL, we reached the end of the list. Bail out.
        if ((*pCurrentNodeNext)->next == NULL)
            return;

        pCurrentNodeNext = &(*pCurrentNodeNext)->next;
    }
    node *nodeToDelete = *pCurrentNodeNext;
    *pCurrentNodeNext = (*pCurrentNodeNext)->next;
    Utils::DeleteTexture(&nodeToDelete->tex);
    sce_paf_free(nodeToDelete);
    num --;
}