#ifndef CLIENT_DEMO_APP_H_
#define CLIENT_DEMO_APP_H_

#include "rapidapp.h"

using namespace rapidapp;

class ClientDemoApp : public RapidApp {
    public:
        ClientDemoApp();
        ~ClientDemoApp();

    public:
        virtual int OnInit(IFrameWork* app_framework);
        virtual int OnFini();

        virtual int OnStop();
        virtual int OnResume();

        virtual int OnUpdate();
        virtual int OnReload();

        virtual int OnRecvCtrl(int argc, char** argv);

        virtual int OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size);
        virtual int OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size);

        virtual int OnTimer(EasyTimer* timer, int timer_id);

    public:
        virtual int OnReportRundata();

    public:
        virtual const char* GetAppVersion();
        virtual size_t GetFrontEndMaxMsgSize();
        virtual size_t GetBackEndMaxMsgSize();

    private:
        IFrameWork* frame_stub_;

    private:
        EasyRpc* rpc_;
        EasyNet* backend_;
};

#endif
