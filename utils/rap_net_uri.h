#ifndef RAP_URI_NET_H_
#define RAP_URI_NET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef _cplusplus
extern "C" {
#endif

int rap_uri_get_socket_addr(const char* uri, struct sockaddr_in* sin);

#ifdef _cplusplus
}
#endif

#endif
