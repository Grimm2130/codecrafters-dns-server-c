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
#include "task.h"
#include "cir_buffer.h"
#include "sync_cir_buffer.h"
#include "network_serializer.h"
#include "network_context.h"
#include "serializer.h"
#include <assert.h>

#define DNS_SECTION_BUFFER_SIZE   255U
#define DNS_RX_BUFFER_SIZE        1024U

bool g_runFlag;
bool g_remote_resolver_avail;
task_t g_receiver;
task_t g_transmitter;
task_t g_resolver;
udp_server_handle_t g_server_handle;
udp_server_handle_t g_remote_resolver_handle;
sync_cir_buffer_t g_transmit_buffer;
sync_cir_buffer_t g_resolver_transmit_buffer;
net_serializer_t g_rxSerializer;


char g_rxBuffer[DNS_RX_BUFFER_SIZE];

dns_hdr_t g_hdr;
cir_buffer_t g_questions;
cir_buffer_t g_answers;

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
static void enqueue_client_msg_for_tx(const client_msg_t *msg);
static void enqueue_client_msg_for_resolver(const client_msg_t *msg);
static void dequeue_client_msg_for_rx(client_msg_t *msg);
static void dequeue_client_msg_for_resolver(client_msg_t *msg);

static void dns_process_msg();
static void dns_process_msg__dns_hdr( dns_hdr_t* hdr );
static void dns_process_msg__dns_hdr_valid( dns_hdr_t* hdr );
static void dns_process_msg__dns_hdr_valid_query( dns_hdr_t* hdr );
static void dns_process_msg__dns_hdr_valid_query_standard( dns_hdr_t* hdr );

static void dns_process_msg__dns_hdr_questions( dns_hdr_t* hdr );
static void dns_process_msg__dns_question( const dns_question_t* question, dns_answer_t* answer );
static void dns_process_msg__dns_question_host_addr( const dns_question_t* question, dns_answer_t* answer );
static void dns_process_msg__dns_question_host_addr_internet( const dns_question_t* question, dns_answer_t* answer );

static void dns_process_msg__answer_host_addr_internet( dns_answer_t* answer, const char *domain_name, const char* ip_addr );


// static uint32_t dns_request_deserialize_hdr(dns_hdr_t* hdr, const char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_hdr(dns_hdr_t *hdr, char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_question(dns_question_t *question, char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_answer(dns_answer_t *answer, char *rply_buf, const uint32_t size);

static void init_remote_resolver_handle( const char * resolver_addr);

static void init_remote_resolver_handle( const char * resolver_addr)
{
    g_remote_resolver_avail = false;

    if( resolver_addr )
    {
        char *ip;
        uint16_t port;
        char *token;
        char temp[20];
        strncpy( temp, resolver_addr, 20 );

        // Get IP
        token = strtok( temp, ":" );
        if( token )
        {
            ip = token;
            
            // Get Port number
            token = strtok( NULL, ":" );
            if( token )
            {
                port = atoi(token);
                
                
                // mark the remote resolver as being available
                g_remote_resolver_avail = true;
                
                udp_server_handle_params_t resolver_udp_handle_params = 
                {
                    .client_allow_mask = -1,
                    .port = port,
                    .rcv_to = 1e6
                };
                
                strncpy( resolver_udp_handle_params.ip, ip, sizeof(resolver_udp_handle_params.ip)/sizeof(resolver_udp_handle_params.ip[0]));
                
                printf("Remote DNS Resolver @<%s>:<%u>\n", ip, port );

                // initialize the IP address
                udp_server_handle_init( &g_remote_resolver_handle, &resolver_udp_handle_params );
            }
        }        
    }
}

static void dns_receiver_routine(void *arg)
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
}

