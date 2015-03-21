#ifndef RAPIDAPP_DEFINES_H_
#define RAPIDAPP_DEFINES_H_

#include <cstdio>
#include <inttypes.h>

// net state
enum {
    NET_NOT_ESTABLISHED = -1,
    NET_CONNECT_FAILED = -2,
    NET_SEND_EXCEPTION = -3,
};

// rpc callback
typedef int (*ON_RPC_REPLY_FUNCTION)(const void* reply, size_t reply_size);

#endif
