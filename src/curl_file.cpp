#include <kernel.h>
#include <paf.h>
#include <curl/curl.h>

#include "curl_file.h"
#include "print.h"

using namespace paf;

static CURLSH *s_share = SCE_NULL;
static thread::RMutex *s_shareLock = SCE_NULL;

CurlFile::OpenArg::OpenArg()
{
	fileType = 3;
	userAgent = CURL_FILE_UA;
	sslFlags = 0;

	url = "";
	resolveTimeout = -1;
	connectTimeout = -1;
	sendTimeout = -1;
	recvTimeout = -1;
	useShare = false;
    getHeadersWithGET = false;
}

CurlFile::OpenArg::~OpenArg()
{

}

SceVoid CurlFile::OpenArg::SetUseShare(bool use)
{
	useShare = use;
}

SceVoid CurlFile::OpenArg::SetSecondaryHeaderMethod(bool use)
{
    getHeadersWithGET = use;
}

SceVoid CurlFile::OpenArg::SetUrl(const char *url)
{
	this->url = url;
}

// SceVoid CurlFile::OpenArg::SetUrl(paf::Url *url)
// {
// 	ScePVoid pobj = (ScePVoid)url;
// 	paf::swstring *swsurl = (paf::swstring *)(pobj + 0x38);
// 	this->url = swsurl->string;
// }

SceInt32 CurlFile::OpenArg::SetOpt(SceInt32 optValue, Opt optType)
{
	SceInt32 ret;

	if (optValue < 0) {
		ret = 0x80AF008F;
	}
	else {
		if ((optType & 1U) != 0) {
			resolveTimeout = optValue;
		}
		if ((optType & 2U) != 0) {
			connectTimeout = optValue;
		}
		if ((optType & 4U) != 0) {
			sendTimeout = optValue;
		}
		if ((optType & 8U) != 0) {
			recvTimeout = optValue;
		}
		ret = 0;
	}

	return ret;
}

CurlFile::CurlFile()
{
	lock = new thread::RMutex("ScePafCurlFileCsLock", 0);
	curl = SCE_NULL;
	pos = 0;
	contentLength = -1;
	isOpened = false;
    isGettingHeaders = false;
}

CurlFile::~CurlFile()
{
	if (isOpened) {
		Close();
	}
	delete lock;
}

SceInt32 CurlFile::GetFileType() const
{
	return 3;
}

SceInt32 CurlFile::GetCapability() const
{
	return 0x7BF7;
}

SceOff CurlFile::GetFileSize() const
{
	lock->Lock();

	if (!isOpened) {
		lock->Unlock();
		return 0x80AF5029;
	}

	lock->Unlock();

	return contentLength;
}

SceInt32 CurlFile::Open(const CurlFile::OpenArg *param)
{
	lock->Lock();

	if (!param) {
		lock->Unlock();
		return 0x80AF5002;
	}

	if (GetFileType() != param->fileType) {
		lock->Unlock();
		return 0x80AF5002;
	}

	if (isOpened) {
		lock->Unlock();
		return 0x80AF5028;
	}

	curl = curl_easy_init();
	if (!curl) {
		lock->Unlock();
		return 0x80AF5003;
	}

	if (param->useShare) {
		if (!s_share)
			InitShare();
		curl_easy_setopt(curl, CURLOPT_SHARE, s_share);
	}

	curl_easy_setopt(curl, CURLOPT_URL, param->url.c_str());
	curl_easy_setopt(curl, CURLOPT_USERAGENT, param->userAgent.c_str());
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, SCE_NULL);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadCore);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
#ifdef _DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
	if (param->connectTimeout != -1) {
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, param->connectTimeout);
	}

	if (param->recvTimeout != -1) {
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, param->recvTimeout);
	}
    print("option: %s -----------------------------------------\n", param->getHeadersWithGET ? "True" : "False");
    if(param->getHeadersWithGET)
	{
        isGettingHeaders = true;
        
        CURLcode ret = curl_easy_perform(curl);
        if (ret != CURLE_WRITE_ERROR) {
            print("Aborting open... 0x%X\n", ret);
            curl_easy_cleanup(curl);
            lock->Unlock();
            return 0x80AF5022;
        }

        ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);
        if (ret != CURLE_OK) {
            curl_easy_cleanup(curl);
            lock->Unlock();
            return 0x80AF5022;
        }

        if (contentLength < 0)
            contentLength = 0;
        
        isGettingHeaders = false;
        print("Got content length: 0x%X\n", contentLength);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        CURLcode ret = curl_easy_perform(curl);
        if (ret != CURLE_OK) {
            curl_easy_cleanup(curl);
            lock->Unlock();
            return 0x80AF5022;
        }

        ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);
        if (ret != CURLE_OK) {
            curl_easy_cleanup(curl);
            lock->Unlock();
            return 0x80AF5022;
        }

        if (contentLength < 0)
            contentLength = 0;
        

        curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

	isOpened = true;

	lock->Unlock();

	return 0;
}

