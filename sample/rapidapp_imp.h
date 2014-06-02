#ifndef RAPIDAPP_IMP_H_
#define RAPIDAPP_IMP_H_

#include "rapidapp.h"

using namespace rapidapp;

class MyApp : public RapidApp {
    public:
        MyApp();
        ~MyApp();

    public:
        virtual int OnInit(IFrameWork* app_framework);
        virtual int OnFini();

        virtual int OnStop();
        virtual int OnResume();

        virtual int OnUpdate();
        virtual int OnReload();

        virtual int OnRecvCtrl(const char* msg);

        virtual int OnRecvFrontEnd(EasyNet* net, const char* msg, size_t size);
        virtual int OnRecvBackEnd(EasyNet* net, const char* msg, size_t size);

    public:
        virtual int OnReportRundata();

    public:
        virtual const char* GetAppVersion();
        virtual size_t GetFrontEndMaxMsgSize();
        virtual size_t GetBackEndMaxMsgSize();
};

#endif
