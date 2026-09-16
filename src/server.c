#include "server.h"
#include "cir_buffer.h"
#include "server_context.h"
#include "utils.h"
#include "tasks.h"

#define ROUTINE_STACK_SIZE

server_t *server_create( server_params_t* params )
{
    server_t * srv = calloc(1, sizeof(server_t));
    server_init( srv, params );
    return srv;
}

void server_init(server_t * srv, server_params_t* params )
{
    memset(&(srv->admin_info), 0, sizeof(srv->admin_info));
    memset(&(srv->rt_info), 0, sizeof(srv->rt_info));
    
    srv->context = server_context_create(params->ip, params->port);

    srv->net_config.server_ip = ip_str_to_n(params->ip);
    srv->net_config.server_port = params->port;
    srv->net_config.server_recv_timout = params->rcv_to;
    srv->admin_info.alive = true;

    // Setup routines

    srv->recv_rtn.prio = params->recv_rtn.prio;
    srv->recv_rtn.stack_size = params->recv_rtn.stack_size;
    srv->recv_rtn._arg = params->recv_rtn._arg;
    srv->recv_rtn._fn = params->recv_rtn._fn;
    
    srv->trmt_rtn.prio = params->trmt_rtn.prio;
    srv->trmt_rtn.stack_size = params->trmt_rtn.stack_size;
    srv->trmt_rtn._arg = params->trmt_rtn._arg;
    srv->trmt_rtn._fn = params->trmt_rtn._fn;
    
    srv->receiver_task = srv->transmitter_task = NULL;
}

void server_start(server_t *srv)
{
    int ret;
    
    // Create socket
    if( (ret = server_context_create_socket( srv->context )) >= 0 )
    {
        int allow_reuse = 1;
        // Set required socket options
        if( (ret = server_context_setsocket_opt( srv->context, SOL_SOCKET, SO_REUSEADDR, &allow_reuse, sizeof(allow_reuse) ) ) >= 0 )
        {
            // Bind socket
            if( (ret = server_context_bind_socket( srv->context)) >= 0 )
            {
                task_params_t task_param;

                
                // Start the receiver and transmitter threads
                if( srv->recv_rtn._fn)
                {
                    task_param.can_join = true;
                    task_param.prio = srv->recv_rtn.prio;
                    task_param.stack_size = srv->recv_rtn.stack_size;
                    task_param.fn = srv->recv_rtn._fn;
                    task_param.arg = srv->recv_rtn._arg;
                    
                    srv->receiver_task = task_create( &task_param );
                    
                    task_start( srv->receiver_task );
                }
                
                if( srv->trmt_rtn._fn)
                {
                    task_param.can_join = true;
                    task_param.prio = srv->trmt_rtn.prio;
                    task_param.stack_size = srv->trmt_rtn.stack_size;
                    task_param.fn = srv->trmt_rtn._fn;
                    task_param.arg = srv->trmt_rtn._arg;

                    srv->receiver_task = task_create( &task_param );

                    task_start( srv->transmitter_task );
                }

            }
            else
            {

            }
        }
        else
        {

        }
    }
    else
    {

    }



}

void server_stop(server_t *srv)
{
    // Stop the receiver and transmitter threads

    // close the socket
}

/// @brief Retrieve server information
/// @param srv Server object
/// @param info buffer to save the information
/// @param cap capacity of the buffer
void server_get_info(server_t *srv, char *info, const int cap)
{
    char ip[20];
    ip_n_to_str(srv->net_config.server_ip, ip, 20),

    snprintf( info, cap, "{R-Stat: %s, T-Stat: %s, Addr: %s:%d, Read: %u, Written: %u, Timeouts: %u, Invalids: %u}",
        srv->admin_info.receiver_active ? "Active" : "Inactive",
        srv->admin_info.transmitter_active ? "Active" : "Inactive",
        ip,
        srv->net_config.server_port,
        srv->rt_info.read_bytes,
        srv->rt_info.written_bytes,
        srv->rt_info.read_timeouts,
        srv->rt_info.invalid_bytes_read
    );
}

void server_deinit(server_t *srv)
{
    
    srv->admin_info.alive = false;
}

void server_destroy(server_t *srv)
{
}