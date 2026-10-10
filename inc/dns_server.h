#if !defined(__DNS_H__)
#define __DNS_H__

#define DNS_PORT 2053U
#define DNS_IP   "0.0.0.0"

void dns_service_start( const char * forwarding_addr );
void dns_service_stop();




#endif // __DNS_H__
