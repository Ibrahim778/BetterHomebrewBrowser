#ifndef CURLFILE_HPP
#define CURLFILE_HPP

#include <paf.h>
#include <psp2_compat/curl/curl.h>

class cURLFile
{
public:
    typedef SceBool (*cancelCheck)(void *cancelData);
    
    struct curlCancel
    {
        cancelCheck cb;
        void *      data;
    };

    cURLFile(const char *url);
    ~cURLFile();

    SceVoid SetUrl(const char *url);

    CURLcode Read();
    char *GetData();
    size_t GetSize();

    SceVoid SetCancelCheck(cancelCheck cancelCB, void *data);

    static SceInt32 SaveFile(const char *url, const char *file, cancelCheck = SCE_NULL, void *cancelData = SCE_NULL);
    
private:

    static size_t SaveCB(char *ptr, size_t size, size_t nmemb, paf::common::SharedPtr<paf::LocalFile> *userdata);
    static size_t ProgressCB(curlCancel *userDat, double dltotal, double dlnow, double ultotal, double ulnow);
    static size_t WriteCB(char *ptr, size_t size, size_t nmemb, cURLFile *userdata);

    CURL *handle;

    char *buff;
    size_t buffSize;

    curlCancel cancel;
};

#endif