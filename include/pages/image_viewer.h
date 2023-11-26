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