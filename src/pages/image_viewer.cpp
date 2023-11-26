/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2023 Muhammad Ibrahim

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

#include <paf.h>
#include <paf_file_ext.h>

#include "pages/image_viewer.h"
#include "bhbb_plugin.h"
#include "bhbb_locale.h"
#include "common.h"
#include "dialog.h"
#include "print.h"

using namespace paf;
using namespace common;
using namespace math;
using namespace thread;

ImageViewer::ImageViewer(const char *path):
    page::Base(
        page_image_viewer,
        Plugin::PageOpenParam(),
        Plugin::PageCloseParam(false, 200.0f, Plugin::TransitionType_SlideFromBottom)
    )
{
    if(path)
        LoadAsync(path);
}

void ImageViewer::LoadAsync(const char *path)
{
    auto item = new DisplayJob("ImageViewer::DisplayJob");
    item->workPage = this;
    item->path = path;
    auto itemParam = SharedPtr<job::JobItem>(item);
    job::JobQueue::default_queue->Enqueue(itemParam);
}

int ImageViewer::LoadNet(const char *path)
{
    int32_t res = -1;
	CurlFile::OpenArg oarg;
	oarg.ParseUrl(path);
	oarg.SetOption(3000, CurlFile::OpenArg::OptionType_ConnectTimeOut);
	oarg.SetOption(5000, CurlFile::OpenArg::OptionType_RecvTimeOut);
    
	CurlFile *file = new CurlFile();
	res = file->Open(&oarg);
	if (res != SCE_PAF_OK)
	{
		delete file;
        print("Failed to open curl file: 0x%X\n", res);
		return res;
	}

	common::SharedPtr<CurlFile> hfile(file);
    auto tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), (common::SharedPtr<File>&)hfile);
    if(tex.get())
    {
        auto plane = root->FindChild(picture);
        plane->SetTexture(tex, 0, 0);
        plane->SetColor(1,1,1,1);        
        return 0;
    }
    
    return -1;
}

int ImageViewer::LoadLocal(const char *path)
{
    int result = 0;
    auto fileParam = LocalFile::Open(path, SCE_O_RDONLY, 0, &result);
    if(result != 0)
        return result;
    
    auto tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), (common::SharedPtr<File>&)fileParam);
    if(tex.get())
    {
        auto plane = root->FindChild(picture);
        plane->SetTexture(tex, 0, 0);
        plane->SetColor(1,1,1,1);
        return 0;
    }
    
    return -1;
}

int ImageViewer::Load(const char *path)
{
    if(sce_paf_strncmp(path, "http", 4) == 0)
        return LoadNet(path);

    return LoadLocal(path);
}

void ImageViewer::DisplayJob::Run()
{
    dialog::OpenPleaseWait(g_appPlugin, nullptr, g_appPlugin->GetString(msg_wait));
    
    int ret = workPage->Load(path.c_str());

    dialog::Close();
    
    if(ret != 0)
    {
        dialog::OpenError(g_appPlugin, ret, g_appPlugin->GetString(msg_screenshot_error));
        page::Base::DeleteCurrentPage();
        return;
    }
}

void ImageViewer::DisplayJob::Finish()
{
    
}