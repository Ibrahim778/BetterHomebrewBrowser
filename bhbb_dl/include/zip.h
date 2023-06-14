#ifndef _ZIP_H_
#define _ZIP_H_

#include <paf.h>
#include <kernel.h>
#include "minizip/unzip.h"

class Zipfile 
{
public:
    typedef void (*ProgressCallback)(uint64_t current, uint64_t total, void *pUserData);

	Zipfile(const paf::string filePath);
	~Zipfile();

	int Unzip(const paf::string outPath, ProgressCallback progressCB, void *progressData);
	int CalculateUncompressedSize();
    int GetLastError()
    {
        return error;
    }
    size_t GetUncompressedSize()
    {
        return uncompressedSize;
    }

private:
	unzFile handle;
	size_t uncompressedSize;
	unz_global_info globalInfo;

    int error;
    char *readBuff;
};

#endif //_ZIP_H_