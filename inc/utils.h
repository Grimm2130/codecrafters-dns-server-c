#if !defined(__UTILS_H__)
#define __UTILS_H__

#include <stdint.h>

#define _MIN(a, b) ((a) < (b) ? (a) : (b))
#define _MAX(a, b) ((a) > (b) ? (a) : (b))

uint32_t ip_str_to_n(const char *ip);
int ip_n_to_str(uint32_t ip_n, char *ip_str, const uint32_t len);

#endif // __UTILS_H__
