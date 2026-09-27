#if !defined(__DNS_RR_TYPES_H__)
#define __DNS_RR_TYPES_H__

/**
 * @brief DNS Resource Record Types
 * 
 */

typedef enum _DNS_RR_TYPE
{
    DNS_RR_TYPE__A = 1,         // Host address
    DNS_RR_TYPE__NS,            // Authoritative Name server
    DNS_RR_TYPE__MD,            // Mail destination (Obsolete)
    DNS_RR_TYPE__MF,            // Mail forwarder (obsolete)
    DNS_RR_TYPE__CNAME,         // Canonical name for alias   
    DNS_RR_TYPE__SOA,           // Start of a zone of authority 
    DNS_RR_TYPE__MB,            // mailbox domain name (EXPERIMENTAL)
    DNS_RR_TYPE__MG,            // mail group member (EXPERIMENTAL)
    DNS_RR_TYPE__MR,            // mail rename domain name (EXPERIMENTAL)
    DNS_RR_TYPE__NULL,          // null RR (EXPERIMENTAL)
    DNS_RR_TYPE__WKS,           // well known service description
    DNS_RR_TYPE__PTR,           // domain name pointer  
    DNS_RR_TYPE__HINFO,         // host information
    DNS_RR_TYPE__MINFO,         // mailbox or mail list information
    DNS_RR_TYPE__MX,            // mail exchange
    DNS_RR_TYPE__TXT,           // text strings
} DNS_RR_TYPE;

typedef enum _DNS_RR_QTYPE
{
    DNS_RR_QTYPE__AXFR = 252U,  // 252 A request for a transfer of an entire zone
    DNS_RR_QTYPE__MAILB,        // 253 A request for mailbox-related records (MB, MG or MR)
    DNS_RR_QTYPE__MAILA         // 254 A request for mail agent RRs (Obsolete - see MX)
} DNS_RR_QTYPE;

#endif // __DNS_RR_TYPES_H__
