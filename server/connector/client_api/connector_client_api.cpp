#include "connector_client_api.h"
#include "../client.pb.h"

namespace hmoon_connector_api {

ConnectorClientApi::ConnectorClientApi()
{}

ConnectorClientApi::~ConnectorClientApi()
{}

// 设置一些参数，比如登录帐号、token、密码等
int ConnectorClientApi::Init()
{
    return 0;
}

// 释放资源
void ConnectorClientApi::CleanUp()
{}

// 发起服务连接
int ConnectorClientApi::Start()
{
    return 0;
}

// 停止服务连接
int ConnectorClientApi::Stop()
{
    return 0;
}

// 恢复服务连接
int ConnectorClientApi::Resume()
{
    return 0;
}

}
