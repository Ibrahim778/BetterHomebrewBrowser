#ifndef BHBB_BGDL_H
#define BHBB_BGDL_H
#ifdef __cplusplus
extern "C" {
#endif

int SendDlRequest(const char *title, const char *url);
void termBhbbDl();

#ifdef __cplusplus
}
#endif
#endif