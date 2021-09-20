#include <kernel.h>
#include "..\bhbb_dl\src\bhbb_dl.h"
#include <paf/stdc.h>

void SendDlRequest(const char *title, const char *url)
{
    SceUID pipeId = sceKernelOpenMsgPipe(BHBB_DL_PIPE_NAME);
    if(pipeId < 0) return;
    
    bhbbPacket pkt;
    sce_paf_memset(&pkt, 0, sizeof(pkt));
    pkt.cmd = INSTALL;
    sce_paf_strncpy(pkt.url, url, sizeof(pkt.url));
    sce_paf_strncpy(pkt.name, title, sizeof(pkt.name));

    sceKernelSendMsgPipe(pipeId, &pkt, sizeof(pkt), SCE_KERNEL_MSG_PIPE_MODE_WAIT, NULL, NULL);

    sceKernelCloseMsgPipe(pipeId);
}

void termBhbbDl()
{
    SceUID pipeId = sceKernelOpenMsgPipe(BHBB_DL_PIPE_NAME);
    if(pipeId < 0) return;
    
    bhbbPacket pkt;
    sce_paf_memset(&pkt, 0, sizeof(pkt));
    pkt.cmd = SHUTDOWN;

    sceKernelSendMsgPipe(pipeId, &pkt, sizeof(pkt), SCE_KERNEL_MSG_PIPE_MODE_WAIT, NULL, NULL);

    sceKernelCloseMsgPipe(pipeId);
}