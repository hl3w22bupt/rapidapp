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

int rap_uri_get_socket_addr(const char* uri, struct sockaddr_in* sin)
{
    char* uri_ptr = NULL;
    char rap_uri[MAX_URI_LEN];

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
        in_addr_t addr = inet_addr(rap_uri);
        if (INADDR_NONE == addr)
        {
            return -3;
        }

        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = addr;
        sin->sin_port = htons(NET_DEF_PORT);
    }
    else
    {
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

        if (i != TYPE_UNIX)
        {
            sin->sin_family = AF_UNIX;
        }
        else
        {
            sin->sin_family = AF_INET;
        }
    }

    return 0;
}
