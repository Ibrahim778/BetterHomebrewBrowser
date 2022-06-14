#ifndef BHBB_PRINT_H
#define BHBB_PRINT_H

#ifdef _DEBUG
#define print(...) sceClibPrintf(__VA_ARGS__)
#else
#define print(...) {(void)NULL;}
#endif

#endif