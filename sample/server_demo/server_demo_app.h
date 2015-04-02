#ifndef SERVER_DEMO_APP_H_
#define SERVER_DEMO_APP_H_

#include "rapidapp.h"

using namespace rapidapp;

class PingPongService : public IRpcService {
    public:
        PingPongService();
        ~PingPongService();

    public:
        virtual const std::string RpcRequestName();
        virtual ::google::protobuf::Message* NewResponse();
        virtual int OnRpcCall(const ::google::protobuf::Message* req,
                              ::google::protobuf::Message* resp);
};

class ServerDemoApp : public RapidApp {
    public:
        ServerDemoApp();
        ~ServerDemoApp();

    public:
        virtual int OnInit(IFrameWork* app_framework);
        virtual int OnFini();

        virtual int OnStop();
        virtual int OnResume();

        virtual int OnUpdate();
        virtual int OnReload();

        virtual int OnRecvCtrl(int argc, char** argv);

        virtual int OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size);

        virtual int OnRpc(const ::google::protobuf::Message* request,
                          ::google::protobuf::Message** response);

        virtual int OnTimer(EasyTimer* timer, int timer_id);

    public:
        virtual int OnReportRundata();

    public:
        virtual const char* GetAppVersion();
        virtual size_t GetFrontEndMaxMsgSize();
        virtual size_t GetBackEndMaxMsgSize();

    private:
        IFrameWork* frame_stub_;
};

#endif
