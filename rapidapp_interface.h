#ifndef RAPIDAPP_BASE_H_
#define RAPIDAPP_BASE_H_

#include <cstdio>

namespace rapidapp {

// App抽象基类
class RapidApp {
    public:
        RapidApp(){};
        virtual ~RapidApp(){};

    public:
        virtual int OnInit() = 0;
        virtual int OnFini() = 0;

        virtual int OnStop() = 0;
        virtual int OnResume() = 0;

        virtual int OnUpdate() = 0;
        virtual int OnReload() = 0;

        virtual int OnRecvCtrl() = 0;

        virtual int OnRecvFrontEnd(const char* msg, size_t size) = 0;
        virtual int OnRecvBackEnd(const char* msg, size_t size) = 0;

    public:
        virtual int OnReportRundata() = 0;

    public:
        virtual size_t GetFrontEndMaxMsgSize() = 0;
        virtual size_t GetBackEndMaxMsgSize() = 0;
        virtual const char* GetAppVersion() = 0;
};

}

#endif
