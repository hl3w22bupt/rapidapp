#ifndef RAPIDAPP_FRAMEWORK_H_
#define RAPIDAPP_FRAMEWORK_H_

#include "rapidapp_defines.h"
#include <google/protobuf/message.h>

namespace rapidapp {

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

        virtual int OnRpcCall(const ::google::protobuf::Message* req, ::google::protobuf::Message* resp) = 0;
};

class EasyTimer;
class EasyNet;
class EasyRpc;
class IFrameWork {
    public:
        IFrameWork(){}
        ~IFrameWork(){}

    public:
        // type用于区别后端服务类型。如果不需要区分后端服务请填0
        virtual EasyNet* CreateBackEnd(const char* url, int type) = 0;
        virtual void DestroyBackEnd(EasyNet** net) = 0;

        virtual void DestroyFrontEnd(EasyNet** net) = 0;

        // 异步上下文相关
        virtual EasyNet* GetFrontEndByAsyncIds(uint32_t fd, uint64_t nid) = 0;
        virtual void* GetUserContext(EasyNet* net) = 0;
        virtual int GetNetIds(EasyNet* net, uint32_t& fd, uint64_t& nid) = 0;

        // 数据发送相关
        virtual int SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        virtual int SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        // TODO SendvTo

    public:
        // timer_id用于区分定时器，使用者保证唯一性
        virtual EasyTimer* CreateTimer(size_t time, int timer_id) = 0;
        virtual void DestroyTimer(EasyTimer** timer) = 0;

    public:
        // rpc 相关
        // rpc client
        virtual EasyRpc* CreateRpc(EasyNet* net) = 0;
        virtual int DestroyRpc(EasyRpc** rpc) = 0;
        virtual int RpcCall(EasyRpc* rpc,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ON_RPC_REPLY_FUNCTION callback) = 0;
        // rpc server
        virtual int RegisterRpcService(IRpcService* rpc_svc) = 0;

    public:
        // 帧驱动回调
        virtual void ScheduleUpdate() = 0;
        virtual void UnScheduleUpdate() = 0;
};

}

#endif
