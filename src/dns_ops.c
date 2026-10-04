#include "dns_hdr.h"
#include "dns_label.h"
#include "dns_common.h"
#include "dns_question.h"
#include "dns_rr_types.h"
#include "dns_rr_classes.h"
#include "dns_answer.h"
#include "network_serializer.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 *	================================== DNS HEADER ==================================
 */

void dns_hdr_init(dns_hdr_t *hdr)
{
    memset(hdr, 0, DSN_HDR_SIZE);
}

uint32_t dns_hdr_serialize(dns_hdr_t *hdr, char *out_buf, const uint32_t size)
{
    uint32_t len = 0;
    net_serializer_t serializer;

    net_serializer_init(&serializer);

    uint16_t temp = hdr->id;
    net_serializer_append_u16(&serializer, &temp, 1);

    temp = DNS_HDR_FLAGS_TO_U16(hdr);
    net_serializer_append_u16(&serializer, &temp, 1);

    temp = hdr->qd_count;
    net_serializer_append_u16(&serializer, &temp, 1);

    temp = hdr->an_count;
    net_serializer_append_u16(&serializer, &temp, 1);

    temp = hdr->ns_count;
    net_serializer_append_u16(&serializer, &temp, 1);

    temp = hdr->ar_count;
    net_serializer_append_u16(&serializer, &temp, 1);

    // Copy into header buffer
    net_serializer_copy_raw(&serializer, 0, out_buf, DSN_HDR_SIZE);

    net_serializer_deinit(&serializer);

    return DSN_HDR_SIZE;
}

uint32_t dns_hdr_deserialize(dns_hdr_t *hdr, const char *in_buf, const uint32_t size)
{
    uint32_t len = 0;
    net_serializer_t serializer;
    uint16_t temp;

    net_serializer_init(&serializer);

    net_serializer_append_raw(&serializer, in_buf, DSN_HDR_SIZE);

    // ID
    net_serializer_remove_u16(&serializer, 0, &temp, 1);
    hdr->id = temp;

    // Flags
    net_serializer_remove_u16(&serializer, 0, &temp, 1);
    DNS_HDR_FLAG_FROM_U16(hdr, temp);

    net_serializer_remove_u16(&serializer, 0, &temp, 1);
    hdr->qd_count = temp;

    net_serializer_remove_u16(&serializer, 0, &temp, 1);
    hdr->an_count = temp;

    net_serializer_remove_u16(&serializer, 0, &temp, 1);
    hdr->ns_count = temp;

    net_serializer_remove_u16(&serializer, 0, &temp, 1);
    hdr->ar_count = temp;

    net_serializer_deinit(&serializer);

    return DSN_HDR_SIZE;
}

const char * dns_hdr_str( const dns_hdr_t* hdr )
{
    static char buf[255];

    snprintf(buf, 255, "{id: %u, qr: %u, opcode: %u, aa: %u, tc: %u, rd: %u, ra: %u, rz: %u, rcode: %u, qd_count: %u, an_count: %u, ns_count: %u, ar_count: %u}",
        hdr->id,
        hdr->qr,
        hdr->opcode,
        hdr->aa,
        hdr->tc,
        hdr->rd,
        hdr->ra,
        hdr->rz,
        hdr->rcode,
        hdr->qd_count,
        hdr->an_count,
        hdr->ns_count,
        hdr->ar_count
    );

    return buf;
}

/*
 *	================================== DNS LABEL ==================================
 */

void dns_label_init(dns_label_t *label, const char *content, const uint8_t len)
{
    if (label)
    {
        label->len = _MIN(len, DNS_LABEL_MAX_LEN);
        memcpy(label->content, content, label->len);
    }
    else
    {
        exit(-1);
    }
}

uint32_t dns_label_serialize(const dns_label_t *label, char *out_buf, const uint8_t size)
{
    net_serializer_t serializer;
    net_serializer_init(&serializer);

    // Serialize len
    net_serializer_append_u8(&serializer, &label->len, 1);

    // serialize content
    net_serializer_append_raw(&serializer, (const char *)(&label->content[0]), label->len);

    // Copy serialized content to buffer
    net_serializer_copy_raw(&serializer, 0, out_buf, (sizeof(label->len) + label->len));

    net_serializer_deinit(&serializer);

    return sizeof(label->len) + label->len;
}

