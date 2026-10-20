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
#include "logger.h"
#include <assert.h>

#define DNS_SECTION_BUFFER_SIZE 255U
#define DNS_RX_BUFFER_SIZE 1024U

bool g_runFlag;
bool g_remote_resolver_avail;
task_t g_receiver;
task_t g_transmitter;
task_t g_resolver;
udp_server_handle_t g_server_handle;
network_context_t g_remote_resolver_context;
sync_cir_buffer_t g_transmit_buffer;
sync_cir_buffer_t g_resolver_transmit_buffer;
net_serializer_t g_rxSerializer;
logger_t g_main_thread_logger;
logger_params_t g_main_thread_logger_params =
    {
        .level = LOG_LEVEL_TRACE,
#ifdef RESOLVER
        .log_fname = "main-resolver",
#else
        .log_fname = "main",
#endif
        .log_fname_ext = "log",
        .log_path = "logs",
        .max_file_size = 1e6,
        .max_log_buf_enttry_cnt = 100,
        .prior = 10,
        .timestamp_formatter = NULL};

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
    DNS_ERROR_HDR_QDCOUNT_LIMIT,
    DNS_ERROR_INVALID
} dns_service_ret_code_t;

static void print_msg(const uint8_t *data, const int len);
static void dns_handle_new_client_data(client_msg_t *msg);
static void enqueue_client_msg_for_tx(const client_msg_t *msg);
static void enqueue_client_msg_for_resolver(const client_msg_t *msg);
static void dequeue_client_msg_for_tx(client_msg_t *msg);
static void dequeue_client_msg_for_resolver(client_msg_t *msg);

static void dns_process_msg();
static void dns_process_msg__dns_hdr(dns_hdr_t *hdr);
static void dns_process_msg__dns_hdr_valid(dns_hdr_t *hdr);
static void dns_process_msg__dns_hdr_valid_response(dns_hdr_t *hdr);
static void dns_process_msg__dns_hdr_valid_query(dns_hdr_t *hdr);
static void dns_process_msg__dns_hdr_valid_query_standard(dns_hdr_t *hdr);

static void dns_process_msg__dns_hdr_questions(dns_hdr_t *hdr);
static void dns_process_msg__dns_question(const dns_question_t *question, dns_answer_t *answer);
static void dns_process_msg__dns_question_host_addr(const dns_question_t *question, dns_answer_t *answer);
static void dns_process_msg__dns_question_host_addr_internet(const dns_question_t *question, dns_answer_t *answer);

static void dns_process_msg__answer_host_addr_internet(dns_answer_t *answer, const char *domain_name, const char *ip_addr);

// static uint32_t dns_request_deserialize_hdr(dns_hdr_t* hdr, const char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_hdr(dns_hdr_t *hdr, char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_question(dns_question_t *question, char *rply_buf, const uint32_t size);
static uint32_t dns_reply_serialize_answer(dns_answer_t *answer, char *rply_buf, const uint32_t size);
static void dns_handle_compressed_query(dns_question_t *question, cir_buffer_t* prev_queries, const uint16_t offset );
static bool dns_remote_resolver_header_check( const dns_hdr_t* hdr );

static void init_remote_resolver_handle(const char *resolver_addr);

static void init_remote_resolver_handle(const char *resolver_addr)
{
    g_remote_resolver_avail = false;

    if (resolver_addr)
    {
        char *ip;
        uint16_t port;
        char *token;
        char temp[20];
        strncpy(temp, resolver_addr, 20);

        // Get IP
        token = strtok(temp, ":");
        if (token)
        {
            ip = token;

            // Get Port number
            token = strtok(NULL, ":");
            if (token)
            {
                port = atoi(token);

                // mark the remote resolver as being available
                g_remote_resolver_avail = true;

                #if defined(LOCAL_BUILD)
                log_debug(&g_main_thread_logger, "Remote DNS Resolver @<%s>:<%u>", ip, port);
                #endif  // LOCAL_BUILD
                
                // initialize the IP address
                network_context_init( &g_remote_resolver_context, ip, port );
                
                #if defined(LOCAL_BUILD)
                log_debug(&g_main_thread_logger, "Remote DNS Resolver Handle Sock (%d) @<%s>:<%u>", g_remote_resolver_context.sock, ip, port);
                #endif  // LOCAL_BUILD
            }
        }
    }
}

