#include "dns_server.h"
#include "dns_hdr.h"
#include "dns_question.h"
#include "dns_answer.h"
#include "dns_rr_types.h"
#include "dns_rr_classes.h"
#include "dns_domain_ip_lookup.h"
#include "udp_server_handle.h"
#include "utils.h"
#include "client_msg.h"
#include "tasks.h"
#include "cir_buffer.h"
#include "sync_cir_buffer.h"
#include "network_serializer.h"
#include "serializer.h"

#define DNS_SECTION_BUFFER_SIZE   255U

bool g_runFlag;
task_t g_receiver;
task_t g_transmitter;
udp_server_handle_t g_udp_server_handle;
sync_cir_buffer_t g_transmit_buffer;
net_serializer_t g_rxSerializer;
net_serializer_t g_txSerializer;
char g_dns_section_buffer[DNS_SECTION_BUFFER_SIZE];

// dns_hdr_t g_header;
// dns_question_t g_question;
// dns_answer_t g_answer;

typedef enum dns_service_ret_code
{
    DNS_OK,
    DNS_ERROR_INVALID
} dns_service_ret_code_t;

static void print_msg(const uint8_t *data, const int len);
static void dns_handle_new_client_data(client_msg_t *msg);
static void enqueue_client_message(const client_msg_t *msg);
static void dequeue_client_message(client_msg_t *msg);

static void dns_process_msg();
static void dns_process_msg__dns_hdr( const dns_hdr_t* hdr );
static void dns_process_msg__dns_hdr_valid( const dns_hdr_t* hdr );
static void dns_process_msg__dns_hdr_valid_query( const dns_hdr_t* hdr );
static void dns_process_msg__dns_hdr_valid_query_standard( const dns_hdr_t* hdr );

static void dns_process_msg__dns_hdr_questions( const dns_hdr_t* hdr );
static void dns_process_msg__dns_question( const dns_question_t* question, dns_answer_t* answer );
static void dns_process_msg__dns_question_host_addr( const dns_question_t* question, dns_answer_t* answer );
static void dns_process_msg__dns_question_host_addr_internet( const dns_question_t* question, dns_answer_t* answer );

static void dns_process_msg__answer_host_addr_internet( dns_answer_t* answer, const char *domain_name, const char* ip_addr );


// static uint32_t dns_request_deserialize_hdr(dns_hdr_t* hdr, const char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_hdr(dns_hdr_t *hdr, char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_question(dns_question_t *question, char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_answer(dns_answer_t *answer, char *rply_buf, const uint32_t size);

static void *dns_receiver_routine(void *arg)
{
    udp_server_handle_t *srv_hndl = (udp_server_handle_t *)arg;
    client_msg_t received;
    int bytes;

    while (g_runFlag)
    {
        bytes = udp_server_handle_receive(srv_hndl, &received);

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
    udp_server_handle_t *srv_hndl = (udp_server_handle_t *)arg;
    client_msg_t to_transmit;

    while (g_runFlag)
    {
        dequeue_client_message(&to_transmit);
        ip_n_to_str(htonl(to_transmit.context.addr.sin_addr.s_addr), ip, 20);

        printf(">>> Transmitting [%d] bytes to %s:%u -> ", to_transmit.len, ip, htons(to_transmit.context.addr.sin_port));
        print_msg((uint8_t *)(&to_transmit.data), to_transmit.len);

        bytes = udp_server_handle_send(srv_hndl, &to_transmit);

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
    // Initialize dns message serializers
    net_serializer_init(&g_rxSerializer);
    net_serializer_init(&g_txSerializer);

    // initialize transmit buffer
    sync_cir_buffer_init(&g_transmit_buffer, false);

    // Intialize task params
    task_params_t receiver_task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_udp_server_handle),
        .fn = dns_receiver_routine};
    task_params_t transmitter_task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_udp_server_handle),
        .fn = dns_transmitter_routine};

    // initialize serve_handle params
    udp_server_handle_params_t udp_server_handle_param = {
        .client_allow_mask = (uint32_t)(-1),
        .port = DNS_PORT,
        .rcv_to = (uint32_t)1e6};

    udp_server_handle_init(&g_udp_server_handle, &udp_server_handle_param);

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
        udp_server_handle_get_info(&g_udp_server_handle, server_status, 200);
        printf("Server Status : %s\n", server_status);

        sleep(3);
    }
}

void dns_service_stop()
{
    g_runFlag = false;

    udp_server_handle_deinit(&g_udp_server_handle);
}

