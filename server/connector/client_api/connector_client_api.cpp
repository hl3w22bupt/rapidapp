#include "connector_client_api.h"
#include "../client.pb.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>

// TODO 引用自封装的跨平台socket文件，封装成Socket类
// TODO 错误码统一
namespace hmoon_connector_api {

////////////////////////////////////////////////
////////////// Connector Protocol //////////////
////////////////////////////////////////////////
ConnectorClientProtocol::ConnectorClientProtocol() :
    protocol_event_listener_(NULL)
{}

ConnectorClientProtocol::~ConnectorClientProtocol()
{}

int ConnectorClientProtocol::Connect()
{
    assert(protocol_event_listener_ != NULL);

    return 0;
}

void ConnectorClientProtocol::Close()
{}

int ConnectorClientProtocol::Start(IProtocolEventListener* protocol_evlistener)
{
    if (NULL == protocol_evlistener)
    {
        return -1;
    }
    protocol_event_listener_ = protocol_evlistener;

    // 设置一些参数，比如登录帐号、token、密码等
    protocol_event_listener_->OnGetSettings(appid_, openid_, token_,
                                            encrypt_mode_, auth_type_,
                                            server_ip_, server_port_);
    // 发起服务连接
    int ret = Connect();
    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

// 停止服务
int ConnectorClientProtocol::Terminate()
{
    // 关闭连接之前把TCP socket接收缓冲区数据收完，避免触发RST到服务端
    Close();

    return 0;
}

// 恢复服务
int ConnectorClientProtocol::Resume()
{
    if (NULL == protocol_event_listener_)
    {
        return -1;
    }

    // 准备至RESUME状态
    // 发起连接
    int ret = Connect();
    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

// 驱动异步事件
int ConnectorClientProtocol::Update()
{
    // 驱动状态机
    return 0;
}

// 发送消息
int ConnectorClientProtocol::PushMessage()
{
    return 0;
}

// 接收消息
int ConnectorClientProtocol::PopMessage()
{
    return 0;
}

///////////////////////////////////////////////////////////////////
////////////// Connector Protocol With Single Thread //////////////
///////////////////////////////////////////////////////////////////
ConnectorClientProtocolThread::ConnectorClientProtocolThread() : ccproto_(NULL), exit_(false)
{}

ConnectorClientProtocolThread::~ConnectorClientProtocolThread()
{}

int ConnectorClientProtocolThread::RunWithThread(IProtocolEventListener* protocol_evlistener)
{
    if (NULL == protocol_evlistener)
    {
        return -1;
    }

    ccproto_ = ConnectorClientProtocol::Create();
    boost::thread protocol_thread(boost::bind(&ConnectorClientProtocolThread::MainLoop,
                                              this, protocol_evlistener));
    protocol_thread.detach();

    return 0;
}

int ConnectorClientProtocolThread::TerminateThread()
{
    exit_ = true;
    return 0;
}

int ConnectorClientProtocolThread::MainLoop(IProtocolEventListener* protocol_evlistener)
{
    assert(ccproto_ != NULL);
    assert(!exit_);
    // 1. Start Connection
    int ret = ccproto_->Start(protocol_evlistener);
    if (ret != 0)
    {
        return -1;
    }

    // 2. Update Connection
    while (!exit_)
    {
        ccproto_->Update();
    }

    return 0;
}

int ConnectorClientProtocolThread::PushMessageToSendQ()
{
    return 0;
}

int ConnectorClientProtocolThread::PopMessageFromRecvQ()
{
    return 0;
}

}
