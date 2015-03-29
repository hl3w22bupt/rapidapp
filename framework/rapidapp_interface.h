#ifndef RAPIDAPP_INTERFACE_H_
#define RAPIDAPP_INTERFACE_H_

#include "rapidapp_framework.h"
#include <google/protobuf/message.h>
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

        virtual int OnRecvCtrl(int argc, char** argv) {
            return -1;
        }

        virtual int OnFrontEndConnect(EasyNet* net, int type) {
            return 0;
        }

        virtual int OnFrontEndClose(EasyNet* net, int type) {
            return 0;
        }

        // 基本消息事件
        virtual int OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size) {
            return -1;
        }
        virtual int OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size) {
            return -1;
        }

        // RPC 消息事件
        virtual int OnRpc(const ::google::protobuf::Message* request,
                          ::google::protobuf::Message** response) {
            return -1;
        }

        virtual int OnTimer(EasyTimer* timer, int timer_id) = 0;

    public:
        virtual int OnReportRundata() = 0;

    public:
        ///< frontend connector context size
        virtual size_t GetFrontEndContextSize() {return 0;}
        ///< backend connector context size
        virtual size_t GetBackEndContextSize() {return 0;}

    public:
        ///< frontend max msg wired size
        virtual size_t GetFrontEndMaxMsgSize() = 0;
        ///< backend max msg wired size
        virtual size_t GetBackEndMaxMsgSize() = 0;
        ///< app version
        virtual const char* GetAppVersion() = 0;
};

}

#endif
