#include "connector_session.h"
#include "./server.pb.h"
#include <glog/logging.h>

const int kDefaultOverloadLimit = 1024;

EasyNet* BackEndSet::backends_[kMaxBackEndNum] = {};
uint32_t BackEndSet::backend_pos_ = 0;
uint32_t BackEndSet::backend_used_ = 0;

int BackEndSet::AddBackEnd(EasyNet* net) 
{
    if (backend_used_ >= kMaxBackEndNum)
    {
        return -1;
    }
    
    backends_[backend_used_] = net;
    ++backend_used_;
}

ConnectorSession::ConnectorSession() :
    state_(STATE_INIT), net_stub_(NULL), channel_id_(-1), sid_(-1), frame_stub_(NULL)
{}

ConnectorSession::~ConnectorSession()
{
    CleanUp();
}

int ConnectorSession::Init(EasyNet* net, IFrameWork* app_framework)
{
    if (NULL == net || NULL == app_framework)
    {
        return -1;
    }

    net_stub_ = net;
    frame_stub_ = app_framework;
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
                return HandShake_OnStopNotify();
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
    assert(frame_stub_ != NULL);
    if (frame_stub_->SendToFrontEnd(net_stub_,
            bin_to_send.c_str(), bin_to_send.size()) != 0)
    {
        frame_stub_->DestroyFrontEnd(&net_stub_);
        return -1;
    }

    return 0;
}

int ConnectorSession::StartToBackEnd(EasyNet* back_net)
{
    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }

    if (NULL == back_net)
    {
        LOG(ERROR)<<"null front net | null session";
        return -1;
    }

    // TODO 有状态记录后端节点或者使用一致性哈希，保证发给同一个后端
    // 无状态服务随机选择一个负载均衡较好的后端节点
    static connector_server::SSMsg msg;
    uint32_t fd = 0;
    uint64_t nid = 0;
    frame_stub_->GetNetIds(net_stub_, fd, nid);
    msg.mutable_head()->set_magic(connector_server::MAGIC_SS_V1);
    msg.mutable_head()->set_sequence(1);
    msg.mutable_head()->set_bodyid(connector_server::SYN);
    msg.mutable_head()->mutable_session()->set_fd(fd);
    msg.mutable_head()->mutable_session()->set_nid(nid);
    msg.mutable_head()->mutable_session()->set_sid(sid_);
    msg.mutable_body()->mutable_syn()->set_result(0);

    LOG(INFO)<<"start handshake to backend: "<<std::endl<<msg.DebugString();

    std::string up_buff;
    msg.SerializeToString(&up_buff);
    int ret = frame_stub_->SendToBackEnd(back_net, up_buff.c_str(), up_buff.size());
    if (ret != 0)
    {
        LOG(INFO)<<"send to backend uri: "<<net_stub_->uri()<<" failed";
        return -1;
    }

    return 0;
}

// 有状态服务才需要StopToBackEnd
int ConnectorSession::StopToBackEnd(EasyNet* back_net)
{
    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }

    if (NULL == back_net)
    {
        LOG(ERROR)<<"null back_net";
        return -1;
    }
    
    static connector_server::SSMsg msg;
    uint32_t fd = 0;
    uint64_t nid = 0;
    frame_stub_->GetNetIds(net_stub_, fd, nid);
    msg.mutable_head()->set_magic(connector_server::MAGIC_SS_V1);
    msg.mutable_head()->set_sequence(1);
    msg.mutable_head()->set_bodyid(connector_server::FIN);
    msg.mutable_head()->mutable_session()->set_fd(fd);
    msg.mutable_head()->mutable_session()->set_nid(nid);
    msg.mutable_head()->mutable_session()->set_sid(sid_);
    msg.mutable_body()->mutable_fin()->set_result(0);

    LOG(INFO)<<"finish to backend: "<<std::endl<<msg.DebugString();

    std::string up_buff;
    msg.SerializeToString(&up_buff);
    int ret = frame_stub_->SendToBackEnd(back_net, up_buff.c_str(), up_buff.size());
    if (ret != 0)
    {
        LOG(INFO)<<"send to backend uri: "<<back_net->uri()<<" failed";
        return -1;
    }

    return 0;
}

int ConnectorSession::SendDataToBackEnd(const char* data, size_t len)
{
    // TODO 如果是有状态服务，需要计算相应的后端来发送（记录下来的或一致性哈希）
    if (BackEndSet::backend_pos_ >= BackEndSet::backend_used_)
    {
        BackEndSet::backend_pos_ = 0;
    }
    EasyNet* back_net = BackEndSet::backends_[BackEndSet::backend_pos_++]; 
    
    assert(frame_stub_ != NULL);
    int ret = frame_stub_->SendToBackEnd(back_net, data, len);
    if (ret != 0)
    {
        LOG(INFO)<<"send to backend uri: "<<back_net->uri()<<" failed";
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
    if (BackEndSet::backend_pos_ >= BackEndSet::backend_used_)
    {
        BackEndSet::backend_pos_ = 0;
    }
    EasyNet* back_net = BackEndSet::backends_[BackEndSet::backend_pos_++]; 
    
    if (StartToBackEnd(back_net) != 0)
    {
        LOG(ERROR)<<"start to backend failed";
        assert(frame_stub_ != NULL && net_stub_ != NULL);
        frame_stub_->DestroyFrontEnd(&net_stub_);
        return -1;
    }

    state_ = STATE_SYNACK;
    LOG(INFO)<<"session state[STATE_SYNING -> STATE_SYNACK]";

    return 0;
}

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
    // TODO 如果是有状态服务，获取到backend，并且stop
    EasyNet* last_backend = NULL;
    
    if (last_backend != NULL)
    {
        return StopToBackEnd(last_backend);
    }
    else
    {
        return 0;
    }
}

int ConnectorSession::HandShake_OnStopNotify()
{
    assert(frame_stub_ != NULL && net_stub_ != NULL);
    LOG(ERROR)<<"backend stopped client uri"<<net_stub_->uri();
    frame_stub_->DestroyFrontEnd(&net_stub_);
    
    return 0;
}

void ConnectorSession::SetChannelID(int channel_id)
{
    channel_id_ = channel_id;
}
