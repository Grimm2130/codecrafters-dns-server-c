#if !defined(__UDP_SERVER_HANDLE_H__)
#define __UDP_SERVER_HANDLE_H__

#include "common.h"
#include "routine.h"

#define IP_ADDR_LEN 15

struct network_context;

typedef struct task task_t;
typedef struct cir_buffer cir_buffer_t;
typedef struct network_context network_context_t;
typedef struct client_msg client_msg_t;

typedef enum udp_server_handle_ret_code
{
    SRV_HNDL_SUCCESS,
    SRV_HNDL_SOCKET_INIT_FAIL,
    SRV_HNDL_SOCKET_BIND_FAIL,
    SRV_HNDL_SET_SOC_OPT_FAIL,
    SRV_HNDL_GET_SOC_OPT_FAIL
} udp_server_handle_ret_code_t;

typedef struct udp_server_handle
{
    // udp_server_handle network configuration information
    struct
    {
        uint16_t udp_server_handle_port;
        uint32_t udp_server_handle_ip;
        uint32_t client_ip_allow_mask;
        uint32_t udp_server_handle_recv_timout;
    } net_config;

    /// @brief udp_server_handle admin information
    struct
    {
        bool active;
    } admin_info;

    /// @brief udp_server_handle runtime information
    struct
    {
        uint32_t read_bytes;
        uint32_t written_bytes;
        uint32_t read_timeouts;
        uint32_t invalid_receives;
        uint32_t invalid_transmits;
    } rt_info;

    network_context_t *context; // udp_server_handle context

    // TODO: Add udp_server_handle configuration manager
} udp_server_handle_t;

typedef struct udp_server_handle_params
{

    uint16_t port;              // port
    uint32_t client_allow_mask; // client allow mask
    uint32_t rcv_to;            // receiver timeout
    char ip[IP_ADDR_LEN + 1];   // ip address
} udp_server_handle_params_t;

udp_server_handle_t *udp_server_handle_create(udp_server_handle_params_t *);
udp_server_handle_ret_code_t udp_server_handle_init(udp_server_handle_t *, udp_server_handle_params_t *);
int udp_server_handle_receive(udp_server_handle_t *, client_msg_t *);
int udp_server_handle_send(udp_server_handle_t *, client_msg_t *);
void udp_server_handle_get_info(udp_server_handle_t *, char *info, const int cap);
udp_server_handle_ret_code_t udp_server_handle_deinit(udp_server_handle_t *);
udp_server_handle_ret_code_t udp_server_handle_destroy(udp_server_handle_t *);

#endif // __UDP_SERVER_HANDLE_H__
