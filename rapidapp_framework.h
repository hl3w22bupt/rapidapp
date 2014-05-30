#ifndef RAPIDAPP_FRAMEWORK_H_
#define RAPIDAPP_FRAMEWORK_H_

#include <cstdio>

namespace rapidapp {

class IEventListener {
    public:
        IEventListener(){}
        ~IEventListener(){}

    public:
        virtual int OnEventReadable(const char* data, size_t size) = 0;
};

class EasyNet;
class IFrameWork {
    public:
        IFrameWork(){}
        ~IFrameWork(){}

    public:
        virtual EasyNet* CreateBackEnd(const char* url, IEventListener* event_listener) = 0;
        virtual void DestroyBackEnd(EasyNet** net) = 0;

    public:
        virtual int SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        virtual int SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        // TODO SendvTo
};

}

#endif
