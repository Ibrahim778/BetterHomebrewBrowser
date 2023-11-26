/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2023 Muhammad Ibrahim
    Copyright (C) 2023 GrapheneCt

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/

// This file is an extended version of @GrapheneCt's downloader.h used in EMPV-A

#ifndef _ELEVENMPV_DOWNLOADER_H_
#define _ELEVENMPV_DOWNLOADER_H_

#include <kernel.h>
#include <paf.h>
#include <ipmi.h>
#include <download_service.h>

#include "bhbb_dl.h"

using namespace paf;

class Downloader
{
public:

	enum
	{
		DownloaderEvent = (ui::Handler::CB_STATE + 0x60000),
	};

	Downloader();

	~Downloader();

	int32_t Enqueue(Plugin *workPlugin, const char *url, const char *name, const char *icon_path = nullptr, BGDLParam *param = nullptr);

	int32_t EnqueueAsync(Plugin *workPlugin, const char *url, const char *name);

    static Downloader *GetCurrentInstance();

private:

	class AsyncEnqueue : public job::JobItem
	{
	public:

		using job::JobItem::JobItem;

		~AsyncEnqueue() {}

		void Run()
		{
			Downloader *pdownloader = (Downloader *)downloader;
			pdownloader->Enqueue(plugin, url8.c_str(), name8.c_str());
		}

		void Finish() {}

		paf::string url8;
		paf::string name8;
		void *downloader;
		paf::Plugin *plugin;
	};

	sce::Download dw;

    static Downloader *s_currentInstance;
};

#endif
