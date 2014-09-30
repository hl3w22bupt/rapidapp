#include "connector_client_api.h"
#include "../client.pb.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

namespace hmoon_connector_api {

ConnectorClientProtocol::ConnectorClientProtocol() :
    protocol_event_listener_(NULL)
{}

ConnectorClientProtocol::~ConnectorClientProtocol()
{}

int ConnectorClientProtocol::Connect()
{
    return 0;
}

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
    return 0;
}

// 恢复服务
int ConnectorClientProtocol::Resume()
{
    return 0;
}

// 驱动异步事件
int ConnectorClientProtocol::Update()
{
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

///////////////////////////////////////////
//////////////  Thread ////////////////////
///////////////////////////////////////////
ConnectorClientProtocolThread::ConnectorClientProtocolThread() : ccproto_(NULL), exit_(false)
{}

ConnectorClientProtocolThread::~ConnectorClientProtocolThread()
{}

int ConnectorClientProtocolThread::MainLoop(IProtocolEventListener* protocol_evlistener)
{
    // 1. Start Connection
    // 2. Update Connection
    return 0;
}

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
