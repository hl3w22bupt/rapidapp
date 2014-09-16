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
    if (msg_bin[MIN_MESSAGE_STAMP_LEN + message_name_len - 1] != '\0')
    {
        LOG(ERROR)<<"message name unexpected, name length NOT null terminated";
        return NULL;
    }
    std::string message_name = std::string(msg_bin + MIN_MESSAGE_STAMP_LEN,
                                           message_name_len - 1);

    Message* message = NewInstance(message_name);
    if (NULL == message)
    {
        LOG(ERROR)<<"NewInstance by name:"<<message_name<<" failed";
        return NULL;
    }

    if (!message->ParseFromArray(msg_bin + MIN_MESSAGE_STAMP_LEN + message_name_len,
                                 msg_bin_size - (MIN_MESSAGE_STAMP_LEN + message_name_len)))
    {
        LOG(ERROR)<<"ParseFromArray failed, msg size:"<<msg_bin_size - (MIN_MESSAGE_STAMP_LEN + message_name_len);
        delete message;
        return NULL;
    }

    return message;
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
    if (msg_bin[MIN_MESSAGE_STAMP_LEN + message_name_len - 1] != '\0')
    {
        LOG(ERROR)<<"message name unexpected, name length NOT null terminated";
        return NULL;
    }
    std::string message_name = std::string(msg_bin + MIN_MESSAGE_STAMP_LEN,
                                           message_name_len - 1);

    Message* message = const_cast<Message*>(DefaultInstance(message_name));
    if (NULL == message)
    {
        LOG(ERROR)<<"DefaultInstance by name:"<<message_name<<" failed";
        return NULL;
    }

    if (!message->ParseFromArray(msg_bin + MIN_MESSAGE_STAMP_LEN + message_name_len,
                                 msg_bin_size - (MIN_MESSAGE_STAMP_LEN + message_name_len)))
    {
        LOG(ERROR)<<"ParseFromArray failed, msg size:"<<msg_bin_size - (MIN_MESSAGE_STAMP_LEN + message_name_len);
        return NULL;
    }

    return message;
}

const char* GetMessageName(const char* msg_bin, size_t msg_bin_size)
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

    unsigned short message_name_len = ntohs(*(unsigned short*)&msg_bin[1]);
    if (msg_bin_size < message_name_len + MIN_MESSAGE_STAMP_LEN)
    {
        LOG(ERROR)<<"got msg size:"<<msg_bin_size<<" less than message stamp:"<<
            MIN_MESSAGE_STAMP_LEN + message_name_len;
        return NULL;
    }

    if (msg_bin[MIN_MESSAGE_STAMP_LEN + message_name_len - 1] != '\0')
    {
        LOG(ERROR)<<"message name unexpected, name length NOT null terminated";
        return NULL;
    }

    return msg_bin + MIN_MESSAGE_STAMP_LEN;
}

int MessageGenerator::MessageToBinary(Message* message, char* data, size_t size)
{
    if (NULL == message)
    {
        LOG(ERROR)<<"null message";
        return -1;
    }

    const std::string message_name = message->GetTypeName();
    if (message_name.size() >= MAX_MESSAGE_NAME)
    {
        LOG(ERROR)<<"message name length:"<<message_name.size()<<" too big";
        return -1;
    }

    if (NULL == data || size <= MIN_MESSAGE_STAMP_LEN + message_name.size())
    {
        LOG(ERROR)<<"invalid data:"<<data<<", size:"<<size;
        return -1;
    }

    data[0] = 0; // type 0
    *(unsigned short*)&data[1] = htons(message_name.size() + 1);
    strcpy(data + MIN_MESSAGE_STAMP_LEN, message_name.c_str());
    data[MIN_MESSAGE_STAMP_LEN + message_name.size()] = '\0';

    LOG(INFO)<<"message to serialize:"<<message->DebugString();
    message->SerializeToArray(data + (MIN_MESSAGE_STAMP_LEN + message_name.size()),
                              size - (MIN_MESSAGE_STAMP_LEN + message_name.size()));

    return 0;
}

}
