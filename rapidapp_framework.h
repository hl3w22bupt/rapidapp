#ifndef RAPIDAPP_FRAMEWORK_H_
#define RAPIDAPP_FRAMEWORK_H_

#include <cstdio>

namespace rapidapp {

class EasyNet;
class IFrameWork {
    public:
        IFrameWork(){}
        ~IFrameWork(){}

    public:
        virtual EasyNet* CreateBackEnd(const char* url) = 0;
        virtual void DestroyBackEnd(EasyNet** net) = 0;

    public:
        virtual int SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        virtual int SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size) = 0;
        // TODO SendvTo
};

}

#endif
