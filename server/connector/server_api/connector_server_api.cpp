#include "connector_server_api.h"
#include "../server.pb.h"


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

    conn_listener_ = conn_listener;
    return 0;
}

int ConnectorServerApi::Dispatch(void* from_net, const char* data, size_t len)
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
    const connector_server::SSHead& head = msg_from_conn.head();
    const connector_server::Tuple& session = head.session();
    switch (head.bodyid())
    {
        case connector_server::SYN:
            {
                conn_listener_->OnConnStart(from_net, session.fd(), session.nid());
                break;
            }
        case connector_server::FIN:
            {
                conn_listener_->OnConnStop(from_net, session.fd(), session.nid(), session.sid());
                break;
            }
        case connector_server::RSM:
            {
                conn_listener_->OnConnResume(from_net, session.fd(), session.nid(), session.sid());
                break;
            }
        case connector_server::DATA:
            {
                conn_listener_->OnData(from_net, session.fd(), session.nid(), session.sid(),
                                       msg_from_conn.body().data().data().c_str(),
                                       msg_from_conn.body().data().data().size());
                break;
            }
    }
    return 0;
}

void ConnectorServerApi::CleanUp()
{}

int ConnectorServerApi::StopConn(void* net, uint32_t fd, uint64_t nid, uint32_t sid)
{
    assert(net != NULL);
    assert(conn_listener_ != NULL);

    static connector_server::SSMsg stop_to_conn;

    stop_to_conn.mutable_head()->set_magic(connector_server::MAGIC_SS_V1);
    stop_to_conn.mutable_head()->set_sequence(0);
    stop_to_conn.mutable_head()->set_bodyid(connector_server::FIN);
    stop_to_conn.mutable_head()->mutable_session()->set_fd(fd);
    stop_to_conn.mutable_head()->mutable_session()->set_nid(nid);
    stop_to_conn.mutable_head()->mutable_session()->set_sid(sid);
    stop_to_conn.mutable_body()->mutable_fin()->set_result(0);

    std::string buf;
    stop_to_conn.SerializeToString(&buf);

    return conn_listener_->SendToConn(net, buf.c_str(), buf.size());
}

int ConnectorServerApi::HandshakeToConn(void* net, uint32_t fd, uint64_t nid, uint32_t sid)
{
    assert(net != NULL);
    assert(conn_listener_ != NULL);
    static connector_server::SSMsg ack_to_conn;
    ack_to_conn.mutable_head()->set_magic(connector_server::MAGIC_SS_V1);
    ack_to_conn.mutable_head()->set_sequence(0);
    ack_to_conn.mutable_head()->set_bodyid(connector_server::ACK);
    ack_to_conn.mutable_head()->mutable_session()->set_fd(fd);
    ack_to_conn.mutable_head()->mutable_session()->set_nid(nid);
    ack_to_conn.mutable_head()->mutable_session()->set_sid(sid);
    ack_to_conn.mutable_body()->mutable_ack()->set_result(0);

    std::string buf;
    ack_to_conn.SerializeToString(&buf);

    return conn_listener_->SendToConn(net, buf.c_str(), buf.size());
}

int ConnectorServerApi::SendDataToConn(void* net, uint32_t fd, uint64_t nid, uint32_t sid,
                                       const char* data, size_t len)
{
    assert(net != NULL);
    assert(conn_listener_ != NULL);
    static connector_server::SSMsg data_to_conn;
    data_to_conn.mutable_head()->set_magic(connector_server::MAGIC_SS_V1);
    data_to_conn.mutable_head()->set_sequence(0);
    data_to_conn.mutable_head()->set_bodyid(connector_server::DATA);
    data_to_conn.mutable_head()->mutable_session()->set_fd(fd);
    data_to_conn.mutable_head()->mutable_session()->set_nid(nid);
    data_to_conn.mutable_head()->mutable_session()->set_sid(sid);
    data_to_conn.mutable_body()->mutable_data()->set_data(data, len);

    std::string buf;
    data_to_conn.SerializeToString(&buf);

    return conn_listener_->SendToConn(net, buf.c_str(), buf.size());
}

}
