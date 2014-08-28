#ifndef RAPIDAPP_MESSAGE_REFLECTION_H_
#define RAPIDAPP_MESSAGE_REFLECTION_H_

#include <google/protobuf/message.h>

namespace rapidapp {

class MessageReflectionFactory {
    public:
        MessageReflectionFactory();
        virtual ~MessageReflectionFactory();

    public:
        static ::google::protobuf::Message* SpawnMessage(const char* msg_bin, size_t msg_bin_size);
        static ::google::protobuf::Message* SharedMessage(const char* msg_bin, size_t msg_bin_size);
};

}

#endif
