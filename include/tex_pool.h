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

// This file is an extended version of @GrapheneCt's tex_pool.h used in NetStream

#ifndef _TEX_POOL_H_
#define _TEX_POOL_H_

#include <kernel.h>
#include <paf.h>
#include <paf_file_ext.h>

#include "event.h"

using namespace paf;

class TexPool
{
public:

	typedef void(*OnAddAsyncComplete)(uint32_t hash, bool result, void *pArgBlock);

	TexPool(Plugin *_cbPlugin = NULL);

	~TexPool();

	void DestroyAsync();

	intrusive_ptr<graph::Surface> Get(IDParam const& id);

	bool Add(Plugin *plugin, IDParam const& id, bool allowReplace = false);
	bool Add(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace = false);
	bool Add(IDParam const& id, const char *path, bool allowReplace = false);
	bool Add(IDParam const& id, intrusive_ptr<graph::Surface>& surf, bool allowReplace = false);

	bool AddAsync(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace = false);
	bool AddAsync(IDParam const& id, const char *path, bool allowReplace = false);

	void Remove(IDParam const& id);
	void RemoveAll();

	bool Exist(IDParam const& id);

	void AddAsyncWaitComplete();
	int32_t AddAsyncActiveNum();

	uint32_t GetSize();

	inline void SetShare(CurlFile::Share *sh)
	{
		share = sh;
	}

	inline bool IsAlive()
	{
		return alive;
	}

	inline void SetAlive(bool value)
	{
		alive = value;
	}

protected:

	class AddAsyncJob : public job::JobItem
	{
	public:

		using job::JobItem::JobItem;

		~AddAsyncJob()
		{

		}

		void Run()
		{
			bool result = false;

			if (buf)
			{
				result = workObj->Add(hash, buf, bufSize, true);
			}
			else
			{
				result = workObj->Add(hash, path.c_str(), true);
			}

			if (result && workObj->IsAlive() && workObj->cbPlugin)
			{
				event::BroadcastGlobalEvent(workObj->cbPlugin, ui::Handler::CB_STATE_READY_CACHEIMAGE, hash);
			}
		}

		void Finish()
		{

		}

		TexPool *workObj;
		uint8_t *buf;
		size_t bufSize;
		uint32_t hash;
		paf::string path;
	};

	class DestroyJob : public job::JobItem
	{
	public:

		using job::JobItem::JobItem;

		~DestroyJob()
		{

		}

		void Run()
		{
			delete pool;
		}

		void Finish()
		{

		}

		TexPool *pool;
	};

	void RemoveForReplace(IDParam const& id);

	bool AddHttp(IDParam const& id, const char *path);

	bool AddLocal(IDParam const& id, const char *path);

	map<uint32_t, intrusive_ptr<graph::Surface>> stor;
	CurlFile::Share *share;
	thread::RMutex *storMtx;
	job::JobQueue *addAsyncQueue;
	bool alive;
	Plugin *cbPlugin;
};

#endif
