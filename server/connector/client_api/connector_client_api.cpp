#include "connector_client_api.h"
#include "utils/tcp_socket.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>

// TODO 错误码统一
namespace hmoon_connector_api {

const int kMaxPkgSize = 1024 * 1024;

const int kMsgLenFieldSize = sizeof(int32_t);
class ConnectorPkgParser : public ITcpPkgParser {
    public:
        ConnectorPkgParser(){};
        ~ConnectorPkgParser(){};

    public:
        virtual int GetPkgLen(const char* buf, size_t buf_len) {
            if (NULL == buf || buf_len <= kMsgLenFieldSize) {
                return 0;
            }

            int32_t msglen = 0;
            // mobile game, SIGBUS
            msglen = *(int32_t*)buf;

            return ntohl(msglen);
        }
};
ConnectorPkgParser g_pkgparser_imp;

enum {
    SESSION_STATE_INITED      = 0,
    SESSION_STATE_TCP_SYNING  = 1,
    SESSION_STATE_KEY_SYNING  = 2,
    SESSION_STATE_AUTHING     = 3,
    SESSION_STATE_READY       = 4,
    SESSION_STATE_DONE        = 5,
    SESSION_STATE_FINI        = 6,
};

////////////////////////////////////////////////
////////////// Connector Protocol //////////////
////////////////////////////////////////////////
ConnectorClientProtocol::ConnectorClientProtocol() :
    protocol_event_listener_(NULL), appid_(), openid_(), token_(), server_uri_(),
      encrypt_mode_(NOT_ENCRYPT), auth_type_(NONE_AUTHENTICATION),
        encrypt_skey_(), passport_(0),
          tcp_sock_(), session_state_(SESSION_STATE_INITED),
            up_msg_(), down_msg_()
{}

ConnectorClientProtocol::~ConnectorClientProtocol()
{}

int ConnectorClientProtocol::Connect(const std::string& server_uri)
{
    assert(protocol_event_listener_ != NULL);

    int ret = tcp_sock_.ConnectNonBlock(server_uri.c_str());
    if (ret != NORMAL)
    {
        return -1;
    }

    session_state_ = SESSION_STATE_TCP_SYNING;
    return 0;
}

int ConnectorClientProtocol::CheckConnect()
{
    return tcp_sock_.CheckNonBlock();
}

void ConnectorClientProtocol::Close()
{
    tcp_sock_.Close();
    session_state_ = SESSION_STATE_FINI;
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
                                            server_uri_);
    tcp_sock_.Init(&g_pkgparser_imp, kMaxPkgSize * 2);
    // 发起服务连接
    int ret = Connect(server_uri_);
    if (ret != 0)
    {
        return -1;
    }

