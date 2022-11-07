#include <kernel.h>
#include <paf/stdc.h>

#include "bgdl.h"
#include "bhbb_dl.h"

int EnqueueCBGDLTask(const char *name, const char *url, const char *dest)
{
    SceUID pipeId = sceKernelOpenMsgPipe(BHBB_DL_PIPE_NAME);
    if(pipeId < 0) return pipeId;
    
    cBGDLItem pkt;
    sce_paf_memset(&pkt, 0, sizeof(pkt));
    
    sce_paf_strncpy(pkt.url, url, sizeof(pkt.url));
    sce_paf_strncpy(pkt.name, name, sizeof(pkt.name));
    sce_paf_strncpy(pkt.dest, dest, sizeof(pkt.dest));

    SceInt32 r = sceKernelSendMsgPipe(pipeId, &pkt, sizeof(pkt), SCE_KERNEL_MSG_PIPE_MODE_WAIT, NULL, NULL);
    if(r < 0) return r;

    return sceKernelCloseMsgPipe(pipeId);
}