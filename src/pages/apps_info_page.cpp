#include <kernel.h>
#include <paf.h>

#include "pages/apps_info_page.h"
#include "utils.h"
#include "print.h"
#include "common.h"

using namespace paf;

apps::info::Page::Page(db::entryInfo& entry):generic::Page::Page("info_page_template"),info(entry),iconSurf(SCE_NULL)
{
    Utils::SetWidgetLabel(Utils::GetChildByHash(root, Utils::GetHashById("info_title_text")), &info.title);
    Utils::SetWidgetLabel(Utils::GetChildByHash(root, Utils::GetHashById("info_author_text")), &info.author);
    Utils::SetWidgetLabel(Utils::GetChildByHash(root, Utils::GetHashById("info_version_text")), &info.version);

    iconLoadThread = new IconLoadThread(this);
    screenshotLoadThread = new ScreenshotLoadThread(this);

    iconLoadThread->Start();
    screenshotLoadThread->Start();
}

apps::info::Page::~Page()
{
    if(iconLoadThread)
    {
        if(iconLoadThread->IsAlive())
        {
            iconLoadThread->Cancel();
            iconLoadThread->Join();
        }
        delete iconLoadThread;
        iconLoadThread = SCE_NULL;
    }
    
    if(screenshotLoadThread)
    {
        if(screenshotLoadThread->IsAlive())
        {
            screenshotLoadThread->Cancel();
            screenshotLoadThread->Join();
        }
        delete screenshotLoadThread;
        screenshotLoadThread = SCE_NULL;
    }

    if(iconSurf)
    {
        Utils::GetChildByHash<ui::Plane>(root, Utils::GetHashById("icon_plane"))->SetSurface(&TransparentTex, 0);
        
        Utils::DeleteTexture(&iconSurf);
    }
}

SceVoid apps::info::Page::IconLoadThread::EntryFunction()
{
    auto busyIndicator = Utils::GetChildByHash<ui::BusyIndicator>(callingPage->root, Utils::GetHashById("icon_busy"));
    auto iconPlane = Utils::GetChildByHash<ui::Plane>(callingPage->root, Utils::GetHashById("icon_plane"));
    busyIndicator->Start();

    SceInt32 fileErr = SCE_OK;
    graph::Surface::Create(&callingPage->iconSurf, mainPlugin->memoryPool, (SharedPtr<File> *)&LocalFile::Open(callingPage->info.iconPath.data(), SCE_O_RDONLY, 0, &fileErr));
    
    if(fileErr != SCE_OK || callingPage->iconSurf == SCE_NULL)
        iconPlane->SetSurface(&BrokenTex, 0);
    else
        iconPlane->SetSurface(&callingPage->iconSurf, 0);
    
    iconPlane->SetColor(&paf::Rgba(1,1,1,1));

    busyIndicator->Stop();
    sceKernelExitDeleteThread(0);
}

SceVoid apps::info::Page::ScreenshotLoadThread::EntryFunction()
{
    sceKernelExitDeleteThread(0);
}