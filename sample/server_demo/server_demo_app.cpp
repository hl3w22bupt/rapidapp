#include "server_demo_app.h"
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

    LOG(INFO)<<"req:"<<req->DebugString();
    LOG(INFO)<<"res:"<<res->DebugString();

    const test_rpc::Ping* ping = dynamic_cast<const test_rpc::Ping*>(req);
    test_rpc::Pong* pong = dynamic_cast<test_rpc::Pong*>(res);

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

int ServerDemoApp::OnRpc(const ::google::protobuf::Message* request,
                         ::google::protobuf::Message** response)
{
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
