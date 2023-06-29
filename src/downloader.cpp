// Based off of downloader by @GrapheneCt, RE by: @GrapheneCt @CreeptNT and @dots_tb

#include <kernel.h>
#include <libsysmodule.h>
#include <paf.h>
#include <ipmi.h>
#include <download_service.h>

#include "downloader.h"
#include "event.h"
#include "print.h"

using namespace paf;

Downloader *Downloader::s_currentInstance = nullptr;

Downloader::Downloader()
{
	IPMI::Client::Config conf;
	IPMI::Client *client;
	uint32_t clMemSize;
	int32_t ret;

	sce_paf_memset(&dw, 0, sizeof(sce::Download));
	sce_paf_memset(&conf, 0, sizeof(IPMI::Client::Config));

	sce_paf_strncpy((char *)conf.serviceName, "SceDownload", sizeof(conf.serviceName));
	conf.pipeBudgetType = IPMI::BudgetType_Default;
	conf.numResponses = 1;
	conf.requestQueueSize = 0x1E00;
	conf.receptionQueueSize = 0x1E00;
	conf.numAsyncResponses = 1;
	conf.requestQueueAddsize1 = 0xF00;
	conf.requestQueueAddsize2 = 0xF00;
	conf.numEventFlags = 1;
	conf.msgQueueSize = 0;
	conf.serverPID = SCE_UID_INVALID_UID;

	clMemSize = conf.estimateClientMemorySize();
	dw.clientMem = sce_paf_malloc(clMemSize);
	dw.bufMem = sce_paf_malloc(SCE_KERNEL_4KiB);

	IPMI::Client::create(&client, &conf, &dw, dw.clientMem);

	dw.client = client;

	sce::Download::ConnectionOpt connOpt;
	sce_paf_memset(&connOpt, 0, sizeof(sce::Download::ConnectionOpt));
	connOpt.budgetType = conf.pipeBudgetType;

	client->connect(&connOpt, sizeof(sce::Download::ConnectionOpt), &ret);

	dw.unk_00 = SCE_UID_INVALID_UID;
	dw.unk_04 = SCE_UID_INVALID_UID;
	dw.unk_08 = SCE_UID_INVALID_UID;

    s_currentInstance = this;
}

Downloader::~Downloader()
{
	dw.client->disconnect();
	dw.client->destroy();
	sce_paf_free(dw.clientMem);
	sce_paf_free(dw.bufMem);
}

void WriteParam(BGDLParam *param, int32_t dwRes)
{
    int ret = SCE_PAF_OK;
    auto param_file = LocalFile::Open(common::FormatString("ux0:/bgdl/t/%08x/bhbb.param", dwRes).c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &ret);
    if(ret < 0)
        return;

    param_file.get()->Write(param, sizeof(BGDLParam));
}

int32_t Downloader::Enqueue(Plugin *workPlugin, const char *url, const char *name, const char *icon_path, BGDLParam *param)
{
	IPMI::DataInfo dtInfo[1];
	IPMI::BufferInfo bfInfo[2];

    char output0[0x2E0];
    sce_paf_memset(output0, 0, sizeof(output0));

	int32_t ret = SCE_OK;
	int32_t ret2 = SCE_OK;
	int32_t dwRes = SCE_OK;

    sce::Download::RegistTaskParam rparam;
    sce_paf_memset(&rparam, 0, sizeof(rparam));

    sce_paf_strcpy((char *)&rparam.downloadName, name);
    sce_paf_strcpy((char *)&rparam.url, url);
    if(icon_path != nullptr)
        sce_paf_strcpy((char *)&rparam.icon, icon_path);

    rparam.contentType = sce::Download::ContentType_Multimedia;

    dtInfo[0].data = &rparam;
    dtInfo[0].dataSize = sizeof(rparam);

    bfInfo[0].data = output0;
    bfInfo[0].dataSize = sizeof(output0);

    bfInfo[1].data = &dwRes;
    bfInfo[1].dataSize = sizeof(dwRes);

    ret = dw.client->invokeSyncMethod(sce::Download::Method_DownloadExt, dtInfo, 1, &ret2, bfInfo, 2);
    print("ret = 0x%X ret2 = 0x%X dwRes = 0x%X\n", ret, ret2, dwRes);
    if(ret2 != 0)
    {
        ret = ret2;
        goto end;
    }

    if(param)
        WriteParam(param, dwRes);
end:
	event::BroadcastGlobalEvent(workPlugin, DownloaderEvent, ret);
	return ret;
}

int32_t Downloader::EnqueueAsync(Plugin *workPlugin, const char *url, const char *name)
{
	AsyncEnqueue *dwJob = new AsyncEnqueue("Downloader::AsyncEnqueue");
	dwJob->downloader = this;
	dwJob->url8 = url;
	dwJob->name8 = name;
	dwJob->plugin = workPlugin;

	common::SharedPtr<job::JobItem> itemParam(dwJob);

	return job::JobQueue::default_queue->Enqueue(itemParam);
}

Downloader *Downloader::GetCurrentInstance()
{
    return s_currentInstance;
}