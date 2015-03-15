#include "connector_server_demo.h"
#include <google/protobuf/message.h>

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

int ServerDemoApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }
    frame_stub_->SendToFrontEnd(net, msg, size);

    return 0;
}

int ServerDemoApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
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
