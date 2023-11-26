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

#include <kernel.h>
#include <paf.h>

#include "zip.h"
#include "print.h"

#include "minizip/unzip.h"

#define dir_delimter '/'
#define MAX_FILENAME 512

Zipfile::Zipfile(const paf::string zip_path) 
{
	handle = unzOpen(zip_path.c_str());
	if (handle == nullptr)
    {
        print("[Error] unzOpen %s -> %p\n", zip_path.c_str(), handle);
        error = -0xC0FFEE;
        return;
    }

    error = unzGetGlobalInfo(handle, &globalInfo);
	if (error != UNZ_OK)
	{
        print("Cannot read zip info %d\n", error);
        return;
    }

    readBuff = (char *)sce_paf_malloc(ZIP_CUNK_SIZE);
}

Zipfile::~Zipfile() 
{
	if (handle != nullptr)
		unzClose(handle);
    if(readBuff != nullptr)
        sce_paf_free(readBuff);
}

int Zipfile::Decompress(const paf::string outPath, ProgressCallback progressCallback, void *progressUserData) 
{
    if(!handle && error != UNZ_OK)
        return error;

	if (uncompressedSize == 0)
		CalculateUncompressedSize();

	error = unzGoToFirstFile(handle);
    if(error != UNZ_OK)
    {
        print("Error going to first file!\n", error);
        return error;
    }

    error = UNZ_OK;
    uint64_t extractedBytes = 0;
    for(unsigned long i = 0; i < globalInfo.number_entry; i++, error = unzGoToNextFile(handle))
    {
        unz_file_info info;
        char fileName[0x200];
        paf::string fullPath;

        if(error != UNZ_OK)
        {
            print("error going to next file %d\n", error);
            return error;
        }

        error = unzGetCurrentFileInfo(handle, &info, fileName, sizeof(fileName), nullptr, 0, nullptr, 0);
        if(error != UNZ_OK)
        {
            print("Error getting file info %d\n", error);
            return error;
        }

        fullPath = outPath + paf::string(fileName);

        if(fullPath.c_str()[fullPath.size() - 1] == '/') // Check to see if entry is a directory
        {
            print("Creating dir: %s\n", fullPath.c_str());
            paf::Dir::CreateRecursive(fullPath.c_str());
        }
        else 
        {
			paf::string destdir = paf::common::StripFilename(fullPath, "P");
			paf::Dir::CreateRecursive(destdir.c_str());

            error = unzOpenCurrentFile(handle);
            if(error != UNZ_OK)
            {
                print("Error opening zip file %s -> %d (Save To: %s)\n", fileName, error, fullPath.c_str());
                unzCloseCurrentFile(handle);
                return error;
            }

            auto fHandle = paf::LocalFile::Open(fullPath.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &error);
            if(error != SCE_PAF_OK)
            {
                print("Error opening file %s -> 0x%X\n", fullPath.c_str(), error);
                unzCloseCurrentFile(handle);
                return error;
            }
            
            do
            {
                error = unzReadCurrentFile(handle, readBuff, ZIP_CUNK_SIZE);
                if(error < 0)
                {
                    print("[Error] failed to read zfile %s %d\n", fileName, error);
                    break;
                }

                fHandle.get()->Write(readBuff, error);

            } while(error > 0);
            
            print("Extracted: %s -> %s\n", fileName, fullPath.c_str());
            if(progressCallback)
                progressCallback(fileName, i, globalInfo.number_entry, progressUserData);

            unzCloseCurrentFile(handle);

        }
    }
    
    return UNZ_OK;
}

int Zipfile::CalculateUncompressedSize() 
{
    error = unzGoToFirstFile(handle);
    if(error != UNZ_OK)
    {
        print("Error going to first file!\n", error);
        unzClose(handle);
        return error;
    }

    uncompressedSize = 0;
    for(unsigned long i = 0; i < globalInfo.number_entry; i++, error = unzGoToNextFile(handle))
    {
        unz_file_info info;
        char fileName[0x200];

        if(error != UNZ_OK)
        {
            print("error going to next file %d\n", error);
            unzClose(handle);
            return error;
        }

        error = unzGetCurrentFileInfo(handle, &info, fileName, sizeof(fileName), nullptr, 0, nullptr, 0);
        if(error != UNZ_OK)
        {
            print("Error getting file info %d\n", error);
            unzClose(handle);
            return error;
        }

        uncompressedSize += info.uncompressed_size;
    }

    return UNZ_OK;
}
