#ifndef LIBEXTRACTOR_H
#define LIBEXTRACTOR_H

#include <psp2/types.h>

typedef void *unzFile;
typedef struct unz_global_info_s
{
    unsigned long number_entry;        /* total number of entries in the central dir on this disk */
    unsigned long number_disk_with_CD; /* number the the disk with central dir, used for spanning ZIP*/
    unsigned long size_comment;        /* size of the global comment of the zipfile */
} unz_global_info;

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

#endif