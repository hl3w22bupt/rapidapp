#include "server_demo_app.h"
#include "../sample.pb.h"
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>

PingPongService::PingPongService()
{}

PingPongService::~PingPongService()
{}

const std::string
PingPongService::RpcRequestName()
{
    return test_rpc::Ping::default_instance().GetTypeName();
}

::google::protobuf::Message*
PingPongService::NewResponse()
{
    return new(std::nothrow) test_rpc::Pong();
}

int
PingPongService::OnRpcCall(const ::google::protobuf::Message* req,
                            ::google::protobuf::Message* resp)
{
    if (NULL == req || NULL == resp)
    {
        LOG(ERROR)<"assert failed, OnRpcCall arguments is null";
        return -1;
    }

    LOG(INFO)<<"recved message ["<<req->GetTypeName()<<"]";

    const test_rpc::Ping* ping = dynamic_cast<const test_rpc::Ping*>(req);
    test_rpc::Pong* pong = dynamic_cast<test_rpc::Pong*>(resp);
    pong->set_pong(ping->ping() + 1);

    return 0;
}

ServerDemoApp::ServerDemoApp()
{}

ServerDemoApp::~ServerDemoApp()
{}

int ServerDemoApp::OnInit(IFrameWork* app_framework)
{
    if (NULL == app_framework)
    {
        LOG(ERROR)<<"null app framework";
        return -1;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    frame_stub_ = app_framework;

    static PingPongService pp_svc;
    frame_stub_->RegisterRpcService(&pp_svc);

    return 0;
}

int ServerDemoApp::OnFini()
{
    ::google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

int ServerDemoApp::OnStop()
{
    return 0;
}
int ServerDemoApp::OnResume()
{
    return 0;
}

int ServerDemoApp::OnUpdate()
{
    return 0;
}
int ServerDemoApp::OnReload()
{
    return 0;
}

int ServerDemoApp::OnRecvCtrl(int argc, char** argv)
{
    return 0;
}

int ServerDemoApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    return 0;
}

// RegisterRpcService之后，OnRpc不会被触发，后续会干掉OnRpc这种模式
int ServerDemoApp::OnRpc(const ::google::protobuf::Message* request,
                         ::google::protobuf::Message** response)
{
    if (NULL == request || NULL == response)
    {
        return -1;
    }

    *response = NULL;

    const std::string& msg_name = request->GetTypeName();

    LOG(INFO)<<"recved msg[name:"<<msg_name<<"] in the way rpc";

#ifdef _DEBUG
    test_rpc::Pong* pong = new(std::nothrow) test_rpc::Pong();
    pong->set_pong(151);
    *response = pong;
#endif

    return 0;
}

int ServerDemoApp::OnTimer(EasyTimer* timer, int timer_id)
{
    return 0;
}

int ServerDemoApp::OnReportRundata()
{
    return 0;
}

size_t ServerDemoApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t ServerDemoApp::GetBackEndMaxMsgSize()
{
    return 0;
}

const char* ServerDemoApp::GetAppVersion()
{
    return "1.0.0";
}