SceInt32 CurlFile::OpenAsync(const CurlFile::OpenArg *param)
{
	lock->Lock();
	lock->Unlock();
	sceClibPrintf("[CurlFile] Async API is not supported\n");
	return 0x80AF5004;
}

SceInt32 CurlFile::Close()
{
	lock->Lock();

	if (!isOpened) {
		lock->Unlock();
		return 0x80AF5029;
	}

	curl_easy_cleanup(curl);
	curl = SCE_NULL;
	pos = 0;
	isOpened = false;

	lock->Unlock();

	return 0;
}

SceInt32 CurlFile::CloseAsync()
{
	lock->Lock();
	lock->Unlock();
	sceClibPrintf("[CurlFile] Async API is not supported\n");
	return 0x80AF5004;
}

bool CurlFile::IsOpened() const
{
	return isOpened;
}

bool CurlFile::IsEof()
{
	lock->Lock();
	lock->Unlock();
	return false;
}

SceInt32 CurlFile::Abort()
{
	lock->Lock();
	lock->Unlock();
	return 0;
}

SceInt32 CurlFile::Read(void *buffer, SceSize nbyte)
{
	lock->Lock();

	if (!isOpened) {
		lock->Unlock();
		return 0x80AF5029;
	}

	if (!buffer) {
		lock->Unlock();
		return 0x80AF5002;
	}

	if (nbyte == 0) {
		lock->Unlock();
		return 0;
	}

	char range[32];
	sce_paf_snprintf(range, 31, "%lld-%u", pos, pos + nbyte - 1);

	curl_easy_setopt(curl, CURLOPT_RANGE, range);

	buf = buffer;
	posInBuf = 0;
    print("CURL Entering\n");
	CURLcode ret = curl_easy_perform(curl);
    print("CURL Returned: 0x%X\n", ret);
	if (ret != CURLE_OK) {
		lock->Unlock();
		return 0x80AF5004;
	}

	pos += posInBuf;

	lock->Unlock();

	return posInBuf;
}

SceInt32 CurlFile::ReadAsync(void *buf, SceSize nbyte)
{
	lock->Lock();
	lock->Unlock();
	sceClibPrintf("[CurlFile] Async API is not supported\n");
	return 0x80AF5004;
}

SceInt32 CurlFile::Write(const void *buf, SceSize nbyte)
{
	return 0x80AF5004;
}

SceInt32 CurlFile::WriteAsync(const void *buf, SceSize nbyte)
{
	return 0x80AF5004;
}

SceOff CurlFile::Seek(SceOff offset, SceInt32 whence)
{
	SceOff ret = 0;

	lock->Lock();

	if (!isOpened) {
		lock->Unlock();
		return 0x80AF5029;
	}

	switch (whence) {
	case SCE_SEEK_CUR:
		if (pos + offset < 0) {
			lock->Unlock();
			return 0x80AF5002;
		}
		pos += offset;
		break;
	case SCE_SEEK_SET:
		if (offset < 0) {
			lock->Unlock();
			return 0x80AF5002;
		}
		pos = offset;
		break;
	case SCE_SEEK_END:
		if (contentLength == 0 || offset > 0) {
			lock->Unlock();
			return 0x80AF5002;
		}
		pos = contentLength + offset;
	}

	lock->Unlock();

	return pos;
}

SceInt32 CurlFile::SeekAsync(SceOff offset, SceInt32 whence)
{
	lock->Lock();
	lock->Unlock();
	sceClibPrintf("[CurlFile] Async API is not supported\n");
	return 0x80AF5004;
}

SceInt32 CurlFile::Flush()
{
	lock->Lock();
	lock->Unlock();
	return 0x80AF5004;
}

SceInt32 CurlFile::FlushAsync()
{
	lock->Lock();
	lock->Unlock();
	return 0x80AF5004;
}

SceInt32 CurlFile::WaitAsync(SceInt64 *pResult)
{
	lock->Lock();
	lock->Unlock();
	sceClibPrintf("[CurlFile] Async API is not supported\n");
	return 0x80AF5004;
}

SceInt32 CurlFile::PollAsync(SceInt64 *pResult)
{
	lock->Lock();
	lock->Unlock();
	sceClibPrintf("[CurlFile] Async API is not supported\n");
	return 0x80AF5004;
}

