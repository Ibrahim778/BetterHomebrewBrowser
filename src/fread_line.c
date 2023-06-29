#include <paf/paf_types.h>
#include <paf/std/stdlib.h>
#include <paf/std/string.h>
#include <paf/std/stdio.h>

#include "csv.h"

#define READ_BLOCK_SIZE 65536
#define QUICK_GETC(ch, fp)                                                          \
    do                                                                              \
    {                                                                               \
        if (read_ptr == read_end)                                                   \
        {                                                                           \
            fread_len = sce_paf_fread(read_buf, sizeof(char), READ_BLOCK_SIZE, fp); \
            if (fread_len < READ_BLOCK_SIZE)                                        \
            {                                                                       \
                read_buf[fread_len] = '\0';                                         \
            }                                                                       \
            read_ptr = read_buf;                                                    \
        }                                                                           \
        ch = *read_ptr++;                                                           \
    } while (0)

char *strdup(const char *src)
{
    size_t len = sce_paf_strlen(src);
    char *buff = sce_paf_malloc(len + 1);
    sce_paf_strncpy(buff, src, len);
    buff[len] = '\0';
    return buff;
}

char *fread_line(sce_paf_FILE *fp, int max_line_size, int *done, int *err)
{
    static sce_paf_FILE *bookmark;
    static char read_buf[READ_BLOCK_SIZE], *read_ptr, *read_end;
    static int fread_len, prev_max_line_size = -1;
    static char *buf;
    char *bptr, *limit;
    char ch;
    int fQuote;

    if (max_line_size > prev_max_line_size)
    {
        if (prev_max_line_size != -1)
        {
            sce_paf_free(buf);
        }
        buf = sce_paf_malloc(max_line_size + 1);
        if (!buf)
        {
            *err = CSV_ERR_NO_MEMORY;
            prev_max_line_size = -1;
            return NULL;
        }
        prev_max_line_size = max_line_size;
    }
    bptr = buf;
    limit = buf + max_line_size;

    if (bookmark != fp)
    {
        read_ptr = read_end = read_buf + READ_BLOCK_SIZE;
        bookmark = fp;
    }

    for (fQuote = 0;;)
    {
        QUICK_GETC(ch, fp);

        if (!ch || (ch == '\n' && !fQuote))
        {
            break;
        }

        if (bptr >= limit)
        {
            sce_paf_free(buf);
            *err = CSV_ERR_LONGLINE;
            return NULL;
        }
        *bptr++ = ch;

        if (fQuote)
        {
            if (ch == '\"')
            {
                QUICK_GETC(ch, fp);

                if (ch != '\"')
                {
                    if (!ch || ch == '\n')
                    {
                        break;
                    }
                    fQuote = 0;
                }
                *bptr++ = ch;
            }
        }
        else if (ch == '\"')
        {
            fQuote = 1;
        }
    }

    *done = !ch;
    *bptr = '\0';
    return strdup(buf);
}