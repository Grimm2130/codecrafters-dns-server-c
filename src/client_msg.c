#if !defined(__CLIENT_MSG_H__)
#define __CLIENT_MSG_H__

#include "client_msg.h"
#include "stdio.h"

const char * client_msg_str( client_msg_t * msg )
{
    uint16_t offset = 0;
    static uint8_t msg_raw_buf[1024];
    static const u_int16_t msg_raw_buf_size = sizeof(msg_raw_buf)/sizeof(msg_raw_buf[0]);

    msg_raw_buf[0] = '\0';

    for (int i = 0; i < msg->len; i++)
    {
        offset += snprintf( (char*)(msg_raw_buf + offset), msg_raw_buf_size - offset, "0x%.2X ", (uint8_t)msg->data[i]);
    }

    return (char*)msg_raw_buf;
}

#endif // __CLIENT_MSG_H__