static void dns_receiver_routine(void *arg)
{
    int bytes;
    char ip[20];
    udp_server_handle_t *srv_hndl = (udp_server_handle_t *)arg;
    client_msg_t received;
    logger_t receiver_logger;
    logger_params_t receiver_logger_params =
        {
            .level = LOG_LEVEL_TRACE,
#ifdef RESOLVER
            .log_fname = "resolver-receiver",
#else
            .log_fname = "receiver",
#endif
            .log_fname_ext = "log",
            .log_path = "logs",
            .max_file_size = 1e6,
            .max_log_buf_enttry_cnt = 100,
            .prior = 10,
            .timestamp_formatter = NULL};

    #if defined(LOCAL_BUILD)
    logger_get_instance(&receiver_logger, &receiver_logger_params);
    #endif

    while (g_runFlag)
    {
        bytes = udp_server_handle_receive(srv_hndl, &received);

        // handle received data
        if (bytes > 0)
        {
            // Valid
            srv_hndl->rt_info.read_bytes += bytes;

            ip_n_to_str(htonl(received.context.addr.sin_addr.s_addr), ip, 20);

            #if defined(LOCAL_BUILD)
            log_debug(&receiver_logger, "Receiver [%d] bytes to %s:%u {%s}",
                received.len,
                ip,
                htons(received.context.addr.sin_port),
                client_msg_str(&received)
            );
            #endif //defined(LOCAL_BUILD)

            dns_handle_new_client_data(&received);
        }
        else if (bytes == 0)
        {
            srv_hndl->rt_info.read_timeouts++;

#if defined(LOCAL_BUILD)
            log_debug(&receiver_logger, "Receiver timed-out ",
                ip,
                htons(received.context.addr.sin_port)
            );
#endif //defined(LOCAL_BUILD)
        }
        else
        {
            srv_hndl->rt_info.invalid_receives++;

#if defined(LOCAL_BUILD)
            log_error(&receiver_logger, "Receiver received some invalid bytes from %s:%u. Error code - '%s'",
                ip,
                htons(received.context.addr.sin_port),
                strerror(errno)
            );
#endif //defined(LOCAL_BUILD)
        }
    }

    #if defined(LOCAL_BUILD)
    logger_release_instance(&receiver_logger);
    #endif
}

static void dns_transmitter_routine(void *arg)
{
    int bytes;
    char ip[20];
    udp_server_handle_t *srv_hndl = (udp_server_handle_t *)arg;
    client_msg_t to_transmit;
    logger_t transmitter_logger;
    logger_params_t transmitter_logger_params =
        {
            .level = LOG_LEVEL_TRACE,
#ifdef RESOLVER
            .log_fname = "resolver-transmitter",
#else
            .log_fname = "transmitter",
#endif
            .log_fname_ext = "log",
            .log_path = "logs",
            .max_file_size = 1e6,
            .max_log_buf_enttry_cnt = 100,
            .prior = 10,
            .timestamp_formatter = NULL};

    #if defined(LOCAL_BUILD)
    logger_get_instance(&transmitter_logger, &transmitter_logger_params);
    #endif

    while (g_runFlag)
    {
        dequeue_client_msg_for_tx(&to_transmit);
        ip_n_to_str(htonl(to_transmit.context.addr.sin_addr.s_addr), ip, 20);

        #if defined(LOCAL_BUILD)
        log_debug(&transmitter_logger, ">>> Transmitting [%d] bytes to %s:%u {%s}",
            to_transmit.len,
            ip,
            htons(to_transmit.context.addr.sin_port),
            client_msg_str(&to_transmit)
        );
        #endif //   defined(LOCAL_BUILD)

        bytes = udp_server_handle_send(srv_hndl, &to_transmit);

        fflush(stdout);

        if (bytes > 0)
        {
            // Valid
#if defined(LOCAL_BUILD)
            log_debug( &transmitter_logger, "Transmitter transmission to %s:%u succeeded", ip, htons(to_transmit.context.addr.sin_port) );
#endif //defined(LOCAL_BUILD)

            srv_hndl->rt_info.written_bytes += bytes;
        }
        else
        {
                #if defined(LOCAL_BUILD)
            log_error( &transmitter_logger, "Transmitter transmission to %s:%u failed", ip, htons(to_transmit.context.addr.sin_port) );
            #endif      
            srv_hndl->rt_info.invalid_transmits++;
        }
    }

    logger_release_instance(&transmitter_logger);
}

