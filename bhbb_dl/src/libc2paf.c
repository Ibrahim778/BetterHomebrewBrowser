#include <stddef.h>
#include <psp2/paf.h>

extern int sce_paf_feof(void *stream);
extern int sce_paf_ferror(void * stream);
extern void * sce_paf_fopen(const char * filename, const char * mode);
extern int sce_paf_fclose(void * stream);
extern size_t sce_paf_fread(void * ptr, size_t size, size_t count, void * stream);
extern size_t sce_paf_fwrite(const void * ptr, size_t size, size_t count, void * stream);
extern int sce_paf_fseek(void * stream, long int offset, int origin);
extern long int sce_paf_ftell(void * stream);

void *malloc(int size) {
    return sce_paf_malloc(size);
}

void free(void *ptr) {
    sce_paf_free(ptr);
}

void *fopen(const char* file, const char* mode) {
    return sce_paf_fopen(file, mode);
}

int fclose(void *fp) {
    return sce_paf_fclose(fp);
}

int fread(void* buf, int size, int count, void* fp) {
    return sce_paf_fread(buf, size, count, fp);
}

int fwrite(void* buf, int size, int count, void* fp) {
    return sce_paf_fwrite(buf, size, count, fp);
}

int fseek(void* fp, long offset, int whence) {
    return sce_paf_fseek(fp, offset, whence);
}

long ftell(void* fp) {
    return sce_paf_ftell(fp);
}

void* memset(void* b, int c, SceSize sz) {
    return sce_paf_memset(b, c, sz);
}

void* memcpy(void* dst, void* src, int sz) {
    return sce_paf_memcpy(dst, src, sz);
}

int strcmp(const char* str1, const char* str2) {
    return sce_paf_strcmp(str1, str2);
}

int strlen(const char *str) {
    return sce_paf_strlen(str);
}

char* strncpy(char* str1, const char *str2, int len) {
    return sce_paf_strncpy(str1, str2, len);
}