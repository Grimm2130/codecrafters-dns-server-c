#if !defined(__SERVER_CONTEXT_H__)
#define __SERVER_CONTEXT_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr sockaddr_t;

/// @brief UDP Context object for managing information for server or client enpoints
typedef struct server_context
{
    int sock;
    sockaddr_in_t addr;
} server_context_t;

void server_context_create_s(server_context_t *context, const char *ip, uint16_t port);
server_context_t *server_context_create(const char *ip, uint16_t port);

int server_context_create_socket(server_context_t *context);
int server_context_bind_socket(server_context_t *context);

int server_context_setsocket_opt(server_context_t *context, int level, int option, const void *val, socklen_t opt_len);
int server_context_getsocket_opt(server_context_t *context, int level, int option, void *val, socklen_t *opt_len);

#endif // __SERVER_CONTEXT_H__
