#include "connector_client_api_imp.h"
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
            memcpy(&msglen, buf, sizeof(msglen));

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

static void CONNECTOR_CLIENT_API_LOG(ILoggable* logger,
                                     int level, const char* log)
{
    if (logger != NULL)
    {
        logger->Log(level, log);
    }
}

////////////////////////////////////////////////
////////////// Connector Protocol //////////////
////////////////////////////////////////////////
ConnectorClientProtocolImp::ConnectorClientProtocolImp() :
    protocol_event_listener_(NULL), logger_(NULL),
      appid_(), openid_(), token_(), server_uri_(),
        encrypt_mode_(NOT_ENCRYPT), auth_type_(NONE_AUTHENTICATION),
          encrypt_skey_(), passport_(0),
            tcp_sock_(), session_state_(SESSION_STATE_INITED),
              seqno_(1), up_msg_(), down_msg_()
{}

ConnectorClientProtocolImp::~ConnectorClientProtocolImp()
{}

int ConnectorClientProtocolImp::Connect(const std::string& server_uri)
{
    assert(protocol_event_listener_ != NULL);

    int ret = tcp_sock_.ConnectNonBlock(server_uri.c_str());
    if (ret != NORMAL)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "ConnectNonBlock failed");
        return -1;
    }

    session_state_ = SESSION_STATE_TCP_SYNING;
    return 0;
}

int ConnectorClientProtocolImp::CheckConnect()
{
    return tcp_sock_.CheckNonBlock();
}

void ConnectorClientProtocolImp::Close()
{
    tcp_sock_.Close();
    session_state_ = SESSION_STATE_FINI;
}

int ConnectorClientProtocolImp::Start(IProtocolEventListener* protocol_evlistener,
                                   ILoggable* logger)
{
    if (NULL == protocol_evlistener)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "null protocol_evlistener");
        return -1;
    }
    protocol_event_listener_ = protocol_evlistener;
    logger_ = logger;

    // 设置一些参数，比如登录帐号、token、密码等
    protocol_event_listener_->OnGetSettings(appid_, openid_, token_,
                                            encrypt_mode_, auth_type_,
                                            server_uri_);
    tcp_sock_.Init(&g_pkgparser_imp, kMaxPkgSize * 2);
    // 发起服务连接
    int ret = Connect(server_uri_);
    if (ret != 0)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "Connect failed");
        return -1;
    }

    tcpsocket_ignore_pipe();

    return 0;
}

// 停止服务
int ConnectorClientProtocolImp::Terminate()
{
    // 关闭连接之前把TCP socket接收缓冲区数据收完，避免触发RST到服务端
    Close();

    return 0;
}

// 恢复服务
int ConnectorClientProtocolImp::Resume()
{
    if (NULL == protocol_event_listener_)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "null protocol_evlistener");
        return -1;
    }

    // 准备至RESUME状态
    tcp_sock_.Close();
    tcp_sock_.Reset();
    // 发起连接
    int ret = Connect(server_uri_);
    if (ret != 0)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "Resume failed");
        return -1;
    }

    return 0;
}

int ConnectorClientProtocolImp::SerializeAndSendToPeer()
{
    std::string bin_to_send;
    up_msg_.SerializeToString(&bin_to_send);

    // send
    uint32_t msglen = htonl(bin_to_send.size() + sizeof(msglen));
    struct iovec iov[2] = {{&msglen, sizeof(msglen)},
        {const_cast<char*>(bin_to_send.c_str()), bin_to_send.size()}};
    int ret = tcp_sock_.PushvToSendQ(&iov[0], sizeof(iov)/sizeof(iov[0]));
    if (ret != NORMAL)
    {
        // send buffer is full
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "PushvToSendQ failed");
        return ret;
    }

    ret = tcp_sock_.Send();
    if (ret != NORMAL && ret != SEND_UNCOMPLETED)
    {
        // send error
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "Socket Send failed");
        return ret;
    }

    return NORMAL;
}

