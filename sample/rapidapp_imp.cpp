#include "rapidapp_imp.h"

MyApp::MyApp()
{}

MyApp::~MyApp()
{}

int MyApp::OnInit()
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

int MyApp::OnRecvCtrl()
{
    return 0;
}

int MyApp::OnRecvFrontEnd(const char* msg, size_t size)
{
    return 0;
}
int MyApp::OnRecvBackEnd(const char* msg, size_t size)
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