static void dns_resolver_routine(void *arg)
{
    bool success;
    int bytes;

    static const char * resolver_handle_ip = "0.0.0.0";
    static const uint16_t resolver_handle_port = 2055;

    uint16_t pointer;
    uint32_t serializer_off;
    
    char ip[20];
    
    #define TEMP_BUFF_SIZE 255U
    char answer_buf[TEMP_BUFF_SIZE];
    char handle_info[TEMP_BUFF_SIZE];
    
    dns_hdr_t header, inner_header;
    dns_question_t question;
    dns_answer_t answer;
    
    client_msg_t resolver_data;      // stores resolver info
    client_msg_t target_client_data; // stores client info
    
    net_serializer_t serializer;
    
    cir_buffer_t questions;
    cir_buffer_t answers;
    
    logger_t resolver_logger;
    
    udp_server_handle_t resolver_handle;
    network_context_t *resolver_context = (network_context_t *)arg;

    logger_params_t resolver_logger_params =
    {
        .level = LOG_LEVEL_TRACE,
        .log_fname_ext = "log",
#ifdef RESOLVER
        .log_fname = "resolver-resolver",
#else
        .log_fname = "resolver",
#endif
        .log_path = "logs",
        .max_file_size = 1e7,
        .max_log_buf_enttry_cnt = 100,
        .prior = 10,
        .timestamp_formatter = NULL
    };

#if defined(LOCAL_BUILD)
logger_get_instance(&resolver_logger, &resolver_logger_params);
#endif

    // intialize the resolver handle
    udp_server_handle_params_t resolver_udp_handle_params =
    {
        .client_allow_mask = -1,
        .port = resolver_handle_port,
        .rcv_to = 1e6,
        .ip = (char*)resolver_handle_ip
    };
    udp_server_handle_init(&resolver_handle, &resolver_udp_handle_params);

    net_serializer_init(&serializer);
    cir_buffer_init(&questions, false);
    cir_buffer_init(&answers, false);

    // Set the network info for the resolver
    memcpy(&resolver_data.context, resolver_context, sizeof(network_context_t));

    fflush(stdout);

    // Get the IP of the resolver
    ip_n_to_str(htonl(resolver_data.context.addr.sin_addr.s_addr), ip, 20);

    while (g_runFlag)
    {
        udp_server_handle_get_info(&resolver_handle, handle_info, TEMP_BUFF_SIZE);
        
        #if defined(LOCAL_BUILD)
        log_trace( &resolver_logger, "Resolver Handle: %s", handle_info );
        #endif // LOCAL_USER
        
        // reset the temp-buffers
        cir_buffer_clear( &questions );
        cir_buffer_clear( &answers );

        dequeue_client_msg_for_resolver( &target_client_data );

        #if defined(LOCAL_BUILD)
        log_trace( &resolver_logger, "Dequeued client data for transfer to remote resolver - {%u : %s}",
            target_client_data.len,
            client_msg_str(&target_client_data)
        );
        #endif // LOCAL_USER
        
        // Copy into the serializer
        if( net_serializer_append_raw( &serializer, target_client_data.data, target_client_data.len ) == 0 )
        {
            // extract header from client
            bytes = dns_hdr_deserialize( &header, net_serializer_get_buf(&serializer), net_serializer_get_size(&serializer));

            if( bytes == DSN_HDR_SIZE )
            {
                #if defined(LOCAL_BUILD)
                log_debug( &resolver_logger, "[Resolver Request] Header: {%s}", dns_hdr_str( &header ) );
                #else
                printf("[Resolver Request] Header: {%s}\n", dns_hdr_str( &header ) );
                #endif // LOCAL_USER

                // Copy the inner header to be used for intermediate transactions
                memcpy( &inner_header, &header, DSN_HDR_SIZE );
                inner_header.qd_count = 1;

                // remove the header from the serializer
                if( net_serializer_remove_raw( &serializer, 0, NULL, bytes ) )
                {
                    #if defined(LOCAL_BUILD)
                    log_fatal( &resolver_logger, "Serializer fault");
                    #endif // LOCAL_USER
                    perror( "Resolver Serializer remove" );
                    sleep(2);
                    exit(1);
                }

                if( dns_remote_resolver_header_check(&header) )
                {
                    // Process each question
                    for( uint8_t q_idx = 0; q_idx < header.qd_count; q_idx++ )
                    {
                        inner_header.qr = 0;
                        #if defined(LOCAL_BUILD)
                        log_trace( &resolver_logger, "Processing Question (%u)", q_idx );
                        #endif // LOCAL_USER
    
                        // reset the pointer index
                        pointer = 0;
    
                        // extract the question
                        bytes = dns_question_deserialize(
                            &question,
                            net_serializer_get_buf(&serializer),
                            net_serializer_get_size(&serializer),
                            &pointer
                        );
    
                        if( bytes )
                        {
                            // Check if a compressed query was sent
                            if( pointer )
                            {
                                printf("Compressed query encountered! for index %u\n", pointer);
                                cir_buffer_t temp_questions;
                                cir_buffer_copy( &temp_questions, &questions, sizeof(dns_question_t) );
    
                                dns_handle_compressed_query(&question, &temp_questions, pointer );
                                
                                cir_buffer_deinit(&temp_questions);
                            }
    
                            // remove the question from the serializer
                            if( net_serializer_remove_raw( &serializer, 0, NULL, bytes ) )
                            {
                                #if defined(LOCAL_BUILD)
                                log_fatal( &resolver_logger, "Serializer fault");
                                #endif // LOCAL_USER
    
                                exit(3);
                            }
    
                            #if defined(LOCAL_BUILD)
                            log_debug( &resolver_logger, "[Resolver Request] Question: {%s}", dns_question_str( &question ) );
                            #else
                            printf( "[Resolver Request] Question(%u): {%s}\n", q_idx, dns_question_str( &question ) );
                            #endif // LOCAL_USER
    
                            // Copy the header and question into the transmission buffer
                            resolver_data.len = 0;
                            resolver_data.len = dns_hdr_serialize( &inner_header, resolver_data.data + resolver_data.len, CLIENT_DATA_SIZE - resolver_data.len );
                            resolver_data.len += dns_question_serialize( &question, resolver_data.data + resolver_data.len, CLIENT_DATA_SIZE - resolver_data.len );
                            bytes = resolver_data.len;
    
                            #if defined(LOCAL_BUILD)
                            log_trace( &resolver_logger, "Transmitting data to resolver: (%u) {%s}", bytes, client_msg_str(&resolver_data) );
                            #else
                            printf("Transmitting data to resolver: (%u) {%s}", bytes, client_msg_str(&resolver_data) );
                            #endif // LOCAL_USER
                            
                            if( bytes == udp_server_handle_send( &resolver_handle, &resolver_data ) )
                            {
                                bytes = udp_server_handle_receive( &resolver_handle, &resolver_data );
                                if( bytes > 0 )
                                {
                                    net_serializer_t inner_serializer;
                                    
                                    resolver_handle.rt_info.read_bytes += bytes;
                                    
                                    net_serializer_init( &inner_serializer );
    
                                    #if defined(LOCAL_BUILD)
                                    log_trace( &resolver_logger, "Receiving data from resolver: (%u) {%s}", bytes, client_msg_str(&resolver_data) );
                                    #else
                                    printf("Receiving data from resolver: (%u) {%s}", bytes, client_msg_str(&resolver_data) );
                                    #endif // LOCAL_USER
                                    
                                    net_serializer_append_raw( &inner_serializer, resolver_data.data, resolver_data.len );
                                    
                                    pointer = 0;
                                    
                                    
                                    // extract the header
                                    bytes = dns_hdr_deserialize(&inner_header, net_serializer_get_buf(&inner_serializer), net_serializer_get_size(&inner_serializer) );
                                    net_serializer_remove_raw(&inner_serializer, 0, NULL, bytes );
                                    
                                    // extract the query
                                    bytes = dns_question_deserialize(&question, net_serializer_get_buf(&inner_serializer), net_serializer_get_size(&inner_serializer), &pointer );
                                    net_serializer_remove_raw(&inner_serializer, 0, NULL, bytes );
                                    
                                    // extract the answer
                                    bytes = dns_answer_deserialize(&answer, net_serializer_get_buf(&inner_serializer), net_serializer_get_size(&inner_serializer));
                                    net_serializer_remove_raw(&inner_serializer, 0, NULL, bytes );
                                
                                    #if defined(LOCAL_BUILD)
                                    log_debug( &resolver_logger, "[Resolver Response] Header: {%s}", dns_hdr_str( &inner_header ) );
                                    log_debug( &resolver_logger, "[Resolver Response] Question: {%s}", dns_question_str( &question ) );
                                    log_debug( &resolver_logger, "[Resolver Response] Answer: {%s}", dns_answer_str( &answer ) );
                                    #else
                                    printf("[Resolver Response (inter)] Header: {%s}\n", dns_hdr_str( &inner_header ) );
                                    printf("[Resolver Response (inter)] Question: {%s}\n", dns_question_str( &question ) );
                                    printf("[Resolver Response (inter)] Answer: {%s}\n", dns_answer_str( &answer ) );
                                    #endif // LOCAL_USER
    
                                    cir_buffer_write(&questions, (const char*)&question, sizeof(dns_question_t) );
                                    cir_buffer_write(&answers, (const char*)&answer, sizeof(dns_answer_t) );
    
                                    net_serializer_deinit( &inner_serializer );
                                }
                                else if( bytes == 0 )
                                {
                                    resolver_handle.rt_info.read_timeouts++;
                                    printf( "Reception Fault %s\n", strerror(errno) );
                                }
                                else
                                {
                                    resolver_handle.rt_info.invalid_receives++;
                                    printf( "Reception Fault %s\n", strerror(errno) );
                                }
                            }
                            else
                            {
                                #if defined(LOCAL_BUILD)
                                log_fatal( &resolver_logger, "Transmission Fault %s", strerror(errno) );
                                #else
                                printf( "Transmission Fault %s\n", strerror(errno) );
                                #endif // LOCAL_USER
                                
                                resolver_handle.rt_info.invalid_transmits++;
                            }
                        }
                    }
                }
                else
                {
                    header.rcode = DNS_HEADER_RCODE__NOT_IMPLEMENTED;
                }

                #if defined(LOCAL_BUILD)
                log_debug( &resolver_logger, "Serializing response to client....");
                #endif // LOCAL_USER
                
                net_serializer_clear(&serializer);

                // Serialize head
                #if defined(LOCAL_BUILD)
                log_debug( &resolver_logger, "Serializing response to client....Header");
                #endif // LOCAL_USER
                
                // Set header as response
                header.qr = 1;
                header.qd_count = questions.size;
                header.an_count = answers.size;
                
                #if defined(LOCAL_BUILD)
                log_debug( &resolver_logger, "[Resolver Response] Header : %s", dns_hdr_str(&header));
                #else
                printf("[Resolver Response] Header : %s\n", dns_hdr_str(&header));
                #endif // LOCAL_USER

                bytes = dns_hdr_serialize( &header, answer_buf, TEMP_BUFF_SIZE );
                net_serializer_append_raw( &serializer, answer_buf, bytes );
                
                // deserialize questions
                #if defined(LOCAL_BUILD)
                log_debug( &resolver_logger, "Serializing response to client....Questions");
                #endif // LOCAL_USER


                while( cir_buffer_is_empty(&questions) == false )
                {
                    cir_buffer_read(&questions, (char*)&question, sizeof(dns_question_t));
                    
                    #if defined(LOCAL_BUILD)
                    log_debug( &resolver_logger, "[Resolver Response] Question [%u] : %s", questions.front, dns_question_str(&question));
                    #else
                    printf("[Resolver Response] Question [%u] : %s\n", questions.front, dns_question_str(&question));
                    #endif
                    
                    bytes = dns_question_serialize( &question, answer_buf, TEMP_BUFF_SIZE );
                    net_serializer_append_raw( &serializer, answer_buf, bytes );
                }
                
                // deserialize answers
                #if defined(LOCAL_BUILD)
                log_debug( &resolver_logger, "Serializing response to client....Anwers");
                #endif // LOCAL_USER
                
                while( cir_buffer_is_empty(&answers) == false )
                {
                    
                    #if defined(LOCAL_BUILD)
                    log_debug( &resolver_logger, "[Resolver Response] Answer [%u] : %s", answers.front, dns_answer_str(&answer));
                    #else
                    printf("[Resolver Response] Answer [%u] : %s\n", answers.front, dns_answer_str(&answer));
                    #endif

                    cir_buffer_read(&answers, (char*)&answer, sizeof(dns_answer_t));
                    bytes = dns_answer_serialize( &answer, answer_buf, TEMP_BUFF_SIZE );
                    net_serializer_append_raw( &serializer, answer_buf, bytes );
                }
            }
            else
            {
                #if defined(LOCAL_BUILD)
                log_error( &resolver_logger, "DNS header deserialization size error. Actual %u, Expected %u",
                    bytes,
                    DSN_HDR_SIZE
                );
                #endif // LOCAL_USER
            }
        }
        else
        {
            #if defined(LOCAL_BUILD)
            log_fatal( &resolver_logger, "Serializer Append fault - %s", strerror(errno) );
            #endif // LOCAL_USER
        }
        
        memcpy(target_client_data.data, net_serializer_get_buf(&serializer), net_serializer_get_size(&serializer));
        target_client_data.len = net_serializer_get_size(&serializer);
        
        #if defined(LOCAL_BUILD)
        log_trace( &resolver_logger, "Enqueueing client data post response from remote resolver - {%u : %s}",
            target_client_data.len,
            client_msg_str(&target_client_data)
        );
        #endif // LOCAL_USER
                
        enqueue_client_msg_for_tx( &target_client_data );
        net_serializer_clear( &serializer );
    }

    net_serializer_deinit( &serializer );
    cir_buffer_deinit( &questions );
    cir_buffer_deinit( &answers );
    udp_server_handle_deinit(&resolver_handle);

    #if defined(LOCAL_BUILD)
    logger_release_instance(&resolver_logger);
    #endif
}

