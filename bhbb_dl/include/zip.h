#ifndef _ZIP_H_
#define _ZIP_H_

#include <paf.h>
#include <kernel.h>
#include <minizip/unzip.h>

class Zipfile 
{
public:
	Zipfile(const paf::string zip_path);
	~Zipfile();

	int Unzip(const paf::string outpath, void (*progressCB)(::uint32_t curr, ::uint32_t total, void *), void *progdat);
	int UncompressedSize();

private:
	unzFile zipfile_;
	uint64_t uncompressed_size_ = 0;
	unz_global_info global_info_;
};

#endif //_ZIP_H_