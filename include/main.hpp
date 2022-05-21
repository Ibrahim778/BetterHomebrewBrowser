#ifndef MAIN_HPP
#define MAIN_HPP

#ifdef _DEBUG
#define print sceClibPrintf
#define LOG_ERROR(prefix, error_code) print("[%s] Got Error: 0x%X\n", prefix, error_code);
#else
#define print (void)NULL;
#define LOG_ERROR(prefix, error_code) (void)NULL;
#endif

#define LOAD_FLAGS_ALL 0xFFFFFFFF
#define LOAD_FLAGS_ICONS 1
#define LOAD_FLAGS_SCREENSHOTS 2

#define DOWNLOAD_ICONS_PER_TIME 5

void OnReady();

#endif