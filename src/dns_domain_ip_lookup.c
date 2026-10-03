#include "dns_domain_ip_lookup.h"
#include <stdint.h>
#include <string.h>

const char *dns_get_ip_from_domain(const char* domain)
{
    for( int i = 0; i < DNS_DOMAIN_IP_LOOKUP_TABLE_COUNT; i++ )
    {
        if( strcmp(domain, s_domain_to_ip_lookup_table[i].domain_name) == 0 )
        {
            return s_domain_to_ip_lookup_table[i].ip_addr;
        }
    }

    return "8.8.8.8";
}