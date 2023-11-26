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

#include "offsets.h"

//Originally by @SKGleba for bgvpk

// This is (glebs) pain
int GetShellOffsets(uint32_t nid, uint32_t *exp_off, uint32_t *rec_off, uint32_t *notif_off)
{
    *exp_off = *rec_off = *notif_off = 0;
    switch (nid)
    {
    case 0x0552F692: // 3.60 retail
    {
        *exp_off = 0x1163F6;
        *rec_off = 0x11B5E4;
        *notif_off = 0xFD8F4;
        break;
    }

    case 0x6CB01295: // 3.60 PDEL
    {
        *exp_off = 0x111D5A;
        *rec_off = 0x116F48;
        *notif_off = 0xF9258;
        break;
    }

    case 0xEAB89D5C: // 3.60 PTEL
    {
        *exp_off = 0x112756;
        *rec_off = 0x117944;
        *notif_off = 0xF9C54;
        break;
    }

    case 0x5549BF1F: // 3.65 retail
    case 0x34B4D82E: // 3.67 retail
    case 0x12DAC0F3: // 3.68 retail
    case 0x0703C828: // 3.69 retail
    case 0x2053B5A5: // 3.70 retail
    case 0xF476E785: // 3.71 retail
    case 0x939FFBE9: // 3.72 retail
    case 0x734D476A: // 3.73 retail
    {
        *exp_off = 0x11644E;
        *rec_off = 0x11B63C;
        *notif_off = 0xFD94C;
        break;
    }

    case 0xE6A02F2B: // 3.65 PDEL
    {
        *exp_off = 0x111DB2;
        *rec_off = 0x116FA0;
        *notif_off = 0xF92B0;
        break;
    }

    case 0x587F9CED: // 3.65 PTEL
    {
        *exp_off = 0x1127AE;
        *rec_off = 0x11799C;
        *notif_off = 0xF9CAC;
        break;
    }
    default:
        return -1;
    }
    if (!*exp_off || !*rec_off || !*notif_off)
        return -1;
    return 0;
}