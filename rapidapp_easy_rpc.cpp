#include "rapidapp_easy_rpc.h"
#include "rapidapp_easy_net.h"
#include <cassert>
#include <glog/logging.h>

namespace rapidapp {

EasyRpc::EasyRpc() : scheduler_(NULL), net_(NULL),
                    request_(NULL), request_size_(0),
                    response_(NULL), response_size_(NULL), crid_list_()
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
        return -1;
    }

    net_ = net;
    scheduler_ = scheduler;

    return 0;
}

int EasyRpc::RpcCall(const void* request, size_t request_size,
                     const void** response, size_t* response_size)
{
    if (NULL == request || 0 == response_size ||
        NULL == response || NULL == response_size)
    {
        return -1;
    }

    if (NULL == scheduler_)
    {
        return -1;
    }

    request_ = request;
    response_size_ = response_size;
    response_ = response;
    response_size_ = response_size;

    // 创建一个协程上下文
    int cid = scheduler_->CreateCoroutine(RpcFunction, this);
    if (cid < 0)
    {
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
        return -1;
    }

    EasyRpc* the_handler = static_cast<EasyRpc*>(arg);
    if (NULL == the_handler->net_ ||
        NULL == the_handler->scheduler_)
    {
        return -1;
    }

    the_handler->net_->Send(static_cast<const char*>(the_handler->request_),
                            the_handler->request_size_);

    LOG(INFO)<<"rpc>>> send buf size:"<<
        the_handler->request_size_<<" to backend success";

    // Yield
    the_handler->scheduler_->YieldCoroutine();

    return 0;
}

int EasyRpc::Resume(const char* buffer, size_t size)
{
    assert(scheduler_ != NULL);
    if (NULL == response_ || NULL == response_size_)
    {
        return -1;
    }

    *response_ = buffer;
    *response_size_ = size;
    // Resume
    int crid = crid_list_.front();
    crid_list_.pop();
    scheduler_->ResumeCoroutine(crid);
    return 0;
}

}
