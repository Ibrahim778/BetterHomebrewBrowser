#ifndef _ELEVENMPV_CURL_FILE_H_
#define _ELEVENMPV_CURL_FILE_H_

#include <kernel.h>
#include <paf.h>
#include <curl/curl.h>

//#define CURL_FILE_UA "Mozilla/5.0 (PlayStation Vita 3.74) AppleWebKit/537.73 (KHTML, like Gecko) Silk/3.2"
#define CURL_FILE_UA "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36"

using namespace paf;

class CurlFile : public paf::File
{
public:

	class CurlFileStat : public HttpFileStat
	{
	};

	class OpenArg : public paf::File::OpenArg
	{
	public:

		enum Opt
		{
			Opt_ResolveTimeOut = 1,
			Opt_ConnectTimeOut = 2,
			Opt_SendTimeOut = 4,
			Opt_RecvTimeOut = 8
		};

		OpenArg();

		~OpenArg();

		SceVoid SetUrl(const char *url);

		//SceVoid SetUrl(paf::Url *url);

		SceInt32 SetOpt(SceInt32 optValue, Opt optType);

		SceVoid SetUseShare(bool use);
        SceVoid SetSecondaryHeaderMethod(bool use);

		SceUInt32 fileType;
		string userAgent;
		SceInt32 sslFlags;

		string url;
		SceInt32 resolveTimeout;
		SceInt32 connectTimeout;
		SceInt32 sendTimeout;
		SceInt32 recvTimeout;
		bool useShare;
        bool getHeadersWithGET;
	};

	CurlFile();

	virtual SceInt32 GetFileType() const;
	virtual SceInt32 GetCapability() const;
	virtual SceOff   GetFileSize() const;
	virtual SceInt32 Open(const CurlFile::OpenArg *param);
	virtual SceInt32 OpenAsync(const CurlFile::OpenArg *param);
	virtual SceInt32 Close();
	virtual SceInt32 CloseAsync();
	virtual bool     IsOpened() const;
	virtual bool     IsEof();
	virtual SceInt32 Abort();
	virtual SceInt32 Read(void *buf, SceSize nbyte);
	virtual SceInt32 ReadAsync(void *buf, SceSize nbyte);
	virtual SceInt32 Write(const void *buf, SceSize nbyte); // not supported
	virtual SceInt32 WriteAsync(const void *buf, SceSize nbyte); // not supported
	virtual SceOff   Seek(SceOff offset, SceInt32 whence);
	virtual SceInt32 SeekAsync(SceOff offset, SceInt32 whence);
	virtual SceInt32 Flush();
	virtual SceInt32 FlushAsync();
	virtual SceInt32 WaitAsync(SceInt64 *pResult);
	virtual SceInt32 PollAsync(SceInt64 *pResult);
	virtual SceInt32 GetStat(FileStat *stat);
	virtual SceInt32 SetPriority(SceInt32 ioPriority);

	virtual ~CurlFile();

	SceInt32 GetResponseCode(SceInt32 *code);

	static SharedPtr<CurlFile> Open(const char *path, SceUInt32 flag, SceUInt32 mode, SceInt32 *error, bool useShare = false, bool secondaryHeaders = false);

	static SharedPtr<CurlFile> Open(const SceWChar16 *url, SceInt32 *error, SceUInt32 flag, bool useShare = false);

	static SharedPtr<CurlFile> Open(const char *url, SceInt32 *error, SceUInt32 flag, bool useShare = false);

	static SceInt32 GetStat(const char *url, CurlFileStat *stat, bool useShare = false);

	static SceVoid InitShare();

	static SceVoid TermShare();

private:

	static SceSize DownloadCore(char *buffer, SceSize size, SceSize nitems, ScePVoid userdata);
	static SceVoid ShareLock(CURL *handle, curl_lock_data data, curl_lock_access access, ScePVoid userptr);
	static SceVoid ShareUnlock(CURL *handle, curl_lock_data data, ScePVoid userptr);

	thread::RMutex *lock;
	CURL *curl;
	curl_off_t contentLength;
	ScePVoid buf;
	SceUInt32 posInBuf;
	SceOff pos;
	bool isOpened;
    bool isGettingHeaders;
};

#endif
