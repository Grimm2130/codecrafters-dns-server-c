#if !defined(__SERVER_H__)
#define __SERVER_H__

#include "common.h"

struct server_context;

typedef struct cir_buffer cir_buffer_t;
typedef struct server_config server_config_t;
typedef struct server_context server_context_t;

typedef struct server
{
    /// @brief Server admin information
    struct
    {
        bool created;
        bool receiver_active;
        bool transmitter_active;
    } admin;
    
    /// @brief Server runtime information
    struct
    {
        uint32_t read_bytes;
        uint32_t written_bytes;
        uint32_t invalid_bytes_read;
        uint32_t read_timeouts;
    };  
    
    cir_buffer_t *rbuffer;          // read buffer
    cir_buffer_t *wbuffer;          // write buffer
    server_context_t *context;      // Server context
    
    // Server handler functions
    struct
    {
        void* receiver(const char* data, const int size);
        void* transmitter(const char* data, const int size);
    } handler_fns;
    
    // TODO: Add server configuration manager
} server_t;

server_t *server_create( server_config_t* );
void server_start( server_t* );
void server_stop( server_t* );

#endif // __SERVER_H__