SceInt32 CurlFile::GetStat(FileStat *stat)
{
	lock->Lock();

	if (!isOpened) {
		lock->Unlock();
		return 0x80AF5029;
	}

	stat->st_size = contentLength;

	lock->Unlock();

	return 0;
}

SceInt32 CurlFile::SetPriority(SceInt32 ioPriority)
{
	return 0x80AF5005;
}

SceInt32 CurlFile::GetResponseCode(SceInt32 *code)
{
	lock->Lock();

	if (!isOpened) {
		lock->Unlock();
		return 0x80AF5029;
	}

	CURLcode ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (ret != CURLE_OK) {
		lock->Unlock();
		return 0x80AF5022;
	}

	lock->Unlock();

	return 0;
}

SharedPtr<CurlFile> CurlFile::Open(const char *path, SceUInt32 flag, SceUInt32 mode, SceInt32 *error, bool useShare, bool secondaryHeader)
{
	if (!path) {
		*error = 0x80AF5002;
		return SharedPtr<CurlFile>();
	}

	CurlFile::OpenArg oarg;
	oarg.SetUrl(path);
	oarg.SetUseShare(useShare);
    oarg.SetSecondaryHeaderMethod(secondaryHeader);

	CurlFile *file = new CurlFile();

	SceInt32 ret = file->Open(&oarg);
	*error = ret;
	if (ret != 0) {
		delete file;
		return SharedPtr<CurlFile>();
	}

	return SharedPtr<CurlFile>(file);
}

SharedPtr<CurlFile> CurlFile::Open(const SceWChar16 *url, SceInt32 *error, SceUInt32 flag, bool useShare)
{
	wstring text16;
	string text8;

	if (!url) {
		*error = 0x80AF5002;
		return SharedPtr<CurlFile>();
	}

	text16 = (wchar_t *)url;

	ccc::UTF16toUTF8(&text16, &text8);

	return CurlFile::Open(text8.c_str(), flag, 0, error);
}

SharedPtr<CurlFile> CurlFile::Open(const char *url, SceInt32 *error, SceUInt32 flag, bool useShare)
{
	return CurlFile::Open(url, flag, 0, error);
}

SceInt32 CurlFile::GetStat(const char *url, CurlFileStat *stat, bool useShare)
{
	CURL *curl = curl_easy_init();
	if (!curl) {
		return 0x80AF5003;
	}

	if (useShare) {
		if (!s_share)
			InitShare();
		curl_easy_setopt(curl, CURLOPT_SHARE, s_share);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, CURL_FILE_UA);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
#ifdef _DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif // _DEBUG
	CURLcode ret = curl_easy_perform(curl);
	if (ret != CURLE_OK) {
		curl_easy_cleanup(curl);
		return 0x80AF5022;
	}

	curl_off_t contentLength = -1;
	ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);
	curl_easy_cleanup(curl);
	if (ret != CURLE_OK) {
		return 0x80AF5022;
	}

	if (contentLength < 0)
		contentLength = 0;

	stat->st_size = contentLength;

	return 0;
}

SceSize CurlFile::DownloadCore(char *buffer, SceSize size, SceSize nitems, ScePVoid userdata)
{
	CurlFile *obj = (CurlFile *)userdata;
    if(obj->isGettingHeaders) return -1;

	SceSize toCopy = size * nitems;
    
	if (toCopy != 0) {
        print("Copying: %ld\n", toCopy);
		sce_paf_memcpy(obj->buf + obj->posInBuf, buffer, toCopy > obj->contentLength ? obj->contentLength : toCopy);
		obj->posInBuf += toCopy;
		return toCopy;
	}

	return 0;
}

SceVoid CurlFile::InitShare()
{
	if (!s_shareLock)
		s_shareLock = new thread::RMutex("ScePafCurlFileShareLock", 1);

	if (!s_share) {
		s_share = curl_share_init();
		curl_share_setopt(s_share, CURLSHOPT_LOCKFUNC, ShareLock);
		curl_share_setopt(s_share, CURLSHOPT_UNLOCKFUNC, ShareUnlock);
		curl_share_setopt(s_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
		curl_share_setopt(s_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
		curl_share_setopt(s_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
		curl_share_setopt(s_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_PSL);
	}
}

SceVoid CurlFile::TermShare()
{
	if (s_share) {
		curl_share_cleanup(s_share);
		s_share = SCE_NULL;
	}

	if (s_shareLock) {
		delete s_shareLock;
		s_shareLock = SCE_NULL;
	}
}

SceVoid CurlFile::ShareLock(CURL *handle, curl_lock_data data, curl_lock_access access, ScePVoid userptr)
{
	s_shareLock->Lock();
}

SceVoid CurlFile::ShareUnlock(CURL *handle, curl_lock_data data, ScePVoid userptr)
{
	s_shareLock->Unlock();
}