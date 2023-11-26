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
#include <common_gui_dialog.h>

#include "tgz.h"
#include "dialog.h"
#include "print.h"

using namespace paf;

TgzFile::TgzFile(paf::string path)
{
    file = LocalFile::Open(path.c_str(), SCE_O_RDONLY, 0, &error);
    if(error != SCE_PAF_OK)
    {
        print("[TgzFile] %s -> 0x%X -> TGZ_FERR\n", path.c_str(), error);
        error = TGZ_FERR;
        return;
    }

    readBuff = (Bytef *)sce_paf_malloc(TGZ_CHUNK_SIZE);
    if(!readBuff)
    {
        print("[TgzFile::TgzFile] TGZ_MEMERR Error allocating buffer\n");
        error = TGZ_MEMERR;
        return;
    }

    zstrm.zalloc = sce_paf_gzip_zalloc;
    zstrm.zfree = sce_paf_gzip_zfree;
    zstrm.opaque = Z_NULL;
    zstrm.avail_in = 0;
    zstrm.next_in = Z_NULL;
    zstrm.avail_out = -1; // Queue file read in next Read() call

    error = inflateInit2_(&zstrm, 15 | 16, "1.2.5", sizeof(z_stream));
    if(error != Z_OK)
    {
        sce_paf_free(readBuff);
        print("[TgzFile::TgzFile] inflateInit2_ error -> 0x%X\n", error);
        return;
    }

    error = TGZ_OK;
}

TgzFile::~TgzFile()
{
    if(readBuff)
        sce_paf_free(readBuff);

    inflateEnd(&zstrm);
}

int64_t TgzFile::Read(void *buff, size_t rsize)
{
    if(zstrm.avail_out != 0 /*buffer has inflated*/)
    {
        if(file->IsEof())
            return TGZ_EOF;

        sce_paf_memset(readBuff, 0, TGZ_CHUNK_SIZE);
        auto rSize = file->Read(readBuff, TGZ_CHUNK_SIZE);
        if(rSize < 0)
        {
            print("[TgzFile::Read] file->Read() FAIL! 0x%X\n", rSize);
            return TGZ_FERR;
        }   
        
        zstrm.avail_in = rSize;
        zstrm.next_in = (Bytef *)readBuff;
    }

    zstrm.next_out = (Bytef *)buff;
    zstrm.avail_out = rsize;
    
    auto ret = inflate(&zstrm, Z_NO_FLUSH);

    switch (ret) 
    {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            return ret;
    }
    
    return rsize - zstrm.avail_out;
}

int TgzFile::Decompress(paf::string outPath, ProgressCallback progressCB, void *progressData)
{
    print("[TgzFile::Decompress] START\n");
    
    paf::Dir::CreateRecursive(outPath.c_str());

    TarHeader tHeader;
    unsigned long fileSize;
    unsigned long remainingBytes;
    size_t padding;
    int64_t ret;

    do
    {
        ret = Read(&tHeader, sizeof(tHeader));
        
        auto fullPath = outPath + tHeader.name;

        switch(tHeader.typeflag)
        {
        case '0':
        case '\0': // Normal file
        {
            auto extractedFile = LocalFile::Open(fullPath.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666, &error);
            if(error != SCE_OK)
            {
                print("[TgzFile::Decompress] Open(%s) FAIL! 0x%X\n", fullPath.c_str(), error);
                error = TGZ_FERR;
                return;
            }

            remainingBytes = fileSize = sce_paf_strtoul(tHeader.size, nullptr, 8);
            padding = ((int)sce_paf_ceilf((float)fileSize / 512.0f) * 512) - fileSize; // Round up to nearest 512th byte and calculate extra padding size

            print("[Tgs::Decompress] Extracting: %s (%zu bytes, %zu padding)\n", tHeader.name, fileSize, padding);

            char *rbuff = (char *)sce_paf_malloc(TGZ_CHUNK_SIZE);
            do 
            {
                ret = Read(rbuff, remainingBytes > TGZ_CHUNK_SIZE ? TGZ_CHUNK_SIZE : remainingBytes);
                if(ret < 0)
                {
                    sce_paf_free(rbuff);
                    return ret;
                }

                extractedFile->Write(rbuff, ret);

                remainingBytes -= ret;

            } while(fileSize > 0 && ret > 0);

            progressCB(tHeader.name, zstrm.total_in, file->GetFileSize(), progressData);
            ret = Read(rbuff, padding); // Read padding just to advance the file pointer in the compressed file
            sce_paf_free(rbuff);
            
            break;
        }
        case '5': //Directory
            Dir::CreateRecursive(fullPath.c_str());

            print("Directory: %s\n", tHeader.name);
            break;
        }
        
    } while(ret >= 0 && !file->IsEof());
    print("[TgzFile::Decompress] success STOP\n");
    return TGZ_OK;
}

int TgzFile::CalculateUncompressedSize()
{
    auto fileSize = file->GetFileSize();
    uncompressedSize = fileSize * 2;
    return 0;
}