void dns_server_main(int argc, char *argv[])
{
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // TODO: Uncomment the code below to pass the first stage
    // RunServer();
    char *resolver_addr = NULL;

    if (argc == 3 && (strcmp(argv[1], "--resolver") == 0))
    {
        resolver_addr = argv[2];
    }

    // initialize the logger
    #if defined(LOCAL_BUILD)
    logger_init();
    #endif

    // initialize the main logger thread instance
    #if defined(LOCAL_BUILD)
    logger_get_instance(&g_main_thread_logger, &g_main_thread_logger_params);
    #endif

    #if defined(LOCAL_BUILD)
    log_trace(&g_main_thread_logger, "Starting DNS Server....");
    #endif

    if( resolver_addr ) 
    {
        #if defined(LOCAL_BUILD)
        log_trace(&g_main_thread_logger, "Resolver address %s", resolver_addr );
        #endif
    }

    dns_service_start(resolver_addr);

    dns_service_stop();

    #if defined(LOCAL_BUILD)
    log_trace(&g_main_thread_logger, "Stopping DNS Server....");
    #endif

    #if defined(LOCAL_BUILD)
    logger_release_instance(&g_main_thread_logger);
    #endif

    // deinitialize the logger
    #if defined(LOCAL_BUILD)
    logger_deinit();
    #endif
}

