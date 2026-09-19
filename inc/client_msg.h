#if !defined(__CLIENT_INFO_H__)
#define __CLIENT_INFO_H__

#include "network_context.h"
#include "memory.h"

#define CLIENT_DATA_SIZE 1024

typedef struct client_msg
{
    int len;
    network_context_t context;
    char data[CLIENT_DATA_SIZE];
} client_msg_t;

#define CLIENT_MSG_SIZE sizeof(client_msg_t)

#define CLIENT_MSG_CPY(dst, src) (memcpy((dst), (src), CLIENT_MSG_SIZE))

#endif // __CLIENT_INFO_H__
