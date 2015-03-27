#include "rapidapp_easy_rpc.h"
#include "rapidapp_easy_net.h"
#include "rapidapp_message_reflection.h"
#include <cassert>
#include <glog/logging.h>

namespace rapidapp {

typedef struct RpcCallContext {
    ON_RPC_REPLY_FUNCTION callback;
    EasyRpc* rpc_stub;
} RPC_CONTEXT;

uint64_t EasyRpc::asyncid_seed = 0;

EasyRpc::EasyRpc() : scheduler_(NULL), net_(NULL),
                    request_(NULL), response_(NULL), crid_list_()
{}

EasyRpc::~EasyRpc()
{
    for (CoroutineList::iterator it = crid_list_.begin();
         it != crid_list_.end(); ++it)
    {
        scheduler_->DestroyCoroutine(it->crid);
    }

    crid_list_.clear();
}

int EasyRpc::Init(magic_cube::CoroutineScheduler* scheduler, EasyNet* net)
{
    if (NULL == net || NULL == scheduler)
    {
        LOG(ERROR)<<"null net OR null scheduler";
        return -1;
    }

    net_ = net;
    scheduler_ = scheduler;

    return 0;
}

int EasyRpc::RpcCall(const ::google::protobuf::Message* request,
                     ON_RPC_REPLY_FUNCTION callback)
{
    if (NULL == request || NULL == callback)
    {
        return -1;
    }

    if (NULL == scheduler_)
    {
        return -1;
    }

    request_ = request;

    // 创建一个协程上下文
    RPC_CONTEXT context;
    context.rpc_stub = this;
    context.callback = callback;
    int cid = scheduler_->CreateCoroutine(RpcFunction, &context);
    if (cid < 0)
    {
        LOG(ERROR)<<"create coroutine failed, net:"<<net_->uri();
        return -1;
    }

    CoroutinePair cp;
    cp.crid = cid;
    cp.asyncid = asyncid_seed;
    crid_list_.push_back(cp);
    // 协程启动
    scheduler_->ResumeCoroutine(cid);

    return 0;
}

int EasyRpc::RpcFunction(void* arg)
{
    if (NULL == arg)
    {
        LOG(ERROR)<<"null argument";
        return -1;
    }

    RPC_CONTEXT* rpc_ctx = static_cast<RPC_CONTEXT*>(arg);
    EasyRpc* the_handler = rpc_ctx->rpc_stub;
    if (NULL == the_handler || NULL == the_handler->net_ ||
        NULL == the_handler->scheduler_ || NULL == rpc_ctx->callback)
    {
        LOG(ERROR)<<"invalid rpc handler OR invalid callback";
        return -1;
    }

    std::string buf;
    MessageGenerator::MessageToBinary(0, asyncid_seed++,
            the_handler->request_, &buf);
    
    the_handler->net_->Send(buf.c_str(), buf.size());

    LOG(INFO)<<"rpc>>> send buf size:"<<buf.size()<<" to backend success";

    // Yield
    the_handler->scheduler_->YieldCoroutine();

    // has been Resumed, async callback
    rpc_ctx->callback(the_handler->response_);

    return 0;
}

void EasyRpc::RemoveByCoroutineId(int crid)
{
    CoroutineList::iterator it = crid_list_.begin();
    for (; it != crid_list_.end(); ++it)
    {
        if (it->crid == crid)
        {
            crid_list_.erase(it);
            return;
        }
    }
}

int EasyRpc::GetCoroutineIdxByAsyncId(uint64_t asyncid)
{
    CoroutineList::iterator it = crid_list_.begin();
    for (; it != crid_list_.end(); ++it)
    {
        if (it->asyncid == asyncid)
        {
            return it->crid;
        }
    }
    
    return -1;    
}

// 目前暂时认为每1个rpc request的reply是严格按顺序的，因此取队列最前面的。
// 可以认为目前是不可用状态，后续通过协议封装异步rpc call id
int EasyRpc::Resume(const char* buffer, size_t size)
{
    assert(scheduler_ != NULL);
    if (NULL == buffer || 0 == size)
    {
        return -1;
    }

    const ::google::protobuf::Message* reply =
        MessageGenerator::SharedMessage(buffer, size);
    if (NULL == reply)
    {
        return -1;
    }
    
    // Resume
    uint64_t asyncid = MessageGenerator::GetAsyncId();
    int crid = GetCoroutineIdxByAsyncId(asyncid);
    scheduler_->ResumeCoroutine(crid);
    if (!scheduler_->CoroutineBeenAlive(crid))
    
    response_ = reply;
    
    return 0;
}

}
