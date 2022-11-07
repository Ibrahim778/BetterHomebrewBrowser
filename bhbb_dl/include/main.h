#ifndef BHBB_DL_MAIN_H
#define BHBB_DL_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <psp2/types.h>
#include <vdsuite/curl/curl.h>

SceInt32 module_start(SceSize args, void *argp);
SceInt32 module_stop(SceSize args, void *argp);
int _start(SceSize s, void *a) __attribute__ ((weak, alias ("module_start")));

extern bool scePafFileExists(const char *file);
extern int scePafRemoveDirRecursive(const char *path);

#define SCE_KERNEL_MUTEX_ATTR_TH_FIFO           (0x00000000U) //VitaSDK moment
#define SCE_KERNEL_MSG_PIPE_TYPE_USER_MAIN      64
#define SCE_KERNEL_ATTR_OPENABLE		        (0x00000080U)
#define SCE_KERNEL_MSG_PIPE_MODE_WAIT			(0x00000000U)
#define SCE_KERNEL_MSG_PIPE_MODE_FULL			(0x00000001U)


typedef void*(*curl_malloc)(unsigned int size);
typedef void(*curl_f)(void *ptr);
typedef void*(*curl_realloc)(void *ptr, unsigned int new_size);

int curl_global_memmanager_set_np(curl_malloc allocate, curl_f deallocate, curl_realloc reallocate);

#define CBGDL_DL_PATH "ux0:temp/bhbb_cbgdl"

#define EXTRACT_PATH "ux0:data/bhbb_prom"
int install(const char *file);

#ifdef __cplusplus
}
#endif

#endif