static void dns_transmitter_routine(void *arg)
{
    int bytes;
    char ip[20];
    udp_server_handle_t *srv_hndl = (udp_server_handle_t *)arg;
    client_msg_t to_transmit;

    while (g_runFlag)
    {
        dequeue_client_msg_for_rx(&to_transmit);
        ip_n_to_str(htonl(to_transmit.context.addr.sin_addr.s_addr), ip, 20);

        printf(">>> Transmitting [%d] bytes to %s:%u {%s}\n", 
            to_transmit.len, 
            ip, 
            htons(to_transmit.context.addr.sin_port),
            client_msg_str(&to_transmit)
        );

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
}

static void dns_resolver_routine(void *arg)
{
    int bytes;
    char ip[20];
    char status_message[255];
    udp_server_handle_t *resolver_handle = (udp_server_handle_t *)arg;
    client_msg_t data;

    while (g_runFlag)
    {
        udp_server_handle_get_info(resolver_handle, status_message, 255);

        printf("Resolver handle: %s\n", status_message);

        // Check the queue for new data requested to be resolved
        dequeue_client_msg_for_resolver(&data);
        
        // transmit to the resolver

        ip_n_to_str(htonl(data.context.addr.sin_addr.s_addr), ip, 20);

        printf(">>> [Resolver] Transmitting [%d] bytes to %s:%u {%s}\n", 
            data.len, 
            ip, 
            htons(data.context.addr.sin_port),
            client_msg_str(&data)
        );

        bytes = udp_server_handle_send(resolver_handle, &data);
        printf("[Resolver] Transmitted\n");
        fflush(stdout);

        if (bytes > 0)
        {
            resolver_handle->rt_info.written_bytes += bytes;
            
            // wait for data to return from the resolver
            bytes = udp_server_handle_receive( resolver_handle, &data );
    
            printf(">>> [Resolver] Received [%d] bytes from %s:%u {%s}\n", 
                data.len, 
                ip, 
                htons(data.context.addr.sin_port),
                client_msg_str(&data)
            );
    
            if (bytes > 0)
            {
                resolver_handle->rt_info.read_bytes += bytes;

                // Append the result to the transmit buffer
                enqueue_client_msg_for_tx( &data );
                
            }
            else if( bytes == 0 )
            {
                resolver_handle->rt_info.read_timeouts++;
            }
            else
            {
                resolver_handle->rt_info.read_timeouts++;
                //
            }
    

        }
        else
        {
            resolver_handle->rt_info.invalid_transmits++;
            //
        }


    }
}



void dns_service_start( const char * forwarding_addr )
{

    if( forwarding_addr )
    {
        init_remote_resolver_handle( forwarding_addr );
    }

    // Initialize dns message serializers
    net_serializer_init(&g_rxSerializer);

    // initialize the questions and answers buffers
    cir_buffer_init( &g_questions, false );
    cir_buffer_init( &g_answers, false );

    // initialize transmit buffer
    sync_cir_buffer_init(&g_transmit_buffer, false);
    sync_cir_buffer_init(&g_resolver_transmit_buffer, false);

    // Intialize task params
    task_params_t receiver_task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_server_handle),
        .fn = dns_receiver_routine
    };

    task_params_t transmitter_task_param = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_server_handle),
        .fn = dns_transmitter_routine
    };

    task_params_t resolver_task_params = {
        .can_join = true,
        .prio = 10,
        .stack_size = 1e6,
        .arg = (void *)(&g_remote_resolver_handle),
        .fn = dns_resolver_routine
    };

    // initialize serve_handle params
    udp_server_handle_params_t udp_server_handle_param = {
        .client_allow_mask = (uint32_t)(-1),
        .port = DNS_PORT,
        .rcv_to = (uint32_t)1e6
    };

    udp_server_handle_init(&g_server_handle, &udp_server_handle_param);

    // initialize rx & tx tasks
    task_init(&g_receiver, &receiver_task_param);
    task_init(&g_transmitter, &transmitter_task_param);
    if( g_remote_resolver_avail )
    {
        task_init(&g_resolver, &resolver_task_params);
    }
    
    g_runFlag = true;
    
    // Start transmitter
    task_start(&g_receiver);
    task_start(&g_transmitter);

    if( g_remote_resolver_avail )
    {
        task_start(&g_resolver);
    }

    static char server_status[200];

    while (1)
    {
        udp_server_handle_get_info(&g_server_handle, server_status, 200);
        printf("Server Status : %s\n", server_status);

        sleep(3);
    }

}

