#include "rapidapp_message_reflection.h"
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>

namespace rapidapp {

MessageReflectionFactory::MessageReflectionFactory()
{}

MessageReflectionFactory::~MessageReflectionFactory()
{}

::google::protobuf::Message*
MessageReflectionFactory::SpawnMessage(const char* msg_bin, size_t msg_bin_size)
{
    return NULL;
}

::google::protobuf::Message*
MessageReflectionFactory::SharedMessage(const char* msg_bin, size_t msg_bin_size)
{
    return NULL;
}

}
