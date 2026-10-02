#if !defined(__DNS_QUESTION_H__)
#define __DNS_QUESTION_H__

#include "dns_label.h"
#include "dns_common.h"
    
/// @brief DNS Query/Question type
typedef struct dns_question
{
    dns_label_t qname[DNS_MAX_LABEL_COUNT]; // Domain name expressed as a sequence of queries
    uint16_t qtype;                         // Type of query
    uint16_t qclass;                        // Class of query
    uint8_t label_cnt;                      // Number of labels
} dns_question_t;

void dns_question_init_as_domain_name(dns_question_t *query, const char *domain_name, const uint32_t size);

uint32_t dns_question_serialize(dns_question_t *query, char *out_buf, const uint32_t size);
uint32_t dns_question_deserialize(dns_question_t *query, const char *in_buf, const uint32_t size);
const char *dns_question_str( dns_question_t* query );

#endif // __DNS_QUESTION_H__
