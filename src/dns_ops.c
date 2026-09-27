#include "dns_hdr.h"
#include "dns_question.h"
#include "dns_label.h"
#include "dns_rr_types.h"
#include "dns_rr_classes.h"
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

uint32_t dns_label_serialize(dns_label_t *label, char *out_buf, const uint8_t size)
{
    net_serializer_t serializer;
    net_serializer_init(&serializer);

    // Serialize len
    net_serializer_append_u8(&serializer, &label->len, 1);

    // serialize content
    net_serializer_append_raw(&serializer, (const char *)(&label->content[0]), label->len);

    // Copy serialized content to buffer
    net_serializer_copy_raw(&serializer, 0, out_buf, (sizeof( label->len ) + label->len) );
    
    net_serializer_deinit(&serializer);

    return sizeof( label->len ) + label->len;
}

uint32_t dns_label_deserialize(dns_label_t *label, const char *in_buf, const uint8_t size)
{
    net_serializer_t serializer;
    net_serializer_init(&serializer);

    // Copy content of buffer into serializer
    net_serializer_append_raw(&serializer, in_buf, size);

    // Copy len
    net_serializer_remove_u8(&serializer, 0, &label->len, 1);
    net_serializer_remove_raw(&serializer, 0, (char *)(&label->content), label->len);
    
    net_serializer_deinit(&serializer);

    return sizeof( label->len ) + label->len;
}

/*
 *	================================== DNS QUESTION ==================================
 */

static void dns_question_init(dns_question_t *query)
{
    query->label_cnt = query->qtype = query->qclass = 0;

    for (uint8_t i = 0; i < DNS_QUESTION_LABEL_COUNT; i++)
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
    } while ((query->label_cnt < DNS_QUESTION_LABEL_COUNT) && (token = strtok(NULL, ".")));

    free(cpyBuf);
}

uint32_t dns_question_serialize(dns_question_t *query, char *out_buf, const uint32_t size)
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
    net_serializer_remove_raw(&serializer, 0, (char *)(out_buf + offset), 2 * sizeof(uint16_t));

    net_serializer_deinit(&serializer);

    return offset + (2 * sizeof(uint16_t));
}

uint32_t dns_question_deserialize(dns_question_t *query, const char *in_buf, const uint32_t size)
{
    net_serializer_t serializer;
    uint8_t label_idx = 0;
    uint32_t offset = 0;
    uint32_t label_end_idx = size - (2 * sizeof(uint16_t));

    net_serializer_init(&serializer);

    // deserialize the labels
    while ((offset < label_end_idx) || (label_idx < query->label_cnt))
    {
        offset += dns_label_deserialize(&query->qname[label_idx++], in_buf + offset, size - offset);
    }

    // deserialize the type and class
    if (offset == label_end_idx)
    {
        offset++; // Skip null character
        net_serializer_append_raw(&serializer, in_buf + offset, size - offset);
        net_serializer_remove_u16(&serializer, 0, &query->qtype, 1);
        net_serializer_remove_u16(&serializer, 0, &query->qclass, 1);
    }

    net_serializer_deinit(&serializer);

    return offset;
}
