#if !defined(__CLIENT_INFO_H__)
#define __CLIENT_INFO_H__

#include "network_context.h"
#include "memory.h"

#define CLIENT_MSG_SIZE 1024

typedef struct client_msg
{
    int len;
    network_context_t context;
    char data[CLIENT_MSG_SIZE];
} client_msg_t;

// #define CLIENT_MSG_READ(client, msg, len )      \

// void client_msg_write( client_msg_t *, const char * _data, const uint32_t len );
// void client_msg_read( client_msg_t *, char * _data, const uint32_t len );

#endif // __CLIENT_INFO_H__
