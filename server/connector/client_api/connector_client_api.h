#ifndef _CONNECTOR_CLIENT_API_H_
#define _CONNECTOR_CLIENT_API_H_

namespace hmoon_connector_api {

class IProtocolEventListener {
    public:
        IProtocolEventListener();
        virtual ~IProtocolEventListener();

    public:
        virtual int Init();
        virtual void CleanUp();

        virtual int Start();
        virtual int Stop();
        virtual int Resume();
};

class ConnectorClientProtocol {
    public:
        static ConnectorClientProtocol& Singleton() {
            static ConnectorClientProtocol instance;
            return instance;
        }

    public:
        static int Run() {
            return 0;
        }
        static int RunWithThread() {
            return 0;
        }

    private:
        IProtocolEventListener* protocol_event_listener_;
};

}

#endif
