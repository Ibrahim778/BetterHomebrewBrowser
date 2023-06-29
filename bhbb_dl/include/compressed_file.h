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