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

#ifndef CSV_DOT_H_INCLUDE_GUARD
#define CSV_DOT_H_INCLUDE_GUARD

#include <paf/paf_types.h>
#include <paf/std/stdio.h>

#define CSV_ERR_LONGLINE -1
#define CSV_ERR_NO_MEMORY -2

// SCE_CDECL_BEGIN

char *fread_line(sce_paf_FILE *fp, int max_line_size, int *done, int *err);

// SCE_CDECL_END

#endif
