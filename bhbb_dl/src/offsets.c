#include "offsets.h"

//Originally By SKGleba

// This is pain
int GetShellOffsets(uint32_t nid, uint32_t *exp_off, uint32_t *rec_off, uint32_t *notif_off)
{
    *exp_off = *rec_off  = *notif_off = 0;
    switch (nid)
    {
    case 0x0552F692: // 3.60 retail
    {
        *exp_off = 0x1163F6;
        *rec_off = 0x11B5E4;
        *notif_off = 0xfd8f4;
        break;
    }

    case 0x6CB01295: // 3.60 PDEL
    {
        *exp_off = 0x111D5A;
        *rec_off = 0x116F48;
        // MISSING OFF
        break;
    }

    case 0xEAB89D5C: // 3.60 PTEL
    {
        *exp_off = 0x112756;
        *rec_off = 0x117944;
        *notif_off = 0xf9c54;
        break;
    }

    case 0x5549BF1F: // 3.65 retail
    case 0x34B4D82E: // 3.67 retail
    case 0x12DAC0F3: // 3.68 retail
    {
        *exp_off = 0x11644E;
        *rec_off = 0x11B63C;
        *notif_off = 0xfd94c;
        break;
    }

    case 0x0703C828: // 3.69 retail
    case 0x2053B5A5: // 3.70 retail
    case 0xF476E785: // 3.71 retail
    case 0x939FFBE9: // 3.72 retail
    case 0x734D476A: // 3.73 retail
    {
        *exp_off = 0x11644E;
        *rec_off = 0x11B63C;
        break;
    }

    case 0xE6A02F2B: // 3.65 PDEL
    {
        *exp_off = 0x111DB2;
        *rec_off = 0x116FA0;
        break;
    }

    case 0x587F9CED: // 3.65 PTEL
    {
        *exp_off = 0x1127AE;
        *rec_off = 0x11799C;
        break;
    }
    default:
        return -1;
    }
    if (!*exp_off || !*rec_off || !*notif_off)
        return -1;
    return 0;
}