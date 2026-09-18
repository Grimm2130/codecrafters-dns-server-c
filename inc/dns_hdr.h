#if !defined(__DNS_HDR_H__)
#define __DNS_HDR_H__

#include <stdint.h>


/*===================================  Header  ==================================*/

typedef struct dns_hdr
{
    uint16_t id : 16;
    uint16_t qr : 1;
    uint16_t opcode : 4;
    uint16_t aa : 1;
    uint16_t tc : 1;
    uint16_t rd : 1;
    uint16_t ra : 1;
    uint16_t rz : 3;
    uint16_t rcode : 4;
    uint16_t qd_count : 16;
    uint16_t an_count : 16;
    uint16_t ns_count : 16;
    uint16_t ar_count : 16;
} __attribute__((packed)) dns_hdr_t;

#define DSN_MSG_SIZE (sizeof(dns_hdr_t))

/*===================================  Header  ==================================*/

#endif // __DNS_HDR_H__
