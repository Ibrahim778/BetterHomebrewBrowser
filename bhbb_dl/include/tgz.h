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

#ifndef _TGZ_H_
#define _TGZ_H_

#include <paf.h>
#include <zlib.h>

#include "compressed_file.h"

class TgzFile : public CompressedFile
{
public:
    struct TarHeader // Taken from GNU tar
    {				          /* byte offset */
        char name[100];		    /*   0 */
        char mode[8];			/* 100 */
        char uid[8];			/* 108 */
        char gid[8];			/* 116 */
        char size[12];		    /* 124 */
        char mtime[12];		    /* 136 */
        char chksum[8];		    /* 148 */
        char typeflag;		    /* 156 */
        char linkname[100];		/* 157 */
        char magic[6];		    /* 257 */
        char version[2];		/* 263 */
        char uname[32];		    /* 265 */
        char gname[32];		    /* 297 */
        char devmajor[8];		/* 329 */
        char devminor[8];		/* 337 */
        char prefix[155];		/* 345 */
                                /* 500 */
        char padd[12];          // I had to add this to fit the 512 size per spec (why wasn't this here before?...)
    };

    enum
    {
        TGZ_OK          = 0,
        TGZ_EVAL        = -101,
        TGZ_FERR        = -102,
        TGZ_MEMERR      = -103,
        TGZ_EOF         = -104,
        TGZ_CHUNK_SIZE  = SCE_KERNEL_128KiB,
    };


    TgzFile(paf::string path);
    ~TgzFile();

    virtual int Decompress(const paf::string outPath, ProgressCallback progressCB, void *progressData) override;
    virtual int CalculateUncompressedSize();

private:
    static voidpf sce_paf_gzip_zalloc(voidpf opaque, uInt items, uInt size)
    {
        return sce_paf_calloc(items, size);
    }

    static void sce_paf_gzip_zfree(voidpf opaque, voidpf address)
    {
        sce_paf_free(address);
    }

    int64_t Read(void *buff, size_t readSize);

    paf::common::SharedPtr<paf::LocalFile> file;
    unsigned char *readBuff;
    z_stream zstrm;
};

#endif