void dns_service_start(const char *forwarding_addr)
{

    if (forwarding_addr)
    {
        init_remote_resolver_handle(forwarding_addr);
    }

    // Initialize dns message serializers
    net_serializer_init(&g_rxSerializer);

    // initialize the questions and answers buffers
    cir_buffer_init(&g_questions, false);
    cir_buffer_init(&g_answers, false);

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
        .arg = (void *)(&g_remote_resolver_context),
        .fn = dns_resolver_routine
    };

    // initialize serve_handle params
    udp_server_handle_params_t udp_server_handle_param = {
        .client_allow_mask = (uint32_t)(-1),
        .port = DNS_PORT,
        .rcv_to = (uint32_t)1e6,
        .ip = DNS_IP};

    udp_server_handle_init(&g_server_handle, &udp_server_handle_param);

    #if defined(LOCAL_BUILD)
    log_debug(&g_main_thread_logger, "Server Handle Sock (%d) @<%s>:<%u>", g_server_handle.context->sock, DNS_IP, DNS_PORT );
    #endif

    // initialize rx & tx tasks
    task_init(&g_receiver, &receiver_task_param);
    task_init(&g_transmitter, &transmitter_task_param);
    if (g_remote_resolver_avail)
    {
        task_init(&g_resolver, &resolver_task_params);
    }

    g_runFlag = true;

    // Start transmitter
    task_start(&g_receiver);
    task_start(&g_transmitter);

    if (g_remote_resolver_avail)
    {
        task_start(&g_resolver);
    }

    static char server_status[200];

    while (1)
    {
        udp_server_handle_get_info(&g_server_handle, server_status, 200);
        #if defined(LOCAL_BUILD)
        log_info(&g_main_thread_logger, "Server Status : %s", server_status);
        #endif
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
    #if defined(LOCAL_BUILD)
    log_trace(&g_main_thread_logger, "New packet received!");
    #endif

    if (g_remote_resolver_avail)
    {
        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "Forwarding packet to remote resolver");
        #endif

        // Append to the resolver's buffer
        enqueue_client_msg_for_resolver(msg);
    }
    else
    {
        uint16_t client_port;
        char client_ip_addr[20];
        client_msg_t response;

        ip_n_to_str(ntohl(msg->context.addr.sin_addr.s_addr), client_ip_addr, 20);
        client_port = ntohs(msg->context.addr.sin_port);

        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, ">>> Received [%d] bytes from %s:%u {%s}",
            msg->len,
            client_ip_addr,
            client_port,
            client_msg_str(msg));
        #endif
            
        // Copy the message context information
        memcpy(&(response.context), &(msg->context), sizeof(msg->context));

        cir_buffer_clear(&g_questions);
        cir_buffer_clear(&g_answers);

        // Clear the receiver serializer
        net_serializer_clear(&g_rxSerializer);

        // Copy the message into the serializer
        net_serializer_append_raw(&g_rxSerializer, msg->data, msg->len);

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
        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "0x%02x ", data[i]);
        #endif
    }
    #if defined(LOCAL_BUILD)
    log_debug(&g_main_thread_logger, "\n");
    #endif
}

