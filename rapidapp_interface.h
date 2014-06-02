#ifndef RAPIDAPP_BASE_H_
#define RAPIDAPP_BASE_H_

#include "rapidapp_framework.h"
#include <cstdio>

namespace rapidapp {

// App抽象基类
class EasyNet;
class RapidApp {
    public:
        RapidApp(){};
        virtual ~RapidApp(){};

    public:
        virtual int OnInit(IFrameWork* app_framework) = 0;
        virtual int OnFini() = 0;

        virtual int OnStop() = 0;
        virtual int OnResume() = 0;

        virtual int OnUpdate() = 0;
        virtual int OnReload() = 0;

        virtual int OnRecvCtrl(const char* msg) {
            return -1;
        }

        virtual int OnRecvFrontEnd(EasyNet* net, const char* msg, size_t size) = 0;
        virtual int OnRecvBackEnd(EasyNet* net, const char* msg, size_t size) = 0;

    public:
        virtual int OnReportRundata() = 0;

    public:
        virtual size_t GetFrontEndMaxMsgSize() = 0;
        virtual size_t GetBackEndMaxMsgSize() = 0;
        virtual const char* GetAppVersion() = 0;
};

}

#endif