uint32_t dns_label_deserialize(dns_label_t *label, const char *in_buf, const uint8_t size)
{
    net_serializer_t serializer;
    net_serializer_init(&serializer);

    // Copy content of buffer into serializer
    net_serializer_append_raw(&serializer, in_buf, size);

    // Copy len
    net_serializer_remove_u8(&serializer, 0, &label->len, 1);

    if( label->len & 0xC0 )
    {
        printf("Compressed label detected. Looking for data @ offset %u\n", label->len & 0x4F );
        fflush(stdout);
    }

    net_serializer_remove_raw(&serializer, 0, (char *)(&label->content), label->len);

    net_serializer_deinit(&serializer);

    return sizeof(label->len) + label->len;
}

const char* dns_label_str( const dns_label_t* label )
{
    static char buf[255];
    static char temp[64];
    memcpy( temp, label->content, label->len );
    temp[label->len] = '\0';

    snprintf( buf, 255, "Label {len: %u, content: %s}", label->len, temp );
    return buf;
}

/*
 *	================================== DNS QUESTION ==================================
 */

static void dns_question_init(dns_question_t *query)
{
    query->label_cnt = query->qtype = query->qclass = 0;

    for (uint8_t i = 0; i < DNS_MAX_LABEL_COUNT; i++)
    {
        query->qname[i].len = 0;
        query->qname[i].content[0] = '\0';
    }
}

void dns_question_init_as_domain_name(dns_question_t *query, const char *domain_name, const uint32_t size)
{
    char *token;

    // initialize the query object
    dns_question_init(query);

    // Set the Domain type and class
    query->qtype = (uint16_t)DNS_RR_TYPE__A;
    query->qclass = (uint16_t)DNS_RR_CLASS__IN;

    char *cpyBuf = calloc(size + 1, sizeof(char));

    memcpy(cpyBuf, domain_name, size);

    token = strtok(cpyBuf, ".");
    do
    {
        // Update the label
        dns_label_init(&(query->qname[query->label_cnt++]), token, strlen(token));
    } while ((query->label_cnt < DNS_MAX_LABEL_COUNT) && (token = strtok(NULL, ".")));

    free(cpyBuf);
}

uint32_t dns_question_serialize( const dns_question_t *query, char *out_buf, const uint32_t size)
{
    uint32_t offset = 0;
    net_serializer_t serializer;
    net_serializer_init(&serializer);

    // Serialize the labels
    for (uint8_t i = 0; i < query->label_cnt; i++)
    {
        offset += dns_label_serialize(&(query->qname[i]), out_buf + offset, size - offset);
    }

    // terminate qname
    out_buf[offset++] = '\0';

    // insert the type and class into serializer
    net_serializer_append_u16(&serializer, &query->qtype, 1);
    net_serializer_append_u16(&serializer, &query->qclass, 1);

    // Insert serialized content into buffer
    net_serializer_remove_raw(&serializer, 0, (char *)(out_buf + offset), net_serializer_get_size( &serializer ));

    net_serializer_deinit(&serializer);

    return offset + (2 * sizeof(uint16_t));
}

