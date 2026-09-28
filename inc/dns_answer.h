#if !defined(__DNS_ANSWER_H__)
#define __DNS_ANSWER_H__

#include "dns_label.h"
#include "dns_common.h"

typedef struct dns_question dns_question_t;

typedef struct dns_answer
{
    dns_label_t name[DNS_MAX_LABEL_COUNT];
    uint16_t type;
    uint16_t _class;
    uint32_t ttl;
    uint16_t rdlength;
    char rddata[UINT16_MAX];
    uint8_t label_cnt;
} dns_answer_t;

void dns_answer_init_domain_name_no_desc(dns_answer_t *answer, const uint32_t ttl, const char* domain_name, const uint32_t domain_name_len );
void dns_answer_init_from_domain_name_with_desc(dns_answer_t *answer, const char* domain_name, const uint32_t domain_name_len, const uint32_t ttl, const char *description, const uint16_t description_len);
void dns_answer_set_description(dns_answer_t *answer, const char *description, const uint16_t description_len );
uint32_t dns_answer_serialize(dns_answer_t *answer, char *out_buf, const uint8_t size);
uint32_t dns_answer_deserialize(dns_answer_t *answer, const char *in_buf, const uint8_t size);
const char * dns_answer_str( dns_answer_t* answer );

#endif // __DNS_ANSWER_H__