void dns_service_stop()
{
    g_runFlag = false;

    udp_server_handle_deinit(&g_server_handle);
}

static void dns_handle_new_client_data(client_msg_t *msg)
{

    if( g_remote_resolver_avail )
    {
        // Append to the resolver's buffer
        enqueue_client_msg_for_resolver( msg );
    }
    else
    {
        uint16_t client_port;
        char client_ip_addr[20];
        client_msg_t response;
        
        ip_n_to_str(ntohl(msg->context.addr.sin_addr.s_addr), client_ip_addr, 20);
        client_port = ntohs(msg->context.addr.sin_port);
    
        printf(">>> Received [%d] bytes from %s:%u {%s}\n", 
            msg->len, 
            client_ip_addr, 
            client_port,
            client_msg_str(msg)
        );
    
        // Copy the message context information
        memcpy( &(response.context), &(msg->context), sizeof(msg->context) );
        
        cir_buffer_clear( &g_questions ); 
        cir_buffer_clear( &g_answers ); 
    
        // Clear the receiver serializer
        net_serializer_clear(&g_rxSerializer);
    
        // Copy the message into the serializer
        net_serializer_append_raw( &g_rxSerializer, msg->data, msg->len );
    
        // process the message
        dns_process_msg();
    
        memset(response.data, 0, CLIENT_DATA_SIZE);
        response.len = dns_reply_serialize(response.data, CLIENT_DATA_SIZE);
    
        enqueue_client_msg_for_tx(&response);
    }

}

static void print_msg(const uint8_t *data, const int len)
{
    for (int i = 0; i < len; i++)
    {
        printf("0x%02x ", data[i]);
    }
    printf("\n");
}

static void enqueue_client_msg_for_tx(const client_msg_t *msg)
{
    sync_cir_buffer_write(&g_transmit_buffer, (const char *)msg, CLIENT_MSG_SIZE, false);
}

static void enqueue_client_msg_for_resolver(const client_msg_t *msg)
{
    sync_cir_buffer_write(&g_resolver_transmit_buffer, (const char *)msg, CLIENT_MSG_SIZE, false);
}

static void dequeue_client_msg_for_rx(client_msg_t *msg)
{
    sync_cir_buffer_read(&g_transmit_buffer, (char *)msg, CLIENT_MSG_SIZE, false);
}

static void dequeue_client_msg_for_resolver(client_msg_t *msg)
{
    sync_cir_buffer_read(&g_resolver_transmit_buffer, (char *)msg, CLIENT_MSG_SIZE, false);
}

/// @brief Process the DNS message that's been received
static void dns_process_msg()
{
    uint32_t bytes;

    bytes = dns_hdr_deserialize( 
        &g_hdr,
        net_serializer_getbuf( &g_rxSerializer),
        net_serializer_get_size( &g_rxSerializer)
    );
    
    // Copy validate the header data deserialized
    if( bytes == DSN_HDR_SIZE )
    {
        printf("[Request] Header %s\n", dns_hdr_str( &g_hdr ) );

        // remove raw data from the serializer
        net_serializer_remove_raw( &g_rxSerializer, 0, NULL, DSN_HDR_SIZE );
        
        // process the header
        dns_process_msg__dns_hdr( &g_hdr );
    }
    else
    {
        // ! Error
    }
}

/// @brief Process the DNS header received
/// @param hdr 
static void dns_process_msg__dns_hdr( dns_hdr_t* hdr )
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

/// @brief Handle a valid DNS header object
/// @param hdr 
static void dns_process_msg__dns_hdr_valid( dns_hdr_t* hdr )
{
    switch ((DNS_HEADER_TYPE)hdr->qr)
    {
        case DNS_HEADER_TYPE__QUERY:
        {
            dns_process_msg__dns_hdr_valid_query( hdr );
        }
        break;
        
        case DNS_HEADER_TYPE__RESPONSE:
        default:
        {
            printf("[ERROR] Unhandled header type\n");
            g_hdr.rcode = DNS_HEADER_RCODE__NOT_IMPLEMENTED;
        }
        break;
    }
}

