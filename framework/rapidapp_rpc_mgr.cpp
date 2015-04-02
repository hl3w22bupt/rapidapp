#include "rapidapp_rpc_mgr.h"

namespace rapidapp {

// rpc client
RpcClientMgr::RpcClientMgr()
{}

RpcClientMgr::~RpcClientMgr()
{}


// rpc service
RpcServerMgr::RpcServerMgr()
{
    rpc_svc_map_.clear();
}

RpcServerMgr::~RpcServerMgr()
{
    rpc_svc_map_.clear();
}

void RpcServerMgr::AddRpcService(IRpcService* rpc_svc)
{
    if (rpc_svc != NULL)
    {
        std::string request_name = rpc_svc->RpcRequestName();
        rpc_svc_map_[request_name] = rpc_svc;
    }
}

IRpcService* RpcServerMgr::GetRpcService(const std::string& request_name)
{
    RpcServiceMap::iterator it = rpc_svc_map_.find(request_name);
    if (it != rpc_svc_map_.end() && it->second != NULL)
    {
        return it->second;
    }

    return NULL;
}

}
