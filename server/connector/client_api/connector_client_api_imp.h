#ifndef _CONNECTOR_CLIENT_API_IMP_H_
#define _CONNECTOR_CLIENT_API_IMP_H_

#include "connector_client_api.h"
#include "../client.pb.h"
#include "utils/tsocket_util.h"
#include <string>

namespace hmoon_connector_api {

using namespace tsocket_util;

// 异步Connector协议处理实现类
class ConnectorClientProtocolImp {
    public:
        ConnectorClientProtocolImp();
        ~ConnectorClientProtocolImp();

    public:
        /// @brief  发起TCP会话
        ///
        /// @param[in]  pel
        ///
        /// @return 0: success
        ///        !0: failed
        int Start(IProtocolEventListener* pel, ILoggable* logger);

        /// @brief  终止通信会话
        ///
        /// @return 0: success
        ///        !0: failed
        int Terminate();

        /// @brief  恢复TCP会话
        ///
        /// @return 0: success
        ///        !0: failed
        int Resume();


        /// @brief  更新会话状态
        ///
        /// @return 0: success
        ///        !0: failed
        int Update();


        /// @brief  发送消息
        ///
        /// @return 0: success
        ///        !0: failed
        int PushMessage(const char* data, size_t size);

        /// @brief  接收消息 - ZeroCopy
        ///
        /// @return 0: success
        ///        !0: failed
        int PeekMessage(const char** buf_ptr, int* buflen_ptr);

        /// @brief  接收消息A - pop
        ///
        /// @return 0: success
        ///        !0: failed
        int PopMessage();

    // 为了编程方便，同时考虑到短期类成员变量变化不频繁，
    // 所有类的非public成员都不采用pimpl实现
    private:
        int Connect(const std::string& server_uri);
        int CheckConnect();
        void Close();

    private:
        int HandShake_SYN();
        int HandShake_TRY_ACK();
        int HandShake_AUTH();
        int HandShake_TRY_READY();
        int HandShake_TRY_DONE();

    private:
        int TryToRecvFromPeerAndParse();
        int SerializeAndSendToPeer();

    private:
        IProtocolEventListener* protocol_event_listener_;
        ILoggable* logger_;

    private:
        std::string appid_;
        std::string openid_;
        std::string token_;
        std::string server_uri_;
        int encrypt_mode_;
        int auth_type_;

    private:
        std::string encrypt_skey_;
        int64_t passport_;

    private:
        TcpSocketUtil tcp_sock_;
        int session_state_;

    private:
        uint64_t seqno_;

    private:
        connector_client::CSMsg up_msg_;
        connector_client::CSMsg down_msg_;
};

// 独占线程的异步Connector协议处理类
class ConnectorClientProtocolThreadImp {
    // 所有 public 接口都由主线程调用
    public:
        ConnectorClientProtocolThreadImp();
        ~ConnectorClientProtocolThreadImp();

    public:
        /// @brief  启动网络协议交互线程
        ///
        /// @param[in]  protocol_evlistener
        ///
        /// @return 0: success
        ///        !0: failed
        int StartThread(IProtocolEventListener* protocol_evlistener,
                        IWorkerThreadListener* thread_listener,
                        ILoggable* logger = NULL);

        /// @brief  终止网络协议交互线程
        ///
        /// @return 0: success
        ///        !0: failed
        int TerminateThread();


        /// @brief  push消息到消息队列
        ///
        /// @return 0: success
        ///        !0: failed
        int PushMessageToSendQ(const char* data, size_t size);

        /// @brief  从消息队列中pop出消息
        ///
        /// @return 0: success
        ///        !0: failed
        int PopMessageFromRecvQ(char* buf_ptr, size_t* buflen_ptr);

    private:
        int MainLoop(IProtocolEventListener* protocol_evlistener,
                     ILoggable* logger);

    private:
        ConnectorClientProtocolImp* ccproto_;
        IWorkerThreadListener* wt_listener_;
        ILoggable* logger_;
        bool exit_;
};

}

#endif
