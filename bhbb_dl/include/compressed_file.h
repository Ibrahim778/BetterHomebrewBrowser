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

#ifndef _COMPRESSED_FILE_H_
#define _COMPRESSED_FILE_H_

#include <paf.h>
#include <kernel.h>

class CompressedFile 
{
public:
    typedef void (*ProgressCallback)(const char *fname, uint64_t current, uint64_t total, void *pUserData);

	CompressedFile();
	virtual ~CompressedFile();

	virtual int Decompress(const paf::string outPath, ProgressCallback progressCB, void *progressData) = 0;
	virtual int CalculateUncompressedSize() = 0;
    
    int GetLastError()
    {
        return error;
    }
    size_t GetUncompressedSize()
    {
        return uncompressedSize;
    }

    static paf::common::SharedPtr<CompressedFile> Create(const char *path);

protected:
	size_t uncompressedSize;
    int error;
};

#endif //_COMPRESSED_FILE_H_