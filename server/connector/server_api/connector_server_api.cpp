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

int ConnectorServerApi::Dispatch(const char* data, size_t len)
{
    if (NULL == data || 0 == len)
    {
        return -1;
    }

    static connector_server::SSMsg msg_from_conn;
    if (!msg_from_conn.ParseFromArray(data, len))
    {
        return -1;
    }

    assert(conn_listener_ != NULL);

    switch (msg_from_conn.head().bodyid())
    {
        case connector_server::SYN:
            {
                conn_listener_->OnConnStart();
                break;
            }
        case connector_server::FIN:
            {
                conn_listener_->OnConnStop();
                break;
            }
        case connector_server::RSM:
            {
                conn_listener_->OnConnResume();
                break;
            }
        case connector_server::DATA:
            {
                conn_listener_->OnData();
                break;
            }
    }
    return 0;
}

void ConnectorServerApi::CleanUp()
{}

int ConnectorServerApi::StopConn(uint32_t fd, uint64_t nid, uint32_t sid)
{
    static connector_server::SSMsg stop_to_conn;

    stop_to_conn.mutable_head()->set_magic(connector_server::MAGIC_SS_V1);
    stop_to_conn.mutable_head()->set_sequence(0);
    stop_to_conn.mutable_head()->set_bodyid(connector_server::FIN);
    stop_to_conn.mutable_head()->mutable_session()->set_fd(fd);
    stop_to_conn.mutable_head()->mutable_session()->set_nid(nid);
    stop_to_conn.mutable_head()->mutable_session()->set_sid(sid);

    std::string buf;
    stop_to_conn.SerializeToString(&buf);

    assert(conn_listener_ != NULL);
    return conn_listener_->SendToConn(buf.c_str(), buf.size());
}

}
