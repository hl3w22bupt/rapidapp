#include "connector_client_api.h"
#include "../client.pb.h"
#include <iostream>

namespace hmoon_connector_api {

IProtocolEventListener::IProtocolEventListener()
{}

IProtocolEventListener::~IProtocolEventListener()
{}

// 设置一些参数，比如登录帐号、token、密码等
int IProtocolEventListener::Init()
{
    return 0;
}

// 释放资源
void IProtocolEventListener::CleanUp()
{
}

// 发起服务连接
int IProtocolEventListener::Start()
{
    return 0;
}

// 停止服务连接
int IProtocolEventListener::Stop()
{
    return 0;
}

// 恢复服务连接
int IProtocolEventListener::Resume()
{
    return 0;
}

}