static void enqueue_client_msg_for_tx(const client_msg_t *msg)
{
    sync_cir_buffer_write(&g_transmit_buffer, (const char *)msg, CLIENT_MSG_SIZE, false);
}

static void enqueue_client_msg_for_resolver(const client_msg_t *msg)
{
    sync_cir_buffer_write(&g_resolver_transmit_buffer, (const char *)msg, CLIENT_MSG_SIZE, false);
}

static void dequeue_client_msg_for_tx(client_msg_t *msg)
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
        net_serializer_get_buf(&g_rxSerializer),
        net_serializer_get_size(&g_rxSerializer));

    // Copy validate the header data deserialized
    if (bytes == DSN_HDR_SIZE)
    {
        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "[Request] Header %s", dns_hdr_str(&g_hdr));
        #endif

        // remove raw data from the serializer
        net_serializer_remove_raw(&g_rxSerializer, 0, NULL, DSN_HDR_SIZE);

        // process the header
        dns_process_msg__dns_hdr(&g_hdr);
    }
    else
    {
        // ! Error
    }
}

/// @brief Process the DNS header received
/// @param hdr
static void dns_process_msg__dns_hdr(dns_hdr_t *hdr)
{
    // Check the response code
    switch ((DNS_HEADER_RCODE)hdr->rcode)
    {
    case DNS_HEADER_RCODE__NO_ERROR:
    {
        dns_process_msg__dns_hdr_valid(hdr);
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
static void dns_process_msg__dns_hdr_valid(dns_hdr_t *hdr)
{
    switch ((DNS_HEADER_TYPE)hdr->qr)
    {
    case DNS_HEADER_TYPE__QUERY:
    {
        dns_process_msg__dns_hdr_valid_query(hdr);
    }
    break;

    case DNS_HEADER_TYPE__RESPONSE:
    {
        // handle a response type
    }   
    break;
    default:
    {
        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "[ERROR] Unhandled header type");
        #endif
        g_hdr.rcode = DNS_HEADER_RCODE__NOT_IMPLEMENTED;
    }
    break;
    }
}

/// @brief Handle a valid DNS query
/// @param hdr
static void dns_process_msg__dns_hdr_valid_query(dns_hdr_t *hdr)
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
        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "[ERROR] Unhandled header opcode");
        #endif
        g_hdr.rcode = DNS_HEADER_RCODE__NOT_IMPLEMENTED;
    }
    break;
    }
}


static void dns_process_msg__dns_hdr_valid_response(dns_hdr_t *hdr)
{

}

