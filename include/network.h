#ifndef BHBB_NETWORK_CPP
#define BHBB_NETWORK_CPP

#define cURL_PATH "app0:module/libcurl.suprx"

typedef void*(*t_curl_malloc)(size_t size);
typedef void(*t_curl_free)(void *ptr);
typedef void*(*t_curl_realloc)(void *ptr, size_t new_size);

extern "C" int curl_global_memmanager_set_np(t_curl_malloc allocate, t_curl_free deallocate, t_curl_realloc reallocate);

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