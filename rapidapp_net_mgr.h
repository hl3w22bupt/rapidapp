#ifndef RAPIDAPP_NET_MGR_H_
#define RAPIDAPP_NET_MGR_H_

#include "event2/bufferevent.h"
#include <cstdio>
#include <tr1/unordered_map>

namespace rapidapp {

struct RapBuffer {
    char* buffer;
    size_t size;
};

typedef std::tr1::unordered_map<evutil_socket_t, struct bufferevent*> HandlerPool;

class AppLauncher;
class NetHandlerMgr {
    public:
        NetHandlerMgr();
        ~NetHandlerMgr();

    public:
        int Init(size_t recv_buff_size);
        void CleanUp();

    public:
        int AddHandler(struct bufferevent* event);
        int RemoveHandler(struct bufferevent* event);

    private:
        HandlerPool handler_pool_;
        struct RapBuffer recv_buffer_;

        friend class AppLauncher;
};

}

#endif
