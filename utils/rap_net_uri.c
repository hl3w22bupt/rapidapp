#include "event2/event.h"
#include "rap_net_uri.h"
#include <string.h>

#define URI_SEPERATOR ':'
#define MAX_URI_LEN 256

#define NET_DEF_PORT 80

#define MAX_PROTO_NAME_LEN 16

enum {
    TYPE_TCP  = 0,
    TYPE_UDP  = 1,
    TYPE_UNIX = 2,
};

static const char* protocol[] = {
    "tcp", "udp", "unix", NULL,
};

#define PROTO_TCP "tcp"
#define PROTO_UDP "udp"
#define PROTO_UNIX "unix"

int rap_uri_get_socket_type(const char* uri)
{
    char* uri_ptr = NULL;
    char rap_uri[MAX_URI_LEN];

    if (NULL == uri)
    {
        return -1;
    }

    if (strlen(uri) >= MAX_URI_LEN)
    {
        return -2;
    }

    strcpy(rap_uri, uri);
    uri_ptr =  strchr(rap_uri, URI_SEPERATOR);
    if (NULL == uri_ptr)
    {
        return TYPE_TCP;
    }

    *uri_ptr = '\0';
    size_t i = 0;
    for(; protocol[i] != NULL; ++i)
    {
        if (0 == strncmp(rap_uri, protocol[i], strlen(rap_uri)))
        {
            break;
        }
    }

    if (NULL == protocol[i])
    {
        return -3;
    }

    return i;
}

evutil_socket_t rap_uri_open_socket(const char* uri)
{
    int type = rap_uri_get_socket_type(uri);
    if (type < 0)
    {
        return -1;
    }

    switch(type)
    {
        case TYPE_TCP:
            {
                return socket(AF_INET, SOCK_STREAM, 0);
            }
        case TYPE_UDP:
            {
                return socket(AF_INET, SOCK_DGRAM, 0);
            }
        case TYPE_UNIX:
            {
                return socket(AF_UNIX, SOCK_STREAM, 0);
            }
        default:
            {
                return -1;
            }
    }

    return 0;
}

// TODO gethostbyname_r
int rap_uri_get_socket_addr(const char* uri, struct sockaddr_in* sin)
{
    char* uri_ptr = NULL;
    char* port_ptr = NULL;
    char rap_uri[MAX_URI_LEN];
    int port = NET_DEF_PORT;
    in_addr_t addr = INADDR_NONE;

    if (NULL == uri || NULL == sin)
    {
        return -1;
    }

    if (strlen(uri) >= MAX_URI_LEN)
    {
        return -2;
    }
    strcpy(rap_uri, uri);

    uri_ptr =  strchr(rap_uri, URI_SEPERATOR);
    if (NULL == uri_ptr)
    {
        addr = inet_addr(rap_uri);
        if (INADDR_NONE == addr)
        {
            return -3;
        }

        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = addr;
        sin->sin_port = htons(NET_DEF_PORT);

        return 0;
    }

    *uri_ptr = '\0';
    size_t i = 0;
    for(; protocol[i] != NULL; ++i)
    {
        if (0 == strncmp(rap_uri, protocol[i], strlen(rap_uri)))
        {
            break;
        }
    }

    if (NULL == protocol[i])
    {
        return -4;
    }

    ++uri_ptr;
    port_ptr = strchr(uri_ptr, URI_SEPERATOR);
    if (NULL != port_ptr)
    {
        *port_ptr = '\0';
        port = atoi(port_ptr + 1);
    }

    addr = inet_addr(uri_ptr);
    if (INADDR_NONE == addr)
    {
        return -3;
    }

    sin->sin_addr.s_addr = addr;
    sin->sin_port = htons(port);
    switch(i)
    {
        case TYPE_TCP:
        case TYPE_UDP:
            {
                sin->sin_family = AF_INET;
                return 0;
            }
        case TYPE_UNIX:
            {
                sin->sin_family = AF_UNIX;
                return 0;
            }
        default:
            {
                return -5;
            }
    }
}
