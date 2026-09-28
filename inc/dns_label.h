#if !defined(__DNS_LABEL_H__)
#define __DNS_LABEL_H__

#include <stdint.h>

#define DNS_LABEL_MAX_LEN 63U

typedef struct dns_label
{
    uint8_t len;
    char content[DNS_LABEL_MAX_LEN];
} dns_label_t;

void dns_label_init( dns_label_t* label, const char* content, const uint8_t len );
uint32_t dns_label_serialize( dns_label_t* label, char* out_buf, const uint8_t size );
uint32_t dns_label_deserialize( dns_label_t* label, const char* in_buf, const uint8_t size );
const char* dns_label_str( dns_label_t* label );

#endif // __DNS_LABEL_H__
