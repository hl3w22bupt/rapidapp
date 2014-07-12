#include "rapidapp_imp.h"
#include "sample.pb.h"
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>

MyApp::MyApp()
{}

MyApp::~MyApp()
{}

int MyApp::OnInit(IFrameWork* app_framework)
{
    if (NULL == app_framework)
    {
        LOG(ERROR)<<"null app framework";
        return -1;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    frame_stub_ = app_framework;

    const char* uri = "tcp:127.0.0.1:8081";
    EasyNet* net = frame_stub_->CreateBackEnd(uri, 0);
    if (NULL == net)
    {
        LOG(ERROR)<<"CreateBackEnd "<<uri<<" failed";
        return -1;
    }
    backend_ = net;

    // 一个transport 对应 一个protocol
    // 因而这里一个后端服务对应一个rpc instance
    rpc_ = frame_stub_->CreateRpc(net);
    if (NULL == rpc_)
    {
        LOG(ERROR)<<"create rpc instace failed";
        return -1;
    }

    return 0;
}

int MyApp::OnFini()
{
    if (rpc_ != NULL)
    {
        frame_stub_->DestroyRpc(&rpc_);
        rpc_ = NULL;
    }

    if (backend_ != NULL)
    {
        frame_stub_->DestroyBackEnd(&backend_);
        backend_ = NULL;
    }

    ::google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

int MyApp::OnStop()
{
    return 0;
}
int MyApp::OnResume()
{
    return 0;
}

int MyApp::OnUpdate()
{
    return 0;
}
int MyApp::OnReload()
{
    return 0;
}

int MyApp::OnRecvCtrl(const char* msg)
{
    return 0;
}

int MyApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    static uint64_t msg_seq = 0;
    LOG(INFO)<<"recv app msg size:"<<size;

    // TODO parse msg from frontend

    rapidapp_sample::Mesg resp;
    resp.mutable_body()->set_seq(++msg_seq);
    resp.mutable_body()->set_echo("rapidapp sample");
    // 先保证调用set msglen，同时保证msglen定义为fixed固定大小，以计算出正确的序列化后大小
    resp.set_msglen(0);
    resp.set_msglen(resp.ByteSize());

    LOG(INFO)<<"resp to frontend:"<<std::endl<<resp.DebugString();

    // protobuf测试代码，后续删除
    std::string out;
    ::google::protobuf::TextFormat::PrintToString(resp, &out);
    LOG(INFO)<<"text format to frontend:"<<std::endl<<out;
    rapidapp_sample::Mesg resp_bak;
    ::google::protobuf::TextFormat::ParseFromString(out, &resp_bak);
    LOG(INFO)<<"resp bak"<<std::endl<<resp_bak.DebugString();

    std::string resp_str;
    resp.SerializeToString(&resp_str);

    //rapidapp_sample::Mesg resp_unpack;
    //resp_unpack.ParseFromString(resp_str);

    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }
    frame_stub_->SendToFrontEnd(net, resp_str.c_str(), resp.ByteSize());

    /*
    const void* resp_buffer = NULL;
    size_t resp_size = 0;
    LOG(INFO)<<"start rpc call...";
    frame_stub_->RpcCall(rpc_, resp_str.c_str(), resp.ByteSize(),
                         &resp_buffer, &resp_size);
    if (NULL == resp_buffer || 0 == resp_size)
    {
        PLOG(ERROR)<<"resp is null";
        return -1;
    }
    rapidapp_sample::Mesg backend_resp;
    backend_resp.ParseFromArray(resp_buffer, resp_size);
    LOG(INFO)<<"RPC resp from backend:"<<std::endl<<backend_resp.DebugString();
    */

    frame_stub_->SendToBackEnd(backend_, resp_str.c_str(), resp.ByteSize());
    return 0;
}

int MyApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    rapidapp_sample::Mesg resp;
    resp.ParseFromArray(msg, size);

    LOG(INFO)<<"resp from backend:"<<std::endl<<resp.DebugString();
    return 0;
}

int MyApp::OnTimer(EasyTimer* timer, int timer_id)
{
    return 0;
}

int MyApp::OnReportRundata()
{
    return 0;
}

size_t MyApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t MyApp::GetBackEndMaxMsgSize()
{
    return 0;
}

size_t MyApp::GetFrontEndMsgLength(const char* buffer, size_t size)
{
    if (NULL == buffer || 4 > size)
    {
        return 0;
    }

#ifndef _DEBUG
    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
#else
    return size;
#endif
}

size_t MyApp::GetBackEndMsgLength(int type, const char* buffer, size_t size)
{
    if (NULL == buffer || 4 > size)
    {
        return 0;
    }

#ifndef _DEBUG
    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
#else
    return size;
#endif
}

const char* MyApp::GetAppVersion()
{
    return "1.0.0";
}
