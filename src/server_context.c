#include "server_context.h"
#include "common.h"

static uint32_t ip_n_to_str(const char *ip)
{
    int step = 3;
    int temp = 0;
    uint32_t num = 0;

    int len = strlen(ip);

    for (int i = 0; i < len; i++)
    {
        if (ip[i] == '.')
        {
            num |= (temp << (8 * step));
            step--;
            temp = 0;
        }
        else
        {
            temp = (temp * 10) + ip[i] - '0';
        }
    }

    return num;
}

void server_context_create_s(server_context_t *context, const char *ip, uint16_t port)
{
    uint32_t ip_n = INADDR_ANY;

    if (ip)
    {
        ip_n = ip_n_to_str(ip);
    }

    context->addr.sin_family = AF_INET;
    context->addr.sin_port = htons(port);
    context->addr.sin_addr.s_addr = htonl(ip_n);
    memset(context->addr.sin_zero, 0, sizeof(context->addr.sin_zero));
}

server_context_t *server_context_create(const char *ip, uint16_t port)
{
    server_context_t *context = calloc(1, sizeof(server_context_t));

    server_context_create_s(context, ip, port);

    return context;
}

int server_context_create_socket(server_context_t *context)
{
    return (context->sock = socket(AF_INET, SOCK_DGRAM, 0));
}

int server_context_bind_socket(server_context_t *context)
{
    return (bind(context->sock, (sockaddr_t *)(&context->addr), sizeof(sockaddr_in_t)));
}

int server_context_setsocket_opt(server_context_t *context, int level, int option, const void *val, socklen_t opt_len)
{
    return setsockopt(context->sock, level, option, val, opt_len);
}

int server_context_getsocket_opt(server_context_t *context, int level, int option, void *val, socklen_t *opt_len)
{
}
