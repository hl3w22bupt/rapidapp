#include "connector_app.h"
#include <google/protobuf/message.h>

ConnectorApp::ConnectorApp()
{}

ConnectorApp::~ConnectorApp()
{}

int ConnectorApp::OnInit(IFrameWork* app_framework)
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

int ConnectorApp::OnFini()
{
    ::google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

int ConnectorApp::OnStop()
{
    return 0;
}
int ConnectorApp::OnResume()
{
    return 0;
}

int ConnectorApp::OnUpdate()
{
    return 0;
}
int ConnectorApp::OnReload()
{
    return 0;
}

int ConnectorApp::OnRecvCtrl(const char* msg)
{
    return 0;
}

int ConnectorApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }
    frame_stub_->SendToFrontEnd(net, msg, size);

    return 0;
}

int ConnectorApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    return 0;
}

int ConnectorApp::OnTimer(EasyTimer* timer, int timer_id)
{
    return 0;
}

int ConnectorApp::OnReportRundata()
{
    return 0;
}

size_t ConnectorApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t ConnectorApp::GetBackEndMaxMsgSize()
{
    return 0;
}

size_t ConnectorApp::GetFrontEndMsgLength(const char* buffer, size_t size)
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

size_t ConnectorApp::GetBackEndMsgLength(int type, const char* buffer, size_t size)
{
    if (NULL == buffer || 4 > size)
    {
        return 0;
    }

    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
}

const char* ConnectorApp::GetAppVersion()
{
    return "1.0.0";
}
