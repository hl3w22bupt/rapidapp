#include "connector_client_api.h"

using namespace hmoon_connector_api;

class ConnectorProtocolListener : public IProtocolEventListener {
    public:
        virtual void OnGetSettings(std::string& appid, std::string& openid,
                                   std::string& token,
                                   int& encrypt_mode, int& auth_type,
                                   std::string& server_uri);

        virtual int OnHandShakeSucceed();
        virtual int OnHandShakeFailed();
        virtual int OnServerClose();
        virtual int OnQueuing();

        virtual int OnIncoming();

    public:
        int Init();
        void Fini();
};

int ConnectorProtocolListener::Init()
{
    return 0;
}

void ConnectorProtocolListener::Fini()
{
}

void ConnectorProtocolListener::OnGetSettings(std::string& appid,
                                              std::string& openid,
                                              std::string& token,
                                              int& encrypt_mode, int& auth_type,
                                              std::string& server_uri)
{
}

int ConnectorProtocolListener::OnHandShakeSucceed()
{
    char hello[] = "hello world";
    ConnectorClientProtocolThread::Default().PushMessageToSendQ(hello, sizeof(hello));
    return 0;
}

int ConnectorProtocolListener::OnHandShakeFailed()
{
    return 0;
}

int ConnectorProtocolListener::OnServerClose()
{
    ConnectorClientProtocolThread::Default().TerminateThread();
    return 0;
}

int ConnectorProtocolListener::OnQueuing()
{
    return 0;
}

int ConnectorProtocolListener::OnIncoming()
{
    static char data[10240];
    size_t len = sizeof(data);
    ConnectorClientProtocolThread::Default().PopMessageFromRecvQ(data, &len);

    return 0;
}

class ConnectorProtocolListener listener;

int main(int argc, char** argv)
{
    ConnectorClientProtocolThread::Default().StartThread(&listener);

    return 0;
}