int ConnectorClientProtocolImp::TryToRecvFromPeerAndParse()
{
    int ret = tcp_sock_.Recv();
    if (ret != NORMAL)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "Socket Recv failed");
        return ret;
    }

    if (!tcp_sock_.HasNewPkg())
    {
        if (tcp_sock_.GetRecvBufLen() != 0)
        {
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "pkg not complete yet");
        }
        else
        {
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_DEBUG, "no more pkg avaiable");
        }

        return CONNECTOR_ERR_NO_MORE_PKG;
    }

    const char* buf = NULL;
    int buflen = 0;
    ret = tcp_sock_.PeekFromRecvQ(&buf, &buflen);
    if (ret != NORMAL && ASSERT_FAILED == ret)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "pkg is complete, but PeekFromRecvQ failed");
        return ret;
    }

    down_msg_.ParseFromArray(buf + kMsgLenFieldSize, buflen - kMsgLenFieldSize);

    ret = tcp_sock_.PopFromRecvQ();
    if (ASSERT_FAILED == ret)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "PopFromRecvQ failed, unexpected");
        return ret;
    }

    // error -- get error code
    if (connector_client::ERROR == down_msg_.mutable_head()->bodyid())
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 down_msg_.mutable_body()->error().desc().c_str());
        return -1;
    }

    return 0;
}

// 发起SYN 密钥协商请求
int ConnectorClientProtocolImp::HandShake_SYN()
{
    up_msg_.mutable_head()->set_magic(connector_client::MAGIC_CS_V1);
    up_msg_.mutable_head()->set_sequence(seqno_++); //
    up_msg_.mutable_head()->set_bodyid(connector_client::SYN);
    // calculate encrypt-key by openid&appid
    up_msg_.mutable_body()->Clear();
    up_msg_.mutable_body()->mutable_syn()->set_openid(openid_);
    up_msg_.mutable_body()->mutable_syn()->set_appid(appid_);

    int ret = SerializeAndSendToPeer();
    if (ret != NORMAL)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "send handshake syn[get secret key] failed");
        return ret;
    }

    session_state_ =  SESSION_STATE_KEY_SYNING;
    return 0;
}

// 试图接收 ACK密钥协商回应
int ConnectorClientProtocolImp::HandShake_TRY_ACK()
{
    // 获取密钥key, 然后发起鉴权
    int ret = TryToRecvFromPeerAndParse();
    if (ret != 0)
    {
        if (ret != CONNECTOR_ERR_NO_MORE_PKG)
        {
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                     "error when try to recv ack");
        }

        return ret;
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::SYNACK)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "bodyid NOT equal to expected SYNACK");
        return -1;
    }

    encrypt_skey_ = down_msg_.mutable_body()->mutable_ack()->secretkey();
    return 0;
}

// 发起AUTH鉴权请求
int ConnectorClientProtocolImp::HandShake_AUTH()
{
    up_msg_.mutable_head()->set_magic(connector_client::MAGIC_CS_V1);
    up_msg_.mutable_head()->set_sequence(seqno_++); //
    up_msg_.mutable_head()->set_bodyid(connector_client::AUTHENTICATION);
    // calculate encrypt-key by openid&appid
    up_msg_.mutable_body()->Clear();
    up_msg_.mutable_body()->mutable_auth()->set_token(token_);

    int ret = SerializeAndSendToPeer();
    if (ret != NORMAL)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "send handshake auth failed");
        return ret;
    }

    session_state_ = SESSION_STATE_AUTHING;
    return 0;
}

