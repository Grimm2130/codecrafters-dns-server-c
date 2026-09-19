#include "server_handle.h"
#include "utils.h"
#include "client_msg.h"
#include "tasks.h"
#include "dns_server.h"
#include "dns_hdr.h"
#include "sync_cir_buffer.h"

bool g_runFlag;
task_t g_receiver;
task_t g_transmitter;
server_handle_t g_server_handle;
sync_cir_buffer_t g_transmit_buffer;

typedef enum dns_service_ret_code
{
    DNS_OK,
    DNS_ERROR_INVALID
} dns_service_ret_code_t;

static void print_msg(const uint8_t *data, const int len);
static void dns_handle_new_client_data(client_msg_t *msg);
static void enqueue_client_message(const client_msg_t *msg);
static void dequeue_client_message(client_msg_t *msg);
static void dns_hdr_serialize(dns_hdr_t *hdr, char *buffer);

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
    char ip[20];
    server_handle_t *srv_hndl = (server_handle_t *)arg;
    client_msg_t to_transmit;

    while (g_runFlag)
    {
        dequeue_client_message(&to_transmit);
        ip_n_to_str(htonl(to_transmit.context.addr.sin_addr.s_addr), ip, 20);

        printf(">>> Transmitting [%d] bytes to %s:%u -> ", to_transmit.len, ip, htons(to_transmit.context.addr.sin_port));
        print_msg((uint8_t*)(&to_transmit.data), to_transmit.len);

        bytes = server_handle_send(srv_hndl, &to_transmit);

        printf("Done\n");

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

void dns_service_start()
{
    // initialize transmit buffer
    sync_cir_buffer_init(&g_transmit_buffer, false);

    // Intialize task params
    task_params_t receiver_task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_server_handle),
        .fn = dns_receiver_routine};
    task_params_t transmitter_task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_server_handle),
        .fn = dns_transmitter_routine};

    // initialize serve_handle params
    server_handle_params_t server_handle_param = {
        .client_allow_mask = (uint32_t)(-1),
        .port = DNS_PORT,
        .rcv_to = (uint32_t)1e6};

    server_handle_init(&g_server_handle, &server_handle_param);

    // initialize rx & tx tasks
    task_init(&g_receiver, &receiver_task_param);
    task_init(&g_transmitter, &transmitter_task_param);

    g_runFlag = true;

    // Start transmitter
    task_start(&g_receiver);
    task_start(&g_transmitter);

    static char server_status[200];

    while (1)
    {
        server_handle_get_info(&g_server_handle, server_status, 200);
        printf("Server Status : %s\n", server_status);

        sleep(3);
    }
}

void dns_service_stop()
{
    g_runFlag = false;

    server_handle_deinit(&g_server_handle);
}

static void dns_handle_new_client_data(client_msg_t *msg)
{
    char ip[20];
    client_msg_t response;

    ip_n_to_str(ntohl(msg->context.addr.sin_addr.s_addr), ip, 20);
    printf(">>> Received [%d] bytes from %s:%u -> ", msg->len, ip, ntohs(msg->context.addr.sin_port));
    print_msg((uint8_t*)msg->data, msg->len);

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

    // Copy client content
    CLIENT_MSG_CPY(&response, msg);

    // copy header info
    memset(response.data, 0, DSN_HDR_SIZE);
    dns_hdr_serialize(&hdr, response.data);
    response.len = DSN_HDR_SIZE;

    enqueue_client_message(&response);
}

static void print_msg(const uint8_t *data, const int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("0x%02x ", data[i]);
    }
    printf("\n");
}

static void enqueue_client_message(const client_msg_t *msg)
{
    sync_cir_buffer_write(&g_transmit_buffer, (const char *)msg, CLIENT_MSG_SIZE, false);
}

static void dequeue_client_message(client_msg_t *msg)
{
    sync_cir_buffer_read(&g_transmit_buffer, (char *)msg, CLIENT_MSG_SIZE, false);
}

static void dns_hdr_serialize(dns_hdr_t *hdr, char *buffer)
{
    int len = 0;
    uint16_t temp;

    temp = htons(hdr->id);
    memcpy(buffer, &temp, sizeof(uint16_t));

    temp = htons(DNS_HDR_FLAGS_AS_U16(hdr));
    memcpy(buffer + 2, &temp, sizeof(uint16_t));

    temp = htons(hdr->qd_count);
    memcpy(buffer + 4, &temp, sizeof(uint16_t));

    temp = htons(hdr->an_count);
    memcpy(buffer + 6, &temp, sizeof(uint16_t));

    temp = htons(hdr->ns_count);
    memcpy(buffer + 8, &temp, sizeof(uint16_t));

    temp = htons(hdr->ar_count);
    memcpy(buffer + 10, &temp, sizeof(uint16_t));
}