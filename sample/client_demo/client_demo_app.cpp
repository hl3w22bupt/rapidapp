#include "client_demo_app.h"
#include "../sample.pb.h"
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>

int PingPongCallBack(const ::google::protobuf::Message* req,
                     ::google::protobuf::Message* res)
{
    if (NULL == req || NULL == res)
    {
        return -1;
    }

    LOG(INFO)<<"rpc req:"<<req->DebugString();
    LOG(INFO)<<"rpc res:"<<res->DebugString();

    const test_rpc::Ping* ping = dynamic_cast<const test_rpc::Ping*>(req);
    test_rpc::Pong* pong = dynamic_cast<test_rpc::Pong*>(res);

    return 0;
}

ClientDemoApp::ClientDemoApp()
{}

ClientDemoApp::~ClientDemoApp()
{}

int ClientDemoApp::OnInit(IFrameWork* app_framework)
{
    if (NULL == app_framework)
    {
        LOG(ERROR)<<"null app framework";
        return -1;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    frame_stub_ = app_framework;
    app_framework->ScheduleUpdate();

    const char* uri = "tcp://127.0.0.1:7891";
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

int ClientDemoApp::OnFini()
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

int ClientDemoApp::OnStop()
{
    return 0;
}
int ClientDemoApp::OnResume()
{
    return 0;
}

int ClientDemoApp::OnUpdate()
{

#ifdef _DEBUG
    test_rpc::Ping* ping = new test_rpc::Ping();
    ping->set_ping(150);
    test_rpc::Pong* pong = new test_rpc::Pong();
    pong->set_pong(0);
    LOG(INFO)<<"start rpc call...";
    frame_stub_->RpcCall(rpc_, ping, pong, PingPongCallBack);
#endif

    frame_stub_->UnScheduleUpdate();

    return 0;
}

int ClientDemoApp::OnReload()
{
    LOG(INFO)<<"Reload happened";
    return 0;
}

int ClientDemoApp::OnRecvCtrl(int argc, char** argv)
{
    return 0;
}

int ClientDemoApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    static uint64_t msg_seq = 0;
    LOG(INFO)<<"recv app msg size:"<<size;

    // TODO parse msg from frontend

    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }
    frame_stub_->SendToFrontEnd(net, msg, size);

    return 0;
}

int ClientDemoApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    return 0;
}

int ClientDemoApp::OnTimer(EasyTimer* timer, int timer_id)
{
    return 0;
}

int ClientDemoApp::OnReportRundata()
{
    return 0;
}

size_t ClientDemoApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t ClientDemoApp::GetBackEndMaxMsgSize()
{
    return 0;
}

const char* ClientDemoApp::GetAppVersion()
{
    return "1.0.0";
}