// 试图接收AUTH鉴权结果
int ConnectorClientProtocolImp::HandShake_TRY_READY()
{
    int ret = TryToRecvFromPeerAndParse();
    if (ret != 0)
    {
        if (ret != CONNECTOR_ERR_NO_MORE_PKG)
        {
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                     "error when try to recv ready");
            return ret;
        }
        else
        {
            return 0;
        }
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::PASSPORT)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "bodyid NOT equal to expected PASSPORT");
        return -1;
    }

    passport_ = down_msg_.mutable_body()->mutable_passport()->passport();
    session_state_ = SESSION_STATE_READY;
    return 0;
}

// 试图检查接入握手是否成功完成
int ConnectorClientProtocolImp::HandShake_TRY_DONE()
{
    int ret = TryToRecvFromPeerAndParse();
    if (ret != 0)
    {
        if (ret != CONNECTOR_ERR_NO_MORE_PKG)
        {
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                     "error when try to recv done");
            return ret;
        }
        else
        {
            return 0;
        }
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::START_APP)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "bodyid NOT equal to expected START_APP");
        return -1;
    }

    session_state_ = SESSION_STATE_DONE;
    return 0;
}

// 驱动异步事件
int ConnectorClientProtocolImp::Update()
{
    if (NULL == protocol_event_listener_)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "null evlistener, assert failed");
        return -1;
    }

    int ret = 0;
    int busy = 0;

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
                if (NORMAL == ret &&
                    NET_CONNECTED == tcp_sock_.socket_state())
                {
                    // connect success
                    ++busy;
                    ret = HandShake_SYN();
                }
                break;
            }
        case SESSION_STATE_KEY_SYNING:
            {
                // 尝试接收 协商出来的密钥KEY
                ret = HandShake_TRY_ACK();
                if (0 == ret)
                {
                    ++busy;
                    ret = HandShake_AUTH();
                }
                else if (CONNECTOR_ERR_NO_MORE_PKG == ret)
                {
                    ret = 0;
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
                        handshake_done = true;
                        ++busy;
                    }
                    else
                    {
                        // update & check DATA_IN event
                        ret = tcp_sock_.Recv();
                        if (NORMAL == ret && tcp_sock_.HasNewPkg())
                        {
                            ++busy;
                            protocol_event_listener_->OnIncoming();
                        }
                    }
                }
                break;
            }
        default:
            {
                CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                         "unexpected state, assert failed");
                return -1;
            }
    }

    if (ret != NORMAL)
    {
        if (session_state_ != SESSION_STATE_DONE)
        {
            protocol_event_listener_->OnHandShakeFailed();
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "handshake failed");

            return -1;
        }
        else if (PEER_CLOSED == ret)
        {
            protocol_event_listener_->OnServerClose();
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "peer close connection");

            return -1;
        }
    }

    return busy;
}

// 发送消息
int ConnectorClientProtocolImp::PushMessage(const char* data, size_t size)
{
    if (NULL == data || 0 == size)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "null data");

        return -1;
    }

    up_msg_.mutable_head()->set_magic(connector_client::MAGIC_CS_V1);
    up_msg_.mutable_head()->set_sequence(seqno_++); //
    up_msg_.mutable_head()->set_bodyid(connector_client::DATA_TRANSPARENT);

    up_msg_.mutable_body()->Clear();
    up_msg_.mutable_body()->set_data(std::string(data, size));

    if (SerializeAndSendToPeer() != 0)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "PushMessage failed");
        return -1;
    }

    return 0;
}

// 接收消息
int ConnectorClientProtocolImp::PeekMessage(const char** buf_ptr, int* buflen_ptr)
{
    if (NULL == buf_ptr || NULL == buflen_ptr)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "invalid arguments");
        return -1;
    }

    const char* buf = NULL;
    int buflen = 0;
    int ret = tcp_sock_.PeekFromRecvQ(&buf, &buflen);
    if (ret != NORMAL && ASSERT_FAILED == ret)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "PeekFromRecvQ failed");
        return -1;
    }

    down_msg_.ParseFromArray(buf + kMsgLenFieldSize, buflen - kMsgLenFieldSize);

    if (down_msg_.mutable_head()->bodyid() != connector_client::DATA_TRANSPARENT)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "bodyid NOT expected(DATA_TRANSPARENT)");
        return -1;
    }

    *buf_ptr = down_msg_.mutable_body()->data().c_str();
    *buflen_ptr = down_msg_.mutable_body()->data().size();

    return 0;
}