/// @brief Handle a valid DNS query
/// @param hdr 
static void dns_process_msg__dns_hdr_valid_query( dns_hdr_t* hdr )
{
    switch ((DNS_HEADER_OPCODE)hdr->opcode)
    {
        case DNS_HEADER_OPCODE__STANDARD_QUERY:
        {
            dns_process_msg__dns_hdr_valid_query_standard(hdr);
        }
        break;

        case DNS_HEADER_OPCODE__INVERSE_QUERY:
        case DNS_HEADER_OPCODE__SERVER_STATUS_REQUEST:
        default:
        {
            printf("[ERROR] Unhandled header opcode\n");
            g_hdr.rcode = DNS_HEADER_RCODE__NOT_IMPLEMENTED;
        }
        break;
    }
}

/// @brief Handle a Standard DNS query
/// @param hdr 
static void dns_process_msg__dns_hdr_valid_query_standard( dns_hdr_t* hdr )
{
    // process questions
    if(hdr->qd_count)
    {
        dns_process_msg__dns_hdr_questions(hdr);
    }
    else
    {
        // ? What to do
    }
}

/// @brief Process DNS header questions
/// @param hdr 
static void dns_process_msg__dns_hdr_questions( dns_hdr_t* hdr )
{
    uint16_t c_pointer;
    uint32_t bytes;
    dns_answer_t answer;
    dns_question_t question;
    uint32_t total_bytes = 0;
    char rxStreamCache[DNS_RX_BUFFER_SIZE];

    // Copy into cache
    memcpy( rxStreamCache, 
        net_serializer_getbuf(&g_rxSerializer),
        _MIN( net_serializer_get_size(&g_rxSerializer), DNS_RX_BUFFER_SIZE )
    );

    // Process all the questions
    for( uint16_t qIdx = 0; qIdx < hdr->qd_count; qIdx++ )
    {
        c_pointer = 0;

        // extract the current question from the serializer
        bytes = dns_question_deserialize( 
            &question,
            net_serializer_getbuf(&g_rxSerializer),
            net_serializer_get_size(&g_rxSerializer),
            &c_pointer
        );

        // question has compressed label
        if( c_pointer )
        {
            bool done = false;
            dns_label_t *compressed_label;
            dns_label_t *referenced_label;
            dns_question_t temp_question;
            cir_buffer_t temp_questions;

            // instantiate offset to account for header
            uint32_t offset = DSN_HDR_SIZE;
            
            // Copy the processed questions
            cir_buffer_copy( &temp_questions, &g_questions, sizeof(dns_question_t) );

            while( (done == false) && cir_buffer_is_empty( &temp_questions ) == false )
            {
                uint16_t l_idx = 0;
                
                printf("[Pointer resolution] Curr off - %u, Expected - %u\n", offset, c_pointer );

                cir_buffer_read( &temp_questions, (char*)&temp_question, sizeof(dns_question_t));
                
                while( (l_idx < temp_question.label_cnt) && offset < c_pointer )
                {
                    printf("[Pointer resolution] Curr off - %u, Expected - %u\n", offset, c_pointer );
                    // increment the offset
                    offset += sizeof(temp_question.qname[l_idx].len);       // label len
                    offset += temp_question.qname[l_idx].len;               // label content
                    l_idx++;
                }
                
                // index found in question
                if( offset == c_pointer )
                {
                    while(l_idx < temp_question.label_cnt)
                    {
                        referenced_label = &temp_question.qname[l_idx];

                        // Copy the referenced label into the compressed label
                        compressed_label = &question.qname[question.label_cnt++];
                        memcpy( compressed_label, referenced_label, sizeof(dns_label_t) );

                        l_idx++;
                    }

                    done = true;
                    break;
                }
                // offset not in question
                else
                {
                    offset += sizeof(uint8_t);      // null
                    offset += sizeof(uint16_t);     // type
                    offset += sizeof(uint16_t);     // class
                }
            }

            if( done == false )
            {
                // ! ERROR
                exit(-1);
            }

            cir_buffer_deinit( &temp_questions );
        }

        if( bytes )
        {
            printf("[Request] [Question] ([%u] bytes) %s\n", bytes, dns_question_str(&question) );

            cir_buffer_write( &g_questions, (const char*)&question, sizeof(dns_question_t)) ;

            // remove question from receive buffer
            if(net_serializer_remove_raw( &g_rxSerializer, 0, NULL, bytes ) == 0 )
            {
                // process the question and generate answer
                dns_process_msg__dns_question( &question, &answer );

                // Append the answer to the question response block
                cir_buffer_write( &g_answers, (const char*)&answer, sizeof(dns_answer_t) );
            }
            else
            {
                // ! ERROR
            }
        }
        else
        {
            // A compressed question has been returned
            printf("Cannot handle compressed label\n");
            exit(-1);

        }

        total_bytes += bytes;
    }
}


