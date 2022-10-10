#pragma once

#include "minizip/unzip.h"

class Zipfile {
public:
	Zipfile(const char* zip_path);
	~Zipfile();

	int Unzip(const char* outpath);
	int UncompressedSize();

private:
	unzFile zipfile_;
	uint64_t uncompressed_size_ = 0;
	unz_global_info global_info_;
};