static void dns_handle_new_client_data(client_msg_t *msg)
{
    char ip[20];
    client_msg_t response;
    
    ip_n_to_str(ntohl(msg->context.addr.sin_addr.s_addr), ip, 20);
    printf(">>> Received [%d] bytes from %s:%u -> ", msg->len, ip, ntohs(msg->context.addr.sin_port));
    print_msg((uint8_t *)msg->data, msg->len);

    // Copy the message context information
    memcpy( &(response.context), &(msg->context), sizeof(msg->context) );
    
    // serialize the message
    net_serializer_append_raw( &g_rxSerializer, msg->data, msg->len );

    // process the message
    dns_process_msg();

    memset(response.data, 0, CLIENT_DATA_SIZE);
    response.len = dns_reply_serialize(response.data, CLIENT_DATA_SIZE);

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

static void dns_process_msg()
{
    dns_hdr_t received_hdr;

    if( net_serializer_remove_raw( &g_rxSerializer, 0, g_dns_section_buffer, DNS_SECTION_BUFFER_SIZE ) == 0 )
    {
        uint32_t bytes = dns_hdr_deserialize( &received_hdr, g_dns_section_buffer, DNS_SECTION_BUFFER_SIZE );
    
        if( bytes == DSN_HDR_SIZE )
        {
            dns_process_msg__dns_hdr( &received_hdr );
        }
    }
    else
    {
        // ! Error
    }

}

static void dns_process_msg__dns_hdr( const dns_hdr_t* hdr )
{
    // Check the response code
    switch( (DNS_HEADER_RCODE)hdr->rcode )
    {
        case DNS_HEADER_RCODE__NO_ERROR:
        {
            dns_process_msg__dns_hdr_valid( hdr );
        }
        break;

        case DNS_HEADER_RCODE__FORMAT_ERROR:
        {

        }
        break;

        case DNS_HEADER_RCODE__SERVER_FAILURE:
        {

        }
        break;

        case DNS_HEADER_RCODE__NAME_ERROR:
        {

        }
        break;

        case DNS_HEADER_RCODE__NOT_IMPLEMENTED:
        {

        }
        break;

        case DNS_HEADER_RCODE__REFUSED:
        {

        }
        break;

        default:
        {

        }
        break;
    }
    
}

static void dns_process_msg__dns_hdr_valid( const dns_hdr_t* hdr )
{
    switch ((DNS_HEADER_TYPE)hdr->qr)
    {
        case DNS_HEADER_TYPE__QUERY:
        {
            dns_process_msg__dns_hdr_valid_query( hdr );
        }
        break;
        
        case DNS_HEADER_TYPE__RESPONSE:
        {

        }
        break;
        
        default:
        {

        }
        break;
    }
}

static void dns_process_msg__dns_hdr_valid_query( const dns_hdr_t* hdr )
{
    switch ((DNS_HEADER_OPCODE)hdr->opcode)
    {
        case DNS_HEADER_OPCODE__STANDARD_QUERY:
        {
            dns_process_msg__dns_hdr_valid_query_standard(hdr);
        }
        break;

        case DNS_HEADER_OPCODE__INVERSE_QUERY:
        {

        }
        break;

        case DNS_HEADER_OPCODE__SERVER_STATUS_REQUEST:
        {

        }
        break;

        default:
        {

        }
        break;
    }
}

static void dns_process_msg__dns_hdr_valid_query_standard( const dns_hdr_t* hdr )
{
    // process questions
    if(hdr->qd_count)
    {
        dns_process_msg__dns_hdr_questions(hdr);
    }
}

static void dns_process_msg__dns_hdr_questions( const dns_hdr_t* hdr )
{
    uint32_t bytes;
    dns_answer_t answer;
    dns_question_t question;
    
    cir_buffer_t answers;
    cir_buffer_init( &answers, false );

    for( uint16_t qIdx = 0; qIdx < hdr->qd_count; qIdx++ )
    {
        // extract the current question from the serializer
        bytes = dns_question_deserialize( 
            &question,
            net_serializer_getbuf( &g_rxSerializer ),
            net_serializer_get_size( &g_rxSerializer )
        );

        if( bytes )
        {
            // remove question from receive buffer
            net_serializer_remove_raw( &g_rxSerializer, 0, NULL, bytes );
    
            // insert the question into the section buffer
            if( dns_question_serialize( &question, g_dns_section_buffer, DNS_SECTION_BUFFER_SIZE ) == bytes )
            {
                net_serializer_append_raw( &g_txSerializer, g_dns_section_buffer, bytes );
                
                // process the questioun
                dns_process_msg__dns_question(  &question, &answer );
                cir_buffer_write( &answers, (const char*)&answer, sizeof(dns_answer_t) );
            }
            else
            {
                // ! ERROR
            }
        }
        else
        {
            // ! ERROR
        }
    }

    // Copy serialize cir
    while( cir_buffer_is_empty( &answers) == false )
    {
        cir_buffer_read( &answers, (char*)&answer, sizeof(dns_answer_t) );

        // Serialize
        bytes = dns_answer_serialize( &answer, g_dns_section_buffer, DNS_SECTION_BUFFER_SIZE );

        // Copy into reply serializer
        net_serializer_append_raw( &g_txSerializer, g_dns_section_buffer, bytes );
    }

    // Cleanup the circular buffer
    cir_buffer_deinit( &answers );
}

static void dns_process_msg__dns_question( const dns_question_t* question, dns_answer_t* answer )
{
    switch((DNS_RR_TYPE)question->qtype)
    {
        case DNS_RR_TYPE__A:
        {
            dns_process_msg__dns_question_host_addr( question, answer );
        }
        break;
        case DNS_RR_TYPE__NS:
        case DNS_RR_TYPE__MD:
        case DNS_RR_TYPE__MF:
        case DNS_RR_TYPE__CNAME:
        case DNS_RR_TYPE__SOA:
        case DNS_RR_TYPE__MB:
        case DNS_RR_TYPE__MG:
        case DNS_RR_TYPE__MR:
        case DNS_RR_TYPE__NULL:
        case DNS_RR_TYPE__WKS:
        case DNS_RR_TYPE__PTR:
        case DNS_RR_TYPE__HINFO:
        case DNS_RR_TYPE__MINFO:
        case DNS_RR_TYPE__MX:
        case DNS_RR_TYPE__TXT:
        default:
        break;
    }
}

static void dns_process_msg__dns_question_host_addr( const dns_question_t* question, dns_answer_t* answer )
{
    switch((DNS_RR_CLASS)question->qclass)
    {
        case DNS_RR_CLASS__IN:
        {
            dns_process_msg__dns_question_host_addr_internet( question, answer );
        }
        break;

        default:
        break;
    }
}

static void dns_process_msg__dns_question_host_addr_internet( const dns_question_t* question, dns_answer_t* answer )
{
    uint32_t offset = 0;
    char domain_name[255];
    memset(domain_name, 0, sizeof(domain_name)/sizeof(domain_name[0]) );

    // extract the domain name
    for( uint8_t i = 0; i < DNS_MAX_LABEL_COUNT; i++ )   
    {
        if( question->qname[i].content[0] == '\0' )
        {
            // terminate string
            domain_name[--offset] = '\0';
            break;
        }

        memcpy( domain_name + offset, question->qname[0].content, question->qname[0].len );
        offset += question->qname[0].len;
        domain_name[offset++] = '.';
    }

    // lookup domain name domain name to address
    const char *ip_addr = dns_get_ip_from_domain(domain_name);

    dns_process_msg__answer_host_addr_internet( answer, domain_name, ip_addr );
}

static void dns_process_msg__answer_host_addr_internet( dns_answer_t* answer, const char *domain_name, const char* ip_addr )
{
    const size_t domain_name_len = strlen(domain_name);
    const size_t desc_len = sizeof(uint32_t);

    // convert ip to 32 bit int format
    uint32_t desc = htonl(ip_str_to_n("8.8.8.8"));

    dns_answer_init_from_domain_name_with_desc( answer, domain_name, domain_name_len, 60, (const char*)&desc, desc_len );
}

// static uint32_t dns_request_deserialize_hdr(dns_hdr_t* hdr, const char *rply_buf, const uint32_t size)
// {
//     memset( hdr, 0, DSN_HDR_SIZE );
//     return dns_hdr_deserialize( hdr, rply_buf, size );
// }


static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size)
{
    uint32_t offset = 0;

    // // Construct DNS header
    // offset += dns_reply_serialize_hdr(rply_buf + offset, size - offset);

    // // Construct DNS Question
    // offset += dns_reply_serialize_question(rply_buf + offset, size - offset);

    // // Construct DNS Answer
    // offset += dns_reply_serialize_answer(rply_buf + offset, size - offset);

    // Construct DNS Authority

    // Construct DNS Additional

    return offset;
}

