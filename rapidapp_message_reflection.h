#ifndef RAPIDAPP_MESSAGE_REFLECTION_H_
#define RAPIDAPP_MESSAGE_REFLECTION_H_

#include <google/protobuf/message.h>

namespace rapidapp {

const int MAX_MESSAGE_NAME = 1024;

typedef struct MessageStamp {
    unsigned char message_type;
    unsigned short message_name_len;
    char message_name[MAX_MESSAGE_NAME];
} MessageStamp;

const int MIN_MESSAGE_STAMP_LEN = sizeof(unsigned char) + sizeof(unsigned short);

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
