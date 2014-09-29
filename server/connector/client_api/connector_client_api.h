#ifndef _CONNECTOR_CLIENT_API_H_
#define _CONNECTOR_CLIENT_API_H_

namespace hmoon_connector_api {

class IProtocolEventListener {
    public:
        IProtocolEventListener(){}
        virtual ~IProtocolEventListener(){};

    public:
        virtual int Init() = 0;
        virtual void CleanUp() = 0;

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
        int Run(IProtocolEventListener* protocol_evlistener);
        int RunWithThread(IProtocolEventListener* protocol_evlistener);

        int Resume();
        int Terminate();

    private:
        ConnectorClientProtocol();
        ConnectorClientProtocol(const ConnectorClientProtocol&);
        ConnectorClientProtocol& operator=(const ConnectorClientProtocol&);

    private:
        IProtocolEventListener* protocol_event_listener_;
};

}

#endif
