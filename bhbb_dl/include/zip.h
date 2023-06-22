#ifndef _ZIP_H_
#define _ZIP_H_

#include <paf.h>
#include <kernel.h>

#include "compressed_file.h"
#include "minizip/unzip.h"

class Zipfile : public CompressedFile
{
public:
    enum 
    {
        ZIP_CUNK_SIZE = SCE_KERNEL_128KiB
    };

	Zipfile(const paf::string filePath);
	~Zipfile();

	virtual int Decompress(const paf::string outPath, ProgressCallback progressCB, void *progressData) override;
	virtual int CalculateUncompressedSize() override;
    
private:
	unzFile handle;
	unz_global_info globalInfo;
    char *readBuff;
};

#endif //_ZIP_H_