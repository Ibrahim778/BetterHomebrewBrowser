#include "zip.h"

#include <psp2/io/stat.h>
#include <psp2/paf.h>
#include <psp2/types.h>
#include <string.h>

#include "notifmgr.h"
#include "print.h"

extern "C" int sceClibPrintf(const char*,...);

#define dir_delimter '/'
#define MAX_FILENAME 512
#define READ_SIZE 8192

#define snprintf(...) sce_paf_snprintf(__VA_ARGS__)

static void mkdir_rec(const char* dir) { //TODO: ScePaf has a function for this too
	
	char tmp[256];
	char* p = nullptr;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);

	if (tmp[len - 1] == '/')
		tmp[len - 1] = 0;

	for (p = tmp + 1; *p; p++)
		if (*p == '/') {
			*p = 0;
			sceIoMkdir(tmp, 0777);
			*p = '/';
		}
		
	sceIoMkdir(tmp, 0777);
}


Zipfile::Zipfile(const char *zip_path) {
	
	zipfile_ = unzOpen(zip_path);
	if (!zipfile_)
	{
		sceClibPrintf("Cannot open zip\n");
	}

	if (unzGetGlobalInfo(zipfile_, &global_info_) != UNZ_OK)
		sceClibPrintf("Cannot read zip info");
}

SceBool hasTrailingSlash(const char *str)
{
#ifndef _DEBUG
	int i = strlen(str) - 1;
	if(str[i] == '/' || str[i] == '\\')	return true;
	else return false;
#else
	int i = strlen(str) - 1;
	if(str[i] == '/' || str[i] == '\\')
    {
        print("hasTrailingSlash %s -> true\n", str);
        return true;
    }	
	else 
    {
        print("hasTralingSlash %s -> false\n", str);
        return false;
    }
#endif
}

Zipfile::~Zipfile() {
	if (zipfile_)
		unzClose(zipfile_);
}

int Zipfile::Unzip(const char *outpath, UnzipProgressCallback cb) {
	
	if (uncompressed_size_ == 0)
	{
        if(UncompressedSize() < 0)
            return -1;
    }
	char read_buffer[READ_SIZE];

	uLong i;
	if (unzGoToFirstFile(zipfile_) != UNZ_OK)
	{
		sceClibPrintf("Error going to first file");
		return -11;
	}
	
	for (i = 0; i < global_info_.number_entry; ++i) {
		
		unz_file_info file_info;
		char filename[MAX_FILENAME];
		char fullfilepath[MAX_FILENAME];
		char destDir[MAX_FILENAME];
		if (unzGetCurrentFileInfo(zipfile_, &file_info, filename, MAX_FILENAME, nullptr, 0, nullptr, 0) != UNZ_OK)
		{
			sceClibPrintf("Error reading zip file info");
			return -9;
		}
		snprintf(fullfilepath, sizeof(fullfilepath), hasTrailingSlash(fullfilepath) ? "%s%s" : "%s/%s", outpath, filename);

		// Check if this entry is a directory or file.
		const size_t filename_length = strlen(fullfilepath);
		if (fullfilepath[filename_length - 1] == dir_delimter) {
			mkdir_rec(fullfilepath);
		} else {
			// Create the dir where the file will be placed

			int lastSlash = filename_length - 1;
			for(; lastSlash > 0 && fullfilepath[lastSlash] != '/' && fullfilepath[lastSlash] != '\\'; lastSlash--);
			sce_paf_strncpy(destDir, fullfilepath, lastSlash + 1);
			destDir[lastSlash] = 0;

			mkdir_rec(destDir);

			// Entry is a file, so extract it.
			if (unzOpenCurrentFile(zipfile_) != UNZ_OK)
			{
				sceClibPrintf("Cannot open file from zip");
				return -8;
			}
			// Open a file to write out the data.
			FILE* out = fopen(fullfilepath, "wb");
			if (out == nullptr) {
				unzCloseCurrentFile(zipfile_);
				sceClibPrintf("Cannot open destination file %s\n", fullfilepath);
				return -7;
			}

			int error;
			do {
				error = unzReadCurrentFile(zipfile_, read_buffer, READ_SIZE);
				if (error < 0) {
					unzCloseCurrentFile(zipfile_);
					sceClibPrintf("Cannot read current zip file");
					return -6;
				}

				if (error > 0)
					fwrite(read_buffer, error, 1, out); // TODO check fwrite return
				
			} while (error > 0);

			fclose(out);
		}

		unzCloseCurrentFile(zipfile_);

		// Go the the next entry listed in the zip file.
		if ((i + 1) < global_info_.number_entry)
			if (unzGoToNextFile(zipfile_) != UNZ_OK)
				{
					sceClibPrintf("Error getting next zip file");
					return -5;
				}
        
        if(cb)
        {
            cb(i, global_info_.number_entry);
            if(NotifMgr_currDlCanceled) break;
        }
    }

	return 0;
}

int Zipfile::UncompressedSize() {

	uncompressed_size_ = 0;

	if (unzGoToFirstFile(zipfile_) != UNZ_OK)
	{
		sceClibPrintf("Error going into first file!\n");
		return -1;
	}

	uLong i;
	for (i = 0; i < global_info_.number_entry && !NotifMgr_currDlCanceled; ++i) {
		
		unz_file_info file_info;
		char filename[MAX_FILENAME];
		if (unzGetCurrentFileInfo(zipfile_, &file_info, filename, MAX_FILENAME, nullptr, 0, nullptr, 0) != UNZ_OK)
		{
			sceClibPrintf("Error reading Zip File Info!\n");
			return -2;
		}

		uncompressed_size_ += file_info.uncompressed_size;

		// Go the the next entry listed in the zip file.
		if ((i + 1) < global_info_.number_entry)
			if (unzGoToNextFile(zipfile_) != UNZ_OK)
				{
					sceClibPrintf("Error calculating zip size!\n");
					return -3;
				}
	}

	return uncompressed_size_;
}

int unZipTo(const char *path, const char *dest)
{   
    print("File: %s -> %s\n", path, dest);     
    if(!path || !dest) return -2;
    
    Zipfile zFile = Zipfile(path);
    SceInt32 ret = zFile.Unzip(dest);
    
    return ret;
}

int unZipToWithProgressCB(const char *path, const char *dest, UnzipProgressCallback cb)
{   
    print("File: %s -> %s\n", path, dest);     
    if(!path || !dest) return -2;
    
    Zipfile zFile = Zipfile(path);
    SceInt32 ret = zFile.Unzip(dest, cb);
    
    return ret;
}