uint32_t dns_question_deserialize(dns_question_t *query, const char *in_buf, const uint32_t size)
{
    char temp[64];
    uint32_t offset = 0;
    net_serializer_t serializer;
    uint8_t len;

    // reset the question
    dns_question_init( query );

    // instantiate the serializer
    net_serializer_init(&serializer);

    // Copy the buffer contents into the serializer
    net_serializer_append_raw( &serializer, in_buf, size);

    do
    {
        // remove len or null terminator
        if( net_serializer_remove_u8( &serializer, 0, &len, 1 ) == 0 )
        {
            printf("????%s\n\n", len >> 6 == 0x3 )
            offset += sizeof(u_int8_t);

            if( len )
            {
                if( query->label_cnt == DNS_MAX_LABEL_COUNT )
                {
                    // ! ERROR
                    printf("Question label content exceeded!\n");
                    exit(-1);
                }
                // Extract the label content
                else if(net_serializer_remove_raw(&serializer, 0, temp, len ) == 0 )
                {
                    // initialize the question label
                    dns_label_init( &query->qname[query->label_cnt++], temp, len );
    
                    offset += len;
                }
                else
                {
                    // ! ERROR
                }
            }
            else
            {
                // ! ERROR
            }
        }
        else
        {
            // ! ERROR
        }
        
    }
    while( (len != 0) );

    if( net_serializer_remove_u16(&serializer, 0, &query->qtype, 1 )  )
    {
        // ! ERROR
    }

    if( net_serializer_remove_u16(&serializer, 0, &query->qclass, 1 )  )
    {
        // ! ERROR
    }

    offset += (2 * sizeof(uint16_t));

    net_serializer_deinit(&serializer);

    return offset;
}

uint16_t dns_question_get_qname_as_domain_name(const dns_question_t *query, char *domain_name, const uint32_t size)
{
    uint16_t offset = 0;

    for( uint8_t i = 0; i < query->label_cnt; i++ )   
    {        
        // Copy the domain name
        memcpy( domain_name + offset, query->qname[i].content, query->qname[i].len );

        // increment the offset size
        offset += query->qname[0].len;

        // Add the domain separator
        domain_name[offset++] = '.';
    }

    // terminate string
    domain_name[--offset] = '\0';

    return offset;
}

const char *dns_question_str( const dns_question_t* query )
{
    int len = 0;
    #define     DNS_QUESTION_STR_SIZE   2048U
    static char buf[DNS_QUESTION_STR_SIZE];

    len += snprintf(buf + len, DNS_QUESTION_STR_SIZE - len, "names: {\n" );

    for( int i = 0; i < query->label_cnt; i++ )
    {
        len += snprintf( buf + len, DNS_QUESTION_STR_SIZE - len, "\t#%d %s,\n", i, dns_label_str( &(query->qname[i]) ) );
    }

    len += snprintf( buf + len, DNS_QUESTION_STR_SIZE - len, "},\n\ttype:%u,\n\tclass: %u\n}", query->qtype, query->qclass );

    return  buf;
}

/*
 *	================================== DNS ANSWER ==================================
 */

static void dns_answer_init(dns_answer_t *answer)
{
    answer->ttl = 0;
    answer->label_cnt = answer->type = answer->_class = answer->rdlength = 0;
    
    for (uint8_t i = 0; i < DNS_MAX_LABEL_COUNT; i++)
    {
        answer->name[i].len = 0;
        answer->name[i].content[0] = '\0';
    }

    memset(answer->rddata, 0, UINT16_MAX );
}

void dns_answer_init_domain_name_no_desc(dns_answer_t *answer, const uint32_t ttl, const char* domain_name, const uint32_t domain_name_len )
{
    char *token;

    // initialize the query object
    dns_answer_init(answer);

    // Set time to live
    answer->ttl = ttl;

    // Set the Domain type and class
    answer->type = (uint16_t)DNS_RR_TYPE__A;
    answer->_class = (uint16_t)DNS_RR_CLASS__IN;

    char *cpyBuf = calloc(domain_name_len + 1, sizeof(char));

    memcpy(cpyBuf, domain_name, domain_name_len);

    token = strtok(cpyBuf, ".");
    do
    {
        // Update the label
        dns_label_init(&(answer->name[answer->label_cnt++]), token, strlen(token));
    } while ((answer->label_cnt < DNS_MAX_LABEL_COUNT) && (token = strtok(NULL, ".")));

    free(cpyBuf);
}


void dns_answer_init_from_domain_name_with_desc(dns_answer_t *answer, const char* domain_name, const uint32_t domain_name_len, const uint32_t ttl, const char *description, const uint16_t description_len)
{
    
    dns_answer_init_domain_name_no_desc( answer, ttl, domain_name, domain_name_len );

    // Copy answer specific information
    answer->ttl = ttl;
    dns_answer_set_description( answer, description, description_len );
}

