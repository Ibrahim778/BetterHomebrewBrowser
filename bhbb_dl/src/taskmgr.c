#include <kernel.h>
#include <paf/std/stdio.h>

#include "taskmgr.h"

static taskNode *head = SCE_NULL;
extern SceUID taskMutex; 

int DoesTaskExist(const char *name)
{
    if(taskMutex == SCE_UID_INVALID_UID)
        return -1;

    sceKernelLockMutex(taskMutex, 1, SCE_NULL);
    
    taskNode *node = head;
    while(head != SCE_NULL)
    {
        if(sce_paf_strncmp(node->data.name, name, sizeof(node->data.name)) == 0)
        {
            sceKernelUnlockMutex(taskMutex, 1);
            return 1;
        }
        node = head->prev;
    }
    
    sceKernelUnlockMutex(taskMutex, 1);
    return 0;
}

int GetCurrentTask(cBGDLItem *pOut)
{
    if(taskMutex == SCE_UID_INVALID_UID)
        return -1;

    sceKernelLockMutex(taskMutex, 1, SCE_NULL);
    
    if(head == SCE_NULL)
    {
        sceKernelUnlockMutex(taskMutex, 1);
        return -1;
    }

    sce_paf_memcpy(pOut, &head->data, sizeof(cBGDLItem));

    sceKernelUnlockMutex(taskMutex, 1);
    return 0;
}

void EnqueueTask(cBGDLItem *item)
{
    if(taskMutex == SCE_UID_INVALID_UID)
        return;
    
    sceKernelLockMutex(taskMutex, 1, SCE_NULL);

    taskNode *t_node = sce_paf_malloc(sizeof(taskNode));
    t_node->prev = head; 
    sce_paf_memcpy(&t_node->data, item, sizeof(cBGDLItem));
    head = t_node;

    sceKernelUnlockMutex(taskMutex, 1);
}

int GetTaskNum()
{
    if(taskMutex == SCE_UID_INVALID_UID)
        return 0;

    sceKernelLockMutex(taskMutex, 1, SCE_NULL);

    int count = 0;
    taskNode *node = head;
    while(node != SCE_NULL)
    {
        count++;
        node = node->prev;
    }    
    
    sceKernelUnlockMutex(taskMutex, 1);
    return count;
}

void DequeueCurrentTask()
{
    if(taskMutex == SCE_UID_INVALID_UID)
        return;

    sceKernelLockMutex(taskMutex, 1, SCE_NULL);
    
    //Check head isn't NULL
    if (head == NULL)
    {
        sceKernelUnlockMutex(taskMutex, 1);
        return;
    }

    taskNode *nodeToDelete = head;
    head = head->prev;
    sce_paf_free(nodeToDelete);
    
    sceKernelUnlockMutex(taskMutex, 1);
}