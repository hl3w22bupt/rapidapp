#ifndef CONNECTOR_SERVER_DEMO_H_
#define CONNECTOR_SERVER_DEMO_H_

#include "rapidapp.h"
#include "connector_server_api.h"

using namespace rapidapp;

class ServerDemoApp : public RapidApp, public hmoon_connector_api::IConnListener {
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

        virtual int OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size);
        virtual int OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size);

        virtual int OnTimer(EasyTimer* timer, int timer_id);

    public:
        virtual int OnReportRundata();

    public:
        virtual const char* GetAppVersion();
        virtual size_t GetFrontEndMaxMsgSize();
        virtual size_t GetBackEndMaxMsgSize();

    public:
        // virtual functions of IConnListener
        virtual int OnConnStart(void* net, uint32_t fd, uint64_t nid);
        virtual int OnConnStop(void* net, uint32_t fd, uint64_t nid, uint32_t sid);
        virtual int OnConnResume(void* net, uint32_t fd, uint64_t nid, uint32_t sid);

        virtual int OnData(void* net, uint32_t fd, uint64_t nid, uint32_t sid,
                           const char* data, size_t len);
        virtual int SendToConn(void* net, const char* data, size_t len);

    private:
        IFrameWork* frame_stub_;
        hmoon_connector_api::ConnectorServerApi sconn_api_;
};

#endif
