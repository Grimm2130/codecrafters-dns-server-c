#include "dns_server.h"
#include "dns_hdr.h"
#include "dns_question.h"
#include "dns_answer.h"
#include "server_handle.h"
#include "utils.h"
#include "client_msg.h"
#include "tasks.h"
#include "sync_cir_buffer.h"
#include "network_serializer.h"
#include "serializer.h"

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
static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_hdr(char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_question(char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_answer(char *rply_buf, const uint32_t size);

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
        print_msg((uint8_t *)(&to_transmit.data), to_transmit.len);

        bytes = server_handle_send(srv_hndl, &to_transmit);

        printf("Transmitted\n");
        fflush(stdout);

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
    print_msg((uint8_t *)msg->data, msg->len);

    // Copy client content
    CLIENT_MSG_CPY(&response, msg);

    // copy header info
    memset(response.data, 0, CLIENT_DATA_SIZE);
    response.len = dns_reply_serialize( response.data, CLIENT_DATA_SIZE );

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

static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size)
{
    uint32_t offset = 0;

    // Construct DNS header
    offset += dns_reply_serialize_hdr(rply_buf + offset, size - offset );
    
    // Construct DNS Question
    offset += dns_reply_serialize_question(rply_buf + offset, size - offset );

    // Construct DNS Answer
    offset += dns_reply_serialize_answer( rply_buf + offset, size - offset );

    // Construct DNS Authority

    // Construct DNS Additional

    return offset;
}

static uint32_t dns_reply_serialize_hdr(char *rply_buf, const uint32_t size)
{
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
        .qd_count = 1,
        .an_count = 1,
        .ns_count = 0,
        .ar_count = 0};

    // copy header info
    memset(rply_buf, 0, DSN_HDR_SIZE);

    printf("Serialized Header: %s\n", dns_hdr_str(&hdr));

    return dns_hdr_serialize(&hdr, rply_buf, size);
}

static uint32_t dns_reply_serialize_question(char *rply_buf, const uint32_t size)
{
    static const char * domain_name = "codecrafters.io";
    static const size_t domain_name_len = strlen("codecrafters.io");

    dns_question_t question;
    dns_question_init_as_domain_name( &question, domain_name, domain_name_len );

    printf("Serialized Question: %s\n", dns_question_str(&question));

    return  dns_question_serialize( &question, rply_buf, size );
}

static uint32_t dns_reply_serialize_answer(char *rply_buf, const uint32_t size)
{
    dns_answer_t answer;
    const char * domain_name = "codecrafters.io";
    const size_t domain_name_len = strlen(domain_name);
    uint32_t desc = htonl(ip_str_to_n("8.8.8.8"));
    const size_t desc_len = sizeof(uint32_t);

    dns_answer_init_from_domain_name_with_desc( &answer, domain_name, domain_name_len, 60, (const char*)&desc, desc_len );

    printf("Serialized Answer: %s\n", dns_answer_str(&answer));

    return dns_answer_serialize( &answer, rply_buf, size );
}