int ConnectorClientProtocolImp::PopMessage()
{
    int ret = tcp_sock_.PopFromRecvQ();
    if (ASSERT_FAILED == ret)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "PopFromRecvQ failed");
        return -1;
    }

    if (down_msg_.mutable_head()->bodyid() != connector_client::DATA_TRANSPARENT)
    {
        // assert failed
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "bodyid NOT expected(DATA_TRANSPARENT)");
        return -1;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////
////////////// Connector Protocol With Single Thread //////////////
///////////////////////////////////////////////////////////////////
ConnectorClientProtocolThreadImp::ConnectorClientProtocolThreadImp() : ccproto_(NULL),
                                                                  wt_listener_(NULL),
                                                                   logger_(NULL),
                                                                    exit_(false)
{}

ConnectorClientProtocolThreadImp::~ConnectorClientProtocolThreadImp()
{
    if (ccproto_ != NULL)
    {
        ConnectorClientProtocol::Destroy(&ccproto_);
    }
}

int ConnectorClientProtocolThreadImp::StartThread(IProtocolEventListener* protocol_evlistener,
                                               IWorkerThreadListener* thread_listener,
                                               ILoggable* logger)
{
    if (NULL == protocol_evlistener ||
        NULL == thread_listener)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                 "null protocol listener OR null thread_listener");
        return -1;
    }

    wt_listener_ = thread_listener;
    logger_ = logger;
    ccproto_ = ConnectorClientProtocol::Create();
    boost::thread protocol_thread(boost::bind(&ConnectorClientProtocolThreadImp::MainLoop,
                                              this, protocol_evlistener, logger));
    protocol_thread.detach();

    return 0;
}

int ConnectorClientProtocolThreadImp::TerminateThread()
{
    exit_ = true;
    return 0;
}

int ConnectorClientProtocolThreadImp::MainLoop(IProtocolEventListener* protocol_evlistener,
                                            ILoggable* logger)
{
    assert(ccproto_ != NULL && wt_listener_ != NULL);
    assert(!exit_);

    // 1. Start Connection
    int ret = ccproto_->Start(protocol_evlistener, logger);
    if (ret != 0)
    {
        wt_listener_->OnWorkerThreadExit();
        return -1;
    }

    // 2. Update Connection
    while (!exit_)
    {
        ret = ccproto_->Update();
        if (ret < 0)
        {
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "Update exception");
            break;
        }
        else if (0 == ret)
        {
            // 当前空闲
            // 客户端api，可以硬编码
            usleep(10 * 1000);
        }
    }

    wt_listener_->OnWorkerThreadExit();
    return 0;
}

int ConnectorClientProtocolThreadImp::PushMessageToSendQ(const char* data,
                                                      size_t size)
{
    assert(ccproto_ != NULL);
    return ccproto_->PushMessage(data, size);
}

int ConnectorClientProtocolThreadImp::PopMessageFromRecvQ(char* buf_ptr,
                                                       size_t* buflen_ptr)
{
    if (NULL == buf_ptr || NULL == buflen_ptr || 0 == *buflen_ptr)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "invalid arguments");
        return -1;
    }

    assert(ccproto_ != NULL);
    const char* data = NULL;
    int len = 0;
    int ret = ccproto_->PeekMessage(&data, &len);
    if (ret != 0)
    {
        CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR, "PeekMessage failed");
        return -1;
    }

    if (data != NULL)
    {
        if (*buflen_ptr < len)
        {
            // buffer is not enough
            CONNECTOR_CLIENT_API_LOG(logger_, LOG_ERROR,
                                     "recv buffer is NOT enough");
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
