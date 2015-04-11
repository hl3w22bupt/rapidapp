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

enum {
    CONNECTOR_ERR_NONE        = 0,
    CONNECTOR_ERR_NO_MORE_PKG = -1,
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

enum {
    LOG_FATAL  = 0,
    LOG_ERROR  = 1,
    LOG_NOTICE = 2,
    LOG_INFO   = 3,
    LOG_DEBUG  = 4,
};

class ILoggable {
    public:
        ILoggable(){};
        virtual ~ILoggable(){};

    public:
        // 为了简单起见，目前只能记录固定内容
        // 记录日志地址可以预分配一个全局日志buffer，
        // 格式化到此buffer后，再调用Log记录
        virtual void Log(int level, const char* log){};
};

class ConnectorClientProtocolImp;

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

    private:
        ConnectorClientProtocol();
        ~ConnectorClientProtocol();
        ConnectorClientProtocol(const ConnectorClientProtocol&);
        ConnectorClientProtocol& operator=(const ConnectorClientProtocol&);

    private:
        ConnectorClientProtocolImp* imp_;
};

class IWorkerThreadListener {
    public:
        IWorkerThreadListener(){};
        virtual ~IWorkerThreadListener(){};

    public:
        virtual void OnWorkerThreadExit(){};
};

class ConnectorClientProtocolThreadImp;

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
        ConnectorClientProtocolThread();
        ~ConnectorClientProtocolThread();
        ConnectorClientProtocolThread(const ConnectorClientProtocolThread&);
        ConnectorClientProtocolThread& operator=(const ConnectorClientProtocol&);

    private:
        ConnectorClientProtocolThreadImp* imp_;
};

}

#endif
