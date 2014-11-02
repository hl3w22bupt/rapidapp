#ifndef RAPIDAPP_IMP_H_
#define RAPIDAPP_IMP_H_

#include "rapidapp.h"
#include "connector_session.h"
#include "config.pb.h"

using namespace rapidapp;

const int kMaxBackEndNum = 32;
class ConnectorApp : public RapidApp {
    public:
        ConnectorApp();
        ~ConnectorApp();

    public:
        virtual int OnInit(IFrameWork* app_framework);
        virtual int OnFini();

        virtual int OnStop();
        virtual int OnResume();

        virtual int OnUpdate();
        virtual int OnReload();

        virtual int OnRecvCtrl(int argc, char** argv);

        virtual int OnFrontEndConnect(EasyNet* net, int type);

        virtual int OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size);
        virtual int OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size);

        virtual int OnTimer(EasyTimer* timer, int timer_id);

    public:
        virtual int OnReportRundata();

    public:
        virtual size_t GetFrontEndContextSize();
        virtual size_t GetBackEndContextSize();

    public:
        virtual const char* GetAppVersion();
        virtual size_t GetFrontEndMaxMsgSize();
        virtual size_t GetBackEndMaxMsgSize();

    private:
        int SetUpAndCheckConfig();

    private:
        int ForwardUpSideMessage(EasyNet* net, ConnectorSession* session,
                                 const char* msg, size_t size);
        int ForwardDownSideMessage(EasyNet* net, ConnectorSession* session,
                                   const char* msg, size_t size);

        int StartToBackEnd(EasyNet* net, ConnectorSession* session);

    private:
        IFrameWork* frame_stub_;
        connector_config::ConnectorConfig config_;
        int backend_pos_;
        int backend_used_;
        EasyNet* backends_[kMaxBackEndNum];
};

#endif
