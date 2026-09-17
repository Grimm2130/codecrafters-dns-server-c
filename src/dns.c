#include "server_handle.h"
#include "utils.h"
#include "client_msg.h"
#include "tasks.h"
#include "./dns.h"
#include "cir_buffer.h"


bool g_runFlag;
cir_buffer_t transmit_buffer;
server_handle_t g_server_handle;
task_t g_receiver;
task_t g_transmitter;

typedef enum dns_service_ret_code
{
    DNS_OK,
    DNS_ERROR_INVALID
} dns_service_ret_code_t;

static void dns_handle_new_client_data( client_msg_t* msg );

static void * dns_receiver_routine( void * arg )
{
    server_handle_t *srv_hndl = (server_handle_t*)arg;
    client_msg_t received;


    while( g_runFlag )
    {
        server_handle_receive( srv_hndl, &received );

        // handle received data
        if( received.bytes > 0 )
        {
            // Valid
            srv_hndl->rt_info.read_bytes += received.bytes;

            dns_handle_new_client_data( &received );

        }
        else if( received.bytes == 0 )
        {
            srv_hndl->rt_info.read_timeouts++;
        }
        else
        {
            srv_hndl->rt_info.invalid_receives++;
        }
    }

    // Display client information

    return NULL;
}

static void * dns_transmitter_routine( void * arg )
{
    return NULL;
}

static void dns_handle_new_client_data( client_msg_t* msg )
{
    
}

void dns_service_start()
{
    // initialize transmit buffer
    cir_buffer_init( &transmit_buffer, false );

    // Intialize task params
    task_params_t task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void*)(&g_server_handle),
        .fn = dns_receiver_routine
    };

    // initialize serve_handle params
    server_handle_params_t server_handle_param = {
        .client_allow_mask = (uint32_t)(-1),
        .port = DNS_PORT,
        .rcv_to = (uint32_t) 1e6
    };

    server_handle_init( &g_server_handle, &server_handle_param );

    task_init( &g_receiver, &task_param );
    
    task_param.fn = dns_transmitter_routine;
    task_init( &g_transmitter, &task_param );
    
    g_runFlag = true;
    
    task_start(&g_receiver);
    task_start(&g_transmitter);
}

void dns_service_stop()
{
    g_runFlag = false;

    server_handle_stop( &g_server_handle );
    

}
