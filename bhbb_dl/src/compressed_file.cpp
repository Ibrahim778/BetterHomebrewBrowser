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

paf::common::SharedPtr<CompressedFile> CompressedFile::Create(const char *path)
{
    auto ext = string(sce_paf_strchr(path, '.'));
    
    if(ext == ".tar.gz")
        return common::SharedPtr<CompressedFile>(new TgzFile(path));
    else if(ext == ".zip" || ext == ".vpk")
        return common::SharedPtr<CompressedFile>(new Zipfile(path));
}