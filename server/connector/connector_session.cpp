#include "connector_session.h"
#include <glog/logging.h>

const int kDefaultOverloadLimit = 1024;

ConnectorSession::ConnectorSession() :
    state_(STATE_INIT), net_stub_(NULL), channel_id_(-1), sid_(-1)
{}

ConnectorSession::~ConnectorSession()
{
    CleanUp();
}

int ConnectorSession::Init(EasyNet* net)
{
    if (NULL == net)
    {
        return -1;
    }

    net_stub_ = net;
    state_ = STATE_INIT;
    return 0;
}

void ConnectorSession::CleanUp()
{
    state_ = STATE_INIT;
}

int ConnectorSession::DriveStateMachine()
{
    switch(state_)
    {
        case STATE_INIT:
            {
                return HandleKeyMaking();
                break;
            }
        case STATE_AUTH:
            {
                return HandleAuthRequest();
                break;
            }
        case STATE_SYNING:
            {
                return HandShake_StartSession();
                break;
            }
        case STATE_SYNACK:
            {
                return HandShake_OnStartAcked();
                break;
            }
        case STATE_OK:
            {
                return 0;
                break;
            }
        default:
            return -1;
    }
}

int ConnectorSession::SerializeAndSendToFrontEnd(const connector_client::CSMsg& msg)
{
    LOG(INFO)<<"msg handshake to client:"<<std::endl<<msg.DebugString();

    std::string bin_to_send;
    msg.SerializeToString(&bin_to_send);

    assert(net_stub_ != NULL);
    if (net_stub_->Send(bin_to_send.c_str(), bin_to_send.size()) != EASY_NET_OK)
    {
        return -1;
    }

    return 0;
}

/* TODO 目前密钥生成固定硬编码，没有发往第三方账号系统的鉴权申请。
 * 同时并没有针对当前状态机状态检查client侧发起的协议的合法性
 * 后面随着功能完善再补充，早前先简单搭建起一个骨架*/
int ConnectorSession::HandleKeyMaking()
{
    static connector_client::CSMsg msg;
    msg.mutable_head()->set_magic(0x3344);
    msg.mutable_head()->set_sequence(1);
    msg.mutable_head()->set_bodyid(connector_client::SYNACK);
    msg.mutable_body()->mutable_ack()->set_secretkey("test secret key");

    if (SerializeAndSendToFrontEnd(msg) != 0)
    {
        return -1;
    }

    state_ = STATE_AUTH;
    LOG(INFO)<<"session state[STATE_INIT -> STATE_AUTH]";

    return 0;
}

int ConnectorSession::HandleAuthRequest()
{
    static connector_client::CSMsg msg;
    msg.mutable_head()->set_magic(0x3344);
    msg.mutable_head()->set_sequence(1);
    msg.mutable_head()->set_bodyid(connector_client::PASSPORT);
    msg.mutable_body()->mutable_passport()->set_passport(102030);

    if (SerializeAndSendToFrontEnd(msg) != 0)
    {
        return -1;
    }

    state_ = STATE_SYNING;
    LOG(INFO)<<"session state[STATE_AUTH -> STATE_SYNING]";

    return 0;
}

int ConnectorSession::HandShake_StartSession()
{
    state_ = STATE_SYNACK;
    LOG(INFO)<<"session state[STATE_SYNING -> STATE_SYNACK]";

    return 0;
}

// TODO connector server api to recv start
int ConnectorSession::HandShake_OnStartAcked()
{
    static connector_client::CSMsg msg;
    msg.mutable_head()->set_magic(0x3344);
    msg.mutable_head()->set_sequence(1);
    msg.mutable_head()->set_bodyid(connector_client::START_APP);
    msg.mutable_body()->mutable_start()->set_code(0);

    if (SerializeAndSendToFrontEnd(msg) != 0)
    {
        return -1;
    }

    state_ = STATE_OK;
    LOG(INFO)<<"session state[STATE_SYNACK -> STATE_OK]";

    return 0;
}

int ConnectorSession::HandShake_StopSession()
{
    return 0;
}

int ConnectorSession::HandShake_OnStopNotify()
{
    return 0;
}

void ConnectorSession::SetChannelID(int channel_id)
{
    channel_id_ = channel_id;
}
