#ifndef BHBB_DL_MAIN_H
#define BHBB_DL_MAIN_H

#include <kernel.h>
#include <psp2_compat/curl/curl.h>

SCE_CDECL_BEGIN

SceInt32 module_start(SceSize args, void *argp);
SceInt32 module_stop(SceSize args, void *argp);

typedef void*(*curl_malloc)(size_t size);
typedef void(*curl_f)(void *ptr);
typedef void*(*curl_realloc)(void *ptr, size_t new_size);

int curl_global_memmanager_set_np(curl_malloc allocate, curl_f deallocate, curl_realloc reallocate);
int zipInitPsp2(void);

#define CBGDL_DL_PATH "ux0:temp/bhbb_cbgdl"
#define EXTRACT_PATH "ux0:data/bhbb_prom/"

int install(const char *file);

SCE_CDECL_END

#endif