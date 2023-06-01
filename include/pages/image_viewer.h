#ifndef _IMAGE_VIEWER_H_
#define _IMAGE_VIEWER_H_

#include <paf.h>
#include <paf_file_ext.h>

#include "page.h"

class ImageViewer : page::Base
{
public:
    ImageViewer(const char *path = nullptr);

    int Load(const char *path);
    void LoadAsync(const char *path);

protected:
    class DisplayJob : public paf::job::JobItem
    {
    public:
        using paf::job::JobItem::JobItem;

        void Run();
        void Finish();
    
        ImageViewer *workPage;
        paf::string path;
    };

    int LoadLocal(const char *path);
    int LoadNet(const char *path);
};

#endif