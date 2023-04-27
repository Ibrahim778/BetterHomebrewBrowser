#ifndef BHBB_UTILS_H
#define BHBB_UTILS_H

#include <paf.h>

namespace Utils
{
    void InitMusic();
    SceVoid StartBGDL();
 
    void HttpsToHttp(paf::string &str);
    bool IsValidURLSCE(const char *url); //Can this URL be used with SceHttp?

    int DownloadFile(const char *url, const char *path);
    void Decapitalise(char *string);
};
#endif