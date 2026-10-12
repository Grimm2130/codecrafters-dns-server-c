#if !defined(__DNS_H__)
#define __DNS_H__

#ifdef RESOLVER
#define DNS_PORT 2054U
// #define DNS_IP   "127.0.0.1"
#else
#define DNS_PORT 2053U
#endif
#define DNS_IP   "0.0.0.0"

void dns_server_main( int argc, char * argv []);
void dns_service_start( const char * forwarding_addr );
void dns_service_stop();


// #ifdef RESOLVER
// #else
// #endif


#endif // __DNS_H__
