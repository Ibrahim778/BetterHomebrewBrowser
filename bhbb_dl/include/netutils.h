#ifndef NET_H
#define NET_H

#include <kernel.h>

SCE_CDECL_BEGIN

void curlInit();
void curlEnd();
int dlFile(const char *url, const char *dest);

SCE_CDECL_END

#endif