#if !defined(__DNS_HDR_H__)
#define __DNS_HDR_H__

#include <stdint.h>


/*===================================  Header  ==================================*/

typedef struct dns_hdr
{
    uint16_t id : 16;               // packet identifier
    uint16_t qr : 1;                // Query = 0, Response = 1
    uint16_t opcode : 4;            // Kind of query
    uint16_t aa : 1;                // Authorative answer
    uint16_t tc : 1;                // Truncation
    uint16_t rd : 1;                // Recursion Desired
    uint16_t ra : 1;                // Recursion Available
    uint16_t rz : 3;                // reserved
    uint16_t rcode : 4;             // Response code
    uint16_t qd_count : 16;         // Question Count
    uint16_t an_count : 16;         // Answer Record Count
    uint16_t ns_count : 16;         // Authority Record count
    uint16_t ar_count : 16;         // Additional Recoud count
} __attribute__((packed)) dns_hdr_t;

#define DSN_HDR_SIZE (sizeof(dns_hdr_t))

#define DNS_HDR_FLAGS_AS_U16(hdr_ptr)     (     \
            ((hdr_ptr)->rcode) |                \
            ((hdr_ptr)->rz << 4) |              \
            ((hdr_ptr)->ra << 7) |              \
            ((hdr_ptr)->rd << 8) |              \
            ((hdr_ptr)->tc << 9) |              \
            ((hdr_ptr)->aa << 10) |             \
            ((hdr_ptr)->opcode << 11) |         \
            ((hdr_ptr)->qr << 15)               \
    )

/*===================================  Header  ==================================*/

#endif // __DNS_HDR_H__
