#ifndef RAPIDAPP_DEFINES_H_
#define RAPIDAPP_DEFINES_H_

#include <cstdio>
#include <inttypes.h>
#include <google/protobuf/message.h>

// net state
enum {
    NET_NOT_ESTABLISHED = -1,
    NET_CONNECT_FAILED = -2,
    NET_SEND_EXCEPTION = -3,
};

// rpc callback
typedef int (*ON_RPC_REPLY_FUNCTION)(const ::google::protobuf::Message* reply);

#endif
