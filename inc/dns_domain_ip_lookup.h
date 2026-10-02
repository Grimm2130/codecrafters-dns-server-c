#if !defined(__DNS_DOMAIN_IP_LOOKUP_H__)
#define __DNS_DOMAIN_IP_LOOKUP_H__

typedef struct domain_ip_mapping
{
    const char * domain_name;
    const char *ip_addr;
} domain_ip_mapping;

static const domain_ip_mapping s_domain_to_ip_lookup_table [] = 
{
    {"codecrafters.io", "8.8.8.8"}
};

#define DNS_DOMAIN_IP_LOOKUP_TABLE_COUNT    (sizeof(s_domain_to_ip_lookup_table)/sizeof(domain_ip_mapping) )

const char *dns_get_ip_from_domain(const char* domain);

#endif // __DNS_DOMAIN_IP_LOOKUP_H__
