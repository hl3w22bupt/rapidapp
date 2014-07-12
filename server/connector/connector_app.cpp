/*
 * connector接入服务器。
 * connector主要功能：
 * 1. 连接管理
 * 2. 广播
 * 3. 串行汇聚
 * 4. 过载保护
 * 5. 断线重连
 * 6. 无状态，方便平行扩展
 *
 * */
#include "connector_app.h"
#include <google/protobuf/message.h>

ConnectorApp::ConnectorApp() : frame_stub_(NULL)
{}

ConnectorApp::~ConnectorApp()
{}

int ConnectorApp::OnInit(IFrameWork* app_framework)
{
    if (NULL == app_framework)
    {
        LOG(ERROR)<<"null app framework, assert failed";
        return -1;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    frame_stub_ = app_framework;

    // 连接的上下文池，状态机驱动
    int ret = conn_session_mgr_.Init();
    if (ret != 0)
    {
        LOG(ERROR)<<"connector session mgr initialized failed";
        return -1;
    }

    // TODO 创建后端连接
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

    ConnectorSession* session = conn_session_mgr_.CreateInstance(net);
    if (NULL == session)
    {
        LOG(ERROR)<<"create session instance failed";
        return -1;
    }

    // TODO 根据路由规则，转发

    return 0;
}

int ConnectorApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    // TODO 根据后端回调，转发
    return 0;
}

int ConnectorApp::OnTimer(EasyTimer* timer, int timer_id)
{
    // TODO 空闲连接检查
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

    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
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
