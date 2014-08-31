#include "rapidapp_message_reflection.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <glog/logging.h>
#include <arpa/inet.h>

namespace rapidapp {

using ::google::protobuf::Message;
using ::google::protobuf::MessageFactory;
using ::google::protobuf::Descriptor;
using ::google::protobuf::DescriptorPool;

inline Message* NewInstance(const std::string& mesg_name)
{
    Message* mesg = NULL;
    // get message type
    const Descriptor* descriptor = DescriptorPool::generated_pool()
        ->FindMessageTypeByName(mesg_name);
    if (descriptor != NULL)
    {
        // get default instance
        const Message* prototype = MessageFactory::generated_factory()
            ->GetPrototype(descriptor);
        if (prototype != NULL)
        {
            // new instance
            return prototype->New();
        }
    }

    return mesg;
}

inline const Message* DefaultInstance(const std::string& mesg_name)
{
    // get message type
    const Descriptor* descriptor = DescriptorPool::generated_pool()
        ->FindMessageTypeByName(mesg_name);
    if (descriptor != NULL)
    {
        // default instance
        return MessageFactory::generated_factory()->GetPrototype(descriptor);
    }

    return NULL;
}

MessageGenerator::MessageGenerator()
{}

MessageGenerator::~MessageGenerator()
{}

Message*
MessageGenerator::SpawnMessage(const char* msg_bin, size_t msg_bin_size)
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
    std::string message_name = std::string(msg_bin + MIN_MESSAGE_STAMP_LEN,
                                           message_name_len - 1);

    return NewInstance(message_name);
}

const Message*
MessageGenerator::SharedMessage(const char* msg_bin, size_t msg_bin_size)
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
    std::string message_name = std::string(msg_bin + MIN_MESSAGE_STAMP_LEN,
                                           message_name_len - 1);

    return DefaultInstance(message_name);
}

SmartMessanger::SmartMessanger()
{}

SmartMessanger::~SmartMessanger()
{}

int SmartMessanger::SendMessage(Message* message)
{
    if (NULL == message)
    {
        return -1;
    }

    const std::string message_name = message->GetTypeName();

    // TODO
    return 0;
}

}