/// @brief Handle a Standard DNS query
/// @param hdr
static void dns_process_msg__dns_hdr_valid_query_standard(dns_hdr_t *hdr)
{
    // process questions
    if (hdr->qd_count)
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
static void dns_process_msg__dns_hdr_questions(dns_hdr_t *hdr)
{
    uint16_t c_pointer;
    uint32_t bytes;
    dns_answer_t answer;
    dns_question_t question;
    uint32_t total_bytes = 0;

    // Process all the questions
    for (uint16_t qIdx = 0; qIdx < hdr->qd_count; qIdx++)
    {
        c_pointer = 0;
        
        // extract the current question from the serializer
        bytes = dns_question_deserialize(
            &question,
            net_serializer_get_buf(&g_rxSerializer),
            net_serializer_get_size(&g_rxSerializer),
            &c_pointer);

        if (bytes)
        {
            // question has compressed label
            if (c_pointer)
            {
                cir_buffer_t temp_questions;
                cir_buffer_copy(&temp_questions, &g_questions, sizeof(dns_question_t));

                dns_handle_compressed_query( &question, &temp_questions, c_pointer );

                cir_buffer_deinit(&temp_questions);
            }

            #if defined(LOCAL_BUILD)
            log_debug(&g_main_thread_logger, "[Request] [Question] ([%u] bytes) %s", bytes, dns_question_str(&question));
            #endif // LOCAL_BUILD
            

            cir_buffer_write(&g_questions, (const char *)&question, sizeof(dns_question_t));

            // remove question from receive buffer
            if (net_serializer_remove_raw(&g_rxSerializer, 0, NULL, bytes) == 0)
            {
                // process the question and generate answer
                dns_process_msg__dns_question(&question, &answer);

                // Append the answer to the question response block
                cir_buffer_write(&g_answers, (const char *)&answer, sizeof(dns_answer_t));
            }
            else
            {
                // ! ERROR
            }
        }
        else
        {
            // A compressed question has been returned
            #if defined(LOCAL_BUILD)
            log_debug(&g_main_thread_logger, "Invalid Header encountered...Returning [Format Error]");
            #endif
            hdr->rcode = DNS_HEADER_RCODE__FORMAT_ERROR;
            
            cir_buffer_clear(&g_questions);
            cir_buffer_clear(&g_answers);
            
            break;
        }

        total_bytes += bytes;
    }
}

static void dns_process_msg__dns_question(const dns_question_t *question, dns_answer_t *answer)
{
    #if defined(LOCAL_BUILD)
    log_debug(&g_main_thread_logger, "Processing Question");
    #endif

    switch ((DNS_RR_TYPE)question->qtype)
    {
    case DNS_RR_TYPE__A:
    {
        dns_process_msg__dns_question_host_addr(question, answer);
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
        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "[INFO] unknown QUESTION type received");
        #endif
        break;
    }

#if defined(LOCAL_BUILD)
log_debug(&g_main_thread_logger, "Processing Quesstion...");
#endif
}

static void dns_process_msg__dns_question_host_addr(const dns_question_t *question, dns_answer_t *answer)
{
    switch ((DNS_RR_CLASS)question->qclass)
    {
    case DNS_RR_CLASS__IN:
    {
        dns_process_msg__dns_question_host_addr_internet(question, answer);
    }
    break;

    case DNS_RR_CLASS__CS:
    case DNS_RR_CLASS__CH:
    case DNS_RR_CLASS__HS:
    default:
        break;
    }
}

static void dns_process_msg__dns_question_host_addr_internet(const dns_question_t *question, dns_answer_t *answer)
{
    uint32_t offset = 0;
    char domain_name[255];
    const size_t domain_name_size = sizeof(domain_name) / sizeof(domain_name[0]);
    memset(domain_name, 0, domain_name_size);

    offset += dns_question_get_qname_as_domain_name(question, domain_name, domain_name_size);

    // lookup IP address from domain name
    const char *ip_addr = dns_get_ip_from_domain(domain_name);

    dns_process_msg__answer_host_addr_internet(answer, domain_name, ip_addr);
}

static void dns_process_msg__answer_host_addr_internet(dns_answer_t *answer, const char *domain_name, const char *ip_addr)
{
    const size_t domain_name_len = strlen(domain_name);
    uint32_t desc = htonl(ip_str_to_n(ip_addr));
    const size_t desc_len = sizeof(uint32_t);

    dns_answer_init_from_domain_name_with_desc(answer, domain_name, domain_name_len, 60, (const char *)&desc, desc_len);
}

// static uint32_t dns_request_deserialize_hdr(dns_hdr_t* hdr, const char *rply_buf, const uint32_t size)
// {
//     memset( hdr, 0, DSN_HDR_SIZE );
//     return dns_hdr_deserialize( hdr, rply_buf, size );
// }

static uint32_t dns_reply_serialize(char *rply_buf, const uint32_t size)
{
    uint32_t bytes = 0; // offset for header
    dns_question_t question;
    dns_answer_t answer;

    #if defined(LOCAL_BUILD)
    log_debug(&g_main_thread_logger, "Serializing Response...");
    #endif

    if( g_hdr.rcode == DNS_HEADER_RCODE__NO_ERROR )
    {
        bytes = DSN_HDR_SIZE;

        memset(g_dns_section_buffer, 0, DNS_SECTION_BUFFER_SIZE);
    
        g_hdr.qr = 1; // mark as response
        g_hdr.qd_count = g_questions.size;
        g_hdr.an_count = g_answers.size;
        
        dns_hdr_serialize(&g_hdr, g_dns_section_buffer, DSN_HDR_SIZE);
    
        // serialize the questions
        while (cir_buffer_is_empty(&g_questions) == false)
        {
            assert(cir_buffer_read(&g_questions, (char *)&question, sizeof(dns_question_t)) == sizeof(dns_question_t));
            bytes += dns_question_serialize(&question, g_dns_section_buffer + bytes, DNS_SECTION_BUFFER_SIZE - bytes);
            
            #if defined(LOCAL_BUILD)
            log_debug(&g_main_thread_logger, "[Response] Question %s", dns_question_str(&question));
            #endif
        }
    
        // Serialize the answers
        while (cir_buffer_is_empty(&g_answers) == false)
        {
            assert(cir_buffer_read(&g_answers, (char *)&answer, sizeof(dns_answer_t)) == sizeof(dns_answer_t));
            bytes += dns_answer_serialize(&answer, g_dns_section_buffer + bytes, DNS_SECTION_BUFFER_SIZE - bytes);
            
            #if defined(LOCAL_BUILD)
            log_debug(&g_main_thread_logger, "[Response] Answer %s", dns_answer_str(&answer));
            #endif
        }

        // insert the header at the front
    
        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "[Response] Header %s", dns_hdr_str(&g_hdr));
        #endif
    
        memcpy(rply_buf, g_dns_section_buffer, bytes);
    }

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

    #if defined(LOCAL_BUILD)
    log_debug(&g_main_thread_logger, "Serialized Header: %s", dns_hdr_str(hdr));
    #endif

    return dns_hdr_serialize(hdr, rply_buf, size);
}

