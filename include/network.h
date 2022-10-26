#ifndef BHBB_NETWORK_CPP
#define BHBB_NETWORK_CPP

#define cURL_PATH "app0:module/libcurl.suprx"

typedef void*(*curl_malloc)(unsigned int size);
typedef void(*curl_free)(void *ptr);
typedef void*(*curl_realloc)(void *ptr, unsigned int new_size);

extern "C" int curl_global_memmanager_set_np(curl_malloc allocate, curl_free deallocate, curl_realloc reallocate);

namespace Network
{
    enum Status {
        Offline,
        Online
    };

    SceVoid Init();
    SceVoid Term();
    SceVoid Check(void (*CheckComplete)(void));
    Status GetCurrentStatus();
    SceInt32 GetLastError();

    class CheckThread : public paf::thread::Thread
    {
        using paf::thread::Thread::Thread;

        SceVoid EntryFunction();
    };
};



#endif