void dns_answer_set_description(dns_answer_t *answer, const char *description, const uint16_t description_len )
{
    answer->rdlength = description_len;
    memcpy( answer->rddata, description, description_len );
}

uint32_t dns_answer_serialize(dns_answer_t *answer, char *out_buf, const uint8_t size)
{
    uint32_t offset = 0;
    net_serializer_t serializer;
    net_serializer_init(&serializer);

    // Serialize the labels
    for (uint8_t i = 0; i < answer->label_cnt; i++)
    {
        offset += dns_label_serialize(&(answer->name[i]), out_buf + offset, size - offset);
    }

    // terminate qname
    out_buf[offset++] = '\0';

    // insert the type and class into serializer
    net_serializer_append_u16(&serializer, &answer->type, 1);
    net_serializer_append_u16(&serializer, &answer->_class, 1);
    
    // insert the ttl
    net_serializer_append_u32(&serializer, &answer->ttl, 1 );
    
    // insert the rdlength
    net_serializer_append_u16(&serializer, &answer->rdlength, 1 );
    
    // insert the rddata
    net_serializer_append_raw(&serializer, answer->rddata, answer->rdlength );
    
    // Insert serialized content into buffer
    net_serializer_remove_raw(&serializer, 0, (char *)(out_buf + offset), net_serializer_get_size( &serializer ) );
    
    offset += (2 * sizeof(uint16_t)) + (sizeof(uint32_t)) + (sizeof(uint16_t)) + (answer->rdlength);
    
    net_serializer_deinit(&serializer);

    return offset;
}

uint32_t dns_answer_deserialize(dns_answer_t *answer, const char *in_buf, const uint8_t size)
{
    net_serializer_t serializer;
    uint8_t label_idx = 0;
    uint32_t offset = 0;
    uint32_t label_end_idx = size - (2 * sizeof(uint16_t));

    net_serializer_init(&serializer);

    // deserialize the labels
    while ((offset < label_end_idx) || (label_idx < answer->label_cnt))
    {
        offset += dns_label_deserialize(&answer->name[label_idx++], in_buf + offset, size - offset);
    }

    // deserialize the type and class
    if (offset == label_end_idx)
    {
        offset++; // Skip null character
        net_serializer_append_raw(&serializer, in_buf + offset, size - offset);
        net_serializer_remove_u16(&serializer, 0, &answer->type, 1);
        net_serializer_remove_u16(&serializer, 0, &answer->_class, 1);
        net_serializer_remove_u32(&serializer, 0, &answer->ttl, 1);
        net_serializer_remove_u16(&serializer, 0, &answer->rdlength, 1);
        net_serializer_remove_raw(&serializer, 0, answer->rddata, answer->rdlength);
        offset += (3 * sizeof(uint16_t)) + sizeof(uint32_t) + answer->rdlength;
    }

    net_serializer_deinit(&serializer);

    return offset;
}

const char * dns_answer_str( dns_answer_t* answer )
{
    int len = 0;
    #define     DNS_ANSWER_STR_SIZE   2048U
    static char buf[DNS_ANSWER_STR_SIZE];
    static char temp[UINT16_MAX];

    len += snprintf(buf + len, DNS_ANSWER_STR_SIZE - len, "names: {\n" );

    for( int i = 0; i < answer->label_cnt; i++ )
    {
        len += snprintf( buf + len, DNS_ANSWER_STR_SIZE - len, "\t#%d %s,\n", i, dns_label_str( &(answer->name[i]) ) );
    }

    memcpy( temp, answer->rddata, answer->rdlength );
    temp[answer->rdlength] = '\0';

    len += snprintf( buf + len, DNS_ANSWER_STR_SIZE - len, "},\n\ttype:%u,\n\tclass: %u,\n\tdesc-len: %u,\n\tdesc: %s\n}",
        answer->type, 
        answer->_class,
        answer->rdlength,
        temp
    );

    return  buf;
}