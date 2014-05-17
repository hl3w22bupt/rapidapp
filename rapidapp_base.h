#ifndef RAPIDAPP_BASE_H_
#define RAPIDAPP_BASE_H_

namespace rapidapp {

// App抽象基类
class RapidApp {
    public:
        RapidApp();
        virtual ~RapidApp();

    public:
        virtual int OnInit() = 0;
        virtual int OnFini() = 0;

        virtual int OnStop() = 0;
        virtual int OnResume() = 0;

        virtual int OnUpdate() = 0;
        virtual int OnReload() = 0;

        virtual int OnRecvCtrl() = 0;

        virtual int OnRecvFrontEnd() = 0;
        virtual int OnRecvBackEnd() = 0;

    public:
        virtual int OnReportRundata() = 0;

    public:
        virtual const char* GetAppVersion() = 0;
};

}

#endif
