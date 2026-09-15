#if !defined(__SERVER_H__)
#define __SERVER_H__

#include "common.h"

#define IP_ADDR_LEN 15

struct server_context;

typedef struct cir_buffer cir_buffer_t;
typedef struct server_context server_context_t;

typedef struct server
{
    // Server network configuration information
    struct
    {
        uint16_t server_port;
        uint32_t server_ip;
        uint32_t client_ip_allow_mask;
        uint32_t server_recv_timout;
    } net_config;
    
    /// @brief Server admin information
    struct
    {
        bool created;
        bool receiver_active;
        bool transmitter_active;
    } admin_info;

    /// @brief Server runtime information
    struct
    {
        uint32_t read_bytes;
        uint32_t written_bytes;
        uint32_t read_timeouts;
        uint32_t invalid_bytes_read;
    } rt_info;

    cir_buffer_t *rbuffer;     // read buffer
    cir_buffer_t *wbuffer;     // write buffer
    server_context_t *context; // Server context

    // Server handler functions
    struct
    {
        void (*receiver)(const char *data, const int size);
        void (*transmitter)(const char *data, const int size);
    } handler_fns;

    // TODO: Add server configuration manager
} server_t;

typedef struct server_params
{
    uint16_t port;                                  // port
    uint32_t client_allow_mask;                     // client allow mask
    uint32_t rcv_to;                                // receiver timeout
    void (*rcv_fn)(const char*, const int );       // receiver function
    void (*tr_fn)(const char*, const int );        // transmitter function
    char ip[IP_ADDR_LEN+1];                        // ip
} server_params_t;


server_t *server_create( server_params_t* );
void server_init(server_t *, server_params_t* );
void server_start(server_t *);
void server_stop(server_t *);
void server_get_info(server_t *, char *info, const int cap);

#endif // __SERVER_H__
