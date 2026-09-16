#include "server.h"
#include "utils.h"
#include "dns.h"


server_t g_server;

static void * dns_receiver( void * arg )
{
    
}

static void * dns_transmitter( void * arg )
{

}

void dns_service_start()
{
    server_params_t server_param = {
        .client_allow_mask = (uint32_t)(-1),
        .port = DNS_PORT,
        .rcv_to = (uint32_t) 1e6,
        .recv_rtn = { 
            .prio = 10,
            .stack_size = 1024 * 1024,
            ._fn = dns_receiver,
            ._arg = &g_server
        },
        .trmt_rtn = { 
            .prio = 10,
            .stack_size = 1024 * 1024,
            ._fn = dns_transmitter,
            ._arg = &g_server
        },
    };

    server_init( &g_server, &server_param );
    server_start( &g_server );
}

void dns_service_stop()
{
    server_stop( &g_server );
    server_

}
