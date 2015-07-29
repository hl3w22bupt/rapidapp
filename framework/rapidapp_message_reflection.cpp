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

rpc_protocol::RpcMessage MessageGenerator::rpc_msg_;

MessageGenerator::MessageGenerator()
{}

MessageGenerator::~MessageGenerator()
{}

Message*
MessageGenerator::SpawnMessage(const char* data, size_t size)
{
    if (UnpackToRpcMsg(data, size) != 0)
    {
        LOG(ERROR)<<"unpack to rpc msg failed";
        return NULL;
    }

    Message* message = NewInstance(rpc_msg_.msg_name());
    if (NULL == message)
    {
        LOG(ERROR)<<"NewInstance by name:"<<rpc_msg_.msg_name()<<" failed";
        return NULL;
    }

    message->Clear();
    // 曾经因为这样写这个判断语句，导致以为ParseFromString失败，定位了好几天
    // if (!message->ParseFromString(rpc_msg_.msg_bin()));
    // !!!!多了一个分号，oh my god!!!!
    if (!message->ParseFromString(rpc_msg_.msg_bin()))
    {
        LOG(ERROR)<<"ParseFromString failed, msg bin size:"<<rpc_msg_.msg_bin().size();
        delete message;
        return NULL;
    }

    LOG(INFO)<<"msg: {\n"<<message->DebugString()<<"}";

    return message;
}

void MessageGenerator::ReleaseMessage(Message* message)
{
    if (message != NULL)
    {
        message->Clear();
        delete message;
    }
}

const Message*
MessageGenerator::SharedMessage(const char* data, size_t size)
{
    if (UnpackToRpcMsg(data, size) != 0)
    {
        LOG(ERROR)<<"unpack to rpc msg failed";
        return NULL;
    }

    Message* message = const_cast<Message*>(DefaultInstance(rpc_msg_.msg_name()));
    if (NULL == message)
    {
        LOG(ERROR)<<"DefaultInstance by name:"<<rpc_msg_.msg_name()<<" failed";
        return NULL;
    }

    message->Clear();
    // 曾经因为这样写这个判断语句，导致以为ParseFromString失败，定位了好几天
    // if (!message->ParseFromString(rpc_msg_.msg_bin()));
    // !!!!多了一个分号，oh my god!!!!
    if (!message->ParseFromString(rpc_msg_.msg_bin()))
    {
        LOG(ERROR)<<"ParseFromString failed, msg bin size:"<<rpc_msg_.msg_bin().size();
        return NULL;
    }

    LOG(INFO)<<"msg: {\n"<<message->DebugString()<<"}";

    return message;
}

int MessageGenerator::MessageToBinary(int32_t type, uint64_t asyncid,
                                      const ::google::protobuf::Message* message,
                                      std::string* out)
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

    if (NULL == out)
    {
        LOG(ERROR)<<"null buffer";
        return -1;
    }

    LOG(INFO)<<"message to serialize: {\n"<<message->DebugString()<<"}";
    std::string buf;
    message->SerializeToString(&buf);
    LOG(INFO)<<"msg bin size:"<<buf.size();

    rpc_protocol::RpcMessage rpc_message;
    rpc_message.set_magic(rpc_protocol::MAGIC_RPCSTAMP_V1);
    rpc_message.set_msg_type(type);
    rpc_message.set_msg_name(message_name);
    rpc_message.set_asyncid(asyncid);
    rpc_message.set_msg_bin(buf.c_str(), buf.size());

    LOG(INFO)<<"rpc message to serialize: {\n"<<rpc_message.DebugString()<<"}";

    rpc_message.SerializeToString(out);

    return 0;
}

int MessageGenerator::RpcErrorToBinary(uint64_t asyncid,
                                       std::string* out)
{
    rpc_protocol::RpcMessage rpc_message;
    rpc_message.set_magic(rpc_protocol::MAGIC_RPCSTAMP_V1);
    rpc_message.set_msg_type(rpc_protocol::RPC_TYPE_ERROR);
    rpc_message.set_msg_name("");
    rpc_message.set_asyncid(asyncid);
    rpc_message.set_msg_bin(NULL, 0);

    LOG(INFO)<<"rpc message to serialize: {\n"<<rpc_message.DebugString()<<"}";

    rpc_message.SerializeToString(out);
    return 0;
}

int MessageGenerator::UnpackToRpcMsg(const char* data, size_t size)
{
    if (NULL == data)
    {
        LOG(ERROR)<<"null msg";
        return -1;
    }

    if (size <= 0)
    {
        LOG(ERROR)<<"binary msg size:"<<size<<" less than 0.";
        return -1;
    }

    rpc_msg_.Clear();
    if (!rpc_msg_.ParseFromArray(data, size))
    {
        LOG(ERROR)<<"ParseFromArray failed, msg size:"<<size;
        return -1;
    }

    LOG(INFO)<<"rpc stamp: {\n"<<rpc_msg_.DebugString()<<"}";

    return 0;
}

int MessageGenerator::BinaryToMessage(const char* data, size_t size,
                                      ::google::protobuf::Message* message)
{
    if (NULL == message)
    {
        LOG(ERROR)<<"null message";
        return -1;
    }

    if (UnpackToRpcMsg(data, size) != 0)
    {
        LOG(ERROR)<<"unpack to rpc msg failed";
        return -1;
    }

    const std::string& type_name = message->GetTypeName();
    if (type_name != rpc_msg_.msg_name())
    {
        LOG(ERROR)<<"recved msg name:"<<rpc_msg_.msg_name()<<" not same as expected:"<<type_name;
        return -1;
    }

    message->Clear();
    // 曾经因为这样写这个判断语句，导致以为ParseFromString失败，定位了好几天
    // if (!message->ParseFromString(rpc_msg_.msg_bin()));
    // !!!!多了一个分号，oh my god!!!!
    if (!message->ParseFromString(rpc_msg_.msg_bin()))
    {
        LOG(ERROR)<<"ParseFromString failed, msg bin size:"<<rpc_msg_.msg_bin().size();
        return -1;
    }

    LOG(INFO)<<"msg: {\n"<<message->DebugString()<<"}";

    return 0;
}

}
