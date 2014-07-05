#ifndef RAPIDAPP_EASY_RPC_H_
#define RAPIDAPP_EASY_RPC_H_

#include "coroutine/mc_coroutine.h"
#include "rapidapp_framework.h"

namespace rapidapp {

// 基于C/S模型的rpc调用，在目前框架中，rpc请求服务端为backend服务
class EasyNet;
class AppFrameWork;
class EasyRpc {
    public:
        EasyRpc();
        virtual ~EasyRpc();

        // 基于协程封装出异步rpc调用
    public:
        int Init(magic_cube::CoroutineScheduler* scheduler, EasyNet* net, IMsgHandler* msg_handler);
        int RpcCall(const void* request, void* response);

    private:
        int Resume(const char* buffer, size_t size);

    private:
        static int RpcFunction(void* arg);

    private:
        magic_cube::CoroutineScheduler* scheduler_;

    private:
        EasyNet* net_;
        IMsgHandler* msg_handler_;
        const void* request_;
        void* response_;

        int cid_;
        friend class AppFrameWork;
};

}

#endif
