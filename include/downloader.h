#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <paf.h>
#include <ipmi.h>
#include <download_service.h>

#include "bhbb_dl.h"

class Downloader
{
public:
    Downloader();
    ~Downloader();

    SceInt32 Enqueue(const char *url, const char *name, BGDLParam* param = SCE_NULL);
    SceInt32 EnqueueAsync(const char *url, const char *name, BGDLParam* param = SCE_NULL);

	class AsyncEnqueue : public paf::thread::JobQueue::Item
	{
	public:

		using paf::thread::JobQueue::Item::Item;

		~AsyncEnqueue() {}

		SceVoid Run()
		{
			Downloader *pdownloader = (Downloader *)downloader;
			pdownloader->Enqueue(url8.data, name8.data, &param);
		}

		SceVoid Finish() {}

		static SceVoid JobKiller(paf::thread::JobQueue::Item *job)
		{
			if (job)
				delete job;
		}

		paf::string url8;
		paf::string name8;
		ScePVoid downloader;
        BGDLParam param;
	};

    ScePVoid downloader;

private:
    sce::Download dw;
};

#endif