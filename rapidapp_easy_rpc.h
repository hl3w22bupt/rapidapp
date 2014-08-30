#ifndef RAPIDAPP_EASY_RPC_H_
#define RAPIDAPP_EASY_RPC_H_

#include "coroutine/mc_coroutine.h"
#include "rapidapp_defines.h"
#include <queue>

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
        int Init(magic_cube::CoroutineScheduler* scheduler, EasyNet* net);
        int RpcCall(const void* request, size_t request_size,
                    ON_RPC_REPLY_FUNCTION callback);

        inline bool IsActive() {
            return (!crid_list_.empty());
        }

    private:
        int Resume(const char* buffer, size_t size);

    private:
        static int RpcFunction(void* arg);

    private:
        magic_cube::CoroutineScheduler* scheduler_;

    private:
        EasyNet* net_;
        const void* request_;
        size_t request_size_;
        const void* response_;
        size_t response_size_;

        std::queue<int> crid_list_;

        friend class AppFrameWork;
};

}

#endif
