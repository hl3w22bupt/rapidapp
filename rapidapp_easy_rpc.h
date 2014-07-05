#ifndef RAPIDAPP_EASY_RPC_H_
#define RAPIDAPP_EASY_RPC_H_

class EasyNet;
namespace rapidapp {

class IResponseHandler {
    public:
        IResponseHandler();
        virtual ~IResponseHandler() {}

    public:
        virtual int HandleResponse(const char* resp, size_t size) = 0;
};

// 基于C/S模型的rpc调用，在目前框架中，rpc请求服务端为backend服务
class EasyRpc {
    public:
        EasyRpc(EasyNet* net);
        virtual ~EasyRpc(){}

        // 基于协程内部封装出异步rpc调用
    public:
        RpcCall(const void* request, void* response,
                IResponseHandler* resp_handler);
};

}

#endif
