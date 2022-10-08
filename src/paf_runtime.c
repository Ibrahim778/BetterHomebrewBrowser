#include <paf/stdc.h>

void * memmove(void *to, const void *from, size_t numBytes)
{
    return sce_paf_memmove(to, from, numBytes);
}