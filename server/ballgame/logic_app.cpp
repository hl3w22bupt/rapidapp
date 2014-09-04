#include "logic_app.h"
#include "ball_game_cs.pb.h"
#include <google/protobuf/message.h>

LogicApp::LogicApp()
{}

LogicApp::~LogicApp()
{}

int LogicApp::OnInit(IFrameWork* app_framework)
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

int LogicApp::OnFini()
{
    ::google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

int LogicApp::OnStop()
{
    return 0;
}
int LogicApp::OnResume()
{
    return 0;
}

int LogicApp::OnUpdate()
{
    return 0;
}
int LogicApp::OnReload()
{
    return 0;
}

int LogicApp::OnRecvCtrl(int argc, char** argv)
{
    return 0;
}

int LogicApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }
    frame_stub_->SendToFrontEnd(net, msg, size);

    return 0;
}

int LogicApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    return 0;
}

int LogicApp::OnTimer(EasyTimer* timer, int timer_id)
{
    return 0;
}

int LogicApp::OnReportRundata()
{
    return 0;
}

size_t LogicApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t LogicApp::GetBackEndMaxMsgSize()
{
    return 0;
}

const char* LogicApp::GetAppVersion()
{
    return "1.0.0";
}
