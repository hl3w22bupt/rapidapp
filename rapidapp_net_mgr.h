#ifndef RAPIDAPP_NET_MGR_H_
#define RAPIDAPP_NET_MGR_H_

#include <cstdio>

namespace rapidapp {

struct RapBuffer {
    char* buffer;
    size_t size;
};

class AppLauncher;
class NetHandlerMgr {
    public:
        NetHandlerMgr();
        ~NetHandlerMgr();

    public:
        int Init(size_t recv_buff_size);
        void CleanUp();

    public:
        int AddHandler();
        int RemoveHandler();

    private:
        /*HandlerPool handler_pool_;*/
        struct RapBuffer recv_buffer_;

        friend class AppLauncher;
};

}

#endif