static uint32_t dns_reply_serialize_question(dns_question_t *question, char *rply_buf, const uint32_t size)
{
    static const char *domain_name = "codecrafters.io";
    static const size_t domain_name_len = strlen("codecrafters.io");

    // dns_question_t question;
    // dns_question_init_as_domain_name( &question, domain_name, domain_name_len );
#if defined(LOCAL_BUILD)
log_debug(&g_main_thread_logger, "Serialized Question: %s", dns_question_str(question));
#endif

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
#if defined(LOCAL_BUILD)
log_debug(&g_main_thread_logger, "Serialized Answer: %s", dns_answer_str(answer));
#endif

    return dns_answer_serialize(answer, rply_buf, size);
}

static void dns_handle_compressed_query(dns_question_t *question, cir_buffer_t* prev_queries, const uint16_t pointer )
{
    bool done = false;
    dns_label_t *compressed_label;
    dns_label_t *referenced_label;
    dns_question_t temp_question;

    // instantiate offset to account for header
    uint32_t offset = DSN_HDR_SIZE;

    while ((done == false) && cir_buffer_is_empty(prev_queries) == false)
    {
        uint16_t l_idx = 0;

        #if defined(LOCAL_BUILD)
        log_debug(&g_main_thread_logger, "[Pointer resolution] Curr off - %u, Expected - %u", offset, pointer);
        #endif

        cir_buffer_read(prev_queries, (char *)&temp_question, sizeof(dns_question_t));

        while ((l_idx < temp_question.label_cnt) && offset < pointer)
        {
            #if defined(LOCAL_BUILD)
            log_debug(&g_main_thread_logger, "[Pointer resolution] Curr off - %u, Expected - %u", offset, pointer);
            #endif
            // increment the offset
            offset += sizeof(temp_question.qname[l_idx].len); // label len
            offset += temp_question.qname[l_idx].len;         // label content
            l_idx++;
        }

        // index found in question
        if (offset == pointer)
        {
            while (l_idx < temp_question.label_cnt)
            {
                referenced_label = &temp_question.qname[l_idx];

                // Copy the referenced label into the compressed label
                compressed_label = &(question->qname[question->label_cnt++]);
                memcpy(compressed_label, referenced_label, sizeof(dns_label_t));

                l_idx++;
            }

            done = true;
            break;
        }
        // offset not in question
        else
        {
            offset += sizeof(uint8_t);  // null
            offset += sizeof(uint16_t); // type
            offset += sizeof(uint16_t); // class
        }
    }

    if (done == false)
    {
        // ! ERROR
        perror("error!!");
        exit(-1);
    }
    else
    {
        printf("Compressed header : %s\n", dns_question_str(question) );
    }
}

static bool dns_remote_resolver_header_check( const dns_hdr_t* hdr )
{
    return (hdr->opcode == DNS_HEADER_OPCODE__STANDARD_QUERY);
}