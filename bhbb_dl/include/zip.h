#ifndef _ZIP_H_
#define _ZIP_H_

#include <kernel.h>
#include <paf.h>

#include <minizip/unzip.h>
using namespace std;


class Zipfile {
public:
	Zipfile(const paf::string zip_path);
	~Zipfile();

	int Unzip(const paf::string outpath, void (*progressCB)(SceUInt curr, SceUInt total));
	int UncompressedSize();

private:
	unzFile zipfile_;
	uint64_t uncompressed_size_ = 0;
	unz_global_info global_info_;
};

#endif //_ZIP_H_