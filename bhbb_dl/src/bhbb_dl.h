#ifndef BHBB_DL
#define BHBB_DL

#define BHBB_DL_PIPE_NAME "bhbb_dl_pipe"

typedef enum
{
    INSTALL,
    CANCEL,
    SHUTDOWN,
} bhbbCommand;

typedef struct
{
    bhbbCommand cmd;
    char url[0x100];
    char name[0x100];
} bhbbPacket;

#endif