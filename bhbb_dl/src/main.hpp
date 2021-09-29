#define SET_TEXT(dest, src) wcsncpy(((wchar_t *)(dest)), ((wchar_t *)(src)), sizeof((dest)))

#ifdef _DEBUG
#define print sceClibPrintf
#define LOG_ERROR(prefix, error_code) print("[%s] Got Error: 0x%X\n", prefix, error_code);
#else
#define print (void)NULL
#define LOG_ERROR(prefix, error_code) (void)NULL;
#endif