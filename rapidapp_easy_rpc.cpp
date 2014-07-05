#include "rapidapp_easy_rpc.h"
#include "rapidapp_easy_net.h"
#include <cassert>

namespace rapidapp {

EasyRpc::EasyRpc() : scheduler_(NULL), net_(NULL), msg_handler_(NULL),
                    request_(NULL), response_(NULL), cid_(-1)
{}

EasyRpc::~EasyRpc()
{
    if (cid_ >= 0 && scheduler_ != NULL)
    {
        scheduler_->DestroyCoroutine(cid_);
    }
}

int EasyRpc::Init(magic_cube::CoroutineScheduler* scheduler,
                  EasyNet* net, IMsgHandler* msg_handler)
{
    if (NULL == net || NULL == msg_handler ||
        NULL == scheduler)
    {
        return -1;
    }

    net_ = net;
    msg_handler_ = msg_handler;
    scheduler_ = scheduler;

    return 0;
}

int EasyRpc::RpcCall(const void* request, void* response)
{
    if (NULL == request || NULL == response)
    {
        return -1;
    }

    if (NULL == scheduler_)
    {
        return -1;
    }

    request_ = request;
    response_ = response;

    // 创建一个协程上下文
    int cid = scheduler_->CreateCoroutine(RpcFunction, this);
    if (cid < 0)
    {
        return -1;
    }

    cid_ = cid;
    // 协程启动
    scheduler_->ResumeCoroutine(cid_);

    return 0;
}

int EasyRpc::RpcFunction(void* arg)
{
    if (NULL == arg)
    {
        return -1;
    }

    EasyRpc* the_handler = static_cast<EasyRpc*>(arg);
    if (NULL == the_handler->net_ || NULL == the_handler->msg_handler_ ||
        NULL == the_handler->scheduler_)
    {
        return -1;
    }

    // TODO
    static char buff[1024];
    size_t size = sizeof(buff);
    the_handler->msg_handler_->HandleRequest(the_handler->request_, buff, &size);
    the_handler->net_->Send(buff, size);
    // Yield
    the_handler->scheduler_->YieldCoroutine();

    return 0;
}

int EasyRpc::Resume(const char* buffer, size_t size)
{
    assert(msg_handler_ != NULL && scheduler_ != NULL);
    // TODO
    msg_handler_->HandleResponse(buffer, size, response_);
    // Resume
    scheduler_->ResumeCoroutine(cid_);
    return 0;
}

}
