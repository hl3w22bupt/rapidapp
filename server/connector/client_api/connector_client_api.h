#ifndef _CONNECTOR_CLIENT_API_H_
#define _CONNECTOR_CLIENT_API_H_

#include <string>

namespace hmoon_connector_api {

enum {
    NOT_ENCRYPT = 0,  // 不加密 - 适用于早期测试阶段
    ENCRYPT_AES = 1,  // AES对称加密
    ENCRYPT_RSA = 2,  // RSA非对称加密
};

enum {
    NONE_AUTHENTICATION = 0,    // 非鉴权 - 适用于早期测试阶段和游客登录模式
    AUTHENTICATION_ATK  = 1,
};

class IProtocolEventListener {
    public:
        IProtocolEventListener(){}
        virtual ~IProtocolEventListener(){};

    public:
        // 为了方便获取多个参数值，同时为了编程方便，采用引用的方式
        virtual void OnGetSettings(std::string& appid, std::string& openid,
                                   std::string& token,
                                   int& encrypt_mode, int& auth_type,
                                   std::string& server_uri) = 0;

        virtual int OnHandShakeSucceed() = 0;
        virtual int OnHandShakeFailed() = 0;
        virtual int OnServerClose() = 0;
        virtual int OnQueuing() = 0;

        virtual int OnIncoming() = 0;
};

// 异步Connector协议处理类
class ConnectorClientProtocol {
    public:
        static ConnectorClientProtocol& Default() {
            static ConnectorClientProtocol instance;
            return instance;
        }

        static ConnectorClientProtocol* Create() {
            return new ConnectorClientProtocol;
        }

    public:
        /// @brief  发起TCP会话
        ///
        /// @param[in]  pel
        ///
        /// @return 0: success
        ///        !0: failed
        int Start(IProtocolEventListener* pel);

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
        int PushMessage();

        /// @brief  接收消息
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
        ConnectorClientProtocol();
        ~ConnectorClientProtocol();
        ConnectorClientProtocol(const ConnectorClientProtocol&);
        ConnectorClientProtocol& operator=(const ConnectorClientProtocol&);

    private:
        IProtocolEventListener* protocol_event_listener_;

    private:
        std::string appid_;
        std::string openid_;
        std::string token_;
        int encrypt_mode_;
        int auth_type_;
        std::string server_uri_;

    private:
        int fd_;
        int session_state_;
};

// 独占线程的异步Connector协议处理类
class ConnectorClientProtocolThread {
    // 所有 public 接口都由主线程调用
    public:
        static ConnectorClientProtocolThread& Default() {
            static ConnectorClientProtocolThread instance;
            return instance;
        }

        static ConnectorClientProtocolThread* Create() {
            return new ConnectorClientProtocolThread;
        }

    public:
        /// @brief  启动网络协议交互线程
        ///
        /// @param[in]  protocol_evlistener
        ///
        /// @return 0: success
        ///        !0: failed
        int StartThread(IProtocolEventListener* protocol_evlistener);

        /// @brief  终止网络协议交互线程
        ///
        /// @return 0: success
        ///        !0: failed
        int TerminateThread();


        /// @brief  push消息到消息队列
        ///
        /// @return 0: success
        ///        !0: failed
        int PushMessageToSendQ();

        /// @brief  从消息队列中pop出消息
        ///
        /// @return 0: success
        ///        !0: failed
        int PopMessageFromRecvQ();

    private:
        int MainLoop(IProtocolEventListener* protocol_evlistener);

    private:
        ConnectorClientProtocolThread();
        ~ConnectorClientProtocolThread();
        ConnectorClientProtocolThread(const ConnectorClientProtocolThread&);
        ConnectorClientProtocolThread& operator=(const ConnectorClientProtocol&);

    private:
        ConnectorClientProtocol* ccproto_;
        bool exit_;
};

}

#endif
