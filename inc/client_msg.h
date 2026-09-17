#if !defined(__CLIENT_INFO_H__)
#define __CLIENT_INFO_H__

#include "network_context.h"

#define CLIENT_MSG_SIZE 1024

typedef struct client_msg
{
    int bytes;
    network_context_t context;
    char data[CLIENT_MSG_SIZE];
} client_msg_t;



#endif // __CLIENT_INFO_H__
