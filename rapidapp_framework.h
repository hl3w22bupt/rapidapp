#ifndef RAPIDAPP_FRAMEWORK_H_
#define RAPIDAPP_FRAMEWORK_H_

#include "rapidapp_defines.h"

namespace rapidapp {

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

        // 异步上下文相关
        virtual EasyNet* GetFrontEndByAsyncIds(uint32_t fd, uint64_t nid) = 0;
        virtual void* GetUserContext(EasyNet* net) = 0;
        virtual int GetNetIds(EasyNet* net, uint32_t& fd, uint64_t& nid) = 0;

        virtual int SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        virtual int SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        // TODO SendvTo

    public:
        // timer_id用于区分定时器，使用者保证唯一性
        virtual EasyTimer* CreateTimer(size_t time, int timer_id) = 0;
        virtual void DestroyTimer(EasyTimer** timer) = 0;

        // 目前实现的rpc，依赖于rpc服务端保证按顺序返回响应
        // 目前rpc基于协程的实现暂未完成预期功能，统一放到后面来完成
        // TODO 后续会在封装的IPC组件上集成rpc，将rpc coroutine id放入协议首部
        // 后续以EasyRpc为基础，结合Message Reflection来完成rpc
    public:
        virtual EasyRpc* CreateRpc(EasyNet* net) = 0;
        virtual int DestroyRpc(EasyRpc** rpc) = 0;
        virtual int RpcCall(EasyRpc* rpc,
                            const void* request, size_t request_size,
                            ON_RPC_REPLY_FUNCTION callback) = 0;
};

}

#endif
