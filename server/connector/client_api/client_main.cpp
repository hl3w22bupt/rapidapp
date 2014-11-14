#include "connector_client_api.h"
#include <stdio.h>
#include <glog/logging.h>

using namespace hmoon_connector_api;

class ConnectorProtocolListener :
public IProtocolEventListener, public IWorkerThreadListener {
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
        virtual void OnWorkerThreadExit();

    public:
        int Init();
        void Fini();
        int MainLoop();

    private:
      bool exit_;
};

class Glogger : public ILoggable {
    public:
        Glogger(){};
        ~Glogger(){};

    public:
        virtual void Log(int level, const char* log) {
            switch(level)
            {
                case LOG_FATAL:
                    {
                        LOG(FATAL)<<log;
                        break;
                    }
                case LOG_ERROR:
                    {
                        LOG(ERROR)<<log;
                    }
                case LOG_NOTICE:
                case LOG_INFO:
                case LOG_DEBUG:
                    {
                        LOG(INFO)<<log;
                    }
            }
        }
};

Glogger g_logger;

ConnectorProtocolListener::ConnectorProtocolListener() : exit_(false)
{}

ConnectorProtocolListener::~ConnectorProtocolListener()
{}

int ConnectorProtocolListener::Init()
{
    fprintf(stdout, "start connector api client\n");
    ConnectorClientProtocolThread::Default().StartThread(this, this, &g_logger);

    return 0;
}

int ConnectorProtocolListener::MainLoop()
{
    char hello[] = "hello";

    while (!exit_)
    {
        ConnectorClientProtocolThread::Default().PushMessageToSendQ(hello, sizeof(hello));
        fprintf(stdout, "send %s\n", hello);
        usleep(1 * 1000);
    }

    fprintf(stdout, "exit connector api client\n");

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
    appid = "TEST123456";
    openid = "GUEST0987654321";
    token = "TOKEN3388";
    encrypt_mode = NOT_ENCRYPT;
    auth_type = NONE_AUTHENTICATION;
    server_uri = "tcp://127.0.0.1:8888";
}

int ConnectorProtocolListener::OnHandShakeSucceed()
{
    char hello[] = "hello world";
    ConnectorClientProtocolThread::Default().PushMessageToSendQ(hello, sizeof(hello));
    return 0;
}

int ConnectorProtocolListener::OnHandShakeFailed()
{
    fprintf(stdout, "HandShake Failed\n");
    exit_ = true;

    return 0;
}

int ConnectorProtocolListener::OnServerClose()
{
    fprintf(stdout, "ServerClose Actively\n");
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
    size_t len = sizeof(data) - 1;
    ConnectorClientProtocolThread::Default().PopMessageFromRecvQ(data, &len);
    data[len] = '\0';
    fprintf(stdout, "recv message %s\n", data);

    return 0;
}

void ConnectorProtocolListener::OnWorkerThreadExit()
{
    fprintf(stdout, "WorkerThreadExit\n");
    exit_ = true;
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
