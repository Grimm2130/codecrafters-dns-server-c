#if !defined(__DNS_QUESTION_H__)
#define __DNS_QUESTION_H__

#include "dns_label.h"
#include "dns_common.h"

#define DNS_QUESTION_LABEL_COMPRESSION_MASK     0xC0
#define DNS_QUESTION_COMPRESSED_POINTER_MASK    0x3FFF
#define DNS_LABEL_IS_COMPRESSED(label_len)      \
    (((label_len) & DNS_QUESTION_LABEL_COMPRESSION_MASK) == DNS_QUESTION_LABEL_COMPRESSION_MASK)
    
/// @brief DNS Query/Question type
typedef struct dns_question
{
    dns_label_t qname[DNS_MAX_LABEL_COUNT]; // Domain name expressed as a sequence of queries
    uint16_t qtype;                         // Type of query
    uint16_t qclass;                        // Class of query
    uint8_t label_cnt;                      // Number of labels
} dns_question_t;

void dns_question_init_as_domain_name(dns_question_t *query, const char *domain_name, const uint32_t size);

uint32_t dns_question_serialize(const dns_question_t *query, char *out_buf, const uint32_t size);
uint32_t dns_question_deserialize(dns_question_t *query, const char *in_buf, const uint32_t size, uint16_t* c_pointer);
uint16_t dns_question_get_qname_as_domain_name( const dns_question_t *query, char *domain_name, const uint32_t size);
const char *dns_question_str( const dns_question_t* query );

#endif // __DNS_QUESTION_H__
