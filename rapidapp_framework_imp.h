#ifndef RAPIDAPP_FRAMEWORK_IMP_H_
#define RAPIDAPP_FRAMEWORK_IMP_H_

#include "rapidapp_framework.h"
#include "rapidapp_net_mgr.h"
#include <event2/event.h>

namespace rapidapp {

class EasyNet;
class AppFrameWork : public IFrameWork {
    public:
        AppFrameWork();
        ~AppFrameWork();

    public:
        int Init(size_t recv_buff_size);
        void CleanUp();

    public:
        virtual EasyNet* CreateBackEnd(const char* url);
        virtual void DestroyBackEnd(EasyNet** net);

    public:
        virtual int SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size);
        virtual int SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size);

    private:
        class NetHandlerMgr* net_mgr_;
};

}

#endif
