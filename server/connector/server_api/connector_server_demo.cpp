#include "connector_server_demo.h"
#include <google/protobuf/message.h>

ServerDemoApp::ServerDemoApp() : frame_stub_(NULL), sconn_api_()
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

    sconn_api_.Init(this);

    return 0;
}

int ServerDemoApp::OnFini()
{
    sconn_api_.CleanUp();

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

    int ret = sconn_api_.Dispatch(net, msg, size);
    if (ret != 0)
    {
        LOG(ERROR)<<"dispatch from sconnapi failed, return "<<ret;
        return -1;
    }

    //frame_stub_->SendToFrontEnd(net, msg, size);

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


int ServerDemoApp::OnConnStart(void* net, uint32_t fd, uint64_t nid)
{
    // 如果是有状态服务，需要为每个连接分配一个seesion id
    // 如果是无状态服务，直接把session id设置为-1
    assert(net != NULL);
    return sconn_api_.HandshakeToConn(net, fd, nid, -1);
}

int ServerDemoApp::OnConnStop(void* net, uint32_t fd, uint64_t nid, uint32_t sid)
{
    return 0;
}

int ServerDemoApp::OnConnResume(void* net, uint32_t fd, uint64_t nid, uint32_t sid)
{
    return 0;
}

int ServerDemoApp::OnData(void* net, uint32_t fd, uint64_t nid, uint32_t sid,
                          const char* data, size_t len)
{
    assert(net != NULL);
    return sconn_api_.SendDataToConn(net, fd, nid, sid, data, len);
}

int ServerDemoApp::SendToConn(void* net, const char* data, size_t len)
{
    assert(frame_stub_ != NULL);
    return frame_stub_->SendToFrontEnd(static_cast<EasyNet*>(net), data, len);
}
