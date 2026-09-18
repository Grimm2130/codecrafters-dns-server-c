#include "server_handle.h"
#include "utils.h"
#include "client_msg.h"
#include "tasks.h"
#include "dns_server.h"
#include "dns_hdr.h"
#include "sync_cir_buffer.h"

bool g_runFlag;
sync_cir_buffer_t transmit_buffer;
server_handle_t g_server_handle;
task_t g_receiver;
task_t g_transmitter;

typedef enum dns_service_ret_code
{
    DNS_OK,
    DNS_ERROR_INVALID
} dns_service_ret_code_t;

static void print_msg(const char *data, const int len);
static void dns_handle_new_client_data(client_msg_t *msg);

static void *dns_receiver_routine(void *arg)
{
    server_handle_t *srv_hndl = (server_handle_t *)arg;
    client_msg_t received;
    int bytes;

    while (g_runFlag)
    {
        bytes = server_handle_receive(srv_hndl, &received);

        // handle received data
        if (bytes > 0)
        {
            // Valid
            srv_hndl->rt_info.read_bytes += bytes;

            dns_handle_new_client_data(&received);
        }
        else if (bytes == 0)
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

static void *dns_transmitter_routine(void *arg)
{
    int bytes;
    server_handle_t *srv_hndl = (server_handle_t *)arg;
    client_msg_t to_transmit;

    while (g_runFlag)
    {
        sync_cir_buffer_read(&transmit_buffer, (char *)&(to_transmit), sizeof(to_transmit), false);

        bytes = server_handle_send(srv_hndl, &to_transmit);

        if (bytes > 0)
        {
            // Valid
            srv_hndl->rt_info.written_bytes += bytes;
        }
        else
        {
            srv_hndl->rt_info.invalid_transmits++;
        }
    }

    return NULL;
}

static void dns_handle_new_client_data(client_msg_t *msg)
{
    static char server_status[200];

    server_handle_get_info(&g_server_handle, server_status, 200);
    printf("Server Status : %s\n", server_status);

    printf(">>> Received: ");
    print_msg(msg->data, msg->len);

    dns_hdr_t hdr = {
        .id = 1234,
        .qr = 1,
        .opcode = 0,
        .aa = 0,
        .tc = 0,
        .rd = 0,
        .ra = 0,
        .rz = 0,
        .rcode = 0,
        .qd_count = 0,
        .an_count = 0,
        .ns_count = 0,
        .ar_count = 0};

    client_msg_t client_response;
    memcpy(&client_response, &msg, sizeof(client_msg_t));
    memset(&client_response.data, 0, CLIENT_DATA_SIZE);
    memcpy(&client_response, &hdr, DSN_HDR_SIZE);
    client_response.len = DSN_HDR_SIZE;

    sync_cir_buffer_write(&transmit_buffer, (const char*)&client_response, CLIENT_MSG_SIZE, false );
}

void dns_service_start()
{
    // initialize transmit buffer
    sync_cir_buffer_init(&transmit_buffer, false);

    // Intialize task params
    task_params_t task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_server_handle),
        .fn = dns_receiver_routine};

    // initialize serve_handle params
    server_handle_params_t server_handle_param = {
        .client_allow_mask = (uint32_t)(-1),
        .port = DNS_PORT,
        .rcv_to = (uint32_t)1e6};

    server_handle_init(&g_server_handle, &server_handle_param);

    // initialize receiver routine
    task_init(&g_receiver, &task_param);

    task_param.fn = dns_transmitter_routine;

    // initialize transmitter routine
    task_init(&g_transmitter, &task_param);

    g_runFlag = true;

    // Start transmitter
    task_start(&g_receiver);
    task_start(&g_transmitter);
}

void dns_service_stop()
{
    g_runFlag = false;

    server_handle_deinit(&g_server_handle);
}

static void print_msg(const char *data, const int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("%x ", data[i]);
    }
    printf("\n");
}