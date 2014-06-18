#include "rapidapp_imp.h"

MyApp::MyApp()
{}

MyApp::~MyApp()
{}

int MyApp::OnInit(IFrameWork* app_framework)
{
    return 0;
}

int MyApp::OnFini()
{
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
    LOG(INFO)<<"recv app msg size:"<<size;
    return 0;
}

int MyApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
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

const char* MyApp::GetAppVersion()
{
    return "1.0.0";
}
