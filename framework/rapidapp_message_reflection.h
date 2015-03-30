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
        static ::google::protobuf::Message* SpawnMessage(const char* data, size_t size);
        static const ::google::protobuf::Message* SharedMessage(const char* data, size_t size);

        // 以下三个接口获取最新rpc反射包消息名、消息类型、以及消息async id
        static inline const char* GetMessageName() {
            return rpc_msg_.msg_name().c_str();
        }

        static inline int32_t GetMessageType() {
            return rpc_msg_.msg_type();
        }

        static inline uint64_t GetAsyncId() {
            return rpc_msg_.asyncid();
        }


        static int MessageToBinary(int32_t type, uint64_t asyncid,
                                   const ::google::protobuf::Message* message,
                                   std::string* out);
        static int BinaryToMessage(const char* data, size_t size,
                                   ::google::protobuf::Message* message);
    private:
        static rpc_protocol::RpcMessage rpc_msg_;
};

}

#endif