    tcpsocket_ignore_pipe();

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
    tcp_sock_.Close();
    tcp_sock_.Reset();
    // 发起连接
    int ret = Connect(server_uri_);
    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

int ConnectorClientProtocol::SerializeAndSendToPeer()
{
    std::string bin_to_send;
    up_msg_.SerializeToString(&bin_to_send);

    // send
    int ret = tcp_sock_.PushToSendQ(bin_to_send.c_str(), bin_to_send.size());
    if (ret != NORMAL)
    {
        // send buffer is full
        return ret;
    }

    ret = tcp_sock_.Send();
    if (ret != NORMAL && ret != SEND_UNCOMPLETED)
    {
        // send error
        return ret;
    }

    return NORMAL;
}

int ConnectorClientProtocol::TryToRecvFromPeerAndParse()
{
    int ret = tcp_sock_.Recv();
    if (ret != NORMAL)
    {
        return ret;
    }

    if (!tcp_sock_.HasNewPkg())
    {
        return 0;
    }

    const char* buf = NULL;
    int buflen = 0;
    ret = tcp_sock_.PeekFromRecvQ(&buf, &buflen);
    if (ret != NORMAL && ASSERT_FAILED == ret)
    {
        return ret;
    }

    down_msg_.ParseFromArray(buf + kMsgLenFieldSize, buflen - kMsgLenFieldSize);

    ret = tcp_sock_.PopFromRecvQ();
    if (ASSERT_FAILED == ret)
    {
        return ret;
    }

    // error -- get error code
    if (connector_client::ERROR == down_msg_.mutable_head()->bodyid())
    {
        return -1;
    }

    return 0;
}

// 发起SYN 密钥协商请求
int ConnectorClientProtocol::HandShake_SYN()
{
    up_msg_.mutable_head()->set_magic(connector_client::MAGIC_CS_V1);
    up_msg_.mutable_head()->set_sequence(0); // sequence NOT care
    up_msg_.mutable_head()->set_bodyid(connector_client::SYN);
    // calculate encrypt-key by openid&appid
    up_msg_.mutable_body()->mutable_syn()->set_openid(openid_);
    up_msg_.mutable_body()->mutable_syn()->set_appid(appid_);

    int ret = SerializeAndSendToPeer();
    if (ret != NORMAL)
    {
        return ret;
    }

    session_state_ =  SESSION_STATE_KEY_SYNING;
    return 0;
}

// 试图接收 ACK密钥协商回应
int ConnectorClientProtocol::HandShake_TRY_ACK()
{
    // 获取密钥key, 然后发起鉴权
    int ret = TryToRecvFromPeerAndParse();
    if (ret != 0)
    {
        return ret;
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::SYNACK)
    {
        return -1;
    }

    encrypt_skey_ = down_msg_.mutable_body()->mutable_ack()->secretkey();
    return 0;
}

// 发起AUTH鉴权请求
int ConnectorClientProtocol::HandShake_AUTH()
{
    up_msg_.mutable_head()->set_magic(connector_client::MAGIC_CS_V1);
    up_msg_.mutable_head()->set_sequence(0); // sequence NOT care
    up_msg_.mutable_head()->set_bodyid(connector_client::AUTHENTICATION);
    // calculate encrypt-key by openid&appid
    up_msg_.mutable_body()->mutable_auth()->set_token(token_);

    int ret = SerializeAndSendToPeer();
    if (ret != NORMAL)
    {
        return ret;
    }

    session_state_ = SESSION_STATE_AUTHING;
    return 0;
}

// 试图接收AUTH鉴权结果
int ConnectorClientProtocol::HandShake_TRY_READY()
{
    int ret = TryToRecvFromPeerAndParse();
    if (ret != 0)
    {
        return ret;
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::PASSPORT)
    {
        return -1;
    }
    
    passport_ = down_msg_.mutable_body()->mutable_passport()->passport();
    session_state_ = SESSION_STATE_READY;
    return 0;
}

// 试图检查接入握手是否成功完成
int ConnectorClientProtocol::HandShake_TRY_DONE()
{
    int ret = TryToRecvFromPeerAndParse();
    if (ret != 0)
    {
        return ret;
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::START_APP)
    {
        return -1;
    }
    
    session_state_ = SESSION_STATE_DONE;
    return 0;
}

// 驱动异步事件
int ConnectorClientProtocol::Update()
{
    if (NULL == protocol_event_listener_)
    {
        return -1;
    }

    int ret = 0;

    // 驱动状态机
    switch (session_state_)
    {
        case SESSION_STATE_INITED:
            {
                ret = Connect(server_uri_);
                break;
            }
        case SESSION_STATE_TCP_SYNING:
            {
                // 检查 TCP三次握手
                ret = CheckConnect();
                if (NET_CONNECTED == ret)
                {
                    // connect success
                    ret = HandShake_SYN();
                }
                break;
            }
        case SESSION_STATE_KEY_SYNING:
            {
                // 尝试接收 协商出来掉密钥KEY
                ret = HandShake_TRY_ACK();
                if (0 == ret)
                {
                    ret = HandShake_AUTH();
                }
                break;
            }
        case SESSION_STATE_AUTHING:
            {
                // 尝试获取 鉴权结果
                ret = HandShake_TRY_READY();
                break;
            }
        case SESSION_STATE_READY:
            {
                // 查看 是否可以开始数据通信，同时尝试刷新排队信息
                ret = HandShake_TRY_DONE();
                // TODO OnQueuing();
                break;
            }
        case SESSION_STATE_DONE:
            {
                // 检查 是否收到对端数据
                static bool handshake_done = false;
                if (protocol_event_listener_ != NULL)
                {
                    if (!handshake_done)
                    {
                        protocol_event_listener_->OnHandShakeSucceed();
                    }
                    else
                    {
                        // TODO check DATA_IN event
                        ret = tcp_sock_.Recv();
                        if (NORMAL == ret && tcp_sock_.HasNewPkg())
                        {
                            protocol_event_listener_->OnIncoming();
                        }
                    }
                }
                break;
            }
        default:
            {
                return -1;
            }
    }

    if (ret != NORMAL)
    {
        if (session_state_ != SESSION_STATE_DONE)
        {
            protocol_event_listener_->OnHandShakeFailed();
        }
        else if (PEER_CLOSED == ret)
        {
            protocol_event_listener_->OnServerClose();
        }
    }

    return 0;
}

// 发送消息
int ConnectorClientProtocol::PushMessage(const char* data, size_t size)
{
    if (NULL == data || 0 == size)
    {
        return -1;
    }

    up_msg_.mutable_head()->set_magic(connector_client::MAGIC_CS_V1);
    up_msg_.mutable_head()->set_sequence(0); // sequence NOT care
    up_msg_.mutable_head()->set_bodyid(connector_client::DATA_TRANSPARENT);
    // calculate encrypt-key by openid&appid
    up_msg_.mutable_body()->set_data(std::string(data, size));

    if (SerializeAndSendToPeer() != 0)
    {
        return -1;
    }

    return 0;
}

// 接收消息
int ConnectorClientProtocol::PeekMessage(const char** buf_ptr, int* buflen_ptr)
{
    if (NULL == buf_ptr || NULL == buflen_ptr)
    {
        return -1;
    }

    const char* buf = NULL;
    int buflen = 0;
    int ret = tcp_sock_.PeekFromRecvQ(&buf, &buflen);
    if (ret != NORMAL && ASSERT_FAILED == ret)
    {
        return -1;
    }

    down_msg_.ParseFromArray(buf + kMsgLenFieldSize, buflen - kMsgLenFieldSize);

    if (down_msg_.mutable_head()->bodyid() != connector_client::DATA_TRANSPARENT)
    {
        return -1;
    }

    *buf_ptr = down_msg_.mutable_body()->data().c_str();
    *buflen_ptr = down_msg_.mutable_body()->data().size();

    return 0;
}

int ConnectorClientProtocol::PopMessage()
{
    int ret = tcp_sock_.PopFromRecvQ();
    if (ASSERT_FAILED == ret)
    {
        return -1;
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::DATA_TRANSPARENT)
    {
        // assert failed
        return -1;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////
////////////// Connector Protocol With Single Thread //////////////
///////////////////////////////////////////////////////////////////
ConnectorClientProtocolThread::ConnectorClientProtocolThread() : ccproto_(NULL), exit_(false)
{}

ConnectorClientProtocolThread::~ConnectorClientProtocolThread()
{}

int ConnectorClientProtocolThread::StartThread(IProtocolEventListener* protocol_evlistener)
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

int ConnectorClientProtocolThread::PushMessageToSendQ(const char* data, size_t size)
{
    assert(ccproto_ != NULL);
    return ccproto_->PushMessage(data, size);
}

int ConnectorClientProtocolThread::PopMessageFromRecvQ(char* buf_ptr, size_t* buflen_ptr)
{
    if (NULL == buf_ptr || NULL == buflen_ptr || 0 == *buflen_ptr)
    {
        return -1;
    }

    assert(ccproto_ != NULL);
    const char* data = NULL;
    int len = 0;
    int ret = ccproto_->PeekMessage(&data, &len);
    if (ret != 0)
    {
        return -1;
    }

    if (data != NULL)
    {
        if (*buflen_ptr < len)
        {
        // buffer is not enough
            return -1;
        }

        *buflen_ptr = len;
        memcpy(buf_ptr, data, len);
    }
    else
    {
        *buflen_ptr = 0;
    }

    return ccproto_->PopMessage();
}

}
