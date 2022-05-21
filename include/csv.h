#ifndef CSV_DOT_H_INCLUDE_GUARD
#define CSV_DOT_H_INCLUDE_GUARD

#define CSV_ERR_LONGLINE 0
#define CSV_ERR_NO_MEMORY 1

#include <paf/stdc.h>
#include <stdio.h>

#define malloc sce_paf_malloc
#define free sce_paf_free
#define strlen sce_paf_strlen
#define memcpy sce_paf_memcpy
#define strdup paf_strdup

char *paf_strdup(const char *s);

#ifdef __cplusplus
extern "C"
{
#endif
char **parse_csv( const char *line );
void free_csv_line( char **parsed );
char **split_on_unescaped_newlines(const char *txt);
char *fread_csv_line(FILE *fp, int max_line_size, int *done, int *err);
#ifdef __cplusplus
}
#endif
#endif
