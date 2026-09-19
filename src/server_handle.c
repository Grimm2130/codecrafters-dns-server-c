#include "server_handle.h"
#include "cir_buffer.h"
#include "network_context.h"
#include "utils.h"
#include "tasks.h"
#include "client_msg.h"

#define ROUTINE_STACK_SIZE

server_handle_t *server_handle_create(server_handle_params_t *params)
{
    server_handle_t *srv = calloc(1, sizeof(server_handle_t));
    server_handle_init(srv, params);
    return srv;
}

server_handle_ret_code_t server_handle_init(server_handle_t *srv, server_handle_params_t *params)
{
    int ret;
    server_handle_ret_code_t retCode;

    memset(&(srv->admin_info), 0, sizeof(srv->admin_info));
    memset(&(srv->rt_info), 0, sizeof(srv->rt_info));

    srv->net_config.server_handle_ip = ip_str_to_n(params->ip);
    srv->net_config.server_handle_port = params->port;
    srv->net_config.server_handle_recv_timout = params->rcv_to;
    srv->admin_info.active = true;

    // initialize server context
    srv->context = network_context_create(params->ip, params->port);

    // Create socket
    if ((ret = network_context_create_socket(srv->context)) >= 0)
    {
        int allow_reuse = 1;
        // Set required socket options
        if ((ret = network_context_setsocket_opt(srv->context, SOL_SOCKET, SO_REUSEADDR, &allow_reuse, sizeof(allow_reuse))) >= 0)
        {
            // Bind socket
            if ((ret = network_context_bind_socket(srv->context)) == 0)
            {
                retCode = SRV_HNDL_SUCCESS;
            }
            else
            {
                // ! EROR
                retCode = SRV_HNDL_SOCKET_BIND_FAIL;
            }
        }
        else
        {
            // ! EROR
            retCode = SRV_HNDL_SET_SOC_OPT_FAIL;
        }
    }
    else
    {
        // ! EROR
        retCode = SRV_HNDL_SOCKET_INIT_FAIL;
    }

    return retCode;
}

int server_handle_receive(server_handle_t *srv, client_msg_t *msg)
{
    socklen_t addrlen = sizeof(sockaddr_in_t);
    msg->len = recvfrom(srv->context->sock, msg->data, CLIENT_DATA_SIZE, 0, (sockaddr_t *)(&(msg->context.addr)), &addrlen);
    return msg->len;
}

int server_handle_send(server_handle_t *srv, client_msg_t *msg)
{
    return sendto(srv->context->sock, msg->data, msg->len, 0, (const sockaddr_t *)(&(msg->context.addr)), sizeof(msg->context.addr));
}

/// @brief Retrieve server_handle information
/// @param srv server_handle object
/// @param info buffer to save the information
/// @param cap capacity of the buffer
void server_handle_get_info(server_handle_t *srv, char *info, const int cap)
{
    char ip[20];
    ip_n_to_str(srv->net_config.server_handle_ip, ip, 20),

        snprintf(info, cap, "{Addr: %s:%d, Read: %u, Written: %u, Timeouts: %u, Invalids: %u}",
                 ip,
                 srv->net_config.server_handle_port,
                 srv->rt_info.read_bytes,
                 srv->rt_info.written_bytes,
                 srv->rt_info.read_timeouts,
                 srv->rt_info.invalid_receives);
}

server_handle_ret_code_t server_handle_deinit(server_handle_t *srv)
{
    srv->admin_info.active = false;

    return SRV_HNDL_SUCCESS;
}

server_handle_ret_code_t server_handle_destroy(server_handle_t *srv)
{
    server_handle_ret_code_t ret;

    if ((ret = server_handle_deinit(srv)) == 0)
    {
        free(srv);
    }

    return ret;
}