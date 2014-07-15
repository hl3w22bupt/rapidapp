#ifndef RAPIDAPP_IMP_H_
#define RAPIDAPP_IMP_H_

#include "rapidapp.h"
#include "connector_session.h"
#include "config.pb.h"

using namespace rapidapp;

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

        virtual int OnRecvCtrl(const char* msg);

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
        int SetUpConfig();

    private:
        IFrameWork* frame_stub_;
        ConnectorSessionMgr* conn_session_mgr_;
        connector_config::ConnectorConfig config_;
};

#endif
