#if !defined(__DNS_H__)
#define __DNS_H__

#include <stdint.h>

typedef struct dns_msg
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
} dns_msg_t;

#define DSN_MSG_SIZE (sizeof(dns_msg_t))

#endif // __DNS_H__
