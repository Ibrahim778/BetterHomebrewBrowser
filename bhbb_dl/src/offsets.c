#include "offsets.h"

//Originally By SKGleba

// This is pain
int GetShellOffsets(SceUInt32 nid, SceUInt32 *a1, SceUInt32 *a2, SceUInt32 *a3, SceUInt32 *a4)
{
    *a1 = *a2 = *a3 = *a4 = 0;
    switch (nid)
    {
    case 0x0552F692: // 3.60 retail
    {
        *a1 = 0x50A4A8;
        *a2 = 0x1163F6;
        *a3 = 0x11B5E4;
        *a4 = 0x2c2f8;
        break;
    }

    case 0x6CB01295: // 3.60 PDEL
    {
        *a1 = 0x4F9A18;
        *a2 = 0x111D5A;
        *a3 = 0x116F48;
        *a4 = 0x2bb74;
        break;
    }

    case 0xEAB89D5C: // 3.60 PTEL
    {
        *a1 = 0x4FEBF8;
        *a2 = 0x112756;
        *a3 = 0x117944;
        *a4 = 0x2c464;
        break;
    }

    case 0x5549BF1F: // 3.65 retail
    case 0x34B4D82E: // 3.67 retail
    case 0x12DAC0F3: // 3.68 retail
    {
        *a1 = 0x50A9E8;
        *a2 = 0x11644E;
        *a3 = 0x11B63C;
        *a4 = 0x2c350;
        break;
    }

    case 0x0703C828: // 3.69 retail
    case 0x2053B5A5: // 3.70 retail
    case 0xF476E785: // 3.71 retail
    case 0x939FFBE9: // 3.72 retail
    case 0x734D476A: // 3.73 retail
    {
        *a1 = 0x50AA28;
        *a2 = 0x11644E;
        *a3 = 0x11B63C;
        *a4 = 0x2c350;
        break;
    }

    case 0xE6A02F2B: // 3.65 PDEL
    {
        *a1 = 0x4F9F58;
        *a2 = 0x111DB2;
        *a3 = 0x116FA0;
        *a4 = 0x2c248;
        break;
    }

    case 0x587F9CED: // 3.65 PTEL
    {
        *a1 = 0x4FF0F8;
        *a2 = 0x1127AE;
        *a3 = 0x11799C;
        *a4 = 0x2c4bc;
        break;
    }
    default:
        return -1;
    }
    if (!*a1 || !*a2 || !*a3 || !*a4)
        return -1;
    return 0;
}