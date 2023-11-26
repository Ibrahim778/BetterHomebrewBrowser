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

#include "compressed_file.h"
#include "print.h"
#include "zip.h"
#include "tgz.h"

using namespace paf;

CompressedFile::CompressedFile()
{
    uncompressedSize = 0;
    error = 0;
}

CompressedFile::~CompressedFile()
{

}

// Check the extension at the end of the string and return the apporpriate handle
paf::common::SharedPtr<CompressedFile> CompressedFile::Create(const char *path)
{
    auto len = sce_paf_strlen(path);

    if(sce_paf_strcasecmp(&path[len - 7], ".tar.gz") == 0)
        return common::SharedPtr<CompressedFile>(new TgzFile(path));
    else if(sce_paf_strcasecmp(&path[len - 4], ".vpk") == 0 || sce_paf_strcasecmp(&path[len - 4], ".zip") == 0)
        return common::SharedPtr<CompressedFile>(new Zipfile(path));
    
    return common::SharedPtr<CompressedFile>(nullptr);
}