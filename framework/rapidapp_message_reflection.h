#ifndef RAPIDAPP_MESSAGE_REFLECTION_H_
#define RAPIDAPP_MESSAGE_REFLECTION_H_

#include "rpc.pb.h"
#include <google/protobuf/message.h>

namespace rapidapp {

// 最大message名字限制
const int MAX_MESSAGE_NAME = 1024;

// message generator
class MessageGenerator {
    public:
        MessageGenerator();
        virtual ~MessageGenerator();

    public:
        static ::google::protobuf::Message* SpawnMessage(const char* msg_bin, size_t msg_bin_size);
        static const ::google::protobuf::Message* SharedMessage(const char* msg_bin, size_t msg_bin_size);
        // 以下三个接口获取最新rpc反射包消息名、消息类型、以及消息async id
        static const char* GetMessageName();
        static int32_t GetMessageType();
        static uint64_t GetAsyncId();

        static int MessageToBinary(int32_t type, uint64_t asyncid, 
                                   const ::google::protobuf::Message* message,
                                   std::string* out);
    private:
        static rpc_protocol::RpcMessage rpc_msg_;
};

}

#endif
