#ifndef RAP_URI_NET_H_
#define RAP_URI_NET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "event2/event.h"

#ifdef __cplusplus
extern "C" {
#endif

evutil_socket_t rap_uri_open_socket(const char* uri);
int rap_uri_get_socket_addr(const char* uri, struct sockaddr_in* sin);

#ifdef __cplusplus
}
#endif

#endif
