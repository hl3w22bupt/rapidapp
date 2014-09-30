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
                                   std::string& token, int& encrypt_mode, int& auth_type,
                                   std::string& server_ip, int& server_port) = 0;

        virtual int OnHandShakeSucceed() = 0;
        virtual int OnHandShakeFailed() = 0;
        virtual int OnServerClose() = 0;
        virtual int OnQueuing() = 0;
        virtual int OnIncoming() = 0;
};

class ConnectorClientProtocol {
    public:
        static ConnectorClientProtocol& Singleton() {
            static ConnectorClientProtocol instance;
            return instance;
        }

        static ConnectorClientProtocol* Create() {
            return new ConnectorClientProtocol;
        }

    public:
        int Start(IProtocolEventListener* pel);
        int Terminate();
        int Resume();

        int Update();

        int PushMessage();
        int PopMessage();

    // 为了编程方便，同时考虑到短期类成员变量变化不频繁，
    // 所有类的非public成员都不采用pimpl实现
    private:
        int Connect();

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
        std::string server_ip_;
        int server_port_;
};

class ConnectorClientProtocolThread {
    public:
        static ConnectorClientProtocolThread& Singleton() {
            static ConnectorClientProtocolThread instance;
            return instance;
        }

        static ConnectorClientProtocolThread* Create() {
            return new ConnectorClientProtocolThread;
        }

    public:
        int RunWithThread(IProtocolEventListener* protocol_evlistener);
        int TerminateThread();

        int PushMessageToSendQ();
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
