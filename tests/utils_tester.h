#if !defined(__UTILS_TESTER_H__)
#define __UTILS_TESTER_H__

#include "utils.h"
#include "assert.h"
#include "string.h"

void run_tests_utils();

void run_tests_utils()
{
    uint32_t ret,
        less = 1,
        more = 2,
        ip_n = (127 << 24) | 1U;
    const char* ip_str = "127.0.0.1";
    char ip_str_ret[20];

    assert( _MIN(less, more) == less );
    assert( _MAX(less, more) == more );

    ret = ip_str_to_n(ip_str);
    printf("Testing ip_str_to_n: %u ?= %u\n", ip_n, ret );
    assert( ret == ip_n );
    
    assert(ip_n_to_str( ip_n, ip_str_ret, 20) > 0 );
    printf("Testing ip_str_to_n: %s ?= %s\n", ip_str, ip_str_ret );
    assert( strcmp( ip_str, ip_str_ret ) == 0 );
}

#endif // __UTILS_TESTER_H__
