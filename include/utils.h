/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2023 Muhammad Ibrahim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <paf.h>

namespace Utils
{
    void InitMusic();
    SceVoid StartBGDL();
 
    void HttpsToHttp(const char *src, paf::string &outURL);
    bool IsValidURLSCE(const char *url); //Can this URL be used with SceHttp?

    int DownloadFile(const char *url, const char *path);
    void Decapitalise(char *string);
};
#endif