#ifndef RAPIDAPP_FRAMEWORK_H_
#define RAPIDAPP_FRAMEWORK_H_

#include <cstdio>

namespace rapidapp {

class IMsgHandler {
    public:
        IMsgHandler(){}
        virtual ~IMsgHandler() {}

    public:
        virtual int HandleRequest(const void* request, void* data, size_t* size) = 0;
        virtual int HandleResponse(const void* data, size_t size, void* response) = 0;
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

        virtual int SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        virtual int SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        // TODO SendvTo

    public:
        // timer_id用于区分定时器，使用者保证唯一性
        virtual EasyTimer* CreateTimer(size_t time, int timer_id) = 0;
        virtual void DestroyTimer(EasyTimer** timer) = 0;

    public:
        virtual EasyRpc* CreateRpc(EasyNet* net, IMsgHandler* handler) = 0;
        virtual int DestroyRpc(EasyRpc** rpc) = 0;
        virtual int RpcCall(EasyRpc* rpc, const void* request, void* response) = 0;
};

}

#endif
