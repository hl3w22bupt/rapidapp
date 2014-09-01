#include "rapidapp_easy_rpc.h"
#include "rapidapp_easy_net.h"
#include <cassert>
#include <glog/logging.h>

namespace rapidapp {

typedef struct RpcCallContext {
    ON_RPC_REPLY_FUNCTION callback;
    EasyRpc* rpc_stub;
} RPC_CONTEXT;

EasyRpc::EasyRpc() : scheduler_(NULL), net_(NULL),
                    request_(NULL), request_size_(0),
                    response_(NULL), response_size_(0), crid_list_()
{}

EasyRpc::~EasyRpc()
{
    while (crid_list_.size() > 0)
    {
        int crid = crid_list_.front();
        scheduler_->DestroyCoroutine(crid);
        crid_list_.pop();
    }
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

int EasyRpc::RpcCall(const void* request, size_t request_size,
                     ON_RPC_REPLY_FUNCTION callback)
{
    if (NULL == request || 0 == request_size ||
        NULL == callback)
    {
        return -1;
    }

    if (NULL == scheduler_)
    {
        return -1;
    }

    request_ = request;
    request_size_ = request_size;

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

    crid_list_.push(cid);
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

    the_handler->net_->Send(static_cast<const char*>(the_handler->request_),
                            the_handler->request_size_);

    LOG(INFO)<<"rpc>>> send buf size:"<<
        the_handler->request_size_<<" to backend success";

    // Yield
    the_handler->scheduler_->YieldCoroutine();

    // has been Resumed, async callback
    rpc_ctx->callback(the_handler->response_, the_handler->response_size_);

    return 0;
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

    response_ = buffer;
    response_size_ = size;
    // Resume
    int crid = crid_list_.front();
    crid_list_.pop();
    scheduler_->ResumeCoroutine(crid);
    return 0;
}

}
