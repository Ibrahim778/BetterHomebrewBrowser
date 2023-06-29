#ifndef BHBB_DL_H
#define BHBB_DL_H

#define BHBB_DL_MAGIC 'BG'
#define BHBB_DL_CFG_VER 3

#pragma pack(push, 1)
struct pdb_flags_t // size is 0xB
{
    char unk[3];
    uint16_t size;
    char unk2[2];
    uint16_t size2;
    char unk3[2];
};
#pragma pack(pop)

typedef enum 
{
    BGDLTarget_CompressedFile = 0,
    BGDLTarget_App = 1
} BGDLTarget;

typedef struct BGDLParam
{
    uint16_t magic;     // bhbb_dl magic | cfg version
    uint8_t type;       // 0 - zip, 1 - app
    char path[0x100];     // This is used when type is set to 0 (custom path)
    char data_icon[0x100]; // Fallback icon for operating with data files and while saving the vpk without install
} BGDLParam;

#endif