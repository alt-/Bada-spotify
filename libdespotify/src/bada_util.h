#ifndef BADA_UTIL_H
#define BADA_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif
unsigned char *hex_ascii_to_bytes (char *, unsigned char *, int);
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
#ifdef __cplusplus
}
#endif

#endif
