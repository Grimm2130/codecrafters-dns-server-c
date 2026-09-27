#if !defined(__DNS_RR_CLASES_H__)
#define __DNS_RR_CLASES_H__

typedef enum _DNS_RR_CLASS
{
    DNS_RR_CLASS__IN = 1,   // 1 the Internet
    DNS_RR_CLASS__CS,       // 2 the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
    DNS_RR_CLASS__CH,       // 3 the CHAOS class
    DNS_RR_CLASS__HS,       // 4 Hesiod [Dyer 87]
} DNS_RR_CLASS;

typedef enum _DNS_RR_QCLASS
{
    DNS_RR_QCLASS__ANY = 255       // Any Class
} DNS_RR_QCLASS;

#endif // __DNS_RR_CLASES_H__
