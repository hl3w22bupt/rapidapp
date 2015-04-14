#include "rapidapp_easy_rpc.h"
#include "rapidapp_easy_net.h"
#include "rapidapp_message_reflection.h"
#include <cassert>
#include <glog/logging.h>

namespace rapidapp {

EasyRpcClosure::EasyRpcClosure() : user_data_(NULL), req_(NULL), rsp_(NULL), net_(NULL), is_done_(false)
{}

EasyRpcClosure::~EasyRpcClosure()
{}

int EasyRpcClosure::Set(EasyNet* net,
                        ::google::protobuf::Message* req,
                        ::google::protobuf::Message* rsp)
{
    if (NULL == net || NULL == req || NULL == rsp)
    {
        return -1;
    }

    net_ = net;
    req_ = req;
    rsp_ = rsp;

    return 0;
}

void EasyRpcClosure::Done()
{
    LOG(INFO)<<"EasyRpc Done";

    if (net_ != NULL && rsp_ != NULL && req_ != NULL)
    {
        std::string rsp_out;
        int ret = MessageGenerator::MessageToBinary(0, 0, rsp_, &rsp_out);
        if (0 == ret)
        {
            LOG(INFO)<<"send to rpc client";
            net_->Send(rsp_out.c_str(), rsp_out.size());
        }
        else
        {
            LOG(ERROR)<<"MessageToBinary failed";
        }

        delete rsp_;
        rsp_ = NULL;

        MessageGenerator::ReleaseMessage(req_);
        req_ = NULL;
    }

    is_done_ = true;
    delete this;
}

bool EasyRpcClosure::IsDone()
{
    return is_done_;
}

void EasyRpcClosure::set_userdata(void* data)
{
    user_data_ = data;
}

void* EasyRpcClosure::userdata() const
{
    return user_data_;
}

::google::protobuf::Message* EasyRpcClosure::request()
{
    return req_;
}

::google::protobuf::Message* EasyRpcClosure::response()
{
    return rsp_;
}

// EasyRpc
//
//
typedef struct RpcCallContext {
    ON_RPC_REPLY_FUNCTION callback;
    EasyRpc* rpc_stub;
    const ::google::protobuf::Message* request;
    ::google::protobuf::Message* response;
    void* arg;
} RPC_CONTEXT;

uint64_t EasyRpc::asyncid_seed = 0;

EasyRpc::EasyRpc() : scheduler_(NULL), net_(NULL), crid_list_()
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
                     ::google::protobuf::Message* response,
                     ON_RPC_REPLY_FUNCTION callback,
                     void* arg)
{
    if (NULL == request || NULL == callback)
    {
        return -1;
    }

    if (NULL == scheduler_)
    {
        return -1;
    }

    // 创建一个协程上下文
    RPC_CONTEXT* context  = new(std::nothrow) RPC_CONTEXT();
    if (NULL == context)
    {
        LOG(ERROR)<<"new async rpc context failed. net:"<<net_->uri();
        return -1;
    }

    context->request = request;
    context->response = response;
    context->rpc_stub = this;
    context->callback = callback;
    context->arg = arg;
    int cid = scheduler_->CreateCoroutine(RpcFunction, context);
    if (cid < 0)
    {
        LOG(ERROR)<<"create coroutine failed return "<<cid<<", net:"<<net_->uri();
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

    static std::string buf;
    MessageGenerator::MessageToBinary(0, asyncid_seed++,
            rpc_ctx->request, &buf);

    int ret = the_handler->net_->Send(buf.c_str(), buf.size());
    if (ret != EASY_NET_OK)
    {
        LOG(ERROR)<<"send to rpc net:"<<the_handler->net_->uri()<<" failed. return "<<ret;
    }

    LOG(INFO)<<"<<<rpc>>> send buf size:"<<buf.size()<<" to backend success";

    // Yield
    the_handler->scheduler_->YieldCoroutine();

    // has been Resumed, async callback
    assert(rpc_ctx->request != NULL);
    assert(rpc_ctx->response != NULL);

    int status = 0;
    const std::string& msg_bin = MessageGenerator::GetBinaryString();
    if (!rpc_ctx->response->ParseFromString(msg_bin))
    {
        LOG(ERROR)<<"ParseFromString[size:%d] failed"<<msg_bin.size();
        status = -1;
    }
    else
    {
        LOG(INFO)<<"recv msg:\n"<<rpc_ctx->response->DebugString();
    }
    rpc_ctx->callback(rpc_ctx->request,
                      rpc_ctx->response,
                      rpc_ctx->arg, status);

    delete rpc_ctx;

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

    /*
    const ::google::protobuf::Message* reply =
        MessageGenerator::SharedMessage(buffer, size);
    if (NULL == reply)
    {
        return -1;
    }
    */
    if (MessageGenerator::UnpackToRpcMsg(buffer, size) != 0)
    {
        LOG(ERROR)<<"unpack to rpc msg failed after Resume";
        return -1;
    }

    // Resume
    uint64_t asyncid = MessageGenerator::GetAsyncId();
    int crid = GetCoroutineIdxByAsyncId(asyncid);
    if (scheduler_->CoroutineBeenAlive(crid))
    {
        scheduler_->ResumeCoroutine(crid);   // 唤醒协程
    }

    if (!scheduler_->CoroutineBeenAlive(crid))
    {
        RemoveByCoroutineId(crid);
    }

    return 0;
}

}
