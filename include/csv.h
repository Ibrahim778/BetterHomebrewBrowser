#ifndef CSV_DOT_H_INCLUDE_GUARD
#define CSV_DOT_H_INCLUDE_GUARD

#include <paf/stdc.h>
#include <stdio.h>
#include <kernel.h>

#define CSV_ERR_LONGLINE 0
#define CSV_ERR_NO_MEMORY 1
#define malloc sce_paf_malloc
#define free sce_paf_free
#define strlen sce_paf_strlen
#define memcpy sce_paf_memcpy
#define strdup paf_strdup
#define memset sce_paf_memset

SCE_CDECL_BEGIN

char *paf_strdup(const char *s);
char **parse_csv( const char *line );
void free_csv_line( char **parsed );
char **split_on_unescaped_newlines(const char *txt);
char *fread_csv_line(FILE *fp, int max_line_size, int *done, int *err);
char *getLine(const char *buffer, int *currentSeek);

SCE_CDECL_END

#endif