static void dns_process_msg__dns_question( const dns_question_t* question, dns_answer_t* answer )
{
    printf("Processing Quesstion\n");
    
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
        printf("[INFO] unknown QUESTION type received\n");
        break;
    }
    
    printf("Processing Quesstion...\n");

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

        case DNS_RR_CLASS__CS:
        case DNS_RR_CLASS__CH:
        case DNS_RR_CLASS__HS:
        default:
        break;
    }
}

static void dns_process_msg__dns_question_host_addr_internet( const dns_question_t* question, dns_answer_t* answer )
{
    uint32_t offset = 0;
    char domain_name[255];
    const size_t domain_name_size = sizeof(domain_name)/sizeof(domain_name[0]);
    memset(domain_name, 0, domain_name_size );

    offset += dns_question_get_qname_as_domain_name( question, domain_name, domain_name_size );

    // lookup IP address from domain name
    const char *ip_addr = dns_get_ip_from_domain(domain_name);

    dns_process_msg__answer_host_addr_internet( answer, domain_name, ip_addr );
}

static void dns_process_msg__answer_host_addr_internet( dns_answer_t* answer, const char *domain_name, const char* ip_addr )
{
    const size_t domain_name_len = strlen(domain_name);
    uint32_t desc = htonl(ip_str_to_n(ip_addr));
    const size_t desc_len = sizeof(uint32_t);

    dns_answer_init_from_domain_name_with_desc( answer, domain_name, domain_name_len, 60, (const char*)&desc, desc_len );
}

// static uint32_t dns_request_deserialize_hdr(dns_hdr_t* hdr, const char *rply_buf, const uint32_t size)
// {
//     memset( hdr, 0, DSN_HDR_SIZE );
//     return dns_hdr_deserialize( hdr, rply_buf, size );
// }


static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size)
{
    uint32_t bytes = DSN_HDR_SIZE;        // offset for header
    dns_question_t question;
    dns_answer_t answer;

    printf("Serializing Response...\n");

    memset(g_dns_section_buffer, 0, DNS_SECTION_BUFFER_SIZE);

    g_hdr.qd_count = 0;
    
    // serialize the questions
    while( cir_buffer_is_empty( &g_questions) == false )
    {
        assert( cir_buffer_read( &g_questions, (char*)&question, sizeof(dns_question_t) ) == sizeof(dns_question_t) );
        bytes += dns_question_serialize( &question, g_dns_section_buffer + bytes, DNS_SECTION_BUFFER_SIZE - bytes );
        printf("[Response] Question %s\n", dns_question_str(&question));
        g_hdr.qd_count++;
    }
    
    g_hdr.an_count = 0;
    // Serialize the answers
    while( cir_buffer_is_empty( &g_answers) == false )
    {
        assert( cir_buffer_read( &g_answers, (char*)&answer, sizeof(dns_answer_t) ) == sizeof(dns_answer_t) );
        bytes += dns_answer_serialize( &answer, g_dns_section_buffer + bytes, DNS_SECTION_BUFFER_SIZE - bytes );
        printf("[Response] Answer %s\n", dns_answer_str(&answer));
        g_hdr.an_count++;
    }

    g_hdr.qr = 1;       // mark as response

    // insert the header at the front
    dns_hdr_serialize( &g_hdr, g_dns_section_buffer, DSN_HDR_SIZE );

    printf("[Response] Header %s\n", dns_hdr_str( &g_hdr ));

    memcpy(rply_buf, g_dns_section_buffer, bytes );

    return bytes;
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