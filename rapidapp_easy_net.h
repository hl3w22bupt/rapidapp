#ifndef RAPIDAPP_EASY_NET_H_
#define RAPIDAPP_EASY_NET_H_

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "rapidapp_framework.h"

namespace rapidapp {

const int MAX_URL_LEN = 128;

class AppFrameWork;
class EasyNet {
    public:
        EasyNet();
        ~EasyNet();

    public:
        int Init(evutil_socket_t sock_fd, struct event_base* ev_base);
        int Connect(const char* uri, struct event_base* ev_base);
        void CleanUp();

    public:
        int Send(const char* msg, size_t size);

    public:
        inline struct bufferevent* bufferevent() {
            return hevent_;
        }

    private:
        struct bufferevent* hevent_;
        char uri_[MAX_URL_LEN];         // 发起后端连接时，后端服务uri
        friend class AppFrameWork;
};

}

#endif
