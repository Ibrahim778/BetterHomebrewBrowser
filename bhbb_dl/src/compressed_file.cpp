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