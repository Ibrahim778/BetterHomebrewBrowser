#ifndef TASKMGR_H
#define TASKMGR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../common/bhbb_dl.h"

typedef struct tNode {
    cBGDLItem data;
    struct tNode *prev;
} taskNode;

int GetTaskNum();
void EnqueueTask(cBGDLItem *task);
void DequeueCurrentTask();
int GetCurrentTask(cBGDLItem *pOut);
int DoesTaskExist(const char *name);

#ifdef __cplusplus
}
#endif
#endif