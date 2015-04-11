#ifndef RAPIDAPP_EASY_RPC_H_
#define RAPIDAPP_EASY_RPC_H_

#include "coroutine/mc_coroutine.h"
#include "rapidapp_defines.h"
#include <google/protobuf/message.h>
#include <vector>

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
        int RpcCall(const ::google::protobuf::Message* request,
                    ::google::protobuf::Message* response,
                    ON_RPC_REPLY_FUNCTION callback);

        inline bool IsActive() {
            return (!crid_list_.empty());
        }

        inline EasyNet* net() {
            return net_;
        }

    private:
        int Resume(const char* buffer, size_t size);

    private:
        void RemoveByCoroutineId(int crid);
        int GetCoroutineIdxByAsyncId(uint64_t asyncid);

    private:
        static int RpcFunction(void* arg);

    private:
        magic_cube::CoroutineScheduler* scheduler_;

    private:
        EasyNet* net_;
        const ::google::protobuf::Message* request_;
        ::google::protobuf::Message* response_;

    private:
        typedef struct {
            int crid;
            uint64_t asyncid;
        } CoroutinePair;

        typedef  std::vector<CoroutinePair> CoroutineList;
        CoroutineList crid_list_;

        static uint64_t asyncid_seed;
        friend class AppFrameWork;
};

}

#endif
