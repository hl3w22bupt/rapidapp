#ifndef RAPIDAPP_NET_MGR_H_
#define RAPIDAPP_NET_MGR_H_

#include "rapidapp_net.h"
#include <cstdio>

namespace rapidapp {

struct RapBuffer {
    char* buffer;
    size_t size;
};

class AppLauncher;
class ConnectionHandlerMgr {
    public:
        ConnectionHandlerMgr();
        ~ConnectionHandlerMgr();

    public:
        int Init(size_t up_size);
        void CleanUp();

    public:
        int AddHandler();
        int RemoveHandler();

    private:
        /*HandlerPool handler_pool_;*/
        struct RapBuffer up_msg_buffer_;

        friend class AppLauncher;
};

}

#endif
