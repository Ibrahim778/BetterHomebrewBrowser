#pragma once

#include "minizip/unzip.h"
#include <psp2/types.h>
typedef void (*UnzipProgressCallback)(uint curr, uint total);

#ifdef __cplusplus

class Zipfile {
public:
	Zipfile(const char* zip_path);
	~Zipfile();

	int Unzip(const char* outpath, UnzipProgressCallback = SCE_NULL);
	int UncompressedSize();

private:
	unzFile zipfile_;
	uint64_t uncompressed_size_ = 0;
	unz_global_info global_info_;
};

extern "C" {
#endif // __cplusplus

int unZipTo(const char *path, const char *dest);
int unZipToWithProgressCB(const char *path, const char *dest, UnzipProgressCallback cb);

#ifdef __cplusplus
}
#endif