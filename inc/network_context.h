#if !defined(__NETWORK_CONTEXT_H__)
#define __NETWORK_CONTEXT_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr sockaddr_t;

/// @brief UDP Context object for managing information for server or client enpoints
typedef struct network_context
{
    int sock;
    sockaddr_in_t addr;
} network_context_t;

network_context_t *network_context_create(const char *ip, uint16_t port);
void network_context_init(network_context_t *context, const char *ip, uint16_t port);

int network_context_create_socket(network_context_t *context);
int network_context_bind_socket(network_context_t *context);

int network_context_setsocket_opt(network_context_t *context, int level, int option, const void *val, socklen_t opt_len);
int network_context_getsocket_opt(network_context_t *context, int level, int option, void *val, socklen_t *opt_len);

void network_context_deinit(network_context_t *context);
void network_context_destroy(network_context_t *context);

#endif // __NETWORK_CONTEXT_H__
