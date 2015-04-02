#ifndef RAPIDAPP_RPC_MGR_H_
#define RAPIDAPP_RPC_MGR_H_

#include "rapidapp_framework.h"
#include <tr1/unordered_map>

namespace rapidapp {

typedef std::tr1::unordered_map<std::string, IRpcService*> RpcServiceMap;

class RpcClientMgr {
    public:
        RpcClientMgr();
        ~RpcClientMgr();
};

class RpcServerMgr {
    public:
        RpcServerMgr();
        ~RpcServerMgr();

    public:
        void AddRpcService(IRpcService* rpc_svc);
        IRpcService* GetRpcService(const std::string& request_name);

    private:
        RpcServiceMap rpc_svc_map_;
};

}

#endif
