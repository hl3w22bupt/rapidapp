#include "connector_server_api.h"

namespace hmoon_connector_api {

ConnectorServerApi::ConnectorServerApi() : conn_listener_(NULL)
{}

ConnectorServerApi::~ConnectorServerApi()
{}

int ConnectorServerApi::Init(IConnListener* conn_listener)
{
    if (NULL == conn_listener)
    {
        return -1;
    }

    return 0;
}

void ConnectorServerApi::CleanUp()
{}

int ConnectorServerApi::StopConn(uint32_t fd, uint64_t nid, uint32_t sid)
{
    return 0;
}

}
