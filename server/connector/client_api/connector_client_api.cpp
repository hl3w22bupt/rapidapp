#include "connector_client_api.h"
#include "utils/tcp_socket.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>

// TODO 错误码统一
namespace hmoon_connector_api {

const int kMaxPkgSize = 1024 * 1024;
class ConnectorPkgParser : public ITcpPkgParser {
    public:
        ConnectorPkgParser(){};
        ~ConnectorPkgParser(){};

    public:
        virtual int GetPkgLen(const char* buf, size_t buf_len) {
            if (NULL == buf || buf_len <= 4) {
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
    protocol_event_listener_(NULL), tcp_sock_(), session_state_(SESSION_STATE_INITED)
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
    int ret = tcp_sock_.CheckNonBlock();
    if (ret != NORMAL && ret != NET_CONNECTED)
    {
        return -1;
    }
    else if(NET_CONNECTED == ret)
    {
        session_state_ = SESSION_STATE_KEY_SYNING;
        return HandShake_SYN();
    }
    else
    {
        return 0;
    }
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
    // 发起连接
    int ret = Connect(server_uri_);
    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

// 发起SYN 密钥协商请求
int ConnectorClientProtocol::HandShake_SYN()
{
    up_msg.mutable_head()->set_magic(connector_client::MAGIC_CS_V1);
    up_msg.mutable_head()->set_sequence(0); // sequence NOT care
    up_msg.mutable_head()->set_bodyid(connector_client::SYN);
    // calculate encrypt-key by openid&appid
    up_msg.mutable_body()->mutable_syn()->set_openid(openid_);
    up_msg.mutable_body()->mutable_syn()->set_appid(appid_);

    std::string bin_to_send;
    up_msg.SerializeToString(&bin_to_send);

    // send
    int ret = tcp_sock_.PushToSendQ(bin_to_send.c_str(), bin_to_send.size());
    if (ret != NORMAL)
    {
        // send buffer is full
        return -1;
    }

    ret = tcp_sock_.Send();
    if (ret != NORMAL && ret != SEND_UNCOMPLETED)
    {
        // send error
        return -1;
    }

    return 0;
}

// 试图接收 ACK密钥协商回应
int ConnectorClientProtocol::HandShake_TRY_ACK()
{
    int ret = tcp_sock_.Recv();
    if (ret != NORMAL)
    {
        return -1;
    }

    if (!tcp_sock_.HasNewPkg())
    {
        return 0;
    }

    // get key, and start authing
    const char* buf = NULL;
    int buflen = 0;
    ret = tcp_sock_.PeekFromRecvQ(&buf, &buflen);
    if (ret != NORMAL && ASSERT_FAILED == ret)
    {
        return -1;
    }

    down_msg.ParseFromArray(buf + 4, buflen - 4);

    ret = tcp_sock_.PopFromRecvQ();
    if (ASSERT_FAILED == ret)
    {
        return -1;
    }

    return HandShake_AUTH();
}

// 发起AUTH鉴权请求
int ConnectorClientProtocol::HandShake_AUTH()
{
    return 0;
}

// 试图接收AUTH鉴权结果
int ConnectorClientProtocol::HandShake_TRY_READY()
{
    return 0;
}

// 试图检查接入握手是否成功完成
int ConnectorClientProtocol::HandShake_TRY_DONE()
{
    return 0;
}

// 驱动异步事件
int ConnectorClientProtocol::Update()
{
    int ret = 0;

    // 驱动状态机
    switch (session_state_)
    {
        case SESSION_STATE_INITED:
            {
                ret = Connect(server_uri_);
                if (ret != 0)
                {
                    return -1;
                }
                break;
            }
        case SESSION_STATE_TCP_SYNING:
            {
                // 检查 TCP三次握手
                ret = CheckConnect();
                if (ret != 0)
                {
                    return -1;
                }
                break;
            }
        case SESSION_STATE_KEY_SYNING:
            {
                // 尝试接收 协商出来掉密钥KEY
                ret = HandShake_TRY_ACK();
                if (ret != 0)
                {
                    return -1;
                }
                break;
            }
        case SESSION_STATE_AUTHING:
            {
                // 尝试获取 鉴权结果
                ret = HandShake_TRY_READY();
                if (ret != 0)
                {
                    return -1;
                }
                break;
            }
        case SESSION_STATE_READY:
            {
                // 查看 是否可以开始数据通信，同时尝试刷新排队信息
                ret = HandShake_TRY_DONE();
                if (ret != 0)
                {
                    return -1;
                }
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
                        protocol_event_listener_->OnIncoming();
                    }
                }
                break;
            }
        default:
            {
                return -1;
            }
    }

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

int ConnectorClientProtocolThread::PushMessageToSendQ()
{
    return 0;
}

int ConnectorClientProtocolThread::PopMessageFromRecvQ()
{
    return 0;
}

}