static uint32_t dns_reply_serialize_hdr(dns_hdr_t *hdr, char *rply_buf, const uint32_t size)
{
    // dns_hdr_t hdr = {
    //     .id = 1234,
    //     .qr = 1,
    //     .opcode = 0,
    //     .aa = 0,
    //     .tc = 0,
    //     .rd = 0,
    //     .ra = 0,
    //     .rz = 0,
    //     .rcode = 0,
    //     .qd_count = 1,
    //     .an_count = 1,
    //     .ns_count = 0,
    //     .ar_count = 0
    // };

    // copy header info
    memset(rply_buf, 0, DSN_HDR_SIZE);

    printf("Serialized Header: %s\n", dns_hdr_str(hdr));

    return dns_hdr_serialize(hdr, rply_buf, size);
}

static uint32_t dns_reply_serialize_question(dns_question_t *question, char *rply_buf, const uint32_t size)
{
    static const char *domain_name = "codecrafters.io";
    static const size_t domain_name_len = strlen("codecrafters.io");

    // dns_question_t question;
    // dns_question_init_as_domain_name( &question, domain_name, domain_name_len );

    printf("Serialized Question: %s\n", dns_question_str(question));

    return dns_question_serialize(question, rply_buf, size);
}

static uint32_t dns_reply_serialize_answer(dns_answer_t *answer, char *rply_buf, const uint32_t size)
{
    // dns_answer_t answer;
    const char *domain_name = "codecrafters.io";
    const size_t domain_name_len = strlen(domain_name);
    uint32_t desc = htonl(ip_str_to_n("8.8.8.8"));
    const size_t desc_len = sizeof(uint32_t);

    // dns_answer_init_from_domain_name_with_desc( &answer, domain_name, domain_name_len, 60, (const char*)&desc, desc_len );

    printf("Serialized Answer: %s\n", dns_answer_str(answer));

    return dns_answer_serialize(answer, rply_buf, size);
}