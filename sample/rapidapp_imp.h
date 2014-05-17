#ifndef RAPIDAPP_IMP_H_
#define RAPIDAPP_IMP_H_

#include "rapidapp_base.h"

using namespace rapidapp;

class MyApp : public RapidApp {
    public:
        MyApp();
        ~MyApp();

    public:
        virtual int OnInit();
        virtual int OnFini();

        virtual int OnStop();
        virtual int OnResume();

        virtual int OnUpdate();
        virtual int OnReload();

        virtual int OnRecvCtrl();

        virtual int OnRecvFrontEnd();
        virtual int OnRecvBackEnd();

    public:
        virtual int OnReportRundata();

    public:
        virtual const char* GetAppVersion();
};

#endif
