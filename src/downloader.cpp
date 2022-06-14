#include <kernel.h>
#include <libsysmodule.h>
#include <paf.h>
#include <ipmi.h>
#include <download_service.h>

#include "downloader.h"
#include "utils.h"
#include "common.h"
#include "print.h"

using namespace paf;

Downloader::Downloader()
{
    IPMI::Client::Config conf;
    IPMI::Client *client;
    SceUInt32 clientMemSize;
    SceInt32 ret;

    sce_paf_memset(&dw, 0, sizeof(sce::Download));
    sce_paf_memset(&conf, 0, sizeof(IPMI::Client::Config));

	sce_paf_strncpy((char *)conf.name, "SceDownload", sizeof(conf.name));
	conf.msgPipeBudgetType = 0;
	conf.numREObjects = 1;
	conf.requestQueueSize = 0x1E00;
	conf.receptionQueueSize = 0x1E00;
	conf.numARObjects = 1;
	conf.requestPipeAddsize1 = 0xF00;
	conf.requestPipeAddsize2 = 0xF00;
	conf.numEventFlags = 1;
	conf.numMessages = 0;
	conf.pipeSmth = SCE_UID_INVALID_UID;

    clientMemSize = conf.estimateClientMemorySize();
	dw.clientMem = sce_paf_malloc(clientMemSize);
	dw.bufMem = sce_paf_malloc(SCE_KERNEL_4KiB);

	IPMI::Client::create(&client, &conf, &dw, dw.clientMem);

    dw.client = client;

    sce::Download::ConnectionOpt connOpt;
    sce_paf_memset(&connOpt, 0, sizeof(sce::Download::ConnectionOpt));
    connOpt.budgetType = conf.msgPipeBudgetType;

    client->connect(&connOpt, sizeof(sce::Download::ConnectionOpt), &ret);

    dw.unk_00 = SCE_UID_INVALID_UID;
	dw.unk_04 = SCE_UID_INVALID_UID;
	dw.unk_08 = SCE_UID_INVALID_UID;
}

Downloader::~Downloader()
{
    dw.client->disconnect();
    dw.client->destroy();
    sce_paf_free(dw.clientMem);
    sce_paf_free(dw.bufMem);    
}

SceInt32 Downloader::Enqueue(const char *url, const char *name, BGDLParam *param)
{
	IPMI::DataInfo dtInfo;
	IPMI::BufferInfo bfInfo;
	SceInt32 ret = SCE_OK;
	SceInt32 ret2 = SCE_OK;
	SceInt32 dwRes = 0;

	sce::Download::HttpParam hparam;
	sce::Download::AuthParam aparam;
	sce::Download::DownloadParam dparam;
	sce::Download::MetadataInfo minfo;
	sce_paf_memset(&hparam, 0, sizeof(sce::Download::HttpParam));
	sce_paf_memset(&aparam, 0, sizeof(sce::Download::AuthParam));
	sce_paf_memset(&dparam, 0, sizeof(sce::Download::DownloadParam));
	sce_paf_memset(&minfo, 0, sizeof(sce::Download::MetadataInfo));

	hparam.paramType1 = paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_RESOLVE_TIME_OUT;
	hparam.paramVal1 = 4000000;
	hparam.paramType2 = paf::HttpFile::Param::SCE_PAF_HTTP_FILE_PARAM_CONNECT_TIME_OUT;
	hparam.paramVal2 = 30000000;
	hparam.paramType2 = 0;
	hparam.paramVal2 = 30000000;
	sce_paf_strcpy((char *)hparam.url, url);

	dtInfo.data1 = &hparam;
	dtInfo.data1Size = sizeof(sce::Download::HttpParam);
	dtInfo.data2 = &aparam;
	dtInfo.data2Size = sizeof(sce::Download::AuthParam);

	bfInfo.data1 = &minfo;
	bfInfo.data1Size = sizeof(sce::Download::MetadataInfo);

	ret = dw.client->invokeSyncMethod(0x1234000D, &dtInfo, 2, &ret2, &bfInfo, 1);
	if (ret != SCE_OK)
		return ret;
	else if (ret2 != SCE_OK)
		return ret2;

	sce_paf_memset(&minfo.name, 0, sizeof(minfo.name));
	sce_paf_strcpy((char *)minfo.name, name);

    print("name: %s\nsize: 0x%X\nmimeType: 0x%X\ncreationDateString: 0x%X\nunk_00: 0x%X\nunk_141: 0x%X\nunk_3C: 0x%X\n", minfo.name, minfo.size, minfo.mimeType, minfo.creationDateString, minfo.unk_00, minfo.unk_141, minfo.unk_3C);

	dparam.unk_00 = 1;
	sce_paf_strcpy((char *)dparam.url, url);

	dtInfo.data1 = &dparam;
	dtInfo.data1Size = sizeof(sce::Download::DownloadParam);
	dtInfo.data2 = &aparam;
	dtInfo.data2Size = sizeof(sce::Download::AuthParam);
	dtInfo.data3 = &minfo;
	dtInfo.data3Size = sizeof(sce::Download::MetadataInfo);

	bfInfo.data1 = &dwRes;
	bfInfo.data1Size = sizeof(SceInt32);

	ret2 = SCE_OK;
	ret = dw.client->invokeSyncMethod(0x12340011, &dtInfo, 3, &ret2, &bfInfo, 1);
	if (ret2 != SCE_OK) 
    {
		//invalid filename?
		sce_paf_memset(&minfo.name, 0, sizeof(minfo.name));
		char *ext = sce_paf_strrchr(name, '.');
		sce_paf_snprintf((char *)minfo.name, sizeof(minfo.name), "DefaultFilename%s", ext);

		ret2 = SCE_OK;
		ret = dw.client->invokeSyncMethod(0x12340011, &dtInfo, 3, &ret2, &bfInfo, 1);
		if (ret2 != SCE_OK)
			return ret2;
	}

    if(param != NULL && param->magic != -1)
    {
        shared_ptr<LocalFile> openResult;
        string paramPath;
        
        paramPath.Setf("ux0:bgdl/t/%08x/install_param.ini", dwRes);

        SceInt32 result = SCE_OK;
        LocalFile::Open(&openResult, paramPath.data, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &result);
        if(result < 0)
        {
            print("open %s -> 0x%X\n", paramPath.data, result);
            return result;
        }

        openResult.get()->Write(param, sizeof(BGDLParam));
    }
	return ret;
}

SceInt32 Downloader::EnqueueAsync(const char *url, const char *name, BGDLParam *param)
{
	AsyncEnqueue *dwJob = new AsyncEnqueue("BHBB::AsyncEnqueue");
	dwJob->downloader = this;
	dwJob->url8 = url;
	dwJob->name8 = name;
    if(param)
        dwJob->param = *param;
    else dwJob->param.magic = -1;

	return paf::thread::s_defaultJobQueue->Enqueue(&paf::shared_ptr<paf::thread::JobQueue::Item>(dwJob));
}