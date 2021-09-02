#ifndef BHBB_NETWORK_CPP
#define BHBB_NETWORK_CPP

#include <vshbridge.h>
#include <net.h>
#include<libsysmodule.h>
#include <libnetctl.h>
#include <libhttp.h>
#include <paf/stdc.h>

#define MODULE_PATH "vs0:data/external/webcore/ScePsp2Compat.suprx"
#define LIBC_PATH "vs0:sys/external/libc.suprx"
#define URL_MAX_SIZE 0x400

// Loads ScePsp2Compat.suprx if not loadeed
void loadPsp2CompatModule();

void netInit();
void netTerm();
void httpInit();
void httpTerm();

void Network_Init(const char *URL);
void Network_End();
SceInt32 Network_SaveFile(const char *dest, void (*onDoneCB)(void));

#endif