#include "rapidapp_message_reflection.h"
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <glog/logging.h>
#include <arpa/inet.h>

namespace rapidapp {

using ::google::protobuf::Message;

MessageReflectionFactory::MessageReflectionFactory()
{}

MessageReflectionFactory::~MessageReflectionFactory()
{}

Message*
MessageReflectionFactory::SpawnMessage(const char* msg_bin, size_t msg_bin_size)
{
    if (NULL == msg_bin)
    {
        LOG(ERROR)<<"null msg";
        return NULL;
    }

    if (msg_bin_size < MIN_MESSAGE_STAMP_LEN)
    {
        LOG(ERROR)<<"binary msg size:"<<msg_bin_size<<" less than min_stamp_len:"<<MIN_MESSAGE_STAMP_LEN;
        return NULL;
    }

    unsigned char type = msg_bin[0];
    unsigned short message_name_len = ntohs(*(unsigned short*)&msg_bin[1]);
    if (msg_bin_size < message_name_len + MIN_MESSAGE_STAMP_LEN)
    {
        LOG(ERROR)<<"got msg size:"<<msg_bin_size<<" less than message stamp:"<<
            MIN_MESSAGE_STAMP_LEN + message_name_len;
        return NULL;
    }

    return NULL;
}

Message*
MessageReflectionFactory::SharedMessage(const char* msg_bin, size_t msg_bin_size)
{
    if (NULL == msg_bin)
    {
        LOG(ERROR)<<"null msg";
        return NULL;
    }

    if (msg_bin_size < MIN_MESSAGE_STAMP_LEN)
    {
        LOG(ERROR)<<"binary msg size:"<<msg_bin_size<<" less than min_stamp_len:"<<MIN_MESSAGE_STAMP_LEN;
        return NULL;
    }

    unsigned char type = msg_bin[0];
    unsigned short message_name_len = ntohs(*(unsigned short*)&msg_bin[1]);
    if (msg_bin_size < message_name_len + MIN_MESSAGE_STAMP_LEN)
    {
        LOG(ERROR)<<"got msg size:"<<msg_bin_size<<" less than message stamp:"<<
            MIN_MESSAGE_STAMP_LEN + message_name_len;
        return NULL;
    }

    return NULL;
}

}
