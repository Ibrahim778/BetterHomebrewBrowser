#ifndef BHBB_NETWORK_CPP
#define BHBB_NETWORK_CPP

#define MODULE_PATH "vs0:data/external/webcore/ScePsp2Compat.suprx"
#define LIBC_PATH "vs0:sys/external/libc.suprx"
#define FIOS2_PATH "vs0:sys/external/libfios2.suprx"

class Network
{
public:

    enum Status {
        Offline,
        Online
    };

    static SceVoid Init();
    static SceVoid Term();
    static SceVoid Check(void (*CheckComplete)(void));
    static Status GetCurrentStatus();
    static SceInt32 GetLastError();

    class CheckThread : public paf::thread::Thread
    {
        using paf::thread::Thread::Thread;

        SceVoid EntryFunction();
    };
private:
    static SceInt32 lastError;
    static Status CurrentStatus;
    static void (*CheckComplete)(void);
};



#endif