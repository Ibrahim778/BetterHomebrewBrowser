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
