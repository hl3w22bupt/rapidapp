#include "connector_client_api.h"

using namespace hmoon_connector_api;

class ConnectorProtocolListener : public IProtocolEventListener {
    public:
        ConnectorProtocolListener();
        ~ConnectorProtocolListener();
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
        int MainLoop();

    private:
      bool exit_;
};

ConnectorProtocolListener::ConnectorProtocolListener() : exit_(false)
{}

ConnectorProtocolListener::~ConnectorProtocolListener()
{}

int ConnectorProtocolListener::Init()
{
    ConnectorClientProtocolThread::Default().StartThread(this);
    return 0;
}

int ConnectorProtocolListener::MainLoop()
{
    return 0;
}

void ConnectorProtocolListener::Fini()
{
    ConnectorClientProtocolThread::Default().TerminateThread();
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
    exit_ = true;
    return 0;
}

int ConnectorProtocolListener::OnServerClose()
{
    exit_ = true;
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

int main(int argc, char** argv)
{
    class ConnectorProtocolListener client_protocol_handler;
    int ret = client_protocol_handler.Init();
    if (ret != 0)
    {
        exit(-1);
    }

    ret = client_protocol_handler.MainLoop();

    client_protocol_handler.Fini();

    return 0;
}
