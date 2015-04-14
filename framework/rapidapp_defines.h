#ifndef RAPIDAPP_DEFINES_H_
#define RAPIDAPP_DEFINES_H_

#include <cstdio>
#include <inttypes.h>
#include <google/protobuf/message.h>

namespace rapidapp {

// net state
enum {
    NET_NOT_ESTABLISHED = -1,
    NET_CONNECT_FAILED = -2,
    NET_SEND_EXCEPTION = -3,
};

// rpc callback
typedef int (*ON_RPC_REPLY_FUNCTION)(const ::google::protobuf::Message* request,
                                     ::google::protobuf::Message* response,
                                     void* arg, int status);

class IRpcClosure {
    public:
        IRpcClosure() {}
        virtual ~IRpcClosure() {}

    public:
        virtual void Done() = 0;
        virtual bool IsDone() = 0;

        virtual void set_userdata(void* data) = 0;
        virtual void* userdata() const = 0;
        virtual ::google::protobuf::Message* request() = 0;
        virtual ::google::protobuf::Message* response() = 0;

};

class IRpcService {
    public:
        IRpcService(){}
        virtual ~IRpcService() {}

    public:
        virtual const std::string RpcRequestName() {
            return "";
        }

        virtual ::google::protobuf::Message* NewResponse() {
            return NULL;
        }

        virtual void OnRpcCall(const ::google::protobuf::Message* req,
                               ::google::protobuf::Message* resp,
                               IRpcClosure* closure) = 0;
};

}
#endif
