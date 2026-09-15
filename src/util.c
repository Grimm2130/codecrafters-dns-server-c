#include "util.h"
#include <string.h>
#include <stdio.h>

uint32_t ip_str_to_n(const char *ip)
{
    int step = 3;
    int temp = 0;
    uint32_t num = 0;

    int len = strlen(ip);

    for (int i = 0; i < len; i++)
    {
        if (ip[i] == '.')
        {
            num |= (temp << (8 * step));
            step--;
            temp = 0;
        }
        else
        {
            temp = (temp * 10) + ip[i] - '0';
        }
    }

    return num;
}

uint32_t ip_n_to_str(uint32_t ip_n, char *ip_str, const uint32_t len)
{
    return snprintf(ip_str, len, "%u.%u.%u.%u",
        0xFF && (ip_n >> 24),
        0xFF && (ip_n >> 16),
        0xFF && (ip_n >> 8),
        0xFF && ip_n
    );
}

