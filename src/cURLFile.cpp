#include <curl/curl.h>

#include "cURLFile.h"
#include "print.h"

cURLFile::cURLFile(const char *url)
{
    buff = SCE_NULL;
    buffSize = 0;

    handle = curl_easy_init();

    SetCancelCheck(SCE_NULL, SCE_NULL);

    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 100000L);
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCB);

    if(url)
        SetUrl(url);

#ifdef _DEBUG
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
#endif
}

SceVoid cURLFile::SetCancelCheck(cancelCheck cb, void *data)
{
    cancel.cb = cb;
    cancel.data = data;

    if(cb)
    {
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &cancel);
        curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, ProgressCB);
    }
    else
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
}

SceInt32 cURLFile::SaveFile(const char *url, const char *file, cURLFile::cancelCheck cancel, void *cancelData)
{
    curlCancel cancelStruct;
    cancelStruct.cb = cancel;
    cancelStruct.data = cancelData;

    SceInt32 ret = SCE_OK;
    paf::SharedPtr<paf::LocalFile> localFile = paf::LocalFile::Open(file, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0666, &ret);
    if(ret != SCE_OK)
        return ret;

    CURL *handle = curl_easy_init();
    if(!handle)
        return -1;

    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
    curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(handle, CURLOPT_URL, url);

    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &localFile);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, SaveCB);

    if(cancel)
    {
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &cancelStruct);
        curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, ProgressCB);
    }
    else
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
    
    ret = curl_easy_perform(handle);

    curl_easy_cleanup(handle);
    if(ret != CURLE_OK)
    {
        localFile.get()->Close();
        paf::LocalFile::RemoveFile(file);
    }    
    print("SaveFile > 0x%X\n", ret);
    return ret;
}

size_t cURLFile::WriteCB(char *ptr, size_t size, size_t nmemb, cURLFile *file)
{
    print("WriteCB\n");
    if(file->cancel.cb)
        if(file->cancel.cb(file->cancel.data))
            return 0;

    size_t actualSize = size * nmemb;

    print("Allocating: 0x%X, 0x%X\n", file->buff, file->buffSize + actualSize);
    file->buff = (char *)sce_paf_realloc(file->buff, file->buffSize + actualSize);
    if(!file->buff)
        return 0;

    sce_paf_memcpy(file->buff + file->buffSize, ptr, actualSize);
    file->buffSize += actualSize;
    print("Buffsize: %x, actualSize: 0x%X\n", file->buffSize, actualSize);
    return actualSize;
}

size_t cURLFile::ProgressCB(curlCancel *cancel, double dltotal, double dlnow, double ultotal, double ulnow)
{
    if(cancel->cb)
        if(cancel->cb(cancel->data))
            return 1;

    return 0;
}

size_t cURLFile::SaveCB(char *ptr, size_t size, size_t nmemb, paf::SharedPtr<paf::LocalFile> *file)
{
    print("Writing 0x%X\n", size * nmemb);
    return file->get()->Write(ptr, size * nmemb);
}

char *cURLFile::GetData()
{
    return buff;
}

size_t cURLFile::GetSize()
{
    return buffSize;
}

CURLcode cURLFile::Read()
{
    return curl_easy_perform(handle);
}

SceVoid cURLFile::SetUrl(const char *url)
{
    print("Setting URL: %s\n", url);
    curl_easy_setopt(handle, CURLOPT_URL, url);
}

cURLFile::~cURLFile()
{
    print("Cleaning cURL\n");
    if(handle)    
        curl_easy_cleanup(handle);
    print("Freeing buff\n");
    if(buff)
        delete buff;
    print("Done ~cURLFile\n");
}
