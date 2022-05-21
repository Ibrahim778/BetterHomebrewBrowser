#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <paf.h>
#include <ipmi.h>
#include <download_service.h>

class Downloader
{
public:
    Downloader();
    ~Downloader();

    SceInt32 Enqueue(const char *url, const char *name);
    SceInt32 EnqueueAsync(const char *url, const char *name);

	class AsyncEnqueue : public paf::thread::JobQueue::Item
	{
	public:

		using paf::thread::JobQueue::Item::Item;

		~AsyncEnqueue() {}

		SceVoid Run()
		{
			Downloader *pdownloader = (Downloader *)downloader;
			pdownloader->Enqueue(url8.data, name8.data);
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
	};

    ScePVoid downloader;

private:
    sce::Download dw;
};

#endif