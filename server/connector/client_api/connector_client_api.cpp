#include "connector_client_api.h"
#include "../client.pb.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

namespace hmoon_connector_api {

ConnectorClientProtocol::ConnectorClientProtocol()
{}

//ConnectorClientProtocol::~ConnectorClientProtocol()
//{}

int ConnectorClientProtocol::Run(IProtocolEventListener* protocol_evlistener)
{
    if (NULL == protocol_evlistener)
    {
        return -1;
    }
    protocol_event_listener_ = protocol_evlistener;

    // 设置一些参数，比如登录帐号、token、密码等
    // 发起服务连接
    return 0;
}

int ConnectorClientProtocol::RunWithThread(IProtocolEventListener* protocol_evlistener)
{
    if (NULL == protocol_evlistener)
    {
        return -1;
    }

    boost::thread protocol_thread(boost::bind(&ConnectorClientProtocol::Run,
                                              this, protocol_evlistener));
    protocol_thread.detach